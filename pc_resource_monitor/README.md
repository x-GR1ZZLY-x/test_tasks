# PC Resource Monitor

Программа для отображения загрузки ресурсов Linux PC.

Суть реализации:

- backend написан на C++;
- frontend сделан как WebUI: HTML + CSS + TypeScript;
- IPC между frontend и backend: локальный HTTP на `127.0.0.1`;
- backend читает данные из `/proc`;
- WebUI обновляет данные раз в секунду через `GET /api/metrics`.

Отображаются CPU, ядра CPU, память, swap, load average, uptime и список самых активных процессов.

`web/app.ts` является TypeScript-исходником. Во время сборки из него генерируется `web/app.js`, который подключается в браузере.

## Сборка на Ubuntu 22.04

```bash
sudo apt update
sudo apt install -y build-essential cmake nodejs npm

npm install
cmake -S . -B build
cmake --build build
./build/pc-monitor
```

Просмотр показателей:

```text
http://127.0.0.1:8080
```

## Как проверить

```bash
curl http://127.0.0.1:8080/api/metrics
```

В браузере данные должны обновляться автоматически раз в секунду.

Можно сравнить значения с системными утилитами:

```bash
top
free -h
cat /proc/loadavg
```

## Параметры запуска

```bash
./build/pc-monitor --port 9090
./build/pc-monitor --web /path/to/web
```

## Структура проекта

```text
src/main.cpp      C++ HTTP server и сборщик метрик Linux
web/index.html    HTML-разметка WebUI
web/styles.css    стили WebUI
web/app.ts        TypeScript-исходник frontend
package.json      npm-скрипт и зависимость TypeScript
package-lock.json зафиксированная версия npm-зависимостей
tsconfig.json     настройки TypeScript
```

## Сборка TypeScript

Frontend собирается из `web/app.ts` в `web/app.js` командой:

```bash
npm run build:web
```

`cmake --build build` также запускает эту сборку перед копированием WebUI в `build/web`.

## Примечания

C++ backend не использует внешние библиотеки и зависимости. В коде используются стандартная библиотека C++ и системные Linux/POSIX API для чтения `/proc` и работы с локальным socket-сервером.