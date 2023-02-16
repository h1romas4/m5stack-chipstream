# chipstream-esp32

## Build

### Require

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

### Build and Execute

1. git clone and build

```
git clone --recursive https://github.com/h1romas4/m5stack-chipstream
cd m5stack-chipstream
idf.py build
```

2. Write Partition table

```
idf.py partition-table-flash
```

3. Write TypeType font to SPIFFS

```
parttool.py write_partition --partition-name=font --partition-subtype=spiffs --input resources/spiffs_font.bin
```

4. Restart M5Stack Core 2

```
idf.py flash monitor
```

## Note

- Create SPIFFS parteation file

```
# for TrueType font
python ${IDF_PATH}/components/spiffs/spiffsgen.py 0x100000 resources/font resources/spiffs_font.bin
```

## Dependencies

Thanks for all the open source.

|Name|Version|License|
|-|-|--|
|[esp-idf](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/get-started/index.html)|`v4.4.3`|BSD License|
|[arduino-esp32](https://github.com/espressif/arduino-esp32)|`2.0.6`|LGPL-2.1 License|
|[M5Core2](https://github.com/m5stack/M5Core2)|`0.1.5`|MIT License|
|[M5EPD](https://github.com/m5stack/M5EPD)|`0.1.4`|FreeType Part(The FreeType License)|
|[M5GFX](https://github.com/m5stack/M5GFX)|`0.1.4`|FreeType Part(The FreeType License)|
|[源真ゴシック](http://jikasei.me/font/genshin/)|-|SIL Open Font License 1.1|

## License

MIT License
