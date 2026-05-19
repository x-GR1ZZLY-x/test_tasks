type CpuCore = {
  name: string;
  usage: number;
};

type Memory = {
  totalKb: number;
  usedKb: number;
  availableKb: number;
  usage: number;
};

type Swap = {
  totalKb: number;
  usedKb: number;
  usage: number;
};

type ProcessInfo = {
  pid: number;
  name: string;
  state: string;
  cpu: number;
  memoryKb: number;
  memoryPercent: number;
};

type Metrics = {
  timestamp: number;
  uptimeSeconds: number;
  cpuUsage: number;
  cores: CpuCore[];
  memory: Memory;
  swap: Swap;
  loadAverage: number[];
  processes: ProcessInfo[];
};

const byId = <T extends HTMLElement>(id: string): T => {
  const element = document.getElementById(id);
  if (!element) {
    throw new Error(`Missing element #${id}`);
  }
  return element as T;
};

const statusEl = byId<HTMLParagraphElement>("status");
const updatedEl = byId<HTMLSpanElement>("updated");
const cpuTotalEl = byId<HTMLElement>("cpu-total");
const cpuBarEl = byId<HTMLSpanElement>("cpu-bar");
const memTotalEl = byId<HTMLElement>("mem-total");
const memBarEl = byId<HTMLSpanElement>("mem-bar");
const memTextEl = byId<HTMLElement>("mem-text");
const swapTotalEl = byId<HTMLElement>("swap-total");
const swapBarEl = byId<HTMLSpanElement>("swap-bar");
const swapTextEl = byId<HTMLElement>("swap-text");
const loadTextEl = byId<HTMLDivElement>("load-text");
const uptimeTextEl = byId<HTMLElement>("uptime-text");
const coresEl = byId<HTMLDivElement>("cores");
const processesEl = byId<HTMLTableSectionElement>("processes");
const loadPeriods = [1, 5, 15];

const percent = (value: number): string => `${Math.max(0, value).toFixed(1)}%`;

const formatBytes = (kb: number): string => {
  const gb = kb / 1024 / 1024;
  if (gb >= 1) {
    return `${gb.toFixed(1)} GiB`;
  }
  return `${(kb / 1024).toFixed(0)} MiB`;
};

const formatUptime = (seconds: number): string => {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const parts: string[] = [];
  if (days) {
    parts.push(`${days}d`);
  }
  if (hours || days) {
    parts.push(`${hours}h`);
  }
  parts.push(`${minutes}m`);
  return parts.join(" ");
};

const setBar = (bar: HTMLSpanElement, value: number): void => {
  const safeValue = Math.max(0, Math.min(100, value));
  bar.style.width = `${safeValue}%`;
  bar.classList.toggle("is-warn", safeValue >= 70 && safeValue < 90);
  bar.classList.toggle("is-bad", safeValue >= 90);
};

const renderCores = (cores: CpuCore[]): void => {
  coresEl.replaceChildren(
    ...cores.map((core) => {
      const item = document.createElement("div");
      item.className = "core";

      const row = document.createElement("div");
      row.className = "core__row";

      const name = document.createElement("span");
      name.textContent = core.name.toUpperCase();

      const value = document.createElement("strong");
      value.textContent = percent(core.usage);

      const bar = document.createElement("div");
      bar.className = "bar";

      const fill = document.createElement("span");
      setBar(fill, core.usage);

      row.append(name, value);
      bar.append(fill);
      item.append(row, bar);
      return item;
    }),
  );
};

const renderProcesses = (processes: ProcessInfo[]): void => {
  processesEl.replaceChildren(
    ...processes.map((process) => {
      const row = document.createElement("tr");

      const pid = document.createElement("td");
      pid.textContent = String(process.pid);

      const name = document.createElement("td");
      name.title = process.name;
      name.textContent = process.name;

      const state = document.createElement("td");
      state.textContent = process.state;

      const cpu = document.createElement("td");
      cpu.textContent = percent(process.cpu);

      const memory = document.createElement("td");
      memory.textContent = `${formatBytes(process.memoryKb)} (${percent(process.memoryPercent)})`;

      row.append(pid, name, state, cpu, memory);
      return row;
    }),
  );
};

const render = (metrics: Metrics): void => {
  cpuTotalEl.textContent = percent(metrics.cpuUsage);
  setBar(cpuBarEl, metrics.cpuUsage);

  memTotalEl.textContent = percent(metrics.memory.usage);
  setBar(memBarEl, metrics.memory.usage);
  memTextEl.textContent = `${formatBytes(metrics.memory.usedKb)} / ${formatBytes(metrics.memory.totalKb)}`;

  swapTotalEl.textContent = percent(metrics.swap.usage);
  setBar(swapBarEl, metrics.swap.usage);
  swapTextEl.textContent = `${formatBytes(metrics.swap.usedKb)} / ${formatBytes(metrics.swap.totalKb)}`;

  loadTextEl.replaceChildren(
    ...metrics.loadAverage.map((value, index) => {
      const item = document.createElement("span");
      const period = document.createElement("small");
      const number = document.createElement("strong");
      period.textContent = `${loadPeriods[index] ?? index + 1}m`;
      number.textContent = value.toFixed(2);
      item.append(period, number);
      return item;
    }),
  );
  uptimeTextEl.textContent = `Uptime ${formatUptime(metrics.uptimeSeconds)}`;
  updatedEl.textContent = new Date(metrics.timestamp * 1000).toLocaleTimeString();

  renderCores(metrics.cores);
  renderProcesses(metrics.processes);
};

const loadMetrics = async (): Promise<void> => {
  try {
    const response = await fetch("/api/metrics", { cache: "no-store" });
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }
    const metrics = (await response.json()) as Metrics;
    render(metrics);
    statusEl.textContent = "Live";
  } catch (error) {
    statusEl.textContent = `Connection error: ${error instanceof Error ? error.message : "unknown"}`;
  }
};

void loadMetrics();
window.setInterval(() => {
  void loadMetrics();
}, 1000);
