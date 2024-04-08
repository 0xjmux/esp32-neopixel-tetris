#ifndef ESPNOW_REMOTE_H
#define ESPNOW_REMOTE_H

#include <stdint.h>
#include "defs.h"

// temporary constants (bc there has to be defaults in esp-idf, right?)
#define STR_MAX_LEN 256
#define SHORT_STR_LEN 64
#define ESPNOW_MAXDELAY 512


/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA



// ESP-NOW Defines
#define CONFIG_ESPNOW_CHANNEL 1
#define CONFIG_ESPNOW_LMK "lmk1234567890123"    // shouldn't be being used

#define ESPNOW_QUEUE_SIZE           6


// wiz remote button definitions
#define WIZMOTE_BUTTON_ON          1
#define WIZMOTE_BUTTON_OFF         2
#define WIZMOTE_BUTTON_NIGHT       3
#define WIZMOTE_BUTTON_ONE         16
#define WIZMOTE_BUTTON_TWO         17
#define WIZMOTE_BUTTON_THREE       18
#define WIZMOTE_BUTTON_FOUR        19
#define WIZMOTE_BUTTON_BRIGHT_UP   9
#define WIZMOTE_BUTTON_BRIGHT_DOWN 8

// data parsing return function enum
enum {DATA_PARSE_OK, DATA_PARSE_STALE, DATA_PARSE_ERR};


#define DEBUG_LEN_ESPNOW_MESSGE 13
// wizmote data structure (thanks wled)
// https://github.com/Aircoookie/WLED/blob/main/wled00/remote.cpp
typedef struct espnow_msg_structure {
  uint8_t program;      // 0x91 for ON button, 0x81 for all others
//   uint8_t seq[4];       // Incremental sequence number 32 bit unsigned integer LSB first
  uint32_t seq;       // Incremental sequence number 32 bit unsigned integer LSB first
  uint8_t byte5;        // Unknown
  uint8_t button;       // Identifies which button is being pressed
  uint8_t byte8;        // Unknown, but always 0x01
  uint8_t byte9;        // Unknown, but always 0x64

  uint8_t byte10;  // Unknown, maybe checksum
  uint8_t byte11;  // Unknown, maybe checksum    // received packet has len 13 instead of 16 - gonan try commenting these
  uint8_t byte12;  // Unknown, maybe checksum
  uint8_t byte13;  // Unknown, maybe checksum
} __attribute__((packed)) espnow_msg_structure;

// static const struct espnow_msg_structure DEFAULT_MSG = {
//     // .program
//     // .seq[4],
//     .byte5 = 32,
//     // .button,
//     .byte8 = 1,
//     .byte9 = 100,
//     .byte10 = 0,
//     .byte11 = 0,
//     .byte12 = 0,
//     .byte13 = 0,
// };

// typedef enum {
//     EXAMPLE_ESPNOW_SEND_CB,
//     EXAMPLE_ESPNOW_RECV_CB,
// } example_espnow_event_id_t;


/**
 * @param uint8_t mac_addr[ESP_NOW_ETH_ALEN];
 * @param uint8_t *data;
 * @param int data_len;
 * @param example_espnow_event_id_t id;
 * 
*/
typedef struct example_espnow_event_recv_cb_t {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
    uint8_t espnow_event_id;
} example_espnow_event_recv_cb_t;

// enum {
//     EXAMPLE_ESPNOW_DATA_BROADCAST,
//     EXAMPLE_ESPNOW_DATA_UNICAST,
//     EXAMPLE_ESPNOW_DATA_MAX,
// };


// /* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
// typedef struct {
//     example_espnow_event_id_t id;
//     example_espnow_event_info_t info;
// } example_espnow_event_t;

typedef struct remote_button_info {
  uint8_t program_val;
  uint8_t button_val;
} remote_button_info;



// FUNCTIONS

void example_wifi_init(void);

void panic_toggle_stat_led(bool ledState);

void get_button_name_from_number(const uint8_t button, char *button_name_str);

/* static uint8_t example_espnow_data_parse(uint8_t *data, uint16_t data_len, uint8_t *program, \
     uint32_t *seq, uint8_t *button);
*/

// void espnow_recv_task(void *pvParameter);
esp_err_t espnow_remote_recv_init(void);
void espnow_remote_recv_deinit(void);

remote_button_info get_buttons_state(void);
void reset_internal_buttons_state(void);

#endif