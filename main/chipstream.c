#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <esp_log.h>

/**
 * Rust chipstream(vgmplay) interface
 *
 * Be sure to call them from the same thread,
 * and the stack size should be 64KB or larger.
 *
 * Originally, bindgen is used to generate them.
 */
extern uint32_t vgm_create(
    uint32_t vgm_index_id,
    uint32_t output_sampling_rate,
    uint32_t output_sample_chunk_size,
    uint32_t memory_index_id);
extern uint32_t vgm_get_gd3_json(uint32_t vgm_index_id);
extern int16_t* vgm_get_sampling_s16le_ref(uint32_t vgm_index_id);
extern void vgm_get_sampling_s16le(uint32_t vgm_index_id, int16_t *s16le);
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
bool cs_create_vgm(
    uint32_t vgm_mem_id,
    uint32_t vgm_instance_id,
    uint32_t sample_rate,
    uint32_t sample_chunk_size)
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
 * Generate waveform for test
 *
 * This is a stub that generates square waves at high speed
 * instead of chipstream waveform operations.
 *
 * This stub is used to check if the slowdown in playback
 * is due to a delay in waveform generation or buffer control.
 */
uint32_t vgm_get_sampling_stub(
    uint32_t sample_rate,
    uint32_t sample_chunk_size,
    uint32_t square_hz,
    int16_t *s16le)
{
    static uint32_t sample_pos = 0;
    static uint32_t time = 0;

    uint32_t period = (uint32_t)(sample_rate / square_hz);
    uint32_t half_period = period / 2;

    /**
     * singed int square wave generation
     */
    for (uint32_t i = 0; i < sample_chunk_size; i++) {
        uint32_t j = sample_pos % period;
        if (j < half_period) {
            s16le[i * 2] = 32767;
            s16le[i * 2 + 1] = 32767;
        } else {
            s16le[i * 2] = -32768;
            s16le[i * 2 + 1] = -32768;
        }
        sample_pos++;
        if(sample_pos >= sample_rate) {
            sample_pos = 0;
            time++;
        }
    }

    // loop count (10 sec)
    if(time > 10) {
        sample_pos = 0;
        time = 0;
        return 1;
    }
    return 0;
}

/**
 * Tick and stream vgmplay instance by sample_chunk_size
 */
void cs_stream_vgm(uint32_t vgm_instance_id, int16_t *s16le, uint32_t *loop_count)
{
    *loop_count = vgm_play(vgm_instance_id);
    vgm_get_sampling_s16le(vgm_instance_id, s16le);

    // To perform a sound test, disable the top two lines and enable the bottom line.
    // *loop_count = vgm_get_sampling_stub(44100, 256, 440, s16le);
}

/**
 * Tick and stream vgmplay instance by sample_chunk_size
 */
int16_t* cs_stream_vgm_ref(uint32_t vgm_instance_id, uint32_t *loop_count)
{
    // TODO: There is a memory layout discrepancy between Rust and C.
    // Pattern 1 (normal)
    // I (2313) main.cpp: written 512 (8000:8000:8000): render time: 128 / 5ms
    // Pattern 2 (out of alignment)
    // I (2313) main.cpp: written 512 (7fff:8000:8000): render time: 128 / 5ms
    *loop_count = vgm_play(vgm_instance_id);
    return (int16_t *)vgm_get_sampling_s16le;
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
uint8_t* cs_alloc_mem(uint32_t cs_mem_id, uint32_t vgm_size)
{
    // alloc chipstream memory
    memory_alloc(cs_mem_id, vgm_size);

    return (uint8_t *)memory_get_ref(cs_mem_id);
}

/**
 * Drop memory on chipstream
 */
void cs_drop_mem(uint32_t cs_mem_id)
{
    memory_drop(cs_mem_id);
}
