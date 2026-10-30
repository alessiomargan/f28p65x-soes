#include <stdint.h>
#include "ethercat_subdevice_cpu1_hal.h"

/* The application linker reserves this retention mailbox. It requests
 * recovery mode from the dedicated bootloader after reset. */
__attribute__((section(".boot_request")))
static volatile uint32_t boot_request[2];
#define BOOT_REQUEST_MAGIC 0xB007B007UL

static void reset_device(void)
{
    DINT;
    SysCtl_resetDevice();
    for (;;) {}
}

void jump_to_bootloader(void)
{
    boot_request[0] = BOOT_REQUEST_MAGIC;
    boot_request[1] = BOOT_REQUEST_MAGIC ^ 0xFFFFFFFFUL;
    reset_device();
}
