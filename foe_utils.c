#include <soes/esc.h>
#include <soes/esc_foe.h>
#include <globals.h>
#include <flash_utils.h>
#include "foe_update_policy.h"
#include "boot/boot_image.h"

/* Never expose the separate commit sector to incoming FoE data. */
#define FOE_FIRMWARE_MAX_OCTETS ((BOOT_SLOT_WORDS - BOOT_SECTOR_WORDS) * 2UL)

typedef char foe_slot_matches_flash[(FLASH_APP_ADDR == BOOT_APP_WORD_ADDR &&
    FLASH_APP_SIZE_WORDS == BOOT_SLOT_WORDS) ? 1 : -1];
static bool firmware_write_open;
static uint32_t programmed_octets;

uint32_t foe_write_flash(foe_file_cfg_t * wr_cfg, uint8_t * data, size_t length);
uint32_t foe_read_flash(foe_file_cfg_t * self, uint8_t * data, size_t length);
uint32_t on_foe_open_cb(uint8_t op);
uint32_t on_foe_close_cb( void );

foe_file_cfg_t      gFOE_custom_fw_files[] = {
	{
		.name =					"f28p65x.bin",
		.max_data = 			FOE_FIRMWARE_MAX_OCTETS,
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
	gFOE_config.buffer_size = sizeof(foe_buffer) / sizeof(foe_buffer[0]);
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

	if (!firmware_write_open || ESCvar.ALstatus != ESCboot ||
        (wr_cfg != &gFOE_custom_fw_files[0]) || (data == 0) ||
        (length > FOE_FIRMWARE_MAX_OCTETS) || (octet_count == 0U) ||
        (wr_cfg->dest_start_address != FLASH_APP_ADDR) ||
        (wr_cfg->max_data != FOE_FIRMWARE_MAX_OCTETS) ||
        (wr_cfg->address_offset != programmed_octets) ||
        ((octet_count % (DATA_FLASH_PROGRAM_SIZE_WORDS * C28_FLASH_WORD_OCTETS)) != 0U) ||
		((wr_cfg->address_offset % C28_FLASH_WORD_OCTETS) != 0U) ||
		(wr_cfg->address_offset > wr_cfg->max_data) ||
		(octet_count > (wr_cfg->max_data - wr_cfg->address_offset))) {
		firmware_write_open = false;
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
    if (status != Fapi_Status_Success) {
        firmware_write_open = false;
        return 1U;
    }
    programmed_octets += octet_count;
    return 0U;
}

uint32_t foe_read_flash(foe_file_cfg_t * self, uint8_t * data, size_t length) {

	return 1;
}

uint32_t on_foe_open_cb(uint8_t op) {
    firmware_write_open = false;
    programmed_octets = 0U;
    if (op != FOE_OP_WRQ || ESCvar.ALstatus != ESCboot) {
        return 1U;
    }
#if FOE_UPDATE_INTEGRATED
    /* The flash helper pads to a programming unit. Require whole units and a
     * buffer that divides the staging region, including the final flush. */
    const uint32_t unit = DATA_FLASH_PROGRAM_SIZE_WORDS * C28_FLASH_WORD_OCTETS;
    if (gFOE_config.buffer_size == 0U ||
        gFOE_config.buffer_size > sizeof(foe_buffer) / sizeof(foe_buffer[0]) ||
        (gFOE_config.buffer_size % unit) != 0U ||
        (FOE_FIRMWARE_MAX_OCTETS % gFOE_config.buffer_size) != 0U ||
        foe_update_prepare() != 0U) {
        return 1U;
    }
    Fapi_StatusType ret = Erase_dataFlashSector(FLASH_APP_ADDR, FLASH_APP_BSIZE);
    firmware_write_open = (ret == Fapi_Status_Success);
    return firmware_write_open ? 0U : 1U;
#else
    /* No verified bootloader/validator is supplied by this repository. */
    return 1U;
#endif
}

uint32_t on_foe_close_cb(void) {
    bool was_open = firmware_write_open;
    firmware_write_open = false;
    const uint32_t received = gFOE_custom_fw_files[0].total_size;
    if (!was_open || ESCvar.ALstatus != ESCboot || received == 0U || received > FOE_FIRMWARE_MAX_OCTETS ||
        programmed_octets < received ||
        programmed_octets - received >= gFOE_config.buffer_size) {
        return 1U;
    }
#if FOE_UPDATE_INTEGRATED
    return foe_update_validate_and_stage(FLASH_APP_ADDR, received);
#else
    return 1U;
#endif
}
