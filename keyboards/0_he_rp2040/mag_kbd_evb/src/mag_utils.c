#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "hal.h"
#include "config.h"
#include "src/include/mag_utils.h"
#include "src/include/key_layout.h"
#include "src/include/key_type.h"

/***----switch input mode, calibration mode----***/
#define SW_CAL_DEBOUNCE 100


static mode_t current_mode = MODE_INPUT;

mode_event_t mode_update(void){
	static systime_t t0 = 0;
    bool pin = !gpio_read_pin(SW_CAL_PIN);
    systime_t now = chVTGetSystemTimeX();

    if (current_mode == MODE_INPUT) {
/**----prev mode = input----**/
        if (pin) {
		//switched pin state
            if (t0 == 0) {
                t0 = now;
            }

            else if ((now - t0) >= chTimeMS2I(SW_CAL_DEBOUNCE)) {
			//change input mode -> cal mode
				current_mode = MODE_CAL;
				t0 = 0;
				return MODE_EVENT_ENTER_CAL;
            }

        } else {
		//stay pin state
            t0 = 0;
        }/**----prev mode = input----**/
		
    } else if (current_mode == MODE_CAL) {
/**----prev mode = calibration----**/
        if (!pin) {
		//switched pin state
            if (t0 == 0) {
                t0 = now;
            }

            else if ((now - t0) >= chTimeMS2I(SW_CAL_DEBOUNCE)) {
			//change cal mode -> input mode
				current_mode = MODE_INPUT;
				t0 = 0;
				return MODE_EVENT_EXIT_CAL;
            }

        } else {
		//stay pin state
            t0 = 0;
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
#ifndef DEF_OFFSET_NUM
#define DEF_OFFSET_NUM 5
#endif

static uint16_t offset_point[DEF_OFFSET_NUM] = DEF_OFFSET_POINT;

/*スライドスイッチのため不要
static bool button_update(void)
{
    static systime_t t0 = 0;
    static bool stable_state = false; // 現在の確定状態
    static bool last_raw = false;     // 前回の生値

    bool raw = !gpio_read_pin(SENSI_OFFSET_PIN); // 押すとtrue
    systime_t now = chVTGetSystemTimeX();

	//debug
	//uint8_t unko = raw;
	//printf("offset pin: %d\n",unko);
	//debug

    // 状態変化検出
    if (raw != last_raw) {//状態が変化したら
        t0 = now;           // デバウンス開始
        last_raw = raw;
    }

    // 一定時間安定したら確定
    else if ((now - t0) >= chTimeMS2I(SW_OFFSET_DEBOUNCE)) {

        // 状態が変わったら更新
        if (stable_state != raw) {
            stable_state = raw;

            // 押した→離した瞬間でイベント発生
            if (stable_state == false) {
                return true;
            }
        }
    }
    return false;
}


uint16_t offset_update(void){
	static uint8_t stage = 0;

	if (button_update()) {
		if (++stage >= DEF_OFFSET_NUM) stage = 0;
			//debug
			printf("offset change: %d\n",offset_point[stage]);
			//debug	
	}

    return offset_point[stage];
}
*/


static const pin_t offset_pins[DEF_OFFSET_NUM] = SENSI_OFFSET_PINS;

uint16_t offset_update(void){
	static uint8_t stage = 0;
	
	uint8_t n = stage;
	
	for (uint8_t i = 0; i < DEF_OFFSET_NUM; i++) {

		if (!gpio_read_pin(offset_pins[n])) {
			#ifdef DEBUG_INFO
			if(stage != n){
			printf("sensi offset change = %d um\n",offset_point[n]);
			}
			#endif
			stage = n;
			return offset_point[n];
		}
		
		if (++n == DEF_OFFSET_NUM) n = 0;
		
	}
	return offset_point[stage];
}

void init_offset_pins(void){
	for(int i = 0; i < DEF_OFFSET_NUM; i++){
		gpio_set_pin_input_high(offset_pins[i]);
	}
}
	
//socd test for select switch

#define SOCD_PINS_NUM 3

#if defined(W_KEY_IDX) && defined(A_KEY_IDX) && defined(S_KEY_IDX) && defined(D_KEY_IDX) && defined(SOCD_PINS)
static const pin_t socd_pins[] = SOCD_PINS;
#endif


uint8_t get_socd_mode(void){
	static uint8_t stage = 0;
	
	#if defined(W_KEY_IDX) && defined(A_KEY_IDX) && defined(S_KEY_IDX) && defined(D_KEY_IDX) && defined(SOCD_PINS)
	uint8_t n = stage;
	
	for (uint8_t i = 0; i < 3; i++) {

		if (!gpio_read_pin(socd_pins[n])) {
			stage = n;
			return n;
		}
		
		if (++n == 3) n = 0;
		
	}
	#endif
	return stage;
}

void init_socd_pins(void){
	#if defined(W_KEY_IDX) && defined(A_KEY_IDX) && defined(S_KEY_IDX) && defined(D_KEY_IDX) && defined(SOCD_PINS)
	for(int i = 0; i < SOCD_PINS_NUM; i++){
		gpio_set_pin_input_high(socd_pins[i]);
	}
	#endif
}


/***-------last index--------***/
uint8_t get_last_valid_index(void) {
    for (int row = MATRIX_ROWS - 1; row >= 0; row--) {
        for (int col = MATRIX_COLS - 1; col >= 0; col--) {
			uint8_t idx = row * MATRIX_COLS + col;
            if (key_layout[idx].valid) {
                return row * MATRIX_COLS + col;
            }
        }
    }
    return 0;
}

/***--------release all keys (action exec)--------***/
bool release_all_keys (key_states_t *states, const key_layout_t *key_layout) {
	bool changed = 0;
		
		//matrix loop
	for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
/**----row----**/
		for (uint8_t col = 0; col < MATRIX_COLS; col++) {
	/**----col----**/
			uint8_t idx = row * MATRIX_COLS + col;
				//NO_USE skip
			if (key_layout[idx].valid == false) continue;
			
			if (states[idx].state == KEY_PRESSED) {
					action_exec(MAKE_KEYEVENT(row, col, false));
					changed = true;
			}
		}
	}
	return changed;
}


