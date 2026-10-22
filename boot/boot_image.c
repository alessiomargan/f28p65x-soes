#include "boot_image.h"

uint32_t boot_crc_octet(uint32_t state, uint16_t octet)
{
    state ^= octet & 0xFFU;
    for (uint16_t bit = 0U; bit < 8U; ++bit) {
        state = (state >> 1U) ^ ((state & 1U) ? 0xEDB88320UL : 0UL);
    }
    return state;
}

static uint32_t read_u32(const boot_flash_io_t *io, uint32_t address)
{
    return (uint32_t)io->read_word(address) |
           ((uint32_t)io->read_word(address + 1UL) << 16U);
}

static uint32_t flash_crc(const boot_flash_io_t *io, uint32_t address,
                          uint32_t octets)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t i = 0UL; i < octets; i += 2UL) {
        uint16_t word = io->read_word(address + i / 2UL);
        crc = boot_crc_octet(crc, word);
        if (i + 1UL < octets) { crc = boot_crc_octet(crc, word >> 8U); }
    }
    return crc ^ 0xFFFFFFFFUL;
}

bool boot_image_valid(const boot_flash_io_t *io, bool require_commit)
{
    uint32_t m = BOOT_MANIFEST_WORD_ADDR;
    uint32_t length = read_u32(io, m + 6UL);
    uint32_t header_crc = read_u32(io, m + 14UL);
    if (read_u32(io, m) != BOOT_IMAGE_MAGIC ||
        read_u32(io, m + 2UL) != 1UL ||
        read_u32(io, m + 4UL) != BOOT_IMAGE_TARGET ||
        length < 4UL || length > BOOT_PAYLOAD_OCTETS || (length & 1UL) ||
        read_u32(io, m + 8UL) != BOOT_APP_WORD_ADDR ||
        read_u32(io, m + 12UL) != 0UL ||
        flash_crc(io, m, 28UL) != header_crc) {
        return false;
    }
    if (require_commit &&
        (read_u32(io, BOOT_COMMIT_WORD_ADDR) != BOOT_COMMIT_MAGIC ||
         read_u32(io, BOOT_COMMIT_WORD_ADDR + 2UL) != (BOOT_COMMIT_MAGIC ^ 0xFFFFFFFFUL) ||
         read_u32(io, BOOT_COMMIT_WORD_ADDR + 4UL) != header_crc ||
         read_u32(io, BOOT_COMMIT_WORD_ADDR + 6UL) != (header_crc ^ 0xFFFFFFFFUL))) {
        return false;
    }
    /* Reject blank entry code. The packer requires a bank-1 linked codestart;
     * C28 instruction semantics and linker placement still need target QA. */
    uint32_t entry = read_u32(io, BOOT_APP_WORD_ADDR);
    if (entry == 0xFFFFFFFFUL || entry == 0UL ||
        flash_crc(io, BOOT_APP_WORD_ADDR, length) != read_u32(io, m + 10UL)) {
        return false;
    }
    for (uint32_t i = length / 2UL; i < BOOT_PAYLOAD_OCTETS / 2UL; ++i) {
        if (io->read_word(BOOT_APP_WORD_ADDR + i) != 0xFFFFU) { return false; }
    }
    return true;
}

bool boot_image_prepare(const boot_flash_io_t *io)
{
    /* Invalidate BEFORE changing any payload word; bootloader stays in bank 0.
     * A reset during this erase can at most retain the previous valid image. */
    return io->erase_words(BOOT_COMMIT_WORD_ADDR, BOOT_SECTOR_WORDS);
}

bool boot_image_finish(const boot_flash_io_t *io, uint32_t wire_octets)
{
    uint16_t marker[8];
    uint32_t header_crc = read_u32(io, BOOT_MANIFEST_WORD_ADDR + 14UL);
    uint32_t values[4] = { BOOT_COMMIT_MAGIC, BOOT_COMMIT_MAGIC ^ 0xFFFFFFFFUL,
                          header_crc, header_crc ^ 0xFFFFFFFFUL };
    if (wire_octets != BOOT_WIRE_OCTETS || !boot_image_valid(io, false)) {
        return false;
    }
    for (uint16_t i = 0U; i < 4U; ++i) {
        marker[i * 2U] = (uint16_t)(values[i] & 0xFFFFUL);
        marker[i * 2U + 1U] = (uint16_t)(values[i] >> 16U);
    }
    return io->program_words(marker, BOOT_COMMIT_WORD_ADDR, 8UL) &&
           boot_image_valid(io, true);
}
