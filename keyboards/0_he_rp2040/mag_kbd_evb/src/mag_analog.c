#include <stdint.h>
#include <stdbool.h>
#include "hal.h"
#include "ch.h"
#include "quantum.h"
#include "config.h"
#include "src/include/mag_analog.h"

/*----mode check----*/
#if (defined(DIRECT_ADC_MODE) + defined(DIRECT_MUX_MODE) + defined(CHAIN_MUX_MODE)) != 1
#error "You can define only one ADC connection mode."
#endif

/**ADC pin**/
	//ADC pins
static const pin_t adc_pins[] = ADC_PINS;

	//ADC pin
#if defined(DIRECT_ADC_MODE) || defined(DIRECT_MUX_MODE)
	static pin_t adc_pin;
#elif defined(CHAIN_MUX_MODE)
	#define adc_pin adc_pins[0]
#endif

	//set adc pin
void set_adc_pin(uint8_t row) {
	#if defined(DIRECT_ADC_MODE) || defined(DIRECT_MUX_MODE)
		adc_pin = adc_pins[row];
	#endif
}



/***--------mux--------***/

	//MUX port Switching delay us
#ifndef MUX_SW_DELAY
	#define MUX_SW_DELAY 1
#endif

/**----direct mux mode only----**/
#if defined(DIRECT_MUX_MODE) || defined(CHAIN_MUX_MODE)

	// MUX channel select order(correspond to col)
static const uint8_t mux_ch_order[MATRIX_COLS] = MUX_CH_ORDER;

	// MUX channel select pins
static const pin_t mux_select_pins[] = MUX_SELECT_PINS;

	//init MUX select pins
void init_mux_select(void) {
	for (uint8_t i = 0; i < ARRAY_SIZE(mux_select_pins); i++) {
        setPinOutput(mux_select_pins[i]);
    }
}

	//Switch MUX channel(active high)
void set_mux_ch(uint8_t col) {
	for (uint8_t i = 0; i < ARRAY_SIZE(mux_select_pins); i++) {
		writePin(mux_select_pins[i], (mux_ch_order[col] >> i) & 1);
	}
	chThdSleepMicroseconds(MUX_SW_DELAY);
}

#endif


/**----chain mux mode only----**/
#if defined(CHAIN_MUX_MODE)

	// MUX enable pins（correspond to row）(active low)
static const pin_t mux_en_pins[MATRIX_ROWS] = MUX_ENABLE_PINS;

void enable_mux(uint8_t row) {
	writePinLow(mux_en_pins[row]);
}

void disable_mux(uint8_t row) {
	writePinHigh(mux_en_pins[row]);
}

void init_mux_en(void) {
	for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
		setPinOutput(mux_en_pins[row]);
		disable_mux(row);
	}
}

#endif


/**----call----**/

void enable_row(uint8_t row) {
	#if defined(DIRECT_ADC_MODE) || defined(DIRECT_MUX_MODE)
		set_adc_pin(row);
	#elif defined(CHAIN_MUX_MODE)
		enable_mux(row);
	#endif
}

void disable_row(uint8_t row) {
	#if defined(CHAIN_MUX_MODE)
		disable_mux(row);
	#endif
}

void enable_col(uint8_t col) {
	#if defined(DIRECT_MUX_MODE) || defined(CHAIN_MUX_MODE)
		set_mux_ch(col);
	#endif
}

void init_mux(void) {
	#if defined(DIRECT_MUX_MODE) || defined(CHAIN_MUX_MODE)
		init_mux_select();
	#endif
	
	#if defined(CHAIN_MUX_MODE)
		init_mux_en();
	#endif
}



/***--------adc--------***/

	//average adc sample size
#ifndef SAMPLING_COUNT_EXP
#define SAMPLING_COUNT_EXP 4
#endif
#define SAMPLING_COUNT (1U << SAMPLING_COUNT_EXP)


	//adc_evt mask
#define ADC_EVT_DONE  (1U << 0)
#define ADC_EVT_ERROR (1U << 1)
#define ADC_EVT_MASK  EVENT_MASK(0)

	//buffer
static adcsample_t adc_buffer[2][SAMPLING_COUNT];

	//buffer[buf_sel]
static bool buf_sel = 0;

	//prev adc conv state
static volatile bool adc_conv_ok = 0;

	//adc conv wait event
static event_source_t adc_evt;
static event_listener_t adc_listener;

	//adc end callback
static void adc_end_callback(ADCDriver *adcp) {
	(void)adcp;
    chSysLockFromISR();
    chEvtBroadcastFlagsI(&adc_evt, ADC_EVT_DONE);
    chSysUnlockFromISR();
}

	//adc error callback
static void adc_error_callback(ADCDriver *adcp, adcerror_t err) {
	(void)adcp;
    (void)err;
    chSysLockFromISR();
    chEvtBroadcastFlagsI(&adc_evt, ADC_EVT_ERROR);
    chSysUnlockFromISR();
}

	//ADC read settings
static ADCConversionGroup adc_group = {
    false,					// circular (after the specified number of times, stop or continue)
    1,						// num_channels (number of pins to read)
    &adc_end_callback,		//end_cb
    &adc_error_callback,	//error_cb
    0                       // channel_mask は後で設定
};

	//init ADC pins
void init_adc_pins(void) {
	for (uint8_t i = 0; i < ARRAY_SIZE(adc_pins); i++) {
		palSetLineMode(adc_pins[i], PAL_MODE_INPUT_ANALOG);
	}
}

	//init ADC
void init_adc(void) { //**********voidに変更しろ！！！！！！！
		//ADC initial settings
	static const ADCConfig adc_config = {
		0,    //div_int
		0,    //div_frac
		false //shift
	};
		//init ADC pins
	init_adc_pins();
		//ADC Launch
	adcStart(&ADCD1, &adc_config);
		//init event_source
	chEvtObjectInit(&adc_evt);
    chEvtRegisterMask(&adc_evt, &adc_listener, ADC_EVT_MASK);
}

	//start adc conversion
void start_adc_conv(void) {
    chEvtGetAndClearFlags(&adc_listener);
	
		//start adc conv
	adc_group.channel_mask = 1U << (adc_pin - 26); //GP26, 27, 28, 29->0, 1, 2, 3
	adcStartConversion(&ADCD1, &adc_group, adc_buffer[buf_sel], SAMPLING_COUNT);
	
	buf_sel = !buf_sel; //switch
}

	//wait adc conversion complete, check status
void wait_adc_conv(void) {
	chEvtWaitAny(ADC_EVT_MASK);

	eventflags_t flags = chEvtGetAndClearFlags(&adc_listener);
	adc_conv_ok = ((flags & ADC_EVT_DONE) != 0U) && ((flags & ADC_EVT_ERROR) == 0U);
}

	//get prev adc conv OK/NG
bool adc_conv_is_ok(void){
	return adc_conv_ok;
}

	//some times been since last scan, set adc_conv_ok false
void set_adc_conv_ng(void){
	adc_conv_ok = false;
}

	//get averaging value
uint16_t get_adc_value(void) {
	uint32_t sum = 0;
	for(int i = 0; i < SAMPLING_COUNT; i++){
		sum += adc_buffer[buf_sel][i];
	}
	return sum >> SAMPLING_COUNT_EXP;
}

uint16_t get_adc_value_pcb_cal(void) {
	uint32_t sum = 0;
	for(int i = 0; i < SAMPLING_COUNT; i++){
		sum += adc_buffer[!buf_sel][i];
	}
	return sum >> SAMPLING_COUNT_EXP;
}