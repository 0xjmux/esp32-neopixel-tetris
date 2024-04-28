#include "unity.h"
#include "esp_log.h"
#include <string.h>

#include "neopixel.h"
#include "neopixel_display.h"


#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"        // for vTaskDelay


#ifdef USE_STAT_LED_TEST
#include "driver/gpio.h"     // for LED panic function
#endif

// #define TAG "esp32_neopixel_test"

tNeopixelContext neopixels;

// set stuff up here
void setUp(void) {
    neopixels = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);
}

void tearDown(void) {
    neopixel_Deinit(neopixels);
}

#ifdef USE_STAT_LED_TEST
/**
 * Turn on stat LED to indicate Tests running
*/
TEST_CASE("test_enableSTATLED", "[test_status]") {
// void test_indicateTestsRunning(void) {

    gpio_reset_pin(STAT_LED_PIN);
    gpio_set_direction(STAT_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(STAT_LED_PIN, 1);
    printf("Turning on LED...\n");

}
#endif

/**
 * Display red in top right corner, blue in bot left
 * This will be manually checked by a human
*/
TEST_CASE("test matrix orientation", "[display]")
{
    uint32_t refreshRate = neopixel_GetRefreshRate(neopixels);
    // uint32_t taskDelay = MAX(1, pdMS_TO_TICKS(1000UL / refreshRate));
    printf("refreshrate=%ld\n", refreshRate);

    // set topR to red
    tNeopixel topRightNeopix = {TOP_RIGHT_LED, NP_RGB(200,0,0)};
    neopixel_SetPixel(neopixels, &topRightNeopix, 1);

    // set botL to blue
    tNeopixel botLeftNeopix = {BOT_LEFT_LED, NP_RGB(0,0,128)};
    neopixel_SetPixel(neopixels, &botLeftNeopix, 1);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    TEST_ASSERT(true);
}



TEST_CASE("test border rendering", "[display]") {

}