/*
 * Convert the controlCARD FoE application into a zero-based byte stream.
 *
 * C28x linker addresses are 16-bit words, while hex2000 binary addresses
 * are bytes.  Bank 1 begins at word 0x0A0000 (byte 0x00140000).  The final
 * two 0x400-word sectors are reserved for the FoE manifest and commit data,
 * leaving 0x1F800 words / 0x3F000 bytes for the application payload.
 */
--order LS
--issue_remarks
--binary
--zero
--image

ROMS
{
    APPLICATION_BINARY:
        origin = 0x00140000,
        length = 0x0003F000,
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
