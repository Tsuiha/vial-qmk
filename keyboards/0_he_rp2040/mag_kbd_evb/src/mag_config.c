#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "eeconfig.h"
#include "config.h"
#include "src/include/key_type.h"
#include "src/include/mag_config.h"

/*
eeconfig_header_t
dyn_key_param_t
ccl_key_param_t
cal_key_param_t
*/

	//eeconfig_header
typedef struct {
	uint16_t magic;
//	uint16_t crc_dyn;
//	uint16_t crc_ccl;
	uint16_t crc_cal;
} eeconfig_header_t;

//ram
static eeconfig_header_t g_header;
static dyn_key_param_t dyn_param[MATRIX_SIZE];
static dyn_key_param2_t dyn_param2[MATRIX_SIZE];
static ccl_key_param_t ccl_param[MATRIX_SIZE];
static cal_key_param_t cal_param[MATRIX_SIZE];

//matrix.c read
static const mag_config_view_t config_view = {
    .dyn  = dyn_param,
//    .dyn  = &dyn_param,
    .dyn2 = dyn_param2,
//    .dyn2 = &dyn_param2,
    .ccl  = ccl_param,
    .cal  = cal_param,
};

const mag_config_view_t *mag_config_get_view(void) {
    return &config_view;
}


	//magic
#define CONFIG_MAGIC 0x4B01

	//eeconfig address
#define EEPROM_ADDR_HEADER 0
//#define EEPROM_ADDR_DYN (EEPROM_ADDR_HEADER + sizeof(eeconfig_header_t))
//#define EEPROM_ADDR_CCL (EEPROM_ADDR_DYN + sizeof(dyn_key_param_t) * MATRIX_SIZE)
//#define EEPROM_ADDR_CAL (EEPROM_ADDR_CCL + sizeof(ccl_key_param_t) * MATRIX_SIZE)
#define EEPROM_ADDR_CAL (EEPROM_ADDR_HEADER + sizeof(eeconfig_header_t)) //ソフトウェアレス


//calc crc
static uint16_t calc_crc16(uint8_t *data, uint16_t len){
	uint16_t crc = 0xFFFF;
	for (uint16_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint8_t j = 0; j < 8; j++) {
			crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
		}
	}
	return crc;
}


	//各param用をloadしてcrcチェック
static bool load_with_crc(void *dst,
                   uint16_t addr,
                   uint16_t size,
                   uint16_t *crc_field)
{
	eeconfig_read_kb_datablock(dst, addr, size);
	uint16_t crc = calc_crc16((uint8_t*)dst, size);//eedconfigのデータから再計算されたものと整合してるだけ。
	return (crc == *crc_field);//一致していたらtrue
}


/***--------save eeconfig--------***/
void save_header(void){
	eeconfig_update_kb_datablock(
		&g_header,
	EEPROM_ADDR_HEADER,
		sizeof(g_header)
	);
}

//ソフトウェアレス
/*
void save_dyn_param(void){
	eeconfig_update_kb_datablock(
		dyn_param,
		EEPROM_ADDR_DYN,
		sizeof(dyn_param)
	);
	g_header.crc_dyn = calc_crc16((uint8_t*)dyn_param, sizeof(dyn_param));
}

void save_ccl_param(void){
	eeconfig_update_kb_datablock(
		ccl_param,
		EEPROM_ADDR_CCL,
		sizeof(ccl_param)
	);
	g_header.crc_ccl = calc_crc16((uint8_t*)ccl_param, sizeof(ccl_param));
}
*/

void save_cal_param(void){
	eeconfig_update_kb_datablock(
		cal_param,
		EEPROM_ADDR_CAL,
		sizeof(cal_param)
	);
	g_header.crc_cal = calc_crc16((uint8_t*)cal_param, sizeof(cal_param));
}


/**--------init dyn param--------**/
//actuation point um
#ifndef DEF_AP
#define DEF_AP 400
#endif
//actuation trigger um
#ifndef DEF_AT
#define DEF_AT 50
#endif
//reset point um
#ifndef DEF_RP
#define DEF_RP 100
#endif
//reset trigger um
#ifndef DEF_RT
#define DEF_RT 50
#endif
//error point um
#ifndef DEF_EP
#define DEF_EP 50
#endif
//switch stroke um
#ifndef DEF_STROKE
#define DEF_STROKE 3500
#endif
//wobble um
#ifndef DEF_WOB
#define DEF_WOB 150
#endif

//actuation point um for gaming key
#ifndef GAME_AP
#define GAME_AP 100
#endif

/*
	//eeprom no data時に呼び出し、def値を入れる
void init_dyn_param(void) {
	for(int i = 0; i < MATRIX_SIZE; i++) {
			//key parameters
		dyn_param[i].act_pt = DEF_AP;
		dyn_param[i].act_trg = DEF_AT;
		dyn_param[i].rst_pt = DEF_RP;
		dyn_param[i].rst_trg = DEF_RT;
		dyn_param[i].stroke = DEF_STROKE;
	}
}
*/

	//eeprom no data
void init_dyn_param(void) {
	for(int i = 0; i < MATRIX_SIZE; i++) {
			//key parameters
		dyn_param[i].act_pt = DEF_AP;
		dyn_param[i].act_trg = DEF_AT;
		dyn_param[i].rst_pt = DEF_RP;
		dyn_param[i].rst_trg = DEF_RT;
		dyn_param[i].stroke = DEF_STROKE;
	}
	#if defined(W_KEY_IDX) && defined(A_KEY_IDX) && defined(S_KEY_IDX) && defined(D_KEY_IDX)
		dyn_param[W_KEY_IDX].act_pt = GAME_AP;
		dyn_param[A_KEY_IDX].act_pt = GAME_AP;
		dyn_param[S_KEY_IDX].act_pt = GAME_AP;
		dyn_param[D_KEY_IDX].act_pt = GAME_AP;
	#endif
}


/**--------init ccl param--------**/
//target none
#ifndef NO_TGT
#define NO_TGT 255
#endif
//cancel threshold
#ifndef DEF_CCL_TH
#define DEF_CCL_TH 100
#endif

//init cancel key parameters 
void init_ccl_param(void) {
	for(int i = 0; i < MATRIX_SIZE; i++) {
			//key parameters
		ccl_param[i].tgt_idx = NO_TGT;
		ccl_param[i].tgt_th = DEF_CCL_TH;
	}
}


	//socd設定
void disable_socd(void){
	#if defined(W_KEY_IDX) && defined(A_KEY_IDX) && defined(S_KEY_IDX) && defined(D_KEY_IDX)
		//w
	ccl_param[W_KEY_IDX].tgt_idx = S_KEY_IDX;
	ccl_param[W_KEY_IDX].tgt_th = DEF_CCL_TH;
		//a
	ccl_param[A_KEY_IDX].tgt_idx = D_KEY_IDX;
	ccl_param[A_KEY_IDX].tgt_th = DEF_CCL_TH;
		//s
	ccl_param[S_KEY_IDX].tgt_idx = W_KEY_IDX;
	ccl_param[S_KEY_IDX].tgt_th = DEF_CCL_TH;
		//d
	ccl_param[D_KEY_IDX].tgt_idx = A_KEY_IDX;
	ccl_param[D_KEY_IDX].tgt_th = DEF_CCL_TH;
	#endif
}

void enable_socd_hybrid(void){
	#if defined(W_KEY_IDX) && defined(A_KEY_IDX) && defined(S_KEY_IDX) && defined(D_KEY_IDX)
		//w
	ccl_param[W_KEY_IDX].tgt_idx = S_KEY_IDX;
	ccl_param[W_KEY_IDX].tgt_th = DEF_CCL_TH;
		//a
	ccl_param[A_KEY_IDX].tgt_idx = D_KEY_IDX;
	ccl_param[A_KEY_IDX].tgt_th = DEF_CCL_TH;
		//s
	ccl_param[S_KEY_IDX].tgt_idx = W_KEY_IDX;
	ccl_param[S_KEY_IDX].tgt_th = DEF_CCL_TH;
		//d
	ccl_param[D_KEY_IDX].tgt_idx = A_KEY_IDX;
	ccl_param[D_KEY_IDX].tgt_th = DEF_CCL_TH;
	#endif
}

void enable_socd_forced(void){
	#if defined(W_KEY_IDX) && defined(A_KEY_IDX) && defined(S_KEY_IDX) && defined(D_KEY_IDX)
		//w
	ccl_param[W_KEY_IDX].tgt_idx = S_KEY_IDX;
	ccl_param[W_KEY_IDX].tgt_th = 0;
		//a
	ccl_param[A_KEY_IDX].tgt_idx = D_KEY_IDX;
	ccl_param[A_KEY_IDX].tgt_th = 0;
		//s
	ccl_param[S_KEY_IDX].tgt_idx = W_KEY_IDX;
	ccl_param[S_KEY_IDX].tgt_th = 0;
		//d
	ccl_param[D_KEY_IDX].tgt_idx = A_KEY_IDX;
	ccl_param[D_KEY_IDX].tgt_th = 0;
	#endif
}

void update_socd(uint8_t mode){
	switch(mode){
		case 0:
			disable_socd();
			#ifdef DEBUG_INFO
			print("SOCD MODE->OFF\n");
			#endif
			break;
		case 1:
			enable_socd_hybrid();
			#ifdef DEBUG_INFO
			print("SOCD MODE->HYBRID\n");
			#endif
			break;
		case 2:
			enable_socd_forced();
			#ifdef DEBUG_INFO
			print("SOCD MODE->FORCED\n");
			#endif
			break;
	}
}



/**--------init cal param--------**/
//ref adc value
#ifndef DEF_REF_ADC_VAL
#define DEF_REF_ADC_VAL 2000
#endif
//ref point
#ifndef DEF_REF_POINT
#define DEF_REF_POINT 1700
#endif
//magnetic gain
#ifndef DEF_MAG_GAIN
#define DEF_MAG_GAIN 8000
#endif

//init
void init_cal_param(void) {
	for (int i = 0; i < MATRIX_SIZE; i++) {
		cal_param[i].ref_adc_val = DEF_REF_ADC_VAL;
		cal_param[i].ref_point = DEF_REF_POINT;
		cal_param[i].mag_gain = DEF_MAG_GAIN;
	}
}




/**--------init no eeprom--------**/
/*ソフトウェアレス
void init_dyn_param2(void) {
	for(int i = 0; i < MATRIX_SIZE; i++) {
		const uint16_t d_ap = dyn_param[i].act_pt;
		const uint16_t d_at = dyn_param[i].act_trg;
		const uint16_t d_rp = dyn_param[i].rst_pt;
		const uint16_t d_rt = dyn_param[i].rst_trg;
			//key parameters2
		dyn_param2[i].act_ht = dyn_param[i].stroke - d_ap;
		
		if(d_rp < d_at + DEF_EP){
			if(d_rp < DEF_EP){
				dyn_param2[i].act_pt2 = 0;
			}else{
				dyn_param2[i].act_pt2 = d_rp - DEF_EP;
			}
		}
		
		if(d_ap < d_rt + DEF_EP){
			if(d_ap < DEF_EP){
				dyn_param2[i].rst_pt2 = 0;
			}else{
				dyn_param2[i].rst_pt2 = d_ap - DEF_EP;
				
			}
		}
	}
}
*/

//ソフトウェアレス
void init_dyn_param2(void){
	for(int i = 0; i < MATRIX_SIZE; i++) {
			//key parameters2
		dyn_param2[i].act_ht = DEF_STROKE - DEF_AP - DEF_WOB;
		
		if(DEF_RP < DEF_AT + DEF_EP){
			if(DEF_RP < DEF_EP){
				dyn_param2[i].act_pt2 = 0;
			}else{
				dyn_param2[i].act_pt2 = DEF_RP - DEF_EP;
			}
		}
		
		if(DEF_AP < DEF_RT + DEF_EP){
			if(DEF_AP < DEF_EP){
				dyn_param2[i].rst_pt2 = 0;
			}else{
				dyn_param2[i].rst_pt2 = DEF_AP - DEF_EP;
				
			}
		}
	}
	#if defined(W_KEY_IDX) && defined(A_KEY_IDX) && defined(S_KEY_IDX) && defined(D_KEY_IDX)
		dyn_param2[W_KEY_IDX].act_ht = DEF_STROKE - GAME_AP - DEF_WOB;
		dyn_param2[A_KEY_IDX].act_ht = DEF_STROKE - GAME_AP - DEF_WOB;
		dyn_param2[S_KEY_IDX].act_ht = DEF_STROKE - GAME_AP - DEF_WOB;
		dyn_param2[D_KEY_IDX].act_ht = DEF_STROKE - GAME_AP - DEF_WOB;
	#endif
	
}



	//起動時呼び出し
void init_mag_config(void){
	bool magic_ng = false;
	bool changed = false;
		//load header
    eeconfig_read_kb_datablock(&g_header, EEPROM_ADDR_HEADER, sizeof(g_header));

		//check magic
	if (g_header.magic != CONFIG_MAGIC) { //eeconfig have no data
		g_header.magic = CONFIG_MAGIC;
		magic_ng = true;
	}
	
	/*ソフトウェアレス
		//check dyn key param
	bool dyn_ng = !load_with_crc(dyn_param,
								EEPROM_ADDR_DYN,
								sizeof(dyn_param),
								&g_header.crc_dyn);
	
	if (dyn_ng || magic_ng) {
		init_dyn_param(); //init param in ram 
		save_dyn_param(); //calc new crc, save param and crc eeconfig
		changed = true;
	}
	init_dyn_param2();
	
	
	bool ccl_ng = !load_with_crc(ccl_param,
								EEPROM_ADDR_CCL,
								sizeof(ccl_param),
								&g_header.crc_ccl);
	
		//check ccl key param
	if (ccl_ng || magic_ng) {
		init_ccl_param(); //init param in ram 		
		save_ccl_param();
		changed = true;
	}
	*/
	
	
	bool cal_ng = !load_with_crc(cal_param,
								EEPROM_ADDR_CAL,
								sizeof(cal_param),
								&g_header.crc_cal);
	
	//check cal key param
	if (cal_ng || magic_ng) {
		init_cal_param(); //init param in ram 
		save_cal_param();
		changed = true;
	}
	
	if(changed) save_header();
	
	//設定ソフトレスなので毎回初期化する
	init_dyn_param();
	init_dyn_param2();
	init_ccl_param();
}







