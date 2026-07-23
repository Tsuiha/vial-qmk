#ifndef MAG_REALTIME_H
#define MAG_REALTIME_H

#include <stdint.h>

int16_t mag_realtime_get_d_cur(uint8_t idx);
uint16_t mag_realtime_get_adc_cur(uint8_t idx);

#endif
