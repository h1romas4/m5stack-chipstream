#include <Arduino.h>
#include <M5Core2.h>

static const char *TAG = "main.cpp";

#include <stdio.h>

extern "C" int rust_chipstream_test(void);

void setup(void)
{
    M5.begin();

    // Test link and call Rust function
    int result = rust_chipstream_test();
    ESP_LOGI(TAG, "rust_chipstream_test(%d)", result);
}

void loop(void)
{
    M5.update();

    ESP_LOGI(TAG, "Hello, M5Stack Core2 world.");

    delay(500);
}
