#ifndef KEY_LAYOUT_H
#define KEY_LAYOUT_H

#include <stdint.h>
#include "config.h"

#define ISO     0
#define U_1     4
#define U_1_25  5
#define U_1_5   6
#define U_1_75  7
#define U_2     8
#define U_2_25  9
#define U_2_75  11
#define U_3     12
#define U_4_5   18
#define U_6     24
#define U_6_25  25
#define U_7     28

#define LAYOUT_OPTION_COMMON 0
#define LAYOUT_OPTION_MAX 8
#define LAYOUT_LABEL_LEN 16

typedef struct __attribute__((packed)) {
    uint8_t idx;
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t option;
    uint8_t choice;
} key_layout_entry_t;

typedef struct __attribute__((packed)) {
    uint8_t option_count;
    uint8_t choices[LAYOUT_OPTION_MAX];
} layout_option_state_t;

typedef struct __attribute__((packed)) {
    uint8_t option;
    uint8_t choice;
    char option_name[LAYOUT_LABEL_LEN];
    char choice_name[LAYOUT_LABEL_LEN];
} key_layout_label_t;

extern const key_layout_entry_t key_layout_entries[];
extern const uint16_t key_layout_entry_count;
extern const layout_option_state_t key_layout_default_state;
extern const key_layout_label_t key_layout_labels[];
extern const uint16_t key_layout_label_count;

#endif
