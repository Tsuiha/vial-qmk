#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "config.h"
#include "src/include/mag_proc.h"
#include "src/include/key_type.h"
#include "src/include/key_layout.h"

//lift of distance um
#ifndef LOD
#define LOD 1000
#endif

#ifndef DYN_INPUT_FILTER_FAST_STEP_UM
#define DYN_INPUT_FILTER_FAST_STEP_UM 100
#endif

#ifndef DYN_INPUT_FILTER_DIR_COUNT
#define DYN_INPUT_FILTER_DIR_COUNT 2
#endif

#ifndef CCL_REPRESS_EXTRA_UM
#define CCL_REPRESS_EXTRA_UM 100
#endif

#define DYN_INPUT_DIR_PRESS -1
#define DYN_INPUT_DIR_RELEASE 1

static bool ccl_enabled = true;

bool ccl_is_enabled(void) {
	return ccl_enabled;
}

void ccl_set_enabled(bool enabled) {
	ccl_enabled = enabled;
}

void ccl_toggle_enabled(void) {
	ccl_enabled = !ccl_enabled;
}

//current position
typedef enum {ZONE_AP, ZONE_RP, ZONE_MID} zone_t;

static void update_move_history(key_states_t *states, int32_t p_cur)
{
	if (!states->pos_prev_valid) {
		states->pos_prev = p_cur;
		states->move_step = 0;
		states->move_dir = 0;
		states->move_count = 0;
		states->pos_prev_valid = 1;
		return;
	}

	int32_t diff = p_cur - states->pos_prev;
	int8_t dir = (p_cur < states->pos_prev) ? DYN_INPUT_DIR_PRESS :
		(p_cur > states->pos_prev) ? DYN_INPUT_DIR_RELEASE : 0;
	states->move_step = (diff < 0) ? (uint16_t)-diff : (uint16_t)diff;

	if (dir == 0) {
		states->move_count = 0;
	} else if (states->move_dir == dir && states->move_count < UINT8_MAX) {
		states->move_count++;
	} else {
		states->move_dir = dir;
		states->move_count = 1;
	}
	states->pos_prev = p_cur;
}

static bool dyn_input_filter_pass(key_states_t *states, int8_t dir)
{
	if (states->move_step > DYN_INPUT_FILTER_FAST_STEP_UM) return true;
	return states->move_dir == dir && states->move_count >= DYN_INPUT_FILTER_DIR_COUNT;
}

	//judgement key status process
key_event_t dyn_key_input(	key_states_t *states,
							const dyn_key_param_t *param,
							const dyn_key_param2_t *param2,
							const uint16_t offset
							)
{
		//key states
	int32_t p_cur = states->pos_cur;
	int32_t *p_ref = &states->pos_ref;
	key_state_t *state = &states->state;
	update_move_history(states, p_cur);
	
	//state == CANCELING
	if (*state == KEY_CANCELING) {
		*p_ref = p_cur;
		states->ccl_repress = 1;
		*state = KEY_RELEASED;
		return KEY_EVENT_RELEASE;
	}
	
	uint16_t d_ap = param->act_pt + offset;
	uint16_t d_at = param->act_trg + offset;
	uint16_t d_rp = param->rst_pt + offset;
	uint16_t d_rt = param->rst_trg + offset;
	uint16_t d_st = param->stroke;
	uint16_t h_ap = (param2->act_ht > offset) ? param2->act_ht - offset : 0;
	uint16_t d_at2 = param2->act_pt2 + offset;
	uint16_t d_rt2 = param2->rst_pt2 + offset;
	
		//delta>0:move up
		//delta<0:move down
	int32_t delta = p_cur - *p_ref;
	

	
		//current area
	zone_t zone = (p_cur > h_ap) ? ZONE_AP :
		(p_cur < d_rp) ? ZONE_RP : ZONE_MID;
	
	uint16_t d_th;
		//threshold
	if (*state == KEY_RELEASED) {
		d_th = (zone == ZONE_AP) ? d_ap :
			(zone == ZONE_RP) ? d_at2 : d_at;
		if (states->ccl_repress) d_th += CCL_REPRESS_EXTRA_UM;
	} else {
		d_th = (zone == ZONE_AP) ? d_rt2 :
			(zone == ZONE_RP) ? d_rp  : d_rt;
	}
	
		//judge key
	if (*state == KEY_RELEASED) {
/**----prev state == released----**/
		if (p_cur > d_st + LOD) {//removed switch
			*state = KEY_REMOVED;
			*p_ref = d_st;
			states->ccl_repress = 0;
		}
		else if (delta > 0) {//move up
			*p_ref = p_cur;
			if (p_cur > d_rp) states->ccl_repress = 0;
		}
		else if (-delta > d_th) {//move down, exceed th
			if (dyn_input_filter_pass(states, DYN_INPUT_DIR_PRESS)) {
				*p_ref = p_cur;
				states->ccl_repress = 0;
				*state = KEY_PRESSED;
				return KEY_EVENT_PRESS;
			}
		}
	}
/**----prev state == pressed----**/
	else if (*state == KEY_PRESSED) {

		if (delta < 0) {//more pressed, update ref position
			*p_ref = p_cur;
		}
		else if (delta > d_th) {//released exceed th, turn released
			if (dyn_input_filter_pass(states, DYN_INPUT_DIR_RELEASE)) {
				*p_ref = p_cur;

				*state = KEY_RELEASED;
				return KEY_EVENT_RELEASE;
			}
		}
	}
/**----prev state == removed----**/
	else if (*state == KEY_REMOVED) {
		if (p_cur < d_st) {
			*state = KEY_RELEASED;
		}
	}
	return KEY_EVENT_NONE;
}


/***--------any key canceling-------***/
	//do cancel, change target state CANCELING
void any_key_ccl(key_states_t *states,
				const ccl_key_param_t *param
				)
{
	if (!ccl_enabled) return;

		//ccl_param
	const uint16_t tgt_idx = param->tgt_idx;
	const uint16_t tgt_th = param->tgt_th;
	
		//cancel target is none or error value(NO_TGT=255)
	if (tgt_idx >= MATRIX_SIZE) return;
	
		//target dont pressed
	if (states[tgt_idx].state != KEY_PRESSED) return;
	
		//target key pressed, position exceed cancel threshold
	if (tgt_th == 0 || states[tgt_idx].pos_cur > tgt_th){
		states[tgt_idx].state = KEY_CANCELING;
	}
	
	//target key pressed, position dont exceed cancel threshold
}


