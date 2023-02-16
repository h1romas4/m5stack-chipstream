#include <Arduino.h>
#include <M5Core2.h>

static const char *TAG = "main.cpp";

#include <stdio.h>

/**
 * chipstream interface test
 */
extern "C" void memory_alloc(uint32_t memory_index_id, uint32_t length);
extern "C" uint8_t* memory_get_ref(uint32_t memory_index_id);
extern "C" void memory_drop(uint32_t memory_index_id);
extern "C" bool vgm_create(uint32_t vgm_index_id, uint32_t output_sampling_rate, uint32_t output_sample_chunk_size, uint32_t memory_index_id);
extern "C" int16_t* vgm_get_sampling_s16le_ref(uint32_t vgm_index_id);
extern "C" uint32_t vgm_play(uint32_t vgm_index_id);
extern "C" void vgm_drop(uint32_t vgm_index_id);

#define CHIPSTREAM_MEMORY_INDEX_ID 0
#define CHIPSTREAM_VGM_INDEX_ID 0

void setup(void)
{
    M5.begin();

    // alloc vgm data (2Mbyte)
    memory_alloc(CHIPSTREAM_MEMORY_INDEX_ID, 2000000);
    uint8_t *vgm_alloc_address  = memory_get_ref(CHIPSTREAM_MEMORY_INDEX_ID);
    ESP_LOGI(TAG, "memory_get_ref(%p)", vgm_alloc_address);

    // create vgm instance
    bool vgm_result = vgm_create(CHIPSTREAM_VGM_INDEX_ID, 44100, 128, CHIPSTREAM_MEMORY_INDEX_ID);
    ESP_LOGI(TAG, "vgm_create(%d)", vgm_result);

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
