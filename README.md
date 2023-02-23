#  m5stack-chipstream (WIP)

![](https://github.com/h1romas4/m5stack-chipstream/workflows/Build/badge.svg)

This is a test to port [C++'s ymfm and Rust's vgmplay](https://github.com/h1romas4/libymfm.wasm) to ESP32(Xtensa).

The repository is a work in progress. Still slow and impractical.

- [M5Stack Core2](https://docs.m5stack.com/en/core/core2)
- [M5Stack RCA Module (I2S)](https://docs.m5stack.com/ja/module/RCA%20Module%2013.2)

## TODO

- [ ] The transfer of PCM waveforms from Rust to C is shifted depending on the memory state. (Is it due to alignment?)
- [ ] There is no interface to stop playback.

## Build

@see [.github/workflows/build.yml](https://github.com/h1romas4/m5stack-chipstream/blob/main/.github/workflows/build.yml)

### Setup

- [Setup ESF-IDF v4.4.3](https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32/get-started/index.html#installation-step-by-step)

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
export LIBCLANG_PATH="/home/hiromasa/.espressif/tools/xtensa-esp32-elf-clang/esp-15.0.0-20221201-x86_64-unknown-linux-gnu/esp-clang/lib"
export CLANG_PATH="/home/hiromasa/.espressif/tools/xtensa-esp32-elf-clang/esp-15.0.0-20221201-x86_64-unknown-linux-gnu/esp-clang/bin/clang"
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
rustc 1.67.0-nightly (725e31c21 2023-01-25)
```

```
$ ls -laF ~/.espressif/tools/xtensa-esp32-elf-clang/
合計 12
drwxrwxr-x  3 hiromasa hiromasa 4096  2月 14 20:58 ./
drwxrwxr-x 13 hiromasa hiromasa 4096  2月 14 20:58 ../
drwxrwxr-x  3 hiromasa hiromasa 4096  2月 14 20:58 esp-15.0.0-20221201-x86_64-unknown-linux-gnu/
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
|[esp-idf](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/get-started/index.html)|`v4.4.3`|BSD License|
|[arduino-esp32](https://github.com/espressif/arduino-esp32)|`2.0.6`|LGPL-2.1 License|
|[M5Core2](https://github.com/m5stack/M5Core2)|`0.1.5`|MIT License|
|[M5EPD](https://github.com/m5stack/M5EPD)|`0.1.4`|The FreeType License(FreeType Part)|
|[M5GFX](https://github.com/m5stack/M5GFX)|`0.1.4`|MIT license|
|[ymfm](https://github.com/aaronsgiles/ymfm)|`d641a806`|BSD-3-Clause license|
|[libymfm.wasm](https://github.com/h1romas4/libymfm.wasm)|`v0.16.0`|BSD-3-Clause license (include MAME's soundchip ports)|

## License

BSD-3-Clause License
