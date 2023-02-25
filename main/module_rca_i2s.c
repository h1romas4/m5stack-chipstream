#include <esp_err.h>
#include <esp_log.h>
#include <driver/i2s.h>

static const char *TAG = "module_rca_i2s.c";

/**
 * Module RCA I2S(PCM5102APWR) initilize
 *
 *  Note that it should be set to I2S_COMM_FORMAT_STAND_I2S.
 *  In I2S_COMM_FORMAT_STAND_MSB, the first bit of PCM is shifted by 1 bit,
 *  resulting in the first 1 bit of PCM being ignored.
 */
void init_module_rca_i2s(uint32_t sample_rate, uint32_t dma_buf_len, uint32_t dma_buf_count)
{
    // i2s_driver_install
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = dma_buf_count,
        .dma_buf_len = dma_buf_len,
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
 * clear_dma_module_rca_i2s
 */
void clear_dma_module_rca_i2s(void)
{
    i2s_zero_dma_buffer(I2S_NUM_1);
}

/**
 * Module RCA I2S write
 */
void write_module_rca_i2s(int16_t *s16le, uint32_t bytes)
{
    size_t written = 0;
    ESP_ERROR_CHECK(i2s_write(I2S_NUM_1, s16le, bytes, &written, portMAX_DELAY));
    if(bytes != written) {
        ESP_LOGE(TAG, "i2s_write error: %d / %d", written, bytes);
    }
}
