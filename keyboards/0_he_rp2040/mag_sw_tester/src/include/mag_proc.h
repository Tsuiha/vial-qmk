#ifndef MAG_PROC_H
#define MAG_PROC_H

#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "config.h"
#include "src/include/key_type.h"

typedef enum {
    KEY_EVENT_NONE,
    KEY_EVENT_PRESS,
    KEY_EVENT_RELEASE
}key_event_t;

//dynamic key input
key_event_t dyn_key_input(
    key_states_t *states, //pass states[idx]
    const dyn_key_param_t *param, //pass param[idx]
    const dyn_key_param2_t *param2, //pass param2[idx]
	const uint16_t offset
);

//any key canceling
void any_key_ccl(
	key_states_t *states, //pass keystates
	const ccl_key_param_t *param //pass ccl_key_param[idx]
);

bool ccl_is_enabled(void);
void ccl_set_enabled(bool enabled);
void ccl_toggle_enabled(void);

#endif
