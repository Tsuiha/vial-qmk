#include QMK_KEYBOARD_H

enum layer_names {
    keymap_0,
    keymap_1,
    keymap_2,
    keymap_3
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
[keymap_0] = LAYOUT(
        _______
    ),
[keymap_1] = LAYOUT(
        _______
    ),
[keymap_2] = LAYOUT(
        _______
    ),
[keymap_3] = LAYOUT(
        _______
    )
};
