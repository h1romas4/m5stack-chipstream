/**
 * ESP32 Xtensa esp-idf first approach Rust and C/C++ project example.
 *
 * It makes interface calls from clang to vgmplay and ymfm(C++), which are built in Rust.
 */
#include <Arduino.h>
#include <M5Core2.h>
#include <esp_task_wdt.h>
#include <driver/i2s.h>

static const char *TAG = "main.cpp";

/**
 * chipstream(vgmplay) interface test
 *
 * Originally, bindgen is used to generate them.
 */
extern "C" void memory_alloc(uint32_t memory_index_id, uint32_t length);
extern "C" uint8_t* memory_get_ref(uint32_t memory_index_id);
extern "C" uint32_t memory_get_len(uint32_t memory_index_id);
extern "C" void memory_drop(uint32_t memory_index_id);
extern "C" bool vgm_create(uint32_t vgm_index_id, uint32_t output_sampling_rate, uint32_t output_sample_chunk_size, uint32_t memory_index_id);
extern "C" uint32_t vgm_get_gd3_json(uint32_t vgm_index_id);
extern "C" int16_t* vgm_get_sampling_s16le_ref(uint32_t vgm_index_id);
extern "C" uint32_t vgm_play(uint32_t vgm_index_id);
extern "C" void vgm_drop(uint32_t vgm_index_id);

#define CHIPSTREAM_MEMORY_INDEX_ID 0
#define CHIPSTREAM_VGM_INDEX_ID 0
#define CHIPSTREAM_SAPMLING_RATE 44100
#define CHIPSTREAM_SAMPLE_CHUNK_SIZE 128

#define STREO 2
#define SAMPLE_BUF_LEN (CHIPSTREAM_SAMPLE_CHUNK_SIZE * STREO * sizeof(int16_t))

/**
 * Module RCA I2S initilize
 */
void init_module_rca_i2s(void)
{
    // i2s_driver_install
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = CHIPSTREAM_SAPMLING_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = SAMPLE_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = I2S_PIN_NO_CHANGE
    };
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL));

    // i2s_set_pin
    i2s_pin_config_t i2s_pin_config = {
        .mck_io_num = GPIO_NUM_0,
        .bck_io_num = GPIO_NUM_19,
        .ws_io_num = GPIO_NUM_0,
        .data_out_num = GPIO_NUM_2,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_1, &i2s_pin_config));
}

/**
 * initalize vgm
 */
esp_err_t init_vgm(const char* filename, uint32_t instance_id, uint32_t mem_id)
{
    // SD open
    File fp = SD.open(filename);
    size_t vgm_size = fp.size();
    ESP_LOGI(TAG, "vgm file(%d)", vgm_size);

    // alloc vgm data
    memory_alloc(mem_id, vgm_size);
    uint8_t *vgm_alloc_address  = memory_get_ref(mem_id);
    ESP_LOGI(TAG, "memory_get_ref(%p)", vgm_alloc_address);

    // load vgm
    size_t read_vgm_size = fp.read(vgm_alloc_address, vgm_size);
    ESP_LOGI(TAG, "read vgm file(%d)", read_vgm_size);
    fp.close();
    if(vgm_size != read_vgm_size) {
        ESP_LOGE(TAG, "read vgm error(%d)", read_vgm_size);
    }

    // heap wwatch
    heap_caps_print_heap_info(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT);

    // create vgm instance
    bool vgm_result = vgm_create(
        instance_id,
        CHIPSTREAM_SAPMLING_RATE,
        CHIPSTREAM_SAMPLE_CHUNK_SIZE,
        mem_id);
    ESP_LOGI(TAG, "vgm_create(%d)", vgm_result);

    // drop vgm data (clone by vgm_create)
    memory_drop(mem_id);

    // get json
    if(vgm_result) {
        uint32_t memory_index_id = vgm_get_gd3_json(instance_id);
        uint8_t *vgm_header_json  = memory_get_ref(memory_index_id);
        uint32_t vgm_heade_json_len = memory_get_len(memory_index_id);
        uint8_t *vgm_header = (uint8_t *)calloc(vgm_heade_json_len + 1, sizeof(uint8_t));
        strncpy((char *)vgm_header, (const char *)vgm_header_json, vgm_heade_json_len);
        ESP_LOGI(TAG, "vgm_create(%s)", vgm_header);
        free(vgm_header);
        memory_drop(memory_index_id);
    }

    if(!vgm_result) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

void stream_vgm(uint32_t instance_id) {
    /**
     * M5Stack Core2 (ESP32) Test Result
     *
     * ymfm YM2151(on Flash) and X68K OKI
     *  I (4365) main.cpp: render time: 44100 / 2001ms (*Takes x2 as long as real time)
     *  I (6540) main.cpp: render time: 44100 / 2105ms
     * ymfm YM2151(on IRAM) and X68K OKI
     *  I (4356) main.cpp: render time: 44100 / 1988ms (*Takes x2 as long as real time)
     *  I (6521) main.cpp: render time: 44100 / 2094ms
     * only X68K OKI
     *  I (2873) main.cpp: render time: 44100 / 522ms  (*This speed is well in time)
     *  I (3443) main.cpp: render time: 44100 / 501ms
     *
     * The time is about 400ms shorter for the ESP32-S3 with PSRAM set to 80 MHz Octa.
     */
    uint32_t time = millis();

    uint32_t loop_count = vgm_play(instance_id);
    int16_t *s16le = vgm_get_sampling_s16le_ref(instance_id);
    // output i2s (test)
    size_t written = 0;
    ESP_ERROR_CHECK(i2s_write(I2S_NUM_1, s16le, SAMPLE_BUF_LEN, &written, 0));

    ESP_LOGI(TAG, "written %d / %d (%04x): render time: %d / %dms",
        written,
        SAMPLE_BUF_LEN,
        (uint16_t)s16le[0],
        CHIPSTREAM_SAMPLE_CHUNK_SIZE,
        (uint32_t)(millis() - time));
}

void drop_vgm(uint32_t instance_id)
{
    // drop vgm instance
    vgm_drop(CHIPSTREAM_VGM_INDEX_ID);
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

    // initialize module RCA I2S
    init_module_rca_i2s();

    // workaround disable watchdog
    esp_task_wdt_init(3600, false);

    // init vgm
    ESP_ERROR_CHECK(
        init_vgm("/M5Stack/VGM/05.vgm",
        CHIPSTREAM_VGM_INDEX_ID,
        CHIPSTREAM_MEMORY_INDEX_ID));

    // stream (test 1 tick)
    stream_vgm(CHIPSTREAM_VGM_INDEX_ID);

    // drop
    drop_vgm(CHIPSTREAM_VGM_INDEX_ID);
}

/**
 * Arduino loop
 */
void loop(void)
{
    M5.update();

    ESP_LOGI(TAG, "Hello, M5Stack Core2 world.");

    delay(500);
}
