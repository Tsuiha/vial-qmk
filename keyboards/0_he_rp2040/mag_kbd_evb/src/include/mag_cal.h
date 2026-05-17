#ifndef MAG_CAL_H
#define MAG_CAL_H

#include <stdint.h>
#include "quantum.h"
#include "config.h"
#include "src/include/key_type.h"
#include "src/include/key_layout.h"


	//scan matrix, update ref point
void pcb_calibration (
	const key_layout_t *key_layout,
	cal_key_param_t *param
);
	
	//scan data proc in calibration mode
void calibrate_adc_range(
	uint16_t adc_value,
	key_states_t *states
);
	
	//exit calbration mode, finalize cal data
bool finalize_calibration(
	key_states_t *states,
	dyn_key_param_t *dyn_param,
	cal_key_param_t *cal_param
);

	//calc current key position
int32_t calc_position(
	const uint16_t adc_value,
	const cal_key_param_t *param
	);

#endif