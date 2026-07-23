#ifndef MAG_CONFIG_H
#define MAG_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "eeconfig.h"
#include "config.h"
#include "src/include/key_type.h"
#include "src/include/key_layout.h"

typedef struct {
    dyn_key_param_t      *dyn;
    dyn_key_param2_t     *dyn2;
    ccl_key_param_t      *ccl;
    cal_key_param_t      *cal;
    sensi_offset_param_t *sensi_offset;
    layout_option_state_t *layout;
    uint8_t              *idx_valid;
} mag_config_view_t;

typedef enum {
    MAG_CONFIG_BLOCK_HEADER       = 0,
    MAG_CONFIG_BLOCK_DYN          = 1,
    MAG_CONFIG_BLOCK_CCL          = 2,
    MAG_CONFIG_BLOCK_CAL          = 3,
    MAG_CONFIG_BLOCK_LAYOUT       = 4,
    MAG_CONFIG_BLOCK_LAYOUT_STATE = 5,
    MAG_CONFIG_BLOCK_IDX_VALID    = 6,
    MAG_CONFIG_BLOCK_LAYOUT_LABEL = 7,
    MAG_CONFIG_BLOCK_SENSI_OFFSET = 8,
    MAG_CONFIG_BLOCK_ALL          = 255
} mag_config_block_t;

const mag_config_view_t *mag_config_get_view(void);

void init_mag_config(void);

void save_header(void);
void save_dyn_param(void);
void save_ccl_param(void);
void save_cal_param(void);
void save_layout_state(void);
void save_sensi_offset_param(void);

uint16_t mag_config_get_block_size(mag_config_block_t block);
uint16_t mag_config_get_crc(mag_config_block_t block);

bool mag_config_read_block(mag_config_block_t block, uint16_t offset, uint8_t *dst, uint8_t length);
bool mag_config_write_block(mag_config_block_t block, uint16_t offset, const uint8_t *src, uint8_t length);
bool mag_config_save_block(mag_config_block_t block);
bool mag_config_reset_block(mag_config_block_t block);

void mag_layout_rebuild_idx_valid(void);
bool mag_config_idx_is_valid(uint8_t idx);
const uint8_t *mag_config_get_idx_valid(void);
uint8_t mag_config_get_idx_valid_generation(void);

void update_socd(uint8_t mode);

#endif
