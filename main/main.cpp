/**
 * ESP32 Xtensa esp-idf first approach Rust and C/C++ project example.
 *
 * It makes interface calls from clang to vgmplay and ymfm(C++), which are built in Rust.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>
#include <Arduino.h>
#include <M5Core2.h>
#include <esp_task_wdt.h>

#include "module_rca_i2s.h"
#include "chipstream.h"

static const char *TAG = "main.cpp";

/**
 * for testing
 */
#define VGM_FILE_NAME "/M5Stack/VGM/01.vgm"
#define CS_MEM_INDEX_ID 0
#define CS_VGM_INSTANCE_ID 0
#define DEBUG 0

/**
 * Audio settings
 */
#define STREO 2
#define SAPMLING_RATE 44100
#define SAMPLE_CHUNK_SIZE 256
#define SAMPLE_BUF_BYTES (SAMPLE_CHUNK_SIZE * STREO * sizeof(int16_t))
#define SAMPLE_BUF_COUNT 32
#define RING_BUF_BYTES (SAMPLE_BUF_BYTES * SAMPLE_BUF_COUNT)

/**
 * Handler
 */
TaskHandle_t task_i2s_write_handle = NULL;
RingbufHandle_t ring_buf_handle;

/**
 * Play state
 */
typedef enum {
  STRAT,
  PLAYING,
  BUFFERD,
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

    #if DEBUG
    ESP_LOGI(TAG, "written %d (%04x:%04x:%04x): render time: %d / %dms",
        SAMPLE_BUF_BYTES,
        (uint16_t)s16le[0],
        (uint16_t)s16le[1],
        (uint16_t)s16le[SAMPLE_CHUNK_SIZE - 1],
        SAMPLE_CHUNK_SIZE,
        (uint32_t)(millis() - time));
    #endif

    // stream to ring buffer (copy)
    UBaseType_t res = xRingbufferSend(
        ring_buf_handle,
        s16le,
        SAMPLE_BUF_BYTES,
        pdMS_TO_TICKS(1000));
    if(res != pdTRUE) {
        ESP_LOGE(TAG, "stream_vgm: failed to xRingbufferSend");
    }

    return loop_count;
}

/**
 * I2S write task
 */
void task_i2s_write(void *pvParameters)
{
    while(1) {
        if(vgm_state == vgm_state_t::PLAYING
            || vgm_state == vgm_state_t::BUFFERD) {
            size_t item_size;
            // wait sample (SAMPLE_BUF_BYTES)
            int16_t *s16le = (int16_t *)xRingbufferReceiveUpTo(
                ring_buf_handle,
                &item_size,
                pdMS_TO_TICKS(999),
                SAMPLE_BUF_BYTES);
            if(item_size == SAMPLE_BUF_BYTES) {
                // write i2s
                write_module_rca_i2s(s16le, SAMPLE_BUF_BYTES);
                // return item to ring buffer
                vRingbufferReturnItem(ring_buf_handle, (void *)s16le);
                continue;
            }
        }
        delay(1);
    }
}

/**
 * Arduino setup
 */
void setup(void)
{
    // M5Stack Core2 initialize
    M5.begin();

    // uninstall M5Stack Core2 initial I2S module
    i2s_driver_uninstall(I2S_NUM_0);

    // initialize Module RCA I2S
    init_module_rca_i2s(SAPMLING_RATE, SAMPLE_BUF_BYTES, SAMPLE_BUF_COUNT);

    // heap watch
    heap_caps_print_heap_info(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT);

    // create ring buffer
    ring_buf_handle = xRingbufferCreate(
        RING_BUF_BYTES,
        RINGBUF_TYPE_BYTEBUF);
    if(ring_buf_handle == nullptr) {
        ESP_LOGE(TAG, "Falied to create ring_buf_handle");
    }

    // create I2S write task
    xTaskCreateUniversal(
        task_i2s_write,
        "task_i2s_write",
        8192,
        NULL,
        configMAX_PRIORITIES - 1,
        &task_i2s_write_handle,
        CONFIG_ARDUINO_RUNNING_CORE);

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
            // pre buffre
            for(uint32_t i = 0; i < SAMPLE_BUF_COUNT; i++) {
               stream_vgm(CS_VGM_INSTANCE_ID);
            }
            vgm_state = vgm_state_t::PLAYING;
            break;
        case vgm_state_t::PLAYING:
            // stream
            if(stream_vgm(CS_VGM_INSTANCE_ID) > 0) {
                vgm_state = vgm_state_t::BUFFERD;
            }
            break;
        case vgm_state_t::BUFFERD:
            // TODO: flash ring buffer
            vgm_state = vgm_state_t::END;
            break;
        case vgm_state_t::END:
            // TODO: wait flash ring buffer
            cs_drop_vgm(CS_VGM_INSTANCE_ID);
            vgm_state = vgm_state_t::SLEEP;
            break;
        default:
            ESP_LOGI(TAG, "sleeping..zzz");
            delay(999);
            break;
    }
}
