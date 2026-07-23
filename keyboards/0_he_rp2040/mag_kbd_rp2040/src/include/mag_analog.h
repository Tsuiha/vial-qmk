#ifndef MAG_ANALOG_H
#define MAG_ANALOG_H

#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "config.h"


/**mux**/

void init_mux(void);

/*row*/

void enable_row(uint8_t row);

void disable_row(uint8_t row);

/*col*/

void enable_col(uint8_t col);


/**adc**/

void init_adc(void);

void start_adc_conv(void);

void wait_adc_conv(void);

bool adc_conv_is_ok(void);

void set_adc_conv_ng(void);

uint16_t get_adc_value(void);

uint16_t get_adc_value_pcb_cal(void);

#endif