#include <Arduino.h>
#include <M5Core2.h>

static const char *TAG = "main.cpp";

void setup(void)
{
    M5.begin();
}

void loop(void)
{
    M5.update();

    ESP_LOGI(TAG, "Hello, M5Stack Core2 world.");

    delay(500);
}
