/**
 * ESP32 Xtensa esp-idf first approach Rust and C/C++ project example.
 *
 * It makes interface calls from clang to vgmplay and ymfm(C++), which are built in Rust.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>
#include <freertos/queue.h>
#include <Arduino.h>
#include <M5Core2.h>
#include <esp_task_wdt.h>

#include "module_rca_i2s.h"
#include "chipstream.h"

static const char *TAG = "main.cpp";

/**
 * for testing
 */
#define DEBUG 0
#define VGM_FILE_NAME "/M5Stack/VGM/12.vgm"
#define CS_MEM_INDEX_ID 0
#define CS_VGM_INSTANCE_ID 0

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
 * System settings
 */
#define CS_TASK_STACK_SIZE 65535
#define IS2_TASK_STACK_SIZE 8192
#define MESSAGE_QUEUE_SIZE 10

/**
 * Handler
 */
TaskHandle_t task_i2s_write_handle;
TaskHandle_t task_cs_handle;
RingbufHandle_t ring_buf_handle;
QueueHandle_t queue_cs_command_handle;
QueueHandle_t queue_cs_state_handle;

/**
 * chipstream messega queue
 */
typedef enum {
    CS_CMD_LOAD,
    CS_CMD_FILL_BUFFER,
    CS_CMD_STREAM,
    CS_CMD_DROP
} cs_command_t;

typedef struct cs_command_message {
    cs_command_t cs_command;
    uint32_t vgm_instance_id;
    uint32_t vgm_mem_id;
    const char* filename;
    uint32_t loop_max_count;
} cs_command_message_t;

typedef struct cs_state_message {
    cs_command_t cs_state;
    uint32_t remain_chunk_count;
} cs_state_message_t;

/**
 * Player state
 */
typedef enum {
  START,
  PLAYING,
  BUFFERD,
  END,
  SLEEP
} player_state_t;

player_state_t player_state;

/**
 * load_sd_vgm_file
 */
void load_sd_vgm_file(
    uint32_t vgm_instance_id,
    uint32_t vgm_mem_id,
    const char *filename)
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
    #if DEBUG
    uint32_t time = millis();
    #endif

    uint32_t loop_count;

    // int16_t *s16le = cs_stream_vgm_ref(vgm_instance_id, &loop_count);
    int16_t s16le[SAMPLE_BUF_BYTES / 2];
    cs_stream_vgm(vgm_instance_id, s16le, &loop_count);

    #if DEBUG
    ESP_LOGI(TAG, "written %d (%04x:%04x:%04x): render time: %d / %dms",
        SAMPLE_BUF_BYTES,
        (uint16_t)s16le[0],
        (uint16_t)s16le[1],
        (uint16_t)s16le[SAMPLE_CHUNK_SIZE - 1],
        SAMPLE_CHUNK_SIZE,
        (uint32_t)(millis() - time));
    #endif

    // stream copy to ring buffer (block if buffer is filled)
    UBaseType_t res = xRingbufferSend(
        ring_buf_handle,
        s16le,
        SAMPLE_BUF_BYTES,
        portMAX_DELAY);
    if(res != pdTRUE) {
        ESP_LOGE(TAG, "stream_vgm: failed to xRingbufferSend");
    }

    return loop_count;
}

/**
 * chipstream task (core 0)
 */
void task_cs(void *pvParameters)
{
    // disable core 0 watch dog
    #if !DEBUG
    esp_task_wdt_init(3600, false);
    #endif

    cs_command_message_t cmd;

    while(1) {
        // wait command queue (block)
        if(xQueueReceive(
            queue_cs_command_handle, &cmd, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "xQueueReceive");
            continue;
        }
        cs_state_message_t state;
        switch (cmd.cs_command) {
            case cs_command_t::CS_CMD_LOAD:
                // init cs and load vgm
                load_sd_vgm_file(
                    cmd.vgm_instance_id,
                    cmd.vgm_mem_id,
                    cmd.filename
                );
                // return state
                state.cs_state = cmd.cs_command;
                xQueueSend(
                    queue_cs_state_handle,
                    &state,
                    portMAX_DELAY);
                // wait for next command
                continue;
            case cs_command_t::CS_CMD_FILL_BUFFER:
                // fill buffer
                for(uint32_t i = 0; i < SAMPLE_BUF_COUNT; i++) {
                    stream_vgm(cmd.vgm_instance_id);
                }
                // return state
                state.cs_state = cmd.cs_command;
                xQueueSend(
                    queue_cs_state_handle,
                    &state,
                    portMAX_DELAY);
                // wait for next command
                continue;
            case cs_command_t::CS_CMD_STREAM:
                // stream (TODO: loop count)
                while(stream_vgm(cmd.vgm_instance_id) == 0);
                // return state
                state.cs_state = cmd.cs_command;
                xQueueSend(
                    queue_cs_state_handle,
                    &state,
                    portMAX_DELAY);
                // wait for next command
                break;
            case cs_command_t::CS_CMD_DROP:
                // drop instance
                cs_drop_vgm(cmd.vgm_instance_id);
                // return state
                state.cs_state = cmd.cs_command;
                xQueueSend(
                    queue_cs_state_handle,
                    &state,
                    portMAX_DELAY);
                // wait for next command
                break;
            default:
                ESP_LOGE(TAG, "not yet impliments");
                continue;;
        }

        // play loop
        delay(1);
    }
}

/**
 * I2S write task (core 1)
 */
void task_i2s_write(void *pvParameters)
{
    while(1) {
        if(player_state == player_state_t::PLAYING
            || player_state == player_state_t::BUFFERD) {
            size_t item_size;
            // wait sample SAMPLE_BUF_BYTES (block)
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
 * transmit_receive_cs_command
 */
bool transmit_receive_cs_command(
    cs_command_message_t cmd,
    cs_state_message_t msg)
{
    xQueueSend(queue_cs_command_handle, &cmd, portMAX_DELAY);
    if(xQueueReceive(queue_cs_state_handle, &msg, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(TAG, "transmit_receive_cs_command xQueueReceive");
        return false;
    }
    if(cmd.cs_command != msg.cs_state) {
        ESP_LOGE(TAG, "transmit_receive_cs_command cs_command");
        return false;
    }

    return true;
}

/**
 * send_cs_command_drop
 */
void send_cs_command_drop(uint32_t vgm_instance_id)
{
    // create init command
    cs_command_message_t cmd;
    cmd.cs_command = cs_command_t::CS_CMD_DROP;
    cmd.vgm_instance_id = vgm_instance_id;
    // receive state
    cs_state_message_t state;
    transmit_receive_cs_command(cmd, state);
    // TODO: if state
}

/**
 * send_cs_command_stream
 */
void send_cs_command_stream(uint32_t vgm_instance_id)
{
    // create init command
    cs_command_message_t cmd;
    cmd.cs_command = cs_command_t::CS_CMD_STREAM;
    cmd.vgm_instance_id = vgm_instance_id;
    // TODO: do not wait receive state
    cs_state_message_t state;
    transmit_receive_cs_command(cmd, state);
    // TODO: if state
}

/**
 * send_cs_command_buffer
 */
void send_cs_command_buffer(uint32_t vgm_instance_id)
{
    // create init command
    cs_command_message_t cmd;
    cmd.cs_command = cs_command_t::CS_CMD_FILL_BUFFER;
    cmd.vgm_instance_id = vgm_instance_id;
    // receive state
    cs_state_message_t state;
    transmit_receive_cs_command(cmd, state);
    // TODO: if state
}

/**
 * send_cs_command_init
 */
void send_cs_command_init(
    uint32_t vgm_instance_id,
    uint32_t vgm_mem_id,
    const char * filename,
    uint32_t loop_max_count)
{
    // create init command
    cs_command_message_t cmd;
    cmd.cs_command = cs_command_t::CS_CMD_LOAD;
    cmd.vgm_instance_id = vgm_instance_id;
    cmd.vgm_mem_id = vgm_mem_id;
    cmd.filename = filename;
    cmd.loop_max_count = loop_max_count;
    // receive state
    cs_state_message_t state;
    transmit_receive_cs_command(cmd, state);
    // TODO: if state
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
    init_module_rca_i2s(
        SAPMLING_RATE,
        SAMPLE_BUF_BYTES,
        SAMPLE_BUF_COUNT);

    // create ring buffer
    ring_buf_handle = xRingbufferCreate(
        RING_BUF_BYTES,
        RINGBUF_TYPE_BYTEBUF);
    if(ring_buf_handle == nullptr) {
        ESP_LOGE(TAG, "Falied to create ring_buf_handle");
    }

    // create message queue
    queue_cs_command_handle = xQueueCreate(
        MESSAGE_QUEUE_SIZE,
        sizeof(struct cs_command_message));
    queue_cs_state_handle = xQueueCreate(
        MESSAGE_QUEUE_SIZE,
        sizeof(struct cs_state_message));

    // create chipstream task on ESP32 core 0
    xTaskCreateUniversal(
        task_cs,
        "task_cs",
        CS_TASK_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 1,
        &task_cs_handle,
        PRO_CPU_NUM);

    // create I2S write task on ESP32 core 1
    xTaskCreateUniversal(
        task_i2s_write,
        "task_i2s_write",
        IS2_TASK_STACK_SIZE,
        NULL,
        2,
        &task_i2s_write_handle,
        CONFIG_ARDUINO_RUNNING_CORE);

    // heap watch
    heap_caps_print_heap_info(
        MALLOC_CAP_8BIT |
        MALLOC_CAP_INTERNAL |
        MALLOC_CAP_DEFAULT);

    // set state
    player_state = player_state_t::START;
}

/**
 * Arduino loop
 */
void loop(void)
{
    M5.update();

    switch (player_state) {
        case player_state_t::START:
            // load and init vgm instance
            send_cs_command_init(
                CS_VGM_INSTANCE_ID,
                CS_MEM_INDEX_ID,
                VGM_FILE_NAME,
                0);
            // fill buffre
            send_cs_command_buffer(CS_VGM_INSTANCE_ID);
            player_state = player_state_t::PLAYING;
            break;
        case player_state_t::PLAYING:
            // stream (TODO: do not blocked)
            send_cs_command_stream(CS_VGM_INSTANCE_ID);
            player_state = player_state_t::BUFFERD;
            break;
        case player_state_t::BUFFERD:
            // TODO: wait flash ring buffer
            player_state = player_state_t::END;
            break;
        case player_state_t::END:
            send_cs_command_drop(CS_VGM_INSTANCE_ID);
            player_state = player_state_t::SLEEP;
            break;
        default:
            ESP_LOGI(TAG, "sleeping..zzz");
            delay(1000);
            break;
    }
}
