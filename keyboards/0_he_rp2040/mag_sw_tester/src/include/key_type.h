#ifndef KEY_TYPE_H
#define KEY_TYPE_H

#include <stdint.h>

#define SENSI_OFFSET_STAGE_COUNT 5

typedef enum {
    KEY_RELEASED,
    KEY_PRESSED,
    KEY_REMOVED,
    KEY_CANCELING
}key_state_t;

typedef struct {
    int32_t pos_cur;	//current position um
    int32_t pos_ref;	//ref position um
    int32_t pos_prev;	//previous position um
    uint16_t adc_cur;	//current adc value
    uint16_t move_step;	//absolute position change from previous scan
    int8_t move_dir;	//-1: press direction, 1: release direction
    uint8_t move_count;	//continuous count in same direction
    uint8_t pos_prev_valid;
    uint8_t ccl_repress;
    key_state_t state;	//key state
} key_states_t;

typedef struct __attribute__((packed)) {
    uint16_t act_pt;
	uint16_t act_trg;
    uint16_t rst_pt;
    uint16_t rst_trg;
    uint16_t stroke;
} dyn_key_param_t;

typedef struct {
    uint16_t act_ht;
    uint16_t act_pt2;
    uint16_t rst_pt2;
} dyn_key_param2_t;

typedef struct __attribute__((packed)) {
    uint16_t tgt_idx;
    uint16_t tgt_th;
} ccl_key_param_t;

typedef struct __attribute__((packed)) {
    uint16_t ref_adc_val;
    uint16_t ref_point;
    uint16_t mag_gain;
} cal_key_param_t;

typedef struct __attribute__((packed)) {
    uint16_t value[SENSI_OFFSET_STAGE_COUNT];
} sensi_offset_param_t;

#endif
