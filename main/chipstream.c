#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <esp_log.h>

/**
 * Rust chipstream(vgmplay) interface
 *
 * Originally, bindgen is used to generate them.
 */
extern uint32_t vgm_create(uint32_t vgm_index_id, uint32_t output_sampling_rate, uint32_t output_sample_chunk_size, uint32_t memory_index_id);
extern uint32_t vgm_get_gd3_json(uint32_t vgm_index_id);
extern int16_t* vgm_get_sampling_s16le_ref(uint32_t vgm_index_id);
extern uint32_t vgm_play(uint32_t vgm_index_id);
extern void vgm_drop(uint32_t vgm_index_id);
extern void memory_alloc(uint32_t memory_index_id, uint32_t length);
extern uint8_t* memory_get_ref(uint32_t memory_index_id);
extern uint32_t memory_get_len(uint32_t memory_index_id);
extern void memory_drop(uint32_t memory_index_id);

static const char *TAG = "chipstream.c";

/**
 * Create chipstream vgmplay instance
 */
bool cs_create_vgm(uint32_t vgm_mem_id, uint32_t vgm_instance_id, uint32_t sample_rate, uint32_t sample_chunk_size)
{
    // create vgm instance
    bool vgm_result = vgm_create(
        vgm_instance_id,
        sample_rate,
        sample_chunk_size,
        vgm_mem_id);
    ESP_LOGI(TAG, "vgm_create(%d)", vgm_result);

    // get json (test)
    if(vgm_result) {
        uint32_t memory_index_id = vgm_get_gd3_json(vgm_instance_id);
        uint8_t *vgm_header_json  = memory_get_ref(memory_index_id);
        uint32_t vgm_heade_json_len = memory_get_len(memory_index_id);
        uint8_t *vgm_header = (uint8_t *)calloc(vgm_heade_json_len + 1, sizeof(uint8_t));
        strncpy((char *)vgm_header, (const char *)vgm_header_json, vgm_heade_json_len);
        ESP_LOGI(TAG, "vgm_create(%s)", vgm_header);
        free(vgm_header);
        memory_drop(memory_index_id);
    }

    return (bool)vgm_result;
}

/**
 * Tick and stream vgmplay instance by sample_chunk_size
 */
int16_t* cs_stream_vgm(uint32_t vgm_instance_id, uint32_t *loop_count)
{
    *loop_count = vgm_play(vgm_instance_id);
    return vgm_get_sampling_s16le_ref(vgm_instance_id);
}

/**
 * Drop vgmplay instance
 */
void cs_drop_vgm(uint32_t vgm_instance_id)
{
    // drop vgm instance
    vgm_drop(vgm_instance_id);
}

/**
 * Alloc memory on chipstream
 */
uint8_t* cs_alloc_mem(uint32_t vgm_mem_id, uint32_t vgm_size)
{
    // alloc chipstream memory
    memory_alloc(vgm_mem_id, vgm_size);

    return (uint8_t *)memory_get_ref(vgm_mem_id);
}

/**
 * Drop memory on chipstream
 */
void cs_drop_mem(uint32_t vgm_mem_id)
{
    memory_drop(vgm_mem_id);
}