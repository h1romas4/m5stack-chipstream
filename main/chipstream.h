#include <stdint.h>

extern "C" {
uint8_t* cs_alloc_mem(uint32_t mem_id, uint32_t vgm_size);
void cs_drop_mem(uint32_t vgm_mem_id);
bool cs_create_vgm(uint32_t vgm_mem_id, uint32_t vgm_instance_id, uint32_t sample_rate, uint32_t sample_chunk_size);
int16_t* cs_stream_vgm(uint32_t vgm_instance_id, uint32_t *loop_count);
void cs_drop_vgm(uint32_t vgm_instance_id);
}
