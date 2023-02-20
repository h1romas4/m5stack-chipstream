/**
 * ESP32 Xtensa esp-idf first approach Rust and C/C++ project example.
 *
 * It makes interface calls from clang to vgmplay and ymfm(C++), which are built in Rust.
 */
#include <Arduino.h>
#include <M5Core2.h>
#include <esp_task_wdt.h>

#include "module_rca_i2s.h"
#include "chipstream.h"

static const char *TAG = "main.cpp";

/**
 * for testing
 */
#define VGM_FILE_NAME "/M5Stack/VGM/05.vgm"
#define CS_MEM_INDEX_ID 0
#define CS_VGM_INSTANCE_ID 0

/**
 * Audio settings
 */
#define STREO 2
#define SAPMLING_RATE 44100
#define SAMPLE_CHUNK_SIZE 128
#define SAMPLE_BUF_LEN (SAMPLE_CHUNK_SIZE * STREO * sizeof(int16_t))
#define I2S_BUF_LEN 16

/**
 * Play state
 */
typedef enum {
  STRAT,
  PLAYING,
  END,
  SLEEP
} vgm_state_t;

vgm_state_t vgm_state;

/**
 * load_sd_vgm_file
 */
void load_sd_vgm_file(const char *filename, uint32_t vgm_instance_id, uint32_t vgm_mem_id)
{
    // SD open
    File fp = SD.open(filename);
    size_t vgm_size = fp.size();
    ESP_LOGI(TAG, "vgm file(%d)", vgm_size);

    // alloc vgmfile mem
    uint8_t* mem = cs_alloc_mem(CS_MEM_INDEX_ID, vgm_size);

    // load vgm from SD
    size_t read_vgm_size = fp.read(mem, vgm_size);
    ESP_LOGI(TAG, "read vgm file(%d)", read_vgm_size);
    fp.close();
    if(vgm_size != read_vgm_size) {
        ESP_LOGE(TAG, "read vgm error(%d)", read_vgm_size);
    }

    // create vgm instance
    cs_create_vgm(
        vgm_mem_id,
        vgm_instance_id,
        SAPMLING_RATE,
        SAMPLE_CHUNK_SIZE);

    // drop vgmfile mem
    // vgm data is cloned and decoded by vgm instance from vgmfile
    cs_drop_mem(CS_MEM_INDEX_ID);
}

/**
 * stream_vgm
 */
uint32_t stream_vgm(uint32_t vgm_instance_id) {
    /**
     * M5Stack Core2 (ESP32 with 40MHz PSRAM) Test Result
     *
     * ymfm YM2151(on Flash) and X68K OKI
     *  I (4365) main.cpp: render time: 44100 / 2001ms (*Takes x2 as long as realtime)
     *  I (6540) main.cpp: render time: 44100 / 2105ms
     * ymfm YM2151(on IRAM) and X68K OKI
     *  I (4356) main.cpp: render time: 44100 / 1988ms (*Takes x2 as long as realtime)
     *  I (6521) main.cpp: render time: 44100 / 2094ms
     * only X68K OKI
     *  I (2873) main.cpp: render time: 44100 / 522ms  (*This speed is well in time)
     *  I (3443) main.cpp: render time: 44100 / 501ms
     *
     * The time is about 400ms shorter for the ESP32-S3 with PSRAM set to 80 MHz Octa.
     */
    uint32_t time = millis();

    uint32_t loop_count;
    int16_t *s16le = cs_stream_vgm(vgm_instance_id, &loop_count);

    // TODO: for test (Waveform generation and I2S playback should be separated)
    write_module_rca_i2s(s16le, SAMPLE_BUF_LEN);

    ESP_LOGI(TAG, "written %d (%04x): render time: %d / %dms",
        SAMPLE_BUF_LEN,
        (uint16_t)s16le[0],
        SAMPLE_CHUNK_SIZE,
        (uint32_t)(millis() - time));

    return loop_count;
}

/**
 * Arduino setup
 */
void setup(void)
{
    // M5Stack Core2 initialize
    M5.begin();

    // uninstall initial I2S module
    i2s_driver_uninstall(I2S_NUM_0);

    // initialize Module RCA I2S
    init_module_rca_i2s(SAPMLING_RATE, SAMPLE_BUF_LEN, I2S_BUF_LEN);

    // heap wwatch
    heap_caps_print_heap_info(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT);

    // set state
    vgm_state = vgm_state_t::STRAT;
}

/**
 * Arduino loop
 */
void loop(void)
{
    M5.update();

    switch (vgm_state) {
        case vgm_state_t::STRAT:
            // load and init vgm instance
            load_sd_vgm_file(
                VGM_FILE_NAME,
                CS_VGM_INSTANCE_ID,
                CS_MEM_INDEX_ID);
            vgm_state = vgm_state_t::PLAYING;
            break;
        case vgm_state_t::PLAYING:
            // stream
            if(stream_vgm(CS_VGM_INSTANCE_ID) > 0) {
                vgm_state = vgm_state_t::END;
            }
            break;
        case vgm_state_t::END:
            cs_drop_vgm(CS_VGM_INSTANCE_ID);
            vgm_state = vgm_state_t::SLEEP;
            break;
        default:
            ESP_LOGI(TAG, "sleeping..zzz");
            delay(999);
            break;
    }
    vTaskDelay(1);
}
