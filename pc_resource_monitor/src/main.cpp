#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <netinet/in.h>
#include <optional>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <utility>
#include <vector>

using namespace std;

volatile sig_atomic_t g_running = 1;

struct CpuTimes {
    unsigned long long user = 0;
    unsigned long long nice = 0;
    unsigned long long system = 0;
    unsigned long long idle = 0;
    unsigned long long iowait = 0;
    unsigned long long irq = 0;
    unsigned long long softirq = 0;
    unsigned long long steal = 0;

    unsigned long long total() const {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }

    unsigned long long idle_all() const {
        return idle + iowait;
    }
};

struct ProcessSample {
    int pid = 0;
    string name;
    string state;
    unsigned long long cpu_ticks = 0;
    unsigned long long rss_kb = 0;
};

struct MemoryInfo {
    unsigned long long total_kb = 0;
    unsigned long long available_kb = 0;
    unsigned long long swap_total_kb = 0;
    unsigned long long swap_free_kb = 0;
};

string json_escape(const string& value) {
    ostringstream out;
    for (char ch : value) {
        switch (ch) {
            case '\\':
                out << "\\\\";
                break;
            case '"':
                out << "\\\"";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    out << "\\u" << hex << setw(4) << setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(ch));
                } else {
                    out << ch;
                }
        }
    }
    return out.str();
}

bool starts_with(const string& value, const string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool is_number(const char* value) {
    if (value == nullptr || *value == '\0') {
        return false;
    }
    while (*value != '\0') {
        if (*value < '0' || *value > '9') {
            return false;
        }
        ++value;
    }
    return true;
}

string read_file(const string& path) {
    ifstream file(path, ios::binary);
    if (!file) {
        return {};
    }
    ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

vector<CpuTimes> read_cpu_times() {
    ifstream stat("/proc/stat");
    vector<CpuTimes> result;
    string label;

    while (stat >> label) {
        if (!starts_with(label, "cpu")) {
            break;
        }

        CpuTimes times;
        stat >> times.user >> times.nice >> times.system >> times.idle >> times.iowait >>
            times.irq >> times.softirq >> times.steal;
        result.push_back(times);
        stat.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    return result;
}

double cpu_usage_between(const CpuTimes& previous, const CpuTimes& current) {
    const unsigned long long total_delta = current.total() - previous.total();
    const unsigned long long idle_delta = current.idle_all() - previous.idle_all();
    if (total_delta == 0 || idle_delta > total_delta) {
        return 0.0;
    }
    return (static_cast<double>(total_delta - idle_delta) * 100.0) /
           static_cast<double>(total_delta);
}

MemoryInfo read_memory_info() {
    ifstream meminfo("/proc/meminfo");
    MemoryInfo info;
    string key;
    unsigned long long value = 0;
    string unit;

    while (meminfo >> key >> value >> unit) {
        if (key == "MemTotal:") {
            info.total_kb = value;
        } else if (key == "MemAvailable:") {
            info.available_kb = value;
        } else if (key == "SwapTotal:") {
            info.swap_total_kb = value;
        } else if (key == "SwapFree:") {
            info.swap_free_kb = value;
        }
    }

    return info;
}

vector<double> read_load_average() {
    ifstream loadavg("/proc/loadavg");
    vector<double> result(3, 0.0);
    loadavg >> result[0] >> result[1] >> result[2];
    return result;
}

unsigned long long read_uptime_seconds() {
    ifstream uptime("/proc/uptime");
    double seconds = 0.0;
    uptime >> seconds;
    return static_cast<unsigned long long>(seconds);
}

optional<ProcessSample> read_process(int pid, long page_size_kb) {
    const string stat_path = "/proc/" + to_string(pid) + "/stat";
    const string content = read_file(stat_path);
    if (content.empty()) {
        return nullopt;
    }

    const size_t open = content.find('(');
    const size_t close = content.rfind(')');
    if (open == string::npos || close == string::npos || close <= open) {
        return nullopt;
    }

    ProcessSample sample;
    sample.pid = pid;
    sample.name = content.substr(open + 1, close - open - 1);

    istringstream fields(content.substr(close + 2));
    vector<string> values;
    string value;
    while (fields >> value) {
        values.push_back(value);
    }

    if (values.size() < 22) {
        return nullopt;
    }

    sample.state = values[0];

    try {
        const unsigned long long utime = stoull(values[11]);
        const unsigned long long stime = stoull(values[12]);
        const long long rss_pages = stoll(values[21]);
        sample.cpu_ticks = utime + stime;
        sample.rss_kb = rss_pages > 0 ? static_cast<unsigned long long>(rss_pages) * page_size_kb : 0;
    } catch (const exception&) {
        return nullopt;
    }

    return sample;
}

vector<ProcessSample> read_processes(long page_size_kb) {
    vector<ProcessSample> processes;
    DIR* proc = opendir("/proc");
    if (proc == nullptr) {
        return processes;
    }

    while (dirent* entry = readdir(proc)) {
        if (!is_number(entry->d_name)) {
            continue;
        }
        const int pid = atoi(entry->d_name);
        if (auto process = read_process(pid, page_size_kb)) {
            processes.push_back(*process);
        }
    }

    closedir(proc);
    return processes;
}

string mime_type(const string& path) {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") {
        return "text/html; charset=utf-8";
    }
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") {
        return "text/css; charset=utf-8";
    }
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") {
        return "application/javascript; charset=utf-8";
    }
    return "application/octet-stream";
}

string sanitize_path(const string& target) {
    string path = target;
    const size_t query_pos = path.find('?');
    if (query_pos != string::npos) {
        path = path.substr(0, query_pos);
    }
    if (path == "/") {
        return "/index.html";
    }
    if (path.find("..") != string::npos) {
        return {};
    }
    return path;
}

string http_response(int status, const string& reason, const string& content_type,
                          const string& body) {
    ostringstream response;
    response << "HTTP/1.1 " << status << ' ' << reason << "\r\n"
             << "Content-Type: " << content_type << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "Cache-Control: no-store\r\n"
             << "\r\n"
             << body;
    return response.str();
}

class MetricsCollector {
public:
    MetricsCollector()
        : previous_cpu_(read_cpu_times()),
          page_size_kb_(max<long>(1, sysconf(_SC_PAGESIZE) / 1024)) {
        for (const ProcessSample& process : read_processes(page_size_kb_)) {
            previous_process_ticks_[process.pid] = process.cpu_ticks;
        }
    }

    string collect_json() {
        const vector<CpuTimes> current_cpu = read_cpu_times();
        const MemoryInfo memory = read_memory_info();
        const vector<double> load_average = read_load_average();
        const unsigned long long uptime_seconds = read_uptime_seconds();
        const vector<ProcessSample> processes = read_processes(page_size_kb_);

        const unsigned long long previous_total =
            previous_cpu_.empty() ? 0 : previous_cpu_.front().total();
        const unsigned long long current_total = current_cpu.empty() ? previous_total : current_cpu.front().total();
        const unsigned long long total_delta = current_total > previous_total ? current_total - previous_total : 0;
        const int cpu_count = max<int>(1, static_cast<int>(current_cpu.size()) - 1);

        vector<double> core_usage;
        for (size_t index = 1; index < current_cpu.size() && index < previous_cpu_.size(); ++index) {
            core_usage.push_back(cpu_usage_between(previous_cpu_[index], current_cpu[index]));
        }
        const double total_cpu_usage =
            (!previous_cpu_.empty() && !current_cpu.empty()) ? cpu_usage_between(previous_cpu_[0], current_cpu[0]) : 0.0;

        struct ProcessView {
            ProcessSample sample;
            double cpu = 0.0;
            double memory = 0.0;
        };

        vector<ProcessView> process_views;
        map<int, unsigned long long> next_process_ticks;

        for (const ProcessSample& process : processes) {
            next_process_ticks[process.pid] = process.cpu_ticks;
            const auto previous = previous_process_ticks_.find(process.pid);
            const unsigned long long previous_ticks =
                previous == previous_process_ticks_.end() ? process.cpu_ticks : previous->second;
            const unsigned long long process_delta =
                process.cpu_ticks > previous_ticks ? process.cpu_ticks - previous_ticks : 0;

            ProcessView view;
            view.sample = process;
            view.cpu = total_delta > 0
                           ? (static_cast<double>(process_delta) * 100.0 * cpu_count) /
                                 static_cast<double>(total_delta)
                           : 0.0;
            view.memory = memory.total_kb > 0
                              ? (static_cast<double>(process.rss_kb) * 100.0) /
                                    static_cast<double>(memory.total_kb)
                              : 0.0;
            process_views.push_back(view);
        }

        sort(process_views.begin(), process_views.end(),
                  [](const ProcessView& left, const ProcessView& right) {
                      if (left.cpu == right.cpu) {
                          return left.sample.rss_kb > right.sample.rss_kb;
                      }
                      return left.cpu > right.cpu;
                  });

        if (process_views.size() > 20) {
            process_views.resize(20);
        }

        previous_cpu_ = current_cpu;
        previous_process_ticks_ = move(next_process_ticks);

        const unsigned long long memory_used =
            memory.total_kb > memory.available_kb ? memory.total_kb - memory.available_kb : 0;
        const unsigned long long swap_used =
            memory.swap_total_kb > memory.swap_free_kb ? memory.swap_total_kb - memory.swap_free_kb : 0;
        const double memory_usage =
            memory.total_kb > 0 ? (static_cast<double>(memory_used) * 100.0) / memory.total_kb : 0.0;
        const double swap_usage =
            memory.swap_total_kb > 0 ? (static_cast<double>(swap_used) * 100.0) / memory.swap_total_kb : 0.0;
        const auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());

        ostringstream json;
        json << fixed << setprecision(2);
        json << "{";
        json << "\"timestamp\":" << now << ",";
        json << "\"uptimeSeconds\":" << uptime_seconds << ",";
        json << "\"cpuUsage\":" << total_cpu_usage << ",";

        json << "\"cores\":[";
        for (size_t index = 0; index < core_usage.size(); ++index) {
            if (index > 0) {
                json << ",";
            }
            json << "{\"name\":\"cpu" << index << "\",\"usage\":" << core_usage[index] << "}";
        }
        json << "],";

        json << "\"memory\":{";
        json << "\"totalKb\":" << memory.total_kb << ",";
        json << "\"usedKb\":" << memory_used << ",";
        json << "\"availableKb\":" << memory.available_kb << ",";
        json << "\"usage\":" << memory_usage;
        json << "},";

        json << "\"swap\":{";
        json << "\"totalKb\":" << memory.swap_total_kb << ",";
        json << "\"usedKb\":" << swap_used << ",";
        json << "\"usage\":" << swap_usage;
        json << "},";

        json << "\"loadAverage\":[";
        for (size_t index = 0; index < load_average.size(); ++index) {
            if (index > 0) {
                json << ",";
            }
            json << load_average[index];
        }
        json << "],";

        json << "\"processes\":[";
        for (size_t index = 0; index < process_views.size(); ++index) {
            if (index > 0) {
                json << ",";
            }
            const ProcessView& process = process_views[index];
            json << "{";
            json << "\"pid\":" << process.sample.pid << ",";
            json << "\"name\":\"" << json_escape(process.sample.name) << "\",";
            json << "\"state\":\"" << json_escape(process.sample.state) << "\",";
            json << "\"cpu\":" << process.cpu << ",";
            json << "\"memoryKb\":" << process.sample.rss_kb << ",";
            json << "\"memoryPercent\":" << process.memory;
            json << "}";
        }
        json << "]";
        json << "}";

        return json.str();
    }

private:
    vector<CpuTimes> previous_cpu_;
    map<int, unsigned long long> previous_process_ticks_;
    long page_size_kb_;
};

class HttpServer {
public:
    HttpServer(int port, string web_root)
        : port_(port), web_root_(move(web_root)) {}

    void run() {
        const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw runtime_error("socket() failed");
        }

        int reuse = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(static_cast<uint16_t>(port_));

        if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            close(server_fd);
            throw runtime_error("bind() failed on 127.0.0.1:" + to_string(port_));
        }

        if (listen(server_fd, 32) < 0) {
            close(server_fd);
            throw runtime_error("listen() failed");
        }

        cout << "PC Resource Monitor is running at http://127.0.0.1:" << port_ << "\n";

        while (g_running) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_fd, &read_fds);

            timeval select_timeout {};
            select_timeout.tv_sec = 1;

            const int ready = select(server_fd + 1, &read_fds, nullptr, nullptr, &select_timeout);
            if (ready < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }
            if (ready == 0) {
                continue;
            }

            sockaddr_in client_address {};
            socklen_t client_length = sizeof(client_address);
            const int client_fd =
                accept(server_fd, reinterpret_cast<sockaddr*>(&client_address), &client_length);
            if (client_fd < 0) {
                if (!g_running) {
                    break;
                }
                if (errno == EINTR) {
                    continue;
                }
                break;
            }

            handle_client(client_fd);
        }

        close(server_fd);
    }

private:
    void handle_client(int client_fd) {
        timeval timeout {};
        timeout.tv_sec = 2;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        char buffer[4096] {};
        const ssize_t received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            close(client_fd);
            return;
        }

        istringstream request(string(buffer, static_cast<size_t>(received)));
        string method;
        string target;
        request >> method >> target;

        string response;
        if (method != "GET") {
            response = http_response(405, "Method Not Allowed", "text/plain; charset=utf-8",
                                     "Only GET is supported.\n");
        } else if (target == "/api/metrics") {
            response = http_response(200, "OK", "application/json; charset=utf-8",
                                     metrics_.collect_json());
        } else {
            response = static_response(target);
        }

        send_all(client_fd, response);
        close(client_fd);
    }

    string static_response(const string& target) const {
        const string path = sanitize_path(target);
        if (path.empty()) {
            return http_response(400, "Bad Request", "text/plain; charset=utf-8", "Bad path.\n");
        }

        const string full_path = web_root_ + path;
        struct stat file_stat {};
        if (stat(full_path.c_str(), &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
            return http_response(404, "Not Found", "text/plain; charset=utf-8", "Not found.\n");
        }

        const string body = read_file(full_path);
        return http_response(200, "OK", mime_type(full_path), body);
    }

    static void send_all(int fd, const string& response) {
        const char* data = response.data();
        size_t remaining = response.size();
        while (remaining > 0) {
            const ssize_t sent = send(fd, data, remaining, 0);
            if (sent <= 0) {
                return;
            }
            data += sent;
            remaining -= static_cast<size_t>(sent);
        }
    }

    int port_;
    string web_root_;
    MetricsCollector metrics_;
};

void on_signal(int) {
    g_running = 0;
}

void install_signal_handlers() {
    struct sigaction action {};
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);

    struct sigaction ignore {};
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    sigaction(SIGPIPE, &ignore, nullptr);
}

string default_web_root(const char* executable_path) {
    string path = executable_path != nullptr ? executable_path : "";
    const size_t slash = path.rfind('/');
    if (slash != string::npos) {
        const string near_binary = path.substr(0, slash) + "/web";
        struct stat file_stat {};
        if (stat((near_binary + "/index.html").c_str(), &file_stat) == 0) {
            return near_binary;
        }
    }
    return "web";
}

int main(int argc, char* argv[]) {
    int port = 8080;
    string web_root = default_web_root(argc > 0 ? argv[0] : nullptr);

    for (int index = 1; index < argc; ++index) {
        const string arg = argv[index];
        if (arg == "--port" && index + 1 < argc) {
            const string port_value = argv[++index];
            try {
                size_t parsed = 0;
                port = stoi(port_value, &parsed);
                if (parsed != port_value.size()) {
                    cerr << "Invalid port value.\n";
                    return 1;
                }
            } catch (const exception&) {
                cerr << "Invalid port value.\n";
                return 1;
            }
            if (port <= 0 || port > 65535) {
                cerr << "Port must be in range 1..65535.\n";
                return 1;
            }
        } else if (arg == "--web" && index + 1 < argc) {
            web_root = argv[++index];
        } else if (arg == "--help") {
            cout << "Usage: pc-monitor [--port 8080] [--web ./web]\n";
            return 0;
        } else {
            cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    install_signal_handlers();

    try {
        HttpServer server(port, web_root);
        server.run();
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
