#ifndef MAG_UTILS_H
#define MAG_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "config.h"
#include "src/include/key_layout.h"
#include "src/include/key_type.h"


	//current mode
typedef enum {
	MODE_INPUT,
	MODE_CAL
}mode_t;

	//switch mode event
typedef enum {
	MODE_EVENT_NONE,
	MODE_EVENT_ENTER_CAL,//press
	MODE_EVENT_EXIT_CAL//release
}mode_event_t;


	//get current mode input/calibration
mode_t mode_get(void);

	//update current mode input/calibration, return event enter calibration/exit calibration
mode_event_t mode_update(void);

	//sensi offset
uint16_t offset_update(void);
void init_offset_pins(void);


	//get last scan key idx
uint8_t get_last_valid_index(void);

	//all keys action_exec(false), not update matrix_row_t, not send report
bool release_all_keys (
	key_states_t *states,
	const key_layout_t *key_layout
);

	//socd test
uint8_t get_socd_mode(void);
void init_socd_pins(void);

#endif

