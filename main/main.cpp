#include <Arduino.h>
#include <M5GFX.h>

static const char *TAG = "main.cpp";

/**
 * M5GFX member
 */
M5GFX display;

void setup(void)
{
    display.begin();
}

void loop(void)
{
    ESP_LOGI(TAG, "Hello, M5Stack Core2 world.");

    delay(500);
}
