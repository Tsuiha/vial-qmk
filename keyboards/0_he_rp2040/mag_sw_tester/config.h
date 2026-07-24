#ifndef CONFIG_H
#define CONFIG_H

/* Magnetic keyboard with RP2040 by vial-qmk */

/* ADC connection mode */
#define DIRECT_ADC_MODE

/* ADC pins: row order */
#define ADC_PINS {GP26}

/* Matrix size */
#define MATRIX_ROWS 1
#define MATRIX_COLS 1
#define MATRIX_SIZE (MATRIX_ROWS * MATRIX_COLS)

#define SAMPLING_COUNT_EXP 5

/* EECONFIG size */
#define EECONFIG_KB_DATA_SIZE 2048

/* Scan-rate check */
//#define DEBUG_MATRIX_SCAN_RATE

#endif
