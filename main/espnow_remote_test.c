/**
 * Test code for ingesting messages from the Wizmote ESP-NOW remote
*/

// this include block copied from espnow_example_main.c
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"

#include "esp_app_trace.h"

#include "driver/gpio.h"     // for LED panic function

#include "espnow_remote_test.h"
// #include "espnow_example.h"

////////////////////////////////////////
// TEMPORARY DEFS
// real impl needs to be able to find and accept any local remote, not just mine
#define MY_REMOTE_MAC 0x444f8ebf15
#define ESPNOW_WIFI_MODE WIFI_MODE_STA

static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
// static uint16_t s_example_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = { 0, 0 };

////////////////////////////////////////

static const char *TAG = "espnow_wizmote_example";

static uint32_t last_msg_seq = 0;      // seq number of last message
// static espnow_msg_structure incoming;           // holds incoming message data
static QueueHandle_t s_example_espnow_queue;    // semaphore for espnow handling

/* WiFi should start before using ESPNOW */
static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

// turn the STAT led on to indicate something went wrong
static void panic_stat_led_on(void) {
    #define STAT_LED_PIN 2
    ESP_LOGE(TAG, "Panic LED triggered!");
    gpio_reset_pin(STAT_LED_PIN);
    gpio_set_direction(STAT_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(STAT_LED_PIN, 1);
}

/**
 * For debugging: given a button id `button`, save 
 * the name of the button as a string to `button_name_str`
 * 
 * for some reason this function causes a crash. wtf
 * 
*/
static void get_button_name_from_number(const uint8_t button, char *button_name_str) {
    // char button_name_str[STR_MAX_LEN] = "";
    switch(button) {
        case (WIZMOTE_BUTTON_ON):
            strcpy(button_name_str, "ON");
            break;
        case (WIZMOTE_BUTTON_OFF):
            strcpy(button_name_str, "OFF");
            break;
        case (WIZMOTE_BUTTON_NIGHT):
            strcpy(button_name_str, "NIGHT");
            break;
        case (WIZMOTE_BUTTON_ONE):
            strcpy(button_name_str, "ONE");
            break;
        case (WIZMOTE_BUTTON_TWO):
            strcpy(button_name_str, "TWO");
            break;
        case (WIZMOTE_BUTTON_THREE):
            strcpy(button_name_str, "THREE");
            break;
        case (WIZMOTE_BUTTON_FOUR):
            strcpy(button_name_str, "FOUR");
            break;
        case (WIZMOTE_BUTTON_BRIGHT_UP):
            strcpy(button_name_str, "BRIGHT_UP");
            break;
        case (WIZMOTE_BUTTON_BRIGHT_DOWN):
            strcpy(button_name_str, "BRIGHT_DOWN");
            break;
        default:
            ESP_LOGE(TAG, "invalid button=%d passed to get_button_name_from_number!", button);
            break;
    }
}

/*
// typedef void (*esp_now_recv_cb_t)(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len);
// static void on_data_recv(const uint8_t *mac_addr, const uint8_t *in_data, size_t len) {
//     // char last_packet_src[STR_MAX_LEN];
//     // parse packet addr and log data to console
//     sprintf(last_signal_src, "%02x%02x%02x%02x%02x%02x", \
//         mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    
//     ESP_LOGI(TAG, "ESP-NOW message received from addr: %s", last_signal_src);
    
//     if (len != sizeof(incoming)) {
//         ESP_LOGI(TAG, "Incorrect len - was expecting sizeof(incoming)=%d, but incoming len=%d\n", \
//             sizeof(incoming), len);
//         // return???
//     }
//     memcpy(&(incoming.program), in_data, sizeof(incoming));
    
//     // WTF is this
//     uint32_t cur_seq = incoming.seq[0] | (incoming.seq[1] << 8) | (incoming.seq[2] << 16) | (incoming.seq[3] << 24);
    
//     // dont parse same packet twice
//     if (cur_seq == last_msg_seq) {
//         return;
//     }
//     ESP_LOGI(TAG, "Incoming ESP-NOW Packet [SEQ=%d, Sender = %s, button=%d] \n", cur_seq, last_signal_src, incoming.button);
//     // switch case logic goes here

//     last_msg_seq = cur_seq;
// }
*/

// FROM EXAMPLE
static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    // example_espnow_event_t evt;
    example_espnow_event_recv_cb_t recv_cb; 
    uint8_t *mac_addr = recv_info->src_addr;

    // check for invalid packet
    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        //  return;     // these functions arent't ever supposed to return
        
    }

    // set id to indicate recv packet
    // copy MAC addr and copy data for recv_cb
    // recv_cb.id = EXAMPLE_ESPNOW_RECV_CB;        // not used
    memcpy(recv_cb.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    
    // this line causes fault????
    recv_cb.data = malloc(len);
    if (recv_cb.data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        // return;      // these  functions aren't ever supposed to return
    }
    memcpy(recv_cb.data, data, len);

    // add packet to queue to be processed
    recv_cb.data_len = len;
    if (xQueueSend(s_example_espnow_queue, &recv_cb, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb.data);
    }


}

// FROM EXAMPLE
/* Parse received ESPNOW data. */
uint8_t example_espnow_data_parse(uint8_t *data, uint16_t data_len, uint8_t *program, \
    uint32_t *seq, uint8_t *button)
{
    espnow_msg_structure *buf = (espnow_msg_structure *)data;
    // uint16_t crc, crc_cal = 0;

    if (data_len < sizeof(espnow_msg_structure)) {
    // if (data_len < DEBUG_LEN_ESPNOW_MESSGE) {
        ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d ; expected=%d", data_len, sizeof(espnow_msg_structure));

        panic_stat_led_on();

        return DATA_PARSE_ERR;
        // return -1;
    }

// possible null dereference here??
    *program = buf->program;
    *seq = buf->seq;
    *button = buf->button;

    // if we've already seen this msg before
    if (*seq <= last_msg_seq) {
        return DATA_PARSE_STALE;
    }

    last_msg_seq = *seq;

    ESP_LOGI(TAG, "completed espnow_data_parse()");

    return DATA_PARSE_OK;
}






/**
 * FreeRTOS task to handle data reception
*/
static void espnow_recv_task(void *pvParameter)
{

    example_espnow_event_recv_cb_t recv_cb;
    uint8_t program = 0;
    // uint8_t seq[4] = { 0 };
    uint32_t seq = 0;
    uint8_t button = 0;
    int ret;

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (xQueueReceive(s_example_espnow_queue, &recv_cb, portMAX_DELAY) == pdTRUE) {
        // example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

        // ret = example_espnow_data_parse(recv_cb.data, recv_cb.data_len, &recv_state, &recv_seq, &recv_magic);
        // SEQ MIGHT NOT NEED & 
        ret = example_espnow_data_parse(recv_cb.data, recv_cb.data_len, &program, &seq, &button);

        // original data not needed since we've parsed out the parts we want
        free(recv_cb.data);

        if (ret == DATA_PARSE_OK) {
            ESP_LOGI(TAG, "Receive seq=%ld data from: "MACSTR", len: %d\n", \
                seq, MAC2STR(recv_cb.mac_addr), recv_cb.data_len);

            /* If MAC address does not exist in peer list, add it to peer list. */
            // CRASH HAPPENED HERE WITH IF STATEMENT
            if (esp_now_is_peer_exist(recv_cb.mac_addr) == false) {
                ESP_LOGI(TAG, "peer not already present in peer list, adding now");
                esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
                if (peer == NULL) {
                    ESP_LOGE(TAG, "Malloc peer information fail");
                    panic_stat_led_on();
                    espnow_remote_recv_deinit();
                    vTaskDelete(NULL);
                }

                memset(peer, 0, sizeof(esp_now_peer_info_t));
                peer->channel = CONFIG_ESPNOW_CHANNEL;
                peer->ifidx = ESPNOW_WIFI_IF;
                peer->encrypt = false;
                memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
                // SAME PTR THING HERE WITH & 
                memcpy(peer->peer_addr, &recv_cb.mac_addr, ESP_NOW_ETH_ALEN);
                
                ESP_LOGI(TAG, "attempting to add peer: %p", peer);
                // PANIC THROWN HERE
                ESP_ERROR_CHECK( esp_now_add_peer(peer) );
                ESP_LOGI(TAG, "Peer added successfully");
                free(peer);
            }
            char button_name[STR_MAX_LEN] = "";
            // get_button_name_from_number(button, button_name);
            // ESP_LOGI(TAG, "Incoming ESP-NOW Packet [SEQ=%ld, button=%d: %s] \n", seq, button, button_name);
            ESP_LOGI(TAG, "Incoming ESP-NOW Packet [SEQ=%ld, button=%d] \n", seq, button);
            // /* Indicates that the device has received broadcast ESPNOW data. */
            // // if (send_param->state == 0) {
            // //     send_param->state = 1;
            // // }

            // /* If receive broadcast ESPNOW data which indicates that the other device has received
            //     * broadcast ESPNOW data and the local magic number is bigger than that in the received
            //     * broadcast ESPNOW data, stop sending broadcast ESPNOW data and start sending unicast
            //     * ESPNOW data.
            //     */

        }
        else if (ret == DATA_PARSE_STALE) {
            /*
            // ESP_LOGI(TAG, "Received stale packet from "MACSTR" [SEQ=%ld, button=%d], last_seq=%ld \n", \
            //     MAC2STR(recv_cb.mac_addr), seq, button, last_msg_seq);
            */
            ESP_LOGI(TAG, ".");

        }
        else {
            ESP_LOGI(TAG, "Receive error data from: "MACSTR"", MAC2STR(recv_cb.mac_addr));
        }

        // break;
    }
}

// a lot of this copied from espnow_example_main.c
static esp_err_t espnow_remote_recv_init(void) {
 
    s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_recv_cb_t));
    if (s_example_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }    
    
    ESP_ERROR_CHECK(esp_now_init()); // must be called to set up ESP-NOW, but after wifi

    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );
    // ESP_ERROR_CHECK( esp_now_register_recv_cb(on_data_recv) );

    /* Set primary master key. (not used in my sketch )*/
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        panic_stat_led_on();
        vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);

    // task params aren't used rn
    uint8_t *recv_task_params = 0;
    TaskHandle_t recv_task_handle = NULL;
    xTaskCreate(espnow_recv_task, "espnow_recv_task",2048, recv_task_params, 4, &recv_task_handle);
    ESP_LOGI(TAG, "Successfully finished remote_recv_init()");

    return ESP_OK;
}

void espnow_remote_recv_deinit(void) {
    vSemaphoreDelete(s_example_espnow_queue);
    esp_now_deinit();
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting main");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    example_wifi_init();
    // set up esp-now functionality

    ESP_LOGI(TAG, "Starting remote: prior vals last_seq=%ld", last_msg_seq);
    espnow_remote_recv_init();


}

