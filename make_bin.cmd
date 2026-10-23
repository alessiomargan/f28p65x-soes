/*
 * Convert the linked C28x FLASH image into a zero-based byte stream.
 * C28x linker addresses are 16-bit words; hex2000 binary addresses are bytes.
 */
--order LS
--issue_remarks
--binary
--zero
--image

ROMS
{
    APPLICATION_BINARY:
        origin = 0x00100000, /* FLASH_BANK0 0x080000 in C28x word addressing */
        length = 0x00040000, /* 0x20000 C28x words = 0x40000 protocol octets */
        romwidth = 16,
        fill = 0xFFFF
}

SECTIONS
{
    codestart:
    .text:
    .cinit:
    .switch:
    .init_array:
    .const:
    .TI.ramfunc:
}
