#include "ethercat_subdevice_cpu1_hal.h"

#define BOOT_REQUEST_MAGIC 0xB007B007UL

/* Must match the bootloader's .boot_request section and linker placement.
 * The inverse guards against an accidental request from uninitialized RAM. */
__attribute__((section(".boot_request")))
static volatile struct {
    uint32_t magic;
    uint32_t magic_inverse;
} boot_request;

void jump_to_bootloader(void)
{
    DINT;
    IER = 0U;
    IFR = 0U;
    boot_request.magic_inverse = ~BOOT_REQUEST_MAGIC;
    /* Commit the request last. */
    boot_request.magic = BOOT_REQUEST_MAGIC;

    /* Enter the bootloader's C runtime without resetting the integrated ESC. */
    asm(" LB 0x080000");
    for (;;) {}
}
