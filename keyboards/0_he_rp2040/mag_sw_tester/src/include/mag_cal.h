#ifndef MAG_CAL_H
#define MAG_CAL_H

#include <stdint.h>
#include "quantum.h"
#include "config.h"
#include "src/include/key_type.h"

void pcb_calibration(cal_key_param_t *param);
bool run_pcb_calibration(void);

void calibrate_adc_range(
    uint16_t adc_value,
    key_states_t *states
);

bool finalize_calibration(
    key_states_t *states,
    dyn_key_param_t *dyn_param,
    cal_key_param_t *cal_param
);

int32_t calc_position(
    const uint16_t adc_value,
    const cal_key_param_t *param
);

#endif
