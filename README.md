#  m5stack-chipstream

![](https://github.com/h1romas4/m5stack-chipstream/workflows/Build/badge.svg)

This is a test to port [C++'s ymfm and Rust's vgmplay](https://github.com/h1romas4/libymfm.wasm) to ESP32(Xtensa).

Still slow and impractical, depending on the sound chip you choose.

## Environment

- [M5Stack Core2](https://docs.m5stack.com/en/core/core2) (ESP32 PSRAM 4MB+4MB)
- [M5Stack RCA Module](https://docs.m5stack.com/ja/module/RCA%20Module%2013.2) (I2S PCM5102APWR)

## Demo

![Main Board](https://raw.githubusercontent.com/h1romas4/m5stack-chipstream/main/docs/images/chipstream-01.jpg)

🎥 https://twitter.com/h1romas4/status/1629345305861947393

## TODO

- [x] The transfer of PCM waveforms from Rust to C is shifted depending on the memory state. (Is it due to int16_t alignment?)
- [x] I may be using the ring buffer or I2S DMA incorrectly. The process slows down from the same time, independent of the waveform generation process time.
- [ ] Player control is not complete. May stop when transitioning to next song.
- [ ] There is no interface to stop playback.

## Build

@see [.github/workflows/build.yml](https://github.com/h1romas4/m5stack-chipstream/blob/main/.github/workflows/build.yml)

### Setup

- [Setup ESF-IDF v4.4.5](https://docs.espressif.com/projects/esp-idf/en/v4.4.5/esp32/get-started/index.html#installation-step-by-step)

get_idf

```
alias get_idf='. $HOME/esp/esp-idf/export.sh'
```

```
$ get_idf
Detecting the Python interpreter
... snip ...
Done! You can now compile ESP-IDF projects.
Go to the project directory and run:

  idf.py build

$ echo ${IDF_PATH}
/home/hiromasa/devel/toolchain/esp/esp-idf

$ xtensa-esp32-elf-gcc -v
Using built-in specs.
COLLECT_GCC=xtensa-esp32-elf-gcc
COLLECT_LTO_WRAPPER=/home/hiromasa/.espressif/tools/xtensa-esp32-elf/esp-2021r2-patch3-8.4.0/xtensa-esp32-elf/bin/../libexec/gcc/xtensa-esp32-elf/8.4.0/lto-wrapper
lto-wrapper
Target: xtensa-esp32-elf
... snip ...
Thread model: posix
gcc version 8.4.0 (crosstool-NG esp-2021r2-patch5)
```

- Setup Rust Xtensa toolchaine

https://github.com/esp-rs/espup

```
curl -L https://github.com/esp-rs/espup/releases/latest/download/espup-x86_64-unknown-linux-gnu -o espup
chmod a+x espup
./espup install
```

```
cat ~/export-esp.sh
export LIBCLANG_PATH="/home/hiromasa/.rustup/toolchains/esp/xtensa-esp32-elf-clang/esp-16.0.0-20230516/esp-clang/lib"
```

```
$ ls -laF ~/.rustup/toolchains/esp/
合計 28
drwxrwxr-x 7 hiromasa hiromasa 4096  2月 14 20:56 ./
drwxrwxr-x 9 hiromasa hiromasa 4096  2月 14 20:55 ../
drwxr-xr-x 2 hiromasa hiromasa 4096  2月 14 20:56 bin/
drwxr-xr-x 3 hiromasa hiromasa 4096  2月 14 20:56 etc/
drwxr-xr-x 3 hiromasa hiromasa 4096  2月 14 20:56 lib/
drwxr-xr-x 2 hiromasa hiromasa 4096  2月 14 20:56 libexec/
drwxr-xr-x 5 hiromasa hiromasa 4096  2月 14 20:56 share/
$ ~/.rustup/toolchains/esp/bin/rustc -V
rustc 1.71.0-nightly (4ca000ac8 2023-07-13) (1.71.0.1)
```

```
$ ls -laF ~/.espressif/tools/xtensa-esp32-elf-clang/
合計 12
drwxrwxr-x  3 hiromasa hiromasa 4096  2月 14  2023 ./
drwxrwxr-x 15 hiromasa hiromasa 4096  2月 28 22:14 ../
drwxrwxr-x  3 hiromasa hiromasa 4096  2月 14  2023 esp-15.0.0-20221201-x86_64-unknown-linux-gnu/
```

### Compile and Flash

1. git clone and build

```
git clone --recursive https://github.com/h1romas4/m5stack-chipstream
cd m5stack-chipstream
idf.py build
```

2. Restart M5Stack Core 2

```
idf.py flash monitor
```

## Dependencies

Thanks for all the open source.

|Name|Version|License|
|-|-|--|
|[esp-idf](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/get-started/index.html)|`v4.4.5`|BSD License|
|[arduino-esp32](https://github.com/espressif/arduino-esp32)|`2.0.11`|LGPL-2.1 License|
|[M5Core2](https://github.com/m5stack/M5Core2)|`0.1.6`|MIT License|
|[M5EPD](https://github.com/m5stack/M5EPD)|`0.1.5`|The FreeType License(FreeType Part)|
|[M5GFX](https://github.com/m5stack/M5GFX)|`0.1.9`|MIT license|
|[ymfm](https://github.com/aaronsgiles/ymfm)|`d641a806`|BSD-3-Clause license|
|[libymfm.wasm](https://github.com/h1romas4/libymfm.wasm)|`v0.16.0`|BSD-3-Clause license (include MAME's soundchip ports)|

## License

BSD-3-Clause License
