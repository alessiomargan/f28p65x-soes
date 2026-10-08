#include <soes/esc.h>
#include <soes/esc_foe.h>
#include <globals.h>
#include <flash_utils.h>

uint32_t foe_write_flash(foe_file_cfg_t * wr_cfg, uint8_t * data, size_t length);
uint32_t foe_read_flash(foe_file_cfg_t * self, uint8_t * data, size_t length);
uint32_t on_foe_open_cb(uint8_t op);
uint32_t on_foe_close_cb( void );

foe_file_cfg_t      gFOE_custom_fw_files[] = {
	{
		.name =					"f28p65x.bin",
		.max_data = 			FLASH_APP_SIZE_OCTETS,
		.dest_start_address =	FLASH_APP_ADDR, 	//
		.address_offset =		0,
		.filepass =				0xF280,
		.write_only_in_boot =	true,
		.write_function =		foe_write_flash,
		.read_function =		foe_read_flash,
		.on_foe_open = 			on_foe_open_cb,
		.on_foe_close = 		on_foe_close_cb,
	},
	{ 0 }
};

void bootstrap_foe_init(void) {

	/* setup application foe_file structs */
	int file_cnt = 0;
	foe_file_cfg_t *tmp_foe_files = gFOE_custom_fw_files;

	while (tmp_foe_files->name != 0) {
		DPRINT("foe_file %s addr 0x%08"PRIX32"\n", tmp_foe_files->name,
				tmp_foe_files->dest_start_address);
		tmp_foe_files++;
		file_cnt++;
	}

	/** Allocate static in caller func to fit buffer_size */
	gFOE_config.fbuffer = foe_buffer;
	/** Buffer size before we flush to destination */
	gFOE_config.buffer_size = sizeof(foe_buffer);
	/** Number of files used in firmware update */
	gFOE_config.n_files = file_cnt;
	/** Pointer to files configured to be used by FoE */
	gFOE_config.files = gFOE_custom_fw_files;

	FOE_config(&gFOE_config);

	DPRINT("config %d foe_file(s)\n", file_cnt);

}



uint32_t foe_write_flash(foe_file_cfg_t *wr_cfg, uint8_t *data, size_t length) {
	uint32_t octet_count = (uint32_t)length;
	uint32_t flash_word_offset;
	uint32_t flash_word_count;
	uint32_t flash_address;
	uint16_t *packed_words = (uint16_t *)data;
	Fapi_StatusType status;

	if ((wr_cfg == 0) || (data == 0) || (octet_count == 0U) ||
		((wr_cfg->address_offset % C28_FLASH_WORD_OCTETS) != 0U) ||
		(wr_cfg->address_offset > wr_cfg->max_data) ||
		(octet_count > (wr_cfg->max_data - wr_cfg->address_offset))) {
		return 1U;
	}

	flash_word_offset = wr_cfg->address_offset / C28_FLASH_WORD_OCTETS;
	flash_word_count = (octet_count + C28_FLASH_WORD_OCTETS - 1U) /
		C28_FLASH_WORD_OCTETS;
	flash_address = wr_cfg->dest_start_address + flash_word_offset;

	/* Pack two EtherCAT octets into each little-endian C28 flash word. */
	for (uint32_t word = 0U; word < flash_word_count; word++) {
		uint32_t octet = word * C28_FLASH_WORD_OCTETS;
		uint16_t low = esc_octet_get(data, octet);
		uint16_t high = (octet + 1U < octet_count) ?
			esc_octet_get(data, octet + 1U) : 0xFFU;

		packed_words[word] = low | (uint16_t)(high << 8U);
	}

	DPRINT("%s FlashAddr 0x%08"PRIX32" with %"PRIu32
		" octets (%"PRIu32" words)\n", __FUNCTION__, flash_address,
		octet_count, flash_word_count);

	status = Program_dataFlashSector(packed_words, flash_address,
								 flash_word_count);
	return (status == Fapi_Status_Success) ? 0U : 1U;
}

uint32_t foe_read_flash(foe_file_cfg_t * self, uint8_t * data, size_t length) {

	return 1;
}

uint32_t on_foe_open_cb(uint8_t op) {
	if (op != FOE_OP_WRQ) {
		return 0U;
	}

	Fapi_StatusType ret = Erase_dataFlashSector(FLASH_APP_ADDR, FLASH_APP_BSIZE);
	DPRINT("%s erase region 0x%08"PRIX32" = %d\n", __FUNCTION__, FLASH_APP_ADDR, ret);
	return (ret == Fapi_Status_Success) ? 0U : 1U;
}

uint32_t on_foe_close_cb( void ) {
	/* CRC and image activation are handled by the validation stage. */
	return 0U;
}
