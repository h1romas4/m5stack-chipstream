#include <stdint.h>

extern "C" {
void init_module_rca_i2s(uint32_t sample_rate, uint32_t dma_buf_len, uint32_t dma_buf_count);
void write_module_rca_i2s(int16_t *s16le, uint32_t len);
}
