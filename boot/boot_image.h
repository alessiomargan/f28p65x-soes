#ifndef F28_BOOT_IMAGE_H
#define F28_BOOT_IMAGE_H
#include <stdint.h>
#include <stdbool.h>

/* Addresses and erase/program counts are C28 16-bit WORDS; wire sizes OCTETS. */
#define BOOT_APP_WORD_ADDR 0x0A0000UL
#define BOOT_SLOT_WORDS 0x20000UL
#define BOOT_SECTOR_WORDS 0x400UL
#define BOOT_PAYLOAD_OCTETS ((BOOT_SLOT_WORDS - 2UL * BOOT_SECTOR_WORDS) * 2UL)
#define BOOT_MANIFEST_WORD_ADDR (BOOT_APP_WORD_ADDR + BOOT_PAYLOAD_OCTETS / 2UL)
#define BOOT_COMMIT_WORD_ADDR (BOOT_APP_WORD_ADDR + BOOT_SLOT_WORDS - BOOT_SECTOR_WORDS)
#define BOOT_WIRE_OCTETS (BOOT_PAYLOAD_OCTETS + 32UL)
#define BOOT_IMAGE_MAGIC 0x46323850UL
#if defined(_LAUNCHXL_F28P65X)
#define BOOT_IMAGE_TARGET 0xF28A6502UL
#else
#define BOOT_IMAGE_TARGET 0xF28A6501UL
#endif
#define BOOT_COMMIT_MAGIC 0xB007CAFEUL

typedef struct {
    uint16_t (*read_word)(uint32_t word_address);
    /* Returns true only after completion, FSM status check AND readback. */
    bool (*program_words)(const uint16_t *words, uint32_t address, uint32_t count);
    bool (*erase_words)(uint32_t address, uint32_t count);
} boot_flash_io_t;

/* CRC-32/ISO-HDLC: reflected 0xEDB88320, init/final XOR FFFFFFFF.
 * Flash serialization is low octet then high octet of each C28 word.
 */
uint32_t boot_crc_octet(uint32_t state, uint16_t octet);
bool boot_image_valid(const boot_flash_io_t *io, bool require_commit);
bool boot_image_prepare(const boot_flash_io_t *io);
bool boot_image_finish(const boot_flash_io_t *io, uint32_t wire_octets);
#endif
