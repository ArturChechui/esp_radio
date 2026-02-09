#include <esp_log.h>
#include <esp_psram.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <nvs_flash.h>

#include <cstring>

#include "AppContext.hpp"

static const char *TAG = "app_main";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "=== ESP Radio App Starting ===");
    ESP_LOGI("mem", "psram: found=%d size=%d bytes", esp_psram_is_initialized(),
             esp_psram_get_size());
    ESP_LOGI(TAG, "Heap: Free=%lu, Min=%lu, Largest=%lu bytes", esp_get_free_heap_size(),
             esp_get_minimum_free_heap_size(),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));

    core::AppContext appContext;
    if (!appContext.init()) {
        ESP_LOGE(TAG, "Failed to initialize AppContext");
        return;
    }

    ESP_LOGI(TAG, "Application initialized");

    char *buf = static_cast<char *>(malloc(2048));
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate stats buffer");
        return;
    }

    int counter = 0;

    while (true) {
        // Print stats every 30 seconds
        if (counter++ >= 6) {
            counter = 0;

            vTaskGetRunTimeStats(buf);
            ESP_LOGI(TAG, "\n=== CPU Task Stats ===\n%s", buf);

            ESP_LOGI(TAG, "Heap: Free=%lu, Min=%lu, Largest=%lu bytes", esp_get_free_heap_size(),
                     esp_get_minimum_free_heap_size(),
                     (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
        }

        ESP_LOGI(TAG, "Application running... (%ds)", counter * 5);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    free(buf);
}