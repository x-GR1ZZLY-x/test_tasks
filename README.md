# Media Scanner

Media Scanner — консольное приложение на C++ под Linux. Программа
через равные интервалы времени рекурсивно обходит заданный каталог, находит
мультимедийные файлы и записывает результат в файл `$HOME/.media_files` в
формате JSON.

Файлы группируются по трем категориям:

- `audio`
- `video`
- `images`

В JSON записываются пути относительно сканируемого каталога.

## Требования

- Linux
- CMake 3.10 или новее
- компилятор с поддержкой C++17, например `g++`

## Сборка

```bash
cmake -S . -B build
cmake --build build
```

После сборки исполняемый файл будет находиться по пути:

```bash
./build/media_scanner
```

## Запуск

Запуск с настройками по умолчанию:

```bash
./build/media_scanner
```

По умолчанию программа сканирует каталог `$HOME` каждые 60 секунд и записывает
результат в `$HOME/.media_files`. Программа работает постоянно, остановить ее
можно сочетанием клавиш `Ctrl+C`.

Запуск с указанием каталога и интервала:

```bash
./build/media_scanner --path /home/user/Music --interval 30
```

## Быстрая проверка

Программа выполняет первый обход сразу после запуска, а затем ждет следующий
интервал. Для короткой проверки можно использовать команду `timeout`:

```bash
mkdir -p /tmp/media-check/Music /tmp/media-check/Pictures /tmp/media-check/Documents
touch /tmp/media-check/Music/song.mp3
touch /tmp/media-check/Pictures/photo.JPG
touch /tmp/media-check/Documents/readme.txt

timeout 30 ./build/media_scanner --path /tmp/media-check --interval 15
cat "$HOME/.media_files"
```

Результат:

```json
{
  "audio": [
    "Music/song.mp3"
  ],
  "video": [],
  "images": [
    "Pictures/photo.JPG"
  ]
}
```

## Поддерживаемые расширения

Audio: `.aac`, `.flac`, `.m4a`, `.mp3`, `.ogg`, `.opus`, `.wav`, `.wma`

Video: `.avi`, `.m4v`, `.mkv`, `.mov`, `.mp4`, `.mpeg`, `.mpg`, `.webm`, `.wmv`

Images: `.bmp`, `.gif`, `.jpeg`, `.jpg`, `.png`, `.svg`, `.tif`, `.tiff`, `.webp`

## Использование в фоновом режиме

В рамках задания программа запускается как обычное консольное приложение. Для
реального использования ее можно оформить как daemon-процесс, например через
`systemd`: тогда система сможет запускать программу в фоне, перезапускать ее
после сбоев и останавливать стандартными средствами Linux.
