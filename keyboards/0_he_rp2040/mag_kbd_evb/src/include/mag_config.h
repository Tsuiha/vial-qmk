#ifndef MAG_CONFIG_H
#define MAG_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "eeconfig.h"
#include "config.h"
#include "src/include/key_type.h"

//matrix.cに渡す
typedef struct {
    dyn_key_param_t  *dyn;
    dyn_key_param2_t *dyn2;
    ccl_key_param_t  *ccl;
    cal_key_param_t  *cal;
} mag_config_view_t;

//読み出し用
const mag_config_view_t *mag_config_get_view(void);

void init_mag_config(void);

void save_header(void);

void save_dyn_param(void);

void save_ccl_param(void);

void save_cal_param(void);

	//socd test
void update_socd(uint8_t mode);

#endif