#ifndef CONFIG_H
#define CONFIG_H

/* Magnetic keyboard with RP2040 by vial-qmk */

/* ADC connection mode */
//#define DIRECT_ADC_MODE
#define DIRECT_MUX_MODE
//#define CHAIN_MUX_MODE

/* ADC pins: row order */
#define ADC_PINS {GP29, GP28, GP27, GP26}
//#define ADC_PINS {GP26}

/* MUX channel order: column order */
#define MUX_CH_ORDER {0, 1, 2, 3, 4, 5, 6, 7}

/* MUX channel select pins: LSB to MSB */
#define MUX_SELECT_PINS {GP13, GP15, GP14}

/* Matrix size */
#define MATRIX_ROWS 4
#define MATRIX_COLS 8
#define MATRIX_SIZE (MATRIX_ROWS * MATRIX_COLS)

/*  optional( software-only control) */
/* Calibration trigger pins*/
#define PCB_CAL_PIN GP9
#define SW_CAL_PIN GP0

/* Sensitivity offset stage pins*/
#define SENSI_OFFSET_PINS {GP1, GP2, GP3, GP4, GP5}

/* Cancel enable slide switch: pin0 = disabled, pin1 = enabled*/
#define SOCD_PINS {GP6, GP7}


/* EECONFIG size */
#define EECONFIG_KB_DATA_SIZE 2048

/* Scan-rate check */
//#define DEBUG_MATRIX_SCAN_RATE

#endif
