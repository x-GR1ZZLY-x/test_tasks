#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace std;

namespace fs = filesystem;


constexpr int kDefaultIntervalSeconds = 60;

const unordered_set<string> kAudioExtensions = {
    ".aac", ".flac", ".m4a", ".mp3", ".ogg", ".opus", ".wav", ".wma"};

const unordered_set<string> kVideoExtensions = {
    ".avi", ".m4v", ".mkv", ".mov", ".mp4", ".mpeg", ".mpg", ".webm", ".wmv"};

const unordered_set<string> kImageExtensions = {
    ".bmp", ".gif", ".jpeg", ".jpg", ".png", ".svg", ".tif", ".tiff", ".webp"};

struct Config {
    fs::path scan_path;
    int interval_seconds = kDefaultIntervalSeconds;
};

struct MediaFiles {
    vector<string> audio;
    vector<string> video;
    vector<string> images;
};

string ToLower(string value) {
    transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });
    return value;
}

string GetHomeDirectory() {
    const char* home = getenv("HOME");
    if (home == nullptr || *home == '\0') {
        throw runtime_error("HOME environment variable is not set");
    }
    return home;
}

int ParsePositiveInt(const string& value, const string& option_name) {
    size_t parsed_chars = 0;
    int result = 0;
    try {
        result = stoi(value, &parsed_chars);
    } catch (const exception&) {
        throw invalid_argument(option_name + " must be a positive integer");
    }

    if (parsed_chars != value.size() || result <= 0) {
        throw invalid_argument(option_name + " must be a positive integer");
    }
    return result;
}

Config ParseArguments(int argc, char* argv[]) {
    Config config;
    config.scan_path = GetHomeDirectory();

    for (int i = 1; i < argc; ++i) {
        const string arg = argv[i];

        if (arg == "--path") {
            if (i + 1 >= argc) {
                throw invalid_argument("--path requires a value");
            }
            config.scan_path = argv[++i];
            continue;
        }

        if (arg == "--interval") {
            if (i + 1 >= argc) {
                throw invalid_argument("--interval requires a value");
            }
            config.interval_seconds = ParsePositiveInt(argv[++i], "--interval");
            continue;
        }

        throw invalid_argument("unknown argument: " + arg);
    }

    return config;
}

string JsonEscape(const string& value) {
    ostringstream out;
    for (unsigned char ch : value) {
        switch (ch) {
            case '"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\b':
                out << "\\b";
                break;
            case '\f':
                out << "\\f";
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
                if (ch < 0x20) {
                    out << "\\u" << hex << setw(4) << setfill('0')
                        << static_cast<int>(ch) << dec << setfill(' ');
                } else {
                    out << static_cast<char>(ch);
                }
        }
    }
    return out.str();
}

void WriteStringArray(ostream& out,
                      const string& key,
                      const vector<string>& values,
                      bool trailing_comma) {
    out << "  \"" << key << "\": [";
    if (!values.empty()) {
        out << "\n";
        for (size_t i = 0; i < values.size(); ++i) {
            out << "    \"" << JsonEscape(values[i]) << "\"";
            if (i + 1 != values.size()) {
                out << ",";
            }
            out << "\n";
        }
        out << "  ";
    }
    out << "]";
    if (trailing_comma) {
        out << ",";
    }
    out << "\n";
}

void WriteJsonFile(const fs::path& output_path, const MediaFiles& files) {
    ofstream out(output_path, ios::trunc);
    if (!out) {
        throw runtime_error("cannot open output file: " + output_path.string());
    }

    out << "{\n";
    WriteStringArray(out, "audio", files.audio, true);
    WriteStringArray(out, "video", files.video, true);
    WriteStringArray(out, "images", files.images, false);
    out << "}\n";
}

void SortResults(MediaFiles& files) {
    sort(files.audio.begin(), files.audio.end());
    sort(files.video.begin(), files.video.end());
    sort(files.images.begin(), files.images.end());
}

MediaFiles ScanMediaFiles(const fs::path& root_path) {
    MediaFiles files;

    error_code error;
    if (!fs::exists(root_path, error) || !fs::is_directory(root_path, error)) {
        throw runtime_error("scan path is not a directory: " + root_path.string());
    }

    fs::recursive_directory_iterator it(
        root_path, fs::directory_options::skip_permission_denied, error);
    fs::recursive_directory_iterator end;

    while (!error && it != end) {
        const fs::directory_entry& entry = *it;

        if (entry.is_regular_file(error) && !error) {
            const string extension = ToLower(entry.path().extension().string());
            vector<string>* target = nullptr;

            if (kAudioExtensions.count(extension) != 0) {
                target = &files.audio;
            } else if (kVideoExtensions.count(extension) != 0) {
                target = &files.video;
            } else if (kImageExtensions.count(extension) != 0) {
                target = &files.images;
            }

            if (target != nullptr) {
                error_code relative_error;
                fs::path relative_path = fs::relative(entry.path(), root_path, relative_error);
                target->push_back((relative_error ? entry.path() : relative_path).generic_string());
            }
        }

        error.clear();
        it.increment(error);
    }

    if (error) {
        cerr << "Warning: scan finished with an error: " << error.message() << "\n";
    }

    SortResults(files);
    return files;
}

int main(int argc, char* argv[]) {
    try {
        const Config config = ParseArguments(argc, argv);
        const fs::path output_path = fs::path(GetHomeDirectory()) / ".media_files";

        while (true) {
            const MediaFiles files = ScanMediaFiles(config.scan_path);
            WriteJsonFile(output_path, files);
            this_thread::sleep_for(chrono::seconds(config.interval_seconds));
        }
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
