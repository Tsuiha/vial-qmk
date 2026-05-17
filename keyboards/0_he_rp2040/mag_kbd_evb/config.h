#ifndef CONFIG_H
#define CONFIG_H

/**magnetic keyboard with rp2040 by vial-qmk**/

	/*
	* you must select the ADC connnection mode from 3 design pattern
	*   1. DIRECT_ADC_MODE -->MUXを使用せず各ADCピンに各ホールセンサを直接接続する。最大4キー。
	*   2. DIRECT_MUX_MODE -->各ADCピンに各MUXを直接接続する。最大4MUX。
	*   3. CHAIN_MUX_MODE -->一つのADCピンにすべてのMUXを接続し、MUXの切り替えをENピンで制御する。5つ以上のMUX。
	*/
//#define DIRECT_ADC_MODE
#define DIRECT_MUX_MODE
//#define CHAIN_MUX_MODE


	/*
	* available ADC pins are GP26, GP27, GP28, GP29
	*   1,2. DIRECT_MODE -->使用するADCピンの配列を定義
	*   3. CHAIN_MODE -->ADCピンは一つだけ使用する
	*/
#define ADC_PINS {GP29, GP28, GP27, GP26}
//#define ADC_PINS {GP26}


	/*
	* 3. CHAIN_MODE -->各MUXのENピンに接続するピン
	*   Order the MUX enable pins according to ROW{row0, row1, ...}
	*   The number of elements matches MATRIX_ROWS.
	*/
//#define MUX_ENABLE_PINS {GP7, GP6, GP5, GP4, GP3, GP2}


	/*
	* mux channel order
	* 2,3. MUX_MODE -->MUXの使用するchをcol順に並べる
	*   Order the MUX channel numbers according to the COL{col0, col1, ...}
	*   The number of elements matches MATRIX_COLS.
	*/
#define MUX_CH_ORDER {0, 1, 2, 3, 4, 5, 6, 7}


	/*
	* mux ch select pins
	* 2,3. MUX_MODE -->MUXch切り替えに使用するselect pinsを設定する
	*   {LSB, ... ,MSB}
	*/
#define MUX_SELECT_PINS {GP13, GP15, GP14}


	/*
	* Matrix size
	* ROWS
	*   1,2. DIRECT_MODE -->ROWS=使用するADCピンの数
	*   3. CHAIN_MODE -->ROWS=使用するMUX数
	* COLS
	*   1. NO_MUX_MODE -->COLS=1
	*   2,3. MUX_MODE -->COLS=使用するMUXのch数
	*/
#define MATRIX_ROWS 4
#define MATRIX_COLS 8
#define MATRIX_SIZE (MATRIX_ROWS * MATRIX_COLS)



//pcb cal trigger pin
#define PCB_CAL_PIN GP9

//switch cal trigger pin
#define SW_CAL_PIN GP0

//sensi offset pin
//#define SENSI_OFFSET_PIN GP2
#define SENSI_OFFSET_PINS {GP1, GP2, GP3, GP4, GP5}


//socd mode slide switch 3pin, not use socd->comment out
#define SOCD_PINS {GP6, GP7, GP8}

/*
*gaming key for high sensi & socd,
*not use gaming key -> plz comment out
*idx = row * MATRIX_COLS + col
*/
#define W_KEY_IDX 13
#define A_KEY_IDX 10
#define S_KEY_IDX 12
#define D_KEY_IDX 14

//adc resolution
#define ADC_RESOLUTION 12

//EECONFIG size
#define EECONFIG_KB_DATA_SIZE 2048

//check scan rate
//#define DEBUG_MATRIX_SCAN_RATE

//debug console
#define DEBUG_INFO




/*----already dfined other file, plz overwrite needed----*/

//switch stroke um
#define DEF_STROKE 3400

//averaging adc sample size (2^SAMPLING_COUNT_EXP)
//#define SAMPLING_COUNT_EXP 4

//actuation point um
//#define DEF_AP 400
//actuation trigger um
//#define DEF_AT 50
//reset point um
//#define DEF_RP 100
//reset trigger um
//#define DEF_RT 50
//error point um
//#define DEF_EP 50
//ap zone um
//#define DEF_WOB 150
//actuation point um for gaming key
//#define GAME_AP 100

//cancel threshold socd hybrid um
//#define DEF_CCL_TH 100

//switch calibration threshold
//#define CAL_TH_MAX 500
//#define CAL_TH_MIN 100

//sensi offset stage um
//#define DEF_OFFSET_PONT {0, 50, 100, 200, 500}
//#define DEF_OFFSET_NUM 5

//ref adc value
//#define DEF_REF_ADC_VAL 2000
//ref point
//#define DEF_REF_POINT 1700
//magnetic gain
//#define DEF_MAG_GAIN 8000


//lift of distance um
//#define LOD 1000

//eepconfig magic
//#define CONFIG_MAGIC 0x4B01

//MUX port Switching delay us
//#define MUX_SW_DELAY 1

#endif