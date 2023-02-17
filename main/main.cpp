#include <Arduino.h>
#include <M5Core2.h>
#include <esp_task_wdt.h>

static const char *TAG = "main.cpp";

/**
 * chipstream interface test
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
#define CHIPSTREAM_SAMPLE_CHUNK_SIZE 44100

void setup(void)
{
    M5.begin();

    // workaround disable watchdog
    esp_task_wdt_init(3600, false);

    // SD open
    File fp = SD.open("/M5Stack/VGM/05.vgm");
    size_t vgm_size = fp.size();
    ESP_LOGI(TAG, "vgm file(%d)", vgm_size);

    // alloc vgm data
    memory_alloc(CHIPSTREAM_MEMORY_INDEX_ID, vgm_size);
    uint8_t *vgm_alloc_address  = memory_get_ref(CHIPSTREAM_MEMORY_INDEX_ID);
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
        CHIPSTREAM_VGM_INDEX_ID,
        CHIPSTREAM_SAPMLING_RATE,
        CHIPSTREAM_SAMPLE_CHUNK_SIZE,
        CHIPSTREAM_MEMORY_INDEX_ID);
    ESP_LOGI(TAG, "vgm_create(%d)", vgm_result);

    // get json
    if(vgm_result) {
        uint32_t memory_index_id = vgm_get_gd3_json(CHIPSTREAM_VGM_INDEX_ID);
        uint8_t *vgm_header_json  = memory_get_ref(memory_index_id);
        uint32_t vgm_heade_json_len = memory_get_len(memory_index_id);
        uint8_t *vgm_header = (uint8_t *)calloc(vgm_heade_json_len + 1, sizeof(uint8_t));
        strncpy((char *)vgm_header, (const char *)vgm_header_json, vgm_heade_json_len);
        ESP_LOGI(TAG, "vgm_create(%s)", vgm_header);
        free(vgm_header);
        memory_drop(memory_index_id);
    }

    // play (WIP but too slow..)
    if(vgm_result) {
        // loop one in vgm
        uint32_t loop_count = 0;
        while(loop_count == 0) {
            // I (2462) main.cpp: render time: 4096 / 168ms
            // I (2622) main.cpp: render time: 4096 / 157ms
            uint32_t time = millis();
            loop_count = vgm_play(CHIPSTREAM_VGM_INDEX_ID);
            ESP_LOGI(TAG, "render time: %d / %dms", CHIPSTREAM_SAMPLE_CHUNK_SIZE, (uint32_t)(millis() - time));

            int16_t* s16le = vgm_get_sampling_s16le_ref(CHIPSTREAM_VGM_INDEX_ID);
            // TODO: output i2s
        }
    }

    // drop vgm instance
    if(vgm_result) {
        vgm_drop(CHIPSTREAM_VGM_INDEX_ID);
    }

    // drop vgm data
    memory_drop(CHIPSTREAM_MEMORY_INDEX_ID);
}

void loop(void)
{
    M5.update();

    ESP_LOGI(TAG, "Hello, M5Stack Core2 world.");

    delay(500);
}
