#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "hal.h"
#include "config.h"
#include "src/include/mag_utils.h"
#include "src/include/mag_config.h"
#include "src/include/key_type.h"

/***----switch input mode, calibration mode----***/
#define SW_CAL_DEBOUNCE 100
#define CAL_SW_TIMEOUT_MS 1500

#if defined(SW_CAL_PIN) || defined(SENSI_OFFSET_PINS) || defined(SOCD_PINS)
	#ifndef MAG_AUX_SWITCH_UPDATE_INTERVAL_MS
	#define MAG_AUX_SWITCH_UPDATE_INTERVAL_MS 10
	#endif

	static bool aux_switch_update_due(systime_t *last_update, systime_t now)
	{
		if (*last_update != 0 && (now - *last_update) < chTimeMS2I(MAG_AUX_SWITCH_UPDATE_INTERVAL_MS)) {
			return false;
		}
		*last_update = now;
		return true;
	}
#endif

static volatile mode_t current_mode = MODE_INPUT;
static volatile cal_origin_t current_cal_origin = CAL_ORIGIN_NONE;
static volatile systime_t cal_sw_last_activity = 0;
static volatile mode_event_t pending_mode_event = MODE_EVENT_NONE;
static volatile cal_origin_t pending_cal_origin = CAL_ORIGIN_NONE;

typedef enum {
	CAL_SW_CMD_NONE,
	CAL_SW_CMD_START,
	CAL_SW_CMD_STOP,
} cal_sw_cmd_t;

static volatile cal_sw_cmd_t cal_sw_cmd = CAL_SW_CMD_NONE;

static void enter_cal(cal_origin_t origin, systime_t now);
static void exit_cal(void);

cal_origin_t cal_origin_get(void) {
	return current_cal_origin;
}

void cal_request_start(void) {
	systime_t now = chVTGetSystemTimeX();

	if (current_mode == MODE_INPUT) {
		pending_mode_event = MODE_EVENT_ENTER_CAL;
		pending_cal_origin = CAL_ORIGIN_SW;
		cal_sw_cmd = CAL_SW_CMD_NONE;
		return;
	}

	if (current_mode == MODE_CAL && current_cal_origin == CAL_ORIGIN_SW) {
		cal_sw_last_activity = now;
	}
}

void cal_request_stop(void) {
	if (pending_mode_event == MODE_EVENT_ENTER_CAL && pending_cal_origin == CAL_ORIGIN_SW) {
		pending_mode_event = MODE_EVENT_NONE;
		pending_cal_origin = CAL_ORIGIN_NONE;
		cal_sw_cmd = CAL_SW_CMD_NONE;
		return;
	}

	if (current_mode == MODE_CAL && current_cal_origin == CAL_ORIGIN_SW) {
		exit_cal();
		pending_mode_event = MODE_EVENT_EXIT_CAL;
		pending_cal_origin = CAL_ORIGIN_NONE;
		cal_sw_cmd = CAL_SW_CMD_NONE;
		return;
	}

	if (current_mode == MODE_INPUT) {
		cal_sw_cmd = CAL_SW_CMD_NONE;
	}
}

void cal_request_keepalive(void) {
	if (current_mode == MODE_CAL && current_cal_origin == CAL_ORIGIN_SW && cal_sw_cmd != CAL_SW_CMD_STOP) {
		cal_sw_last_activity = chVTGetSystemTimeX();
	}
}

static void enter_cal(cal_origin_t origin, systime_t now) {
	current_mode = MODE_CAL;
	current_cal_origin = origin;
	cal_sw_last_activity = (origin == CAL_ORIGIN_SW) ? now : 0;
}

static void exit_cal(void) {
	current_mode = MODE_INPUT;
	current_cal_origin = CAL_ORIGIN_NONE;
	cal_sw_last_activity = 0;
}

mode_event_t mode_update(void){
#ifdef SW_CAL_PIN
	static systime_t t0 = 0;
	static systime_t last_update = 0;
#endif
    systime_t now = chVTGetSystemTimeX();
	cal_sw_cmd_t cmd;
	mode_event_t event = pending_mode_event;
	cal_origin_t origin = pending_cal_origin;

	if (event != MODE_EVENT_NONE) {
		pending_mode_event = MODE_EVENT_NONE;
		pending_cal_origin = CAL_ORIGIN_NONE;
#ifdef SW_CAL_PIN
		if (event == MODE_EVENT_ENTER_CAL) {
			enter_cal(origin, now);
			t0 = 0;
		} else if (event == MODE_EVENT_EXIT_CAL) {
			t0 = 0;
		}
#else
		if (event == MODE_EVENT_ENTER_CAL) {
			enter_cal(origin, now);
		}
#endif
		return event;
	}

#ifdef SW_CAL_PIN
	if (!aux_switch_update_due(&last_update, now)) {
		return MODE_EVENT_NONE;
	}

    bool pin = !palReadLine(SW_CAL_PIN);
#endif
	cmd = cal_sw_cmd;

    if (current_mode == MODE_INPUT) {
/**----prev mode = input----**/
		if (cmd == CAL_SW_CMD_START) {
			cal_sw_cmd = CAL_SW_CMD_NONE;
			enter_cal(CAL_ORIGIN_SW, now);
#ifdef SW_CAL_PIN
			t0 = 0;
#endif
			return MODE_EVENT_ENTER_CAL;
		}
		if (cmd == CAL_SW_CMD_STOP) {
			cal_sw_cmd = CAL_SW_CMD_NONE;
		}

#ifdef SW_CAL_PIN
        if (pin) {
		//switched pin state
            if (t0 == 0) {
                t0 = now;
            }

			else if ((now - t0) >= chTimeMS2I(SW_CAL_DEBOUNCE)) {
			//change input mode -> cal mode
				enter_cal(CAL_ORIGIN_HW, now);
				t0 = 0;
				cal_sw_cmd = CAL_SW_CMD_NONE;
				return MODE_EVENT_ENTER_CAL;
            }

        } else {
		//stay pin state
            t0 = 0;
        }/**----prev mode = input----**/
#endif
		
    } else if (current_mode == MODE_CAL) {
/**----prev mode = calibration----**/
		if (current_cal_origin == CAL_ORIGIN_HW) {
#ifdef SW_CAL_PIN
			cal_sw_cmd = CAL_SW_CMD_NONE;
			if (pin) {
				t0 = 0;
			} else if (t0 == 0) {
				t0 = now;
			} else if ((now - t0) >= chTimeMS2I(SW_CAL_DEBOUNCE)) {
				exit_cal();
				t0 = 0;
				return MODE_EVENT_EXIT_CAL;
			}
#else
			exit_cal();
			return MODE_EVENT_EXIT_CAL;
#endif
		} else if (current_cal_origin == CAL_ORIGIN_SW) {
#ifdef SW_CAL_PIN
			t0 = 0;
#endif
			if (cmd == CAL_SW_CMD_STOP) {
				cal_sw_cmd = CAL_SW_CMD_NONE;
				exit_cal();
				return MODE_EVENT_EXIT_CAL;
			}
			if (cal_sw_last_activity != 0 && (now - cal_sw_last_activity) > chTimeMS2I(CAL_SW_TIMEOUT_MS)) {
				exit_cal();
				return MODE_EVENT_EXIT_CAL;
			}
			if (cmd == CAL_SW_CMD_START) {
				cal_sw_cmd = CAL_SW_CMD_NONE;
			}
		} else {
			exit_cal();
		}
    }/**----prev mode = calibration----**/
	
	return MODE_EVENT_NONE;
}

	//get current mode input/calibration
mode_t mode_get(void){
	return current_mode;
}


/***--------sensitivity offset value-------***/
#define SW_OFFSET_DEBOUNCE 10
#ifndef DEF_OFFSET_POINT
#define DEF_OFFSET_POINT {0, 50, 100, 200, 500}
#endif

static const uint16_t offset_point_defaults[SENSI_OFFSET_STAGE_COUNT] = DEF_OFFSET_POINT;

static uint16_t offset_point_value(uint8_t stage) {
	const mag_config_view_t *cfg = mag_config_get_view();
	if (stage >= SENSI_OFFSET_STAGE_COUNT) stage = SENSI_OFFSET_STAGE_COUNT - 1;
	return cfg && cfg->sensi_offset ? cfg->sensi_offset->value[stage] : offset_point_defaults[stage];
}

static uint8_t offset_stage = 0;

#if defined(SENSI_OFFSET_PINS)
static uint8_t offset_physical_stage = 0xFF;
static const pin_t offset_pins[SENSI_OFFSET_STAGE_COUNT] = SENSI_OFFSET_PINS;

static uint8_t read_offset_physical_stage(uint8_t start_stage, bool *found) {
	uint8_t n = start_stage;

	for (uint8_t i = 0; i < SENSI_OFFSET_STAGE_COUNT; i++) {
		if (!palReadLine(offset_pins[n])) {
			*found = true;
			return n;
		}

		if (++n == SENSI_OFFSET_STAGE_COUNT) n = 0;
	}

	*found = false;
	return start_stage;
}
#endif

uint8_t offset_stage_get(void) {
	return offset_stage;
}

uint16_t offset_value_get(void) {
	return offset_point_value(offset_stage);
}

void offset_stage_set(uint8_t stage) {
	if (stage >= SENSI_OFFSET_STAGE_COUNT) stage = SENSI_OFFSET_STAGE_COUNT - 1;
	offset_stage = stage;
}

void offset_stage_up(void) {
	offset_stage_set((offset_stage + 1) % SENSI_OFFSET_STAGE_COUNT);
}

void offset_stage_down(void) {
	offset_stage_set(offset_stage == 0 ? SENSI_OFFSET_STAGE_COUNT - 1 : offset_stage - 1);
}

uint16_t offset_update(void){
#if !defined(SENSI_OFFSET_PINS)
	return offset_value_get();
#else
	static systime_t last_update = 0;
	systime_t now = chVTGetSystemTimeX();

	if (!aux_switch_update_due(&last_update, now)) {
		return offset_value_get();
	}
	
	bool found = false;
	uint8_t physical_stage = read_offset_physical_stage(offset_physical_stage == 0xFF ? offset_stage : offset_physical_stage, &found);

	if (found && (physical_stage != offset_physical_stage || physical_stage != offset_stage)) {
		offset_physical_stage = physical_stage;
		offset_stage = physical_stage;
	}

	return offset_value_get();
#endif
}

void init_offset_pins(void){
#if defined(SENSI_OFFSET_PINS)
	for(int i = 0; i < SENSI_OFFSET_STAGE_COUNT; i++){
		palSetLineMode(offset_pins[i], PAL_MODE_INPUT_PULLUP);
	}

	bool found = false;
	uint8_t physical_stage = read_offset_physical_stage(offset_stage, &found);
	if (found) {
		offset_physical_stage = physical_stage;
		offset_stage = physical_stage;
	}
#endif
}
	
#define SOCD_PINS_NUM 2

#if defined(SOCD_PINS)
static const pin_t socd_pins[] = SOCD_PINS;
#endif


uint8_t get_socd_mode(void){
#if !defined(SOCD_PINS)
	return 1;
#else
	static uint8_t stage = 0;
	static systime_t last_update = 0;
	systime_t now = chVTGetSystemTimeX();

	if (!aux_switch_update_due(&last_update, now)) {
		return stage == 0 ? 0 : 1;
	}
	
	uint8_t n = stage;
	
	for (uint8_t i = 0; i < SOCD_PINS_NUM; i++) {

		if (!palReadLine(socd_pins[n])) {
			stage = n;
			return n == 0 ? 0 : 1;
		}
		
		if (++n == SOCD_PINS_NUM) n = 0;
		
	}
	return stage == 0 ? 0 : 1;
#endif
}

void init_socd_pins(void){
#if defined(SOCD_PINS)
	for(int i = 0; i < SOCD_PINS_NUM; i++){
		palSetLineMode(socd_pins[i], PAL_MODE_INPUT_PULLUP);
	}
#endif
}


/***-------last index--------***/
uint8_t get_last_valid_index(void) {
    for (int row = MATRIX_ROWS - 1; row >= 0; row--) {
        for (int col = MATRIX_COLS - 1; col >= 0; col--) {
			uint8_t idx = row * MATRIX_COLS + col;
            if (mag_config_idx_is_valid(idx)) {
                return row * MATRIX_COLS + col;
            }
        }
    }
    return 0;
}

/***--------release all keys (action exec)--------***/
static bool key_state_needs_release(key_state_t state) {
	return state == KEY_PRESSED || state == KEY_CANCELING;
}

bool release_all_keys (key_states_t *states) {
	bool changed = 0;
		
		//matrix loop
	for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
/**----row----**/
		for (uint8_t col = 0; col < MATRIX_COLS; col++) {
	/**----col----**/
			uint8_t idx = row * MATRIX_COLS + col;
			
			if (key_state_needs_release(states[idx].state)) {
				action_exec(MAKE_KEYEVENT(row, col, false));
				changed = true;
			}
		}
	}
	return changed;
}


