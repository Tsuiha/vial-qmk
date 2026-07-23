#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "quantum.h"
#include "eeconfig.h"
#include "config.h"
#include "src/include/key_type.h"
#include "src/include/key_layout.h"
#include "src/include/mag_config.h"
#include "src/include/mag_proc.h"

typedef struct {
    uint16_t magic;
    uint16_t crc_dyn;
    uint16_t crc_ccl;
    uint16_t crc_cal;
    uint16_t crc_layout;
    uint16_t crc_sensi_offset;
} eeconfig_header_t;

static eeconfig_header_t g_header;
static dyn_key_param_t dyn_param[MATRIX_SIZE];
static dyn_key_param2_t dyn_param2[MATRIX_SIZE];
static ccl_key_param_t ccl_param[MATRIX_SIZE];
static cal_key_param_t cal_param[MATRIX_SIZE];
static sensi_offset_param_t sensi_offset_param;
static layout_option_state_t layout_state;
static uint8_t idx_valid[MATRIX_SIZE];
static uint8_t idx_valid_generation;

static const mag_config_view_t config_view = {
    .dyn       = dyn_param,
    .dyn2      = dyn_param2,
    .ccl       = ccl_param,
    .cal       = cal_param,
    .sensi_offset = &sensi_offset_param,
    .layout    = &layout_state,
    .idx_valid = idx_valid,
};

const mag_config_view_t *mag_config_get_view(void) {
    return &config_view;
}

#define CONFIG_MAGIC 0x4B04

#define EEPROM_ADDR_HEADER 0
#define EEPROM_ADDR_DYN (EEPROM_ADDR_HEADER + sizeof(eeconfig_header_t))
#define EEPROM_ADDR_CCL (EEPROM_ADDR_DYN + sizeof(dyn_key_param_t) * MATRIX_SIZE)
#define EEPROM_ADDR_CAL (EEPROM_ADDR_CCL + sizeof(ccl_key_param_t) * MATRIX_SIZE)
#define EEPROM_ADDR_SENSI_OFFSET (EEPROM_ADDR_CAL + sizeof(cal_key_param_t) * MATRIX_SIZE)
#define EEPROM_ADDR_LAYOUT (EEPROM_ADDR_SENSI_OFFSET + sizeof(sensi_offset_param_t))

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

static bool load_with_crc(void *dst, uint16_t addr, uint16_t size, uint16_t *crc_field)
{
    eeconfig_read_kb_datablock(dst, addr, size);
    uint16_t crc = calc_crc16((uint8_t*)dst, size);
    return (crc == *crc_field);
}

void save_header(void){
    eeconfig_update_kb_datablock(&g_header, EEPROM_ADDR_HEADER, sizeof(g_header));
}

void save_dyn_param(void){
    eeconfig_update_kb_datablock(dyn_param, EEPROM_ADDR_DYN, sizeof(dyn_param));
    g_header.crc_dyn = calc_crc16((uint8_t*)dyn_param, sizeof(dyn_param));
}

void save_ccl_param(void){
    eeconfig_update_kb_datablock(ccl_param, EEPROM_ADDR_CCL, sizeof(ccl_param));
    g_header.crc_ccl = calc_crc16((uint8_t*)ccl_param, sizeof(ccl_param));
}

void save_cal_param(void){
    eeconfig_update_kb_datablock(cal_param, EEPROM_ADDR_CAL, sizeof(cal_param));
    g_header.crc_cal = calc_crc16((uint8_t*)cal_param, sizeof(cal_param));
}

void save_sensi_offset_param(void){
    eeconfig_update_kb_datablock(&sensi_offset_param, EEPROM_ADDR_SENSI_OFFSET, sizeof(sensi_offset_param));
    g_header.crc_sensi_offset = calc_crc16((uint8_t*)&sensi_offset_param, sizeof(sensi_offset_param));
}

void save_layout_state(void){
    eeconfig_update_kb_datablock(&layout_state, EEPROM_ADDR_LAYOUT, sizeof(layout_state));
    g_header.crc_layout = calc_crc16((uint8_t*)&layout_state, sizeof(layout_state));
}

#ifndef DEF_AP
#define DEF_AP 400
#endif
#ifndef DEF_AT
#define DEF_AT 50
#endif
#ifndef DEF_RP
#define DEF_RP 100
#endif
#ifndef DEF_RT
#define DEF_RT 50
#endif
#ifndef DEF_EP
#define DEF_EP 50
#endif
#ifndef DEF_STROKE
#define DEF_STROKE 3500
#endif
#ifndef DEF_WOB
#define DEF_WOB 150
#endif
void init_dyn_param(void) {
    for(int i = 0; i < MATRIX_SIZE; i++) {
        dyn_param[i].act_pt = DEF_AP;
        dyn_param[i].act_trg = DEF_AT;
        dyn_param[i].rst_pt = DEF_RP;
        dyn_param[i].rst_trg = DEF_RT;
        dyn_param[i].stroke = DEF_STROKE;
    }
}

#ifndef NO_TGT
#define NO_TGT 255
#endif
#ifndef DEF_CCL_TH
#define DEF_CCL_TH 100
#endif

void init_ccl_param(void) {
    for(int i = 0; i < MATRIX_SIZE; i++) {
        ccl_param[i].tgt_idx = NO_TGT;
        ccl_param[i].tgt_th = DEF_CCL_TH;
    }
}

void update_socd(uint8_t mode){
    ccl_set_enabled(mode != 0);
}

#ifndef DEF_REF_ADC_VAL
#define DEF_REF_ADC_VAL 2000
#endif
#ifndef DEF_REF_POINT
#define DEF_REF_POINT 1700
#endif
#ifndef DEF_MAG_GAIN
#define DEF_MAG_GAIN 8000
#endif
#ifndef DEF_OFFSET_POINT
#define DEF_OFFSET_POINT {0, 50, 100, 200, 500}
#endif

void init_cal_param(void) {
    for (int i = 0; i < MATRIX_SIZE; i++) {
        cal_param[i].ref_adc_val = DEF_REF_ADC_VAL;
        cal_param[i].ref_point = DEF_REF_POINT;
        cal_param[i].mag_gain = DEF_MAG_GAIN;
    }
}

void init_sensi_offset_param(void) {
    const uint16_t defaults[SENSI_OFFSET_STAGE_COUNT] = DEF_OFFSET_POINT;
    for (uint8_t i = 0; i < SENSI_OFFSET_STAGE_COUNT; i++) {
        sensi_offset_param.value[i] = defaults[i];
    }
}

static uint16_t sub_floor_u16(uint16_t value, uint16_t sub)
{
    return value > sub ? value - sub : 0;
}

void init_dyn_param2(void){
    for(int i = 0; i < MATRIX_SIZE; i++) {
        const dyn_key_param_t *param = &dyn_param[i];
        dyn_param2[i].act_ht = sub_floor_u16(param->stroke, param->act_pt + DEF_WOB);
        dyn_param2[i].act_pt2 = (param->rst_pt < param->act_trg + DEF_EP) ? sub_floor_u16(param->rst_pt, DEF_EP) : param->act_trg;
        dyn_param2[i].rst_pt2 = (param->act_pt < param->rst_trg + DEF_EP) ? sub_floor_u16(param->act_pt, DEF_EP) : param->rst_trg;
    }
}

static void init_layout_state(void) {
    layout_state = key_layout_default_state;
}

static uint8_t key_layout_required_option_count(void) {
    uint8_t option_count = 0;
    for (uint16_t i = 0; i < key_layout_entry_count; i++) {
        uint8_t option = key_layout_entries[i].option;
        if (option != LAYOUT_OPTION_COMMON && option <= LAYOUT_OPTION_MAX && option > option_count) {
            option_count = option;
        }
    }
    return option_count;
}

static bool key_layout_choice_exists(uint8_t option, uint8_t choice) {
    for (uint16_t i = 0; i < key_layout_entry_count; i++) {
        const key_layout_entry_t *entry = &key_layout_entries[i];
        if (entry->option == option && entry->choice == choice) {
            return true;
        }
    }
    return false;
}

static bool layout_state_is_valid(void) {
    uint8_t required_option_count = key_layout_required_option_count();
    if (layout_state.option_count != required_option_count || layout_state.option_count > LAYOUT_OPTION_MAX) {
        return false;
    }
    for (uint8_t option = 1; option <= required_option_count; option++) {
        if (!key_layout_choice_exists(option, layout_state.choices[option - 1])) {
            return false;
        }
    }
    return true;
}

void mag_layout_rebuild_idx_valid(void) {
    memset(idx_valid, 0, sizeof(idx_valid));
    for (uint16_t i = 0; i < key_layout_entry_count; i++) {
        const key_layout_entry_t *entry = &key_layout_entries[i];
        if (entry->idx >= MATRIX_SIZE) continue;

        bool active = false;
        if (entry->option == LAYOUT_OPTION_COMMON) {
            active = true;
        } else if (entry->option <= LAYOUT_OPTION_MAX) {
            uint8_t option_index = entry->option - 1;
            if (option_index < layout_state.option_count) {
                active = layout_state.choices[option_index] == entry->choice;
            }
        }

        if (active) {
            idx_valid[entry->idx] = 1;
        }
    }
    idx_valid_generation++;
}

bool mag_config_idx_is_valid(uint8_t idx) {
    return idx < MATRIX_SIZE && idx_valid[idx] != 0;
}

const uint8_t *mag_config_get_idx_valid(void) {
    return idx_valid;
}

uint8_t mag_config_get_idx_valid_generation(void) {
    return idx_valid_generation;
}

void init_mag_config(void){
    bool magic_ng = false;
    bool changed = false;
    eeconfig_read_kb_datablock(&g_header, EEPROM_ADDR_HEADER, sizeof(g_header));

    if (g_header.magic != CONFIG_MAGIC) {
        g_header.magic = CONFIG_MAGIC;
        magic_ng = true;
    }

    bool dyn_ng = !load_with_crc(dyn_param, EEPROM_ADDR_DYN, sizeof(dyn_param), &g_header.crc_dyn);
    if (dyn_ng || magic_ng) {
        init_dyn_param();
        save_dyn_param();
        changed = true;
    }
    init_dyn_param2();

    bool ccl_ng = !load_with_crc(ccl_param, EEPROM_ADDR_CCL, sizeof(ccl_param), &g_header.crc_ccl);
    if (ccl_ng || magic_ng) {
        init_ccl_param();
        save_ccl_param();
        changed = true;
    }

    bool cal_ng = !load_with_crc(cal_param, EEPROM_ADDR_CAL, sizeof(cal_param), &g_header.crc_cal);
    if (cal_ng || magic_ng) {
        init_cal_param();
        save_cal_param();
        changed = true;
    }

    bool sensi_offset_ng = !load_with_crc(&sensi_offset_param, EEPROM_ADDR_SENSI_OFFSET, sizeof(sensi_offset_param), &g_header.crc_sensi_offset);
    if (sensi_offset_ng || magic_ng) {
        init_sensi_offset_param();
        save_sensi_offset_param();
        changed = true;
    }

    bool layout_ng = !load_with_crc(&layout_state, EEPROM_ADDR_LAYOUT, sizeof(layout_state), &g_header.crc_layout);
    if (layout_ng || magic_ng || !layout_state_is_valid()) {
        init_layout_state();
        save_layout_state();
        changed = true;
    }

    mag_layout_rebuild_idx_valid();
    if(changed) save_header();
}

static void *get_block_ptr(mag_config_block_t block)
{
    switch (block) {
        case MAG_CONFIG_BLOCK_HEADER:
            return &g_header;
        case MAG_CONFIG_BLOCK_DYN:
            return dyn_param;
        case MAG_CONFIG_BLOCK_CCL:
            return ccl_param;
        case MAG_CONFIG_BLOCK_CAL:
            return cal_param;
        case MAG_CONFIG_BLOCK_SENSI_OFFSET:
            return &sensi_offset_param;
        case MAG_CONFIG_BLOCK_LAYOUT:
            return (void *)key_layout_entries;
        case MAG_CONFIG_BLOCK_LAYOUT_LABEL:
            return (void *)key_layout_labels;
        case MAG_CONFIG_BLOCK_LAYOUT_STATE:
            return &layout_state;
        case MAG_CONFIG_BLOCK_IDX_VALID:
            return idx_valid;
        default:
            return NULL;
    }
}

uint16_t mag_config_get_block_size(mag_config_block_t block)
{
    switch (block) {
        case MAG_CONFIG_BLOCK_HEADER:
            return sizeof(g_header);
        case MAG_CONFIG_BLOCK_DYN:
            return sizeof(dyn_param);
        case MAG_CONFIG_BLOCK_CCL:
            return sizeof(ccl_param);
        case MAG_CONFIG_BLOCK_CAL:
            return sizeof(cal_param);
        case MAG_CONFIG_BLOCK_SENSI_OFFSET:
            return sizeof(sensi_offset_param);
        case MAG_CONFIG_BLOCK_LAYOUT:
            return sizeof(key_layout_entries[0]) * key_layout_entry_count;
        case MAG_CONFIG_BLOCK_LAYOUT_LABEL:
            return sizeof(key_layout_labels[0]) * key_layout_label_count;
        case MAG_CONFIG_BLOCK_LAYOUT_STATE:
            return sizeof(layout_state);
        case MAG_CONFIG_BLOCK_IDX_VALID:
            return sizeof(idx_valid);
        default:
            return 0;
    }
}

uint16_t mag_config_get_crc(mag_config_block_t block)
{
    switch (block) {
        case MAG_CONFIG_BLOCK_DYN:
            return g_header.crc_dyn;
        case MAG_CONFIG_BLOCK_CCL:
            return g_header.crc_ccl;
        case MAG_CONFIG_BLOCK_CAL:
            return g_header.crc_cal;
        case MAG_CONFIG_BLOCK_SENSI_OFFSET:
            return g_header.crc_sensi_offset;
        case MAG_CONFIG_BLOCK_LAYOUT_STATE:
            return g_header.crc_layout;
        default:
            return 0;
    }
}

bool mag_config_read_block(mag_config_block_t block, uint16_t offset, uint8_t *dst, uint8_t length)
{
    void *src = get_block_ptr(block);
    uint16_t size = mag_config_get_block_size(block);
    if (!src || !dst || offset > size || length > size - offset) return false;
    memcpy(dst, (uint8_t *)src + offset, length);
    return true;
}

bool mag_config_write_block(mag_config_block_t block, uint16_t offset, const uint8_t *src, uint8_t length)
{
    void *dst = get_block_ptr(block);
    uint16_t size = mag_config_get_block_size(block);
    if (!dst || !src || offset > size || length > size - offset) return false;
    if (block == MAG_CONFIG_BLOCK_HEADER || block == MAG_CONFIG_BLOCK_LAYOUT || block == MAG_CONFIG_BLOCK_LAYOUT_LABEL || block == MAG_CONFIG_BLOCK_IDX_VALID) return false;
    memcpy((uint8_t *)dst + offset, src, length);
    if (block == MAG_CONFIG_BLOCK_DYN) init_dyn_param2();
    if (block == MAG_CONFIG_BLOCK_LAYOUT_STATE) mag_layout_rebuild_idx_valid();
    return true;
}

bool mag_config_save_block(mag_config_block_t block)
{
    switch (block) {
        case MAG_CONFIG_BLOCK_DYN:
            save_dyn_param();
            save_header();
            return true;
        case MAG_CONFIG_BLOCK_CCL:
            save_ccl_param();
            save_header();
            return true;
        case MAG_CONFIG_BLOCK_CAL:
            save_cal_param();
            save_header();
            return true;
        case MAG_CONFIG_BLOCK_SENSI_OFFSET:
            save_sensi_offset_param();
            save_header();
            return true;
        case MAG_CONFIG_BLOCK_LAYOUT_STATE:
            save_layout_state();
            save_header();
            return true;
        case MAG_CONFIG_BLOCK_ALL:
            save_dyn_param();
            save_ccl_param();
            save_cal_param();
            save_sensi_offset_param();
            save_layout_state();
            save_header();
            return true;
        default:
            return false;
    }
}

bool mag_config_reset_block(mag_config_block_t block)
{
    switch (block) {
        case MAG_CONFIG_BLOCK_DYN:
            init_dyn_param();
            init_dyn_param2();
            return true;
        case MAG_CONFIG_BLOCK_CCL:
            init_ccl_param();
            return true;
        case MAG_CONFIG_BLOCK_CAL:
            init_cal_param();
            return true;
        case MAG_CONFIG_BLOCK_SENSI_OFFSET:
            init_sensi_offset_param();
            return true;
        case MAG_CONFIG_BLOCK_LAYOUT_STATE:
            init_layout_state();
            mag_layout_rebuild_idx_valid();
            return true;
        case MAG_CONFIG_BLOCK_ALL:
            init_dyn_param();
            init_dyn_param2();
            init_ccl_param();
            init_cal_param();
            init_sensi_offset_param();
            init_layout_state();
            mag_layout_rebuild_idx_valid();
            return true;
        default:
            return false;
    }
}
