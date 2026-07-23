#include <stdint.h>
#include <stdbool.h>
#include "matrix.h"
#include "quantum.h"
#include "hal.h"
#include "ch.h"
#include "config.h"
#include "src/include/key_type.h"
#include "src/include/mag_analog.h"
#include "src/include/mag_config.h"
#include "src/include/mag_proc.h"
#include "src/include/mag_utils.h"
#include "src/include/mag_cal.h"
#include "src/include/mag_realtime.h"

static matrix_row_t matrix[MATRIX_ROWS] = {0};
static uint8_t last_idx; //last scan key index
static uint8_t idx_valid_generation;
static key_states_t key_states[MATRIX_SIZE] = {0};


//param
static dyn_key_param_t  *dyn_param;
static dyn_key_param2_t *dyn_param2;
static ccl_key_param_t  *ccl_param;
static cal_key_param_t  *cal_param;
static uint8_t          *idx_valid;


//key event
static inline void press_key_event(uint8_t row, uint8_t col){
	matrix[row] |= (1UL << col);
	action_exec(MAKE_KEYEVENT(row, col, true));
	send_keyboard_report();
}

static inline void release_key_event(uint8_t row, uint8_t col){
	matrix[row] &= ~(1UL << col);
	action_exec(MAKE_KEYEVENT(row, col, false));
	send_keyboard_report();
}

static inline void reset_all_keys(void){
			//release all keys
	bool released = release_all_keys(key_states);
	if (released) send_keyboard_report();
		//init
	memset(matrix, 0, sizeof(matrix));
	memset(key_states, 0, sizeof(key_states));
	set_adc_conv_ng();
}


//init
void matrix_init(void) {
	//mux standby
	init_mux();
	
	//adc standby
	init_adc();
	
	//direct pins standby
#ifdef PCB_CAL_PIN
	palSetLineMode(PCB_CAL_PIN, PAL_MODE_INPUT_PULLUP);
#endif
#ifdef SW_CAL_PIN
	palSetLineMode(SW_CAL_PIN, PAL_MODE_INPUT_PULLUP);
#endif
	init_offset_pins();
	init_socd_pins();
	
	//eeprom load, if eeprom NG -> init
	init_mag_config();
	
	//get param
	const mag_config_view_t *cfg = mag_config_get_view();
	dyn_param  = cfg->dyn;
	dyn_param2 = cfg->dyn2;
	ccl_param  = cfg->ccl;
	cal_param  = cfg->cal;
	idx_valid  = cfg->idx_valid;

	//get const
	last_idx = get_last_valid_index();
	idx_valid_generation = mag_config_get_idx_valid_generation();
		
	//wait directpins standby (min 2ms)
	chThdSleepMilliseconds(5);
	
#ifdef PCB_CAL_PIN
	if (!palReadLine(PCB_CAL_PIN)) {
		run_pcb_calibration();
	}
#endif
}

//main
uint8_t matrix_scan(void) {
	
	bool changed = false;
	bool calibrated = false;

	uint8_t current_idx_valid_generation = mag_config_get_idx_valid_generation();
	if (idx_valid_generation != current_idx_valid_generation) {
		reset_all_keys();
		last_idx = get_last_valid_index();
		idx_valid_generation = current_idx_valid_generation;
	}

	uint8_t prev_idx = last_idx;
	uint8_t prev_row = prev_idx / MATRIX_COLS;
	uint8_t prev_col = prev_idx % MATRIX_COLS;
	
/**----mode switch event----**/
	switch (mode_update()) {
		case MODE_EVENT_NONE://keep current mode
			break;
		
		case MODE_EVENT_ENTER_CAL: /*----enter calibration mode----*/
			reset_all_keys();
			break; /*----enter calibration mode----*/
			
		case MODE_EVENT_EXIT_CAL: /*----exit calibration mode----*/
				//calc cal param
				calibrated = finalize_calibration(key_states,dyn_param,cal_param);
				//set cal param
			if (calibrated) {
				save_cal_param();
				save_header();
			}
				//init
			memset(key_states, 0, sizeof(key_states));
			set_adc_conv_ng();
			break; /*----exit calibration mode----*/
			
	}/**----mode switch event----**/
	
/**----sensi offset btn----**/
	uint16_t offset_value = offset_update();
	
	
/**----socd btn----**/
		static uint8_t socd_mode_prev = 0xFF;
		uint8_t socd_mode_cur = get_socd_mode();
		
		if(socd_mode_prev != socd_mode_cur || ccl_is_enabled() != (socd_mode_cur != 0)){
			socd_mode_prev = socd_mode_cur;
			update_socd(socd_mode_prev);
		}
	
	
	
	
		//matrix loop
	for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
/**----row----**/
		enable_row(row);
		
		for (uint8_t col = 0; col < MATRIX_COLS; col++) {
	/**----col----**/
			uint8_t idx = row * MATRIX_COLS + col;
			
				//NO_USE skip
			if (!idx_valid[idx]) continue;
			
			enable_col(col);
			
			start_adc_conv();
			
			if (adc_conv_is_ok()) {/*----prev adc conversion completed----*/
					//get prev idx adc value
				uint16_t adc_value = get_adc_value();
				key_states_t *st = &key_states[prev_idx];
				st->adc_cur = adc_value;
				
				
				switch (mode_get()) {
					case MODE_INPUT:/*----input mode----*/
							//get prev idx current position
						st->pos_cur = calc_position(adc_value, &cal_param[prev_idx]);
						//judge current state
						key_event_t key_event = dyn_key_input(
							st,
							&dyn_param[prev_idx],
							&dyn_param2[prev_idx],
							offset_value
							);
						
							
							//input key event	
						switch(key_event) {
							case KEY_EVENT_NONE:
								break;
								
							case KEY_EVENT_PRESS:
									//any key canceling
								any_key_ccl(key_states, &ccl_param[prev_idx]);
								press_key_event(prev_row, prev_col);
								changed = true;
								break;
								
							case KEY_EVENT_RELEASE:
								release_key_event(prev_row, prev_col);
								changed = true;
								break;
						}
						break;/*----input mode----*/
					
					case MODE_CAL:/*----cal mode----*/
						calibrate_adc_range(adc_value, st);
						
						break;/*----cal mode----*/
						
				}/*----switch(mode_get())----*/
				
			}/*----prev adc conversion completed----*/
			
				//prev idx not include invalid key
			prev_idx = idx;
			prev_row = prev_idx / MATRIX_COLS;
			prev_col = prev_idx % MATRIX_COLS;
			
				//wait adc conversion
			wait_adc_conv();
				//next key
		}/**----col----**/
			
		disable_row(row);
		
		
	}/**----row----**/
	
    return changed; //any key state changed?
}

matrix_row_t matrix_get_row(uint8_t row) {
	return matrix[row];
}

int16_t mag_realtime_get_d_cur(uint8_t idx) {
	if (idx >= MATRIX_SIZE) return 0;
	int32_t value = key_states[idx].pos_cur;
	if (value > INT16_MAX) return INT16_MAX;
	if (value < INT16_MIN) return INT16_MIN;
	return (int16_t)value;
}

uint16_t mag_realtime_get_adc_cur(uint8_t idx) {
	if (idx >= MATRIX_SIZE) return 0;
	return key_states[idx].adc_cur;
}

void matrix_get_calibration_live(uint8_t start_idx, uint8_t count, uint8_t *data) {
	for (uint8_t i = 0; i < count; i++) {
		uint8_t idx = start_idx + i;
		const key_states_t *st = &key_states[idx];
		uint16_t y_cur = st->adc_cur;
		uint16_t y_min = st->pos_ref < 0 ? 0 : (uint16_t)st->pos_ref;
		uint16_t y_max = st->pos_cur < 0 ? 0 : (uint16_t)st->pos_cur;
		uint8_t *dst = &data[i * 6];
		dst[0] = (y_cur >> 8) & 0xFF;
		dst[1] = y_cur & 0xFF;
		dst[2] = (y_min >> 8) & 0xFF;
		dst[3] = y_min & 0xFF;
		dst[4] = (y_max >> 8) & 0xFF;
		dst[5] = y_max & 0xFF;
	}
}

void matrix_print(void) {
	
}
