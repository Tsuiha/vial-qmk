#ifndef MAG_UTILS_H
#define MAG_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "config.h"
#include "src/include/key_type.h"

typedef enum {
    MODE_INPUT,
    MODE_CAL
} mode_t;

typedef enum {
    CAL_ORIGIN_NONE,
    CAL_ORIGIN_HW,
    CAL_ORIGIN_SW,
} cal_origin_t;

typedef enum {
    MODE_EVENT_NONE,
    MODE_EVENT_ENTER_CAL,
    MODE_EVENT_EXIT_CAL
} mode_event_t;

mode_t mode_get(void);
mode_event_t mode_update(void);
cal_origin_t cal_origin_get(void);
void cal_request_start(void);
void cal_request_stop(void);
void cal_request_keepalive(void);

uint16_t offset_update(void);
void init_offset_pins(void);
uint8_t offset_stage_get(void);
uint16_t offset_value_get(void);
void offset_stage_set(uint8_t stage);
void offset_stage_up(void);
void offset_stage_down(void);

uint8_t get_last_valid_index(void);
bool release_all_keys(key_states_t *states);

uint8_t get_socd_mode(void);
void init_socd_pins(void);

#endif
