#include <stdint.h>
#include <stdbool.h>
#include "util.h"
#include "atomic_util.h"
#include "matrix.h"
#include "quantum.h"
#include "hal.h"
#include "ch.h"
#include "config.h"
#include "src/include/key_layout.h"
#include "src/include/key_type.h"
#include "src/include/mag_analog.h"
#include "src/include/mag_config.h"
#include "src/include/mag_proc.h"
#include "src/include/mag_utils.h"
#include "src/include/mag_cal.h"

#include "wait.h"
#include "print.h"
#include "debug.h"



static matrix_row_t matrix[MATRIX_ROWS] = {0};
static uint8_t last_idx; //last scan key index
static key_states_t key_states[MATRIX_SIZE] = {0};


//param
static dyn_key_param_t  *dyn_param;
static dyn_key_param2_t *dyn_param2;
static ccl_key_param_t  *ccl_param;
static cal_key_param_t  *cal_param;


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
	bool released = release_all_keys(key_states, key_layout);
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
	gpio_set_pin_input_high(PCB_CAL_PIN);
	gpio_set_pin_input_high(SW_CAL_PIN);
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

	//get const
	last_idx = get_last_valid_index();
		
	//wait directpins standby (min 2ms)
	chThdSleepMilliseconds(5);
	
	if (!gpio_read_pin(PCB_CAL_PIN)) {
		pcb_calibration(key_layout, cal_param);
		save_cal_param();
		save_header();
	}
	
	#ifdef DEBUG_INFO
	chThdSleepMilliseconds(1000);
	print("----calibration param----\n");
	for(int i = 0; i < MATRIX_SIZE; i++) {
		printf("idx: %d, y0: %d, d0: %d, A: %d\n",i,cal_param[i].ref_adc_val,cal_param[i].ref_point,cal_param[i].mag_gain);
	}
	#endif
}

//main
uint8_t matrix_scan(void) {
	
	bool changed = false;
	bool calibrated = false;
	uint8_t prev_idx = last_idx;
	uint8_t prev_row = prev_idx / MATRIX_COLS;
	uint8_t prev_col = prev_idx % MATRIX_COLS;
	
/**----mode switch event----**/
	switch (mode_update()) {
		case MODE_EVENT_NONE://keep current mode
			break;
		
		case MODE_EVENT_ENTER_CAL: /*----enter calibration mode----*/
			reset_all_keys();
			#ifdef DEBUG_INFO
			print("ENTER CAL MODE\n");
			#endif
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
			#ifdef DEBUG_INFO
			print("EXIT CAL MODE\n");
			#endif
			break; /*----exit calibration mode----*/
			
	}/**----mode switch event----**/
	
/**----sensi offset btn----**/
	uint16_t offset_value = offset_update();
	
	
/**----socd btn----**/
		static uint8_t socd_mode_prev = 0;
		uint8_t socd_mode_cur = get_socd_mode();
		
		if(socd_mode_prev != socd_mode_cur){
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
			if (key_layout[idx].valid == false) continue;
			
			enable_col(col);
			
			start_adc_conv();
			
			if (adc_conv_is_ok()) {/*----prev adc conversion completed----*/
					//get prev idx adc value
				uint16_t adc_value = get_adc_value();
				key_states_t *st = &key_states[prev_idx];
				
				
				switch (mode_get()) {
					case MODE_INPUT:/*----input mode----*/
							//get prev idx current position
						st->pos_cur = calc_position(adc_value, &cal_param[prev_idx]);
						
						//debug
						//printf("index: %d, position: %ld, state: %d\n",prev_idx, st->pos_cur,st->state);
						//printf("index: %d, adc: %d\n",prev_idx,adc_value);
						//debug
						
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
								
								/*
								#ifdef DEBUG_INFO
								printf("pressed-> index: %d \n",prev_idx);
								#endif
								*/
								break;
								
							case KEY_EVENT_RELEASE:
								release_key_event(prev_row, prev_col);
								changed = true;
								break;
								/*
								#ifdef DEBUG_INFO
								printf("released-> index: %d \n",prev_idx);
								#endif
								*/
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
				
				
				//debug
				//wait_ms(500);
				//debug
		}/**----col----**/
			
		disable_row(row);
		
		
	}/**----row----**/
	
    return changed; //any key state changed?
}

matrix_row_t matrix_get_row(uint8_t row) {
	return matrix[row];
}

void matrix_print(void) {
	
}
