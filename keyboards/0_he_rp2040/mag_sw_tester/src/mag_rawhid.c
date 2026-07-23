#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "quantum.h"
#include "raw_hid.h"
#include "via.h"
#include "config.h"
#include "src/include/key_type.h"
#include "src/include/key_layout.h"
#include "src/include/mag_config.h"
#include "src/include/mag_cal.h"
#include "src/include/mag_proc.h"
#include "src/include/mag_rawhid.h"
#include "src/include/mag_realtime.h"
#include "src/include/mag_utils.h"

extern void matrix_get_calibration_live(uint8_t start_idx, uint8_t count, uint8_t *data);

#define MAG_RAWHID_CHANNEL 0
#define MAG_RAWHID_REPORT_SIZE 32
#define MAG_RAWHID_PAYLOAD_OFFSET 7
#define MAG_RAWHID_MAX_PAYLOAD (MAG_RAWHID_REPORT_SIZE - MAG_RAWHID_PAYLOAD_OFFSET)

static uint16_t read_u16(const uint8_t *data)
{
	return ((uint16_t)data[0] << 8) | data[1];
}

static void write_u16(uint8_t *data, uint16_t value)
{
	data[0] = value >> 8;
	data[1] = value & 0xFF;
}

static void write_i16_le(uint8_t *data, int16_t value)
{
	data[0] = value & 0xFF;
	data[1] = (value >> 8) & 0xFF;
}

static void clear_body(uint8_t *data)
{
	memset(&data[2], 0, MAG_RAWHID_REPORT_SIZE - 2);
}

static void set_status(uint8_t *data, uint8_t status)
{
	data[1] = MAG_RAWHID_CHANNEL;
	data[2] = status;
}

static void handle_get_info(uint8_t *data)
{
	clear_body(data);
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = MAG_RAWHID_PROTOCOL_VERSION;
	data[4] = MATRIX_ROWS;
	data[5] = MATRIX_COLS;
	data[6] = MATRIX_SIZE;
	data[7] = sizeof(dyn_key_param_t);
	data[8] = sizeof(ccl_key_param_t);
	data[9] = sizeof(cal_key_param_t);
	data[10] = sizeof(key_layout_entry_t);
	write_u16(&data[11], mag_config_get_block_size(MAG_CONFIG_BLOCK_HEADER));
	write_u16(&data[13], mag_config_get_block_size(MAG_CONFIG_BLOCK_DYN));
	write_u16(&data[15], mag_config_get_block_size(MAG_CONFIG_BLOCK_CCL));
	write_u16(&data[17], mag_config_get_block_size(MAG_CONFIG_BLOCK_CAL));
	write_u16(&data[19], mag_config_get_block_size(MAG_CONFIG_BLOCK_LAYOUT));
	write_u16(&data[21], mag_config_get_crc(MAG_CONFIG_BLOCK_DYN));
	write_u16(&data[23], mag_config_get_crc(MAG_CONFIG_BLOCK_CCL));
	write_u16(&data[25], mag_config_get_crc(MAG_CONFIG_BLOCK_CAL));
	write_u16(&data[27], key_layout_entry_count);
	data[29] = sizeof(layout_option_state_t);
	data[30] = mag_config_get_block_size(MAG_CONFIG_BLOCK_IDX_VALID);
	data[31] = LAYOUT_OPTION_MAX;
}

static void handle_read_block(uint8_t *data)
{
	mag_config_block_t block = (mag_config_block_t)data[3];
	uint16_t offset = read_u16(&data[4]);
	uint8_t length = data[6];
	if (length > MAG_RAWHID_MAX_PAYLOAD) {
		clear_body(data);
		set_status(data, MAG_RAWHID_STATUS_BAD_LENGTH);
		return;
	}
	clear_body(data);
	if (!mag_config_read_block(block, offset, &data[MAG_RAWHID_PAYLOAD_OFFSET], length)) {
		set_status(data, MAG_RAWHID_STATUS_BAD_BLOCK);
		return;
	}
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = block;
	write_u16(&data[4], offset);
	data[6] = length;
}

static void handle_write_block(uint8_t *data)
{
	mag_config_block_t block = (mag_config_block_t)data[3];
	uint16_t offset = read_u16(&data[4]);
	uint8_t length = data[6];
	bool ok;
	if (length > MAG_RAWHID_MAX_PAYLOAD) {
		clear_body(data);
		set_status(data, MAG_RAWHID_STATUS_BAD_LENGTH);
		return;
	}
	ok = mag_config_write_block(block, offset, &data[MAG_RAWHID_PAYLOAD_OFFSET], length);
	clear_body(data);
	if (!ok) {
		set_status(data, MAG_RAWHID_STATUS_BAD_BLOCK);
		return;
	}
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = block;
	write_u16(&data[4], offset);
	data[6] = length;
}

static void handle_save_block(uint8_t *data)
{
	mag_config_block_t block = (mag_config_block_t)data[3];
	clear_body(data);
	if (!mag_config_save_block(block)) {
		set_status(data, MAG_RAWHID_STATUS_BAD_BLOCK);
		return;
	}
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = block;
}

static void handle_reset_block(uint8_t *data)
{
	mag_config_block_t block = (mag_config_block_t)data[3];
	bool save_after_reset = data[4] != 0;
	bool ok = mag_config_reset_block(block);
	if (ok && save_after_reset) {
		ok = mag_config_save_block(block);
	}
	clear_body(data);
	if (!ok) {
		set_status(data, MAG_RAWHID_STATUS_BAD_BLOCK);
		return;
	}
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = block;
	data[4] = save_after_reset;
}

static void handle_read_d_cur(uint8_t *data)
{
	uint8_t start_idx = data[3];
	uint8_t count = data[4];
	uint8_t max_count = MAG_RAWHID_MAX_PAYLOAD / 2;

	if (count > max_count) {
		clear_body(data);
		set_status(data, MAG_RAWHID_STATUS_BAD_LENGTH);
		return;
	}
	if (start_idx >= MATRIX_SIZE || start_idx + count > MATRIX_SIZE) {
		clear_body(data);
		set_status(data, MAG_RAWHID_STATUS_BAD_LENGTH);
		return;
	}

	clear_body(data);
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = start_idx;
	data[4] = count;
	for (uint8_t i = 0; i < count; i++) {
		write_i16_le(&data[MAG_RAWHID_PAYLOAD_OFFSET + i * 2], mag_realtime_get_d_cur(start_idx + i));
	}
}

static void handle_read_adc_cur(uint8_t *data)
{
	uint8_t start_idx = data[3];
	uint8_t count = data[4];
	uint8_t max_count = MAG_RAWHID_MAX_PAYLOAD / 2;

	if (count > max_count) {
		clear_body(data);
		set_status(data, MAG_RAWHID_STATUS_BAD_LENGTH);
		return;
	}
	if (start_idx >= MATRIX_SIZE || start_idx + count > MATRIX_SIZE) {
		clear_body(data);
		set_status(data, MAG_RAWHID_STATUS_BAD_LENGTH);
		return;
	}

	clear_body(data);
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = start_idx;
	data[4] = count;
	for (uint8_t i = 0; i < count; i++) {
		write_u16(&data[MAG_RAWHID_PAYLOAD_OFFSET + i * 2], mag_realtime_get_adc_cur(start_idx + i));
	}
}

static void handle_read_realtime_key(uint8_t *data)
{
	uint8_t idx = data[3];
	uint8_t metric = data[4];

	if (idx >= MATRIX_SIZE || metric > 1) {
		clear_body(data);
		set_status(data, MAG_RAWHID_STATUS_BAD_LENGTH);
		return;
	}

	clear_body(data);
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = idx;
	data[4] = metric;
	if (metric == 1) {
		write_u16(&data[MAG_RAWHID_PAYLOAD_OFFSET], mag_realtime_get_adc_cur(idx));
	} else {
		write_i16_le(&data[MAG_RAWHID_PAYLOAD_OFFSET], mag_realtime_get_d_cur(idx));
	}
}

static void handle_get_state(uint8_t *data)
{
	clear_body(data);
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = offset_stage_get();
	write_u16(&data[4], offset_value_get());
	data[6] = ccl_is_enabled() ? 1 : 0;
}

static void handle_set_state(uint8_t *data)
{
	uint8_t flags = data[3];
	if (flags & 0x01) {
		offset_stage_set(data[4]);
	}
	if (flags & 0x02) {
		ccl_set_enabled(data[5] != 0);
	}
	handle_get_state(data);
}

static void handle_cal_control(uint8_t *data)
{
	uint8_t action = data[3];
	clear_body(data);
	if (action == 1) {
		cal_request_start();
	} else if (action == 0) {
		cal_request_stop();
	} else {
		set_status(data, MAG_RAWHID_STATUS_BAD_LENGTH);
		return;
	}
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = action;
}

static void handle_get_cal_state(uint8_t *data)
{
	bool keepalive = data[3] != 0;
	if (keepalive) {
		cal_request_keepalive();
	}
	clear_body(data);
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = mode_get();
	data[4] = cal_origin_get();
}

static void handle_read_cal_view(uint8_t *data)
{
	uint8_t start_idx = data[3];
	uint8_t count = data[4];
	uint8_t max_count = MAG_RAWHID_MAX_PAYLOAD / 6;

	if (count > max_count) {
		clear_body(data);
		set_status(data, MAG_RAWHID_STATUS_BAD_LENGTH);
		return;
	}
	if (start_idx >= MATRIX_SIZE || start_idx + count > MATRIX_SIZE) {
		clear_body(data);
		set_status(data, MAG_RAWHID_STATUS_BAD_LENGTH);
		return;
	}

	clear_body(data);
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = start_idx;
	data[4] = count;
	matrix_get_calibration_live(start_idx, count, &data[MAG_RAWHID_PAYLOAD_OFFSET]);
}

static void handle_run_pcb_cal(uint8_t *data)
{
	clear_body(data);
	if (!run_pcb_calibration()) {
		set_status(data, MAG_RAWHID_STATUS_ERROR);
		return;
	}
	set_status(data, MAG_RAWHID_STATUS_OK);
	data[3] = 1;
}

static bool mag_rawhid_handle(uint8_t *data, uint8_t length)
{
	uint8_t mag_command = data[2];

	if (length != MAG_RAWHID_REPORT_SIZE || data[0] != MAG_RAWHID_COMMAND_ID || data[1] != MAG_RAWHID_CHANNEL) {
		return false;
	}

	switch (mag_command) {
		case MAG_RAWHID_CMD_GET_INFO:
			handle_get_info(data);
			break;
		case MAG_RAWHID_CMD_READ_BLOCK:
			handle_read_block(data);
			break;
		case MAG_RAWHID_CMD_WRITE_BLOCK:
			handle_write_block(data);
			break;
		case MAG_RAWHID_CMD_SAVE_BLOCK:
			handle_save_block(data);
			break;
		case MAG_RAWHID_CMD_RESET_BLOCK:
			handle_reset_block(data);
			break;
		case MAG_RAWHID_CMD_READ_D_CUR:
			handle_read_d_cur(data);
			break;
		case MAG_RAWHID_CMD_READ_ADC_CUR:
			handle_read_adc_cur(data);
			break;
		case MAG_RAWHID_CMD_READ_REALTIME_KEY:
			handle_read_realtime_key(data);
			break;
		case MAG_RAWHID_CMD_GET_STATE:
			handle_get_state(data);
			break;
		case MAG_RAWHID_CMD_SET_STATE:
			handle_set_state(data);
			break;
		case MAG_RAWHID_CMD_CAL_CONTROL:
			handle_cal_control(data);
			break;
		case MAG_RAWHID_CMD_GET_CAL_STATE:
			handle_get_cal_state(data);
			break;
		case MAG_RAWHID_CMD_READ_CAL_VIEW:
			handle_read_cal_view(data);
			break;
		case MAG_RAWHID_CMD_RUN_PCB_CAL:
			handle_run_pcb_cal(data);
			break;
		default:
			clear_body(data);
			set_status(data, MAG_RAWHID_STATUS_BAD_COMMAND);
			break;
	}
	data[0] = MAG_RAWHID_COMMAND_ID;
	return true;
}

#ifdef VIA_ENABLE
void raw_hid_receive_kb(uint8_t *data, uint8_t length)
{
	if (!mag_rawhid_handle(data, length)) {
		data[0] = id_unhandled;
	}
}
#else
void raw_hid_receive(uint8_t *data, uint8_t length)
{
	if (!mag_rawhid_handle(data, length)) {
		data[0] = id_unhandled;
	}
	raw_hid_send(data, length);
}
#endif
