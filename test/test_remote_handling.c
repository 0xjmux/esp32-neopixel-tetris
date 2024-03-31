
#include <unity.h>

#include "espnow_remote_test.h"


void test_peer_exist(void) {
    ESP_ERROR_CHECK(esp_now_init()); // must be called to set up ESP-NOW, but after wifi






    esp_now_deinit();
}