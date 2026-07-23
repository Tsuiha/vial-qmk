#include "src/include/mag_keycodes.h"
#include "src/include/mag_proc.h"
#include "src/include/mag_utils.h"

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch (keycode) {
            case MAG_SENS_UP:
                offset_stage_up();
                return false;
            case MAG_SENS_DN:
                offset_stage_down();
                return false;
            case MAG_SENS_0:
                offset_stage_set(0);
                return false;
            case MAG_SENS_1:
                offset_stage_set(1);
                return false;
            case MAG_SENS_2:
                offset_stage_set(2);
                return false;
            case MAG_SENS_3:
                offset_stage_set(3);
                return false;
            case MAG_SENS_4:
                offset_stage_set(4);
                return false;
            case MAG_CCL_ON:
                ccl_set_enabled(true);
                return false;
            case MAG_CCL_OFF:
                ccl_set_enabled(false);
                return false;
        }
    }

    return process_record_user(keycode, record);
}
