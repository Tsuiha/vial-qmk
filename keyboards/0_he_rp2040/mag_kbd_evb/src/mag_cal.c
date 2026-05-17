#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "config.h"
#include "src/include/mag_cal.h"
#include "src/include/cal_sqrt_lut.h"
#include "src/include/a2p_sqrt_lut.h"
#include "src/include/key_layout.h"
#include "src/include/key_type.h"
#include "src/include/mag_analog.h"

//threshold whether key have pushed in cal
#ifndef CAL_TH_MAX
#define CAL_TH_MAX 500
#endif
#ifndef CAL_TH_MIN
#define CAL_TH_MIN 100
#endif

/***--------pcb calibration--------***/
void pcb_calibration (const key_layout_t *key_layout,
					cal_key_param_t *param)
{
	
	for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
/**----row----**/
		enable_row(row);
		
		for (uint8_t col = 0; col < MATRIX_COLS; col++) {
	/**----col----**/
			uint8_t idx = row * MATRIX_COLS + col;
			
				//NO_USE skip
			if (key_layout[idx].valid == false) continue;
			
			enable_col(col);
			
		/*----calibrating...----*/
				//retry max 5
			bool ok = false;
			for (uint8_t i = 0; i < 5 && !ok; i++) {
				start_adc_conv();
				wait_adc_conv();
				ok = adc_conv_is_ok();
			}
			
			if(!ok) continue; //error
			
			uint16_t val = get_adc_value_pcb_cal();
			param[idx].ref_adc_val = val;
			
		}
		disable_row(row);
		
	}
	set_adc_conv_ng();
	
}




/***--------swtich calibration--------***/
	//calibration mode scan data proc
void calibrate_adc_range(	uint16_t adc_value,
							key_states_t *states
						)
{
	if(states->pos_ref == 0 ){
		states->pos_ref = adc_value; //init
		states->pos_cur = adc_value; //init
	}else if(states->pos_ref > adc_value){
		states->pos_ref = adc_value; //update min
		
			#ifdef DEBUG_INFO
			printf("update range-> min: %d \n", adc_value);
			#endif
		
	}else if(states->pos_cur < adc_value){
		states->pos_cur = adc_value; //update max
		
			#ifdef DEBUG_INFO
			printf("update range-> max: %d \n", adc_value);
			#endif
			
	}
}

	//exit calbration mode, finalize cal data
bool finalize_calibration(	key_states_t *key_states,
							dyn_key_param_t *dyn_key_param,
							cal_key_param_t *cal_key_param
							)
{
	#ifdef DEBUG_INFO
	print("switch calibration finalizing......\n");
	#endif
	
	bool changed = false;
	
	for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
/**----row----**/
		for (uint8_t col = 0; col < MATRIX_COLS; col++) {
	/**----col----**/
			int8_t idx = row * MATRIX_COLS + col;
				//NO_USE skip
			if (key_layout[idx].valid == false) continue;
			
			key_states_t *st = &key_states[idx];
			dyn_key_param_t *dp = &dyn_key_param[idx];
			cal_key_param_t *cp = &cal_key_param[idx];
			
			uint16_t stroke = dp->stroke;
			uint16_t ref = cp->ref_adc_val;
			uint16_t dmax = st->pos_cur - ref;
			uint16_t dmin = st->pos_ref - ref;
			
				//detect error
			if (dmin < CAL_TH_MIN) continue;
			if (dmax - dmin < CAL_TH_MAX) continue;
			
				//get index
			uint16_t idx_min = dmin - CAL_SQRT_START;
			uint16_t idx_max = dmax - CAL_SQRT_START;
			
				//limit max
			idx_max = (idx_max > CAL_SQRT_SIZE-1) ? CAL_SQRT_SIZE-1 : idx_max;
			
				//lut(Y) = sqrt(Y)*2^10
			uint32_t y1 = cal_sqrt_lut[idx_max];
			uint32_t y2 = cal_sqrt_lut[idx_min];
			
			uint16_t ref_point = (uint32_t)stroke * y2 / (y1 - y2);
			cp->ref_point = ref_point;
			
			//uint64_t mag_gain = ref_point * ref_point * dmax;
			uint16_t mag_gain = ((uint32_t)ref_point * y1) >>13; // = A/1024(lut)/8(a)
			cp->mag_gain = mag_gain;
			
			#ifdef DEBUG_INFO
			printf("calibrated->index: %d, d0: %d, A: %d \n",idx, ref_point, mag_gain);
			#endif
			
			changed = true;
		}
	}
	
	#ifdef DEBUG_INFO
	print("switch calibration finalized!!!!!!\n");
	#endif
	
	return changed; //changed = true, save eeprom
}


/***--------calculate current position--------***/
static inline uint16_t clamp(int16_t x, int16_t min, int16_t max) {
    return (x < min) ? min : (x > max) ? max : x;
}


int32_t calc_position(	const uint16_t adc_value,
						const cal_key_param_t *param
						)
{
		//cal parameters
	uint16_t ref_adc_val = param->ref_adc_val;
	uint16_t ref_point = param->ref_point;
	uint16_t mag_gain = param->mag_gain;
	
		//limit
	uint16_t index = clamp(adc_value - ref_adc_val - A2P_SQRT_START,
		0, A2P_SQRT_SIZE-1);
	
		//calculate adc element
	uint32_t adc_element = adc_calc_lut[index];
		//calculate key position
	uint32_t mag_position = ((uint64_t)mag_gain * adc_element ) >> 14;//要調整 Yは*2^17, Aは/2^3
	int32_t key_position = mag_position - ref_point;
	
	return key_position;
}