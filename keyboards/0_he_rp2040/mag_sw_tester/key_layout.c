#include <stdint.h>
#include "config.h"
#include "src/include/key_layout.h"

const layout_option_state_t key_layout_default_state = {
    .option_count = 0,
    .choices = {0}
};

const key_layout_label_t key_layout_labels[] = {
    { 0, 0, "", "" },
};

const uint16_t key_layout_label_count = 0;

const key_layout_entry_t key_layout_entries[] = {
    {   0,   0,   0, U_1   ,                    0, 0 },
};

const uint16_t key_layout_entry_count = sizeof(key_layout_entries) / sizeof(key_layout_entries[0]);
