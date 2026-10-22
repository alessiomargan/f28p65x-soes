#if defined(F28_BOOTLOADER) || defined(F28_APPLICATION)
#include <stdint.h>
#include "ethercat_subdevice_cpu1_hal.h"
#include "flash_utils.h"
#include "soes/esc.h"
#include "boot_image.h"
#include "boot_f28p65x.h"

/* Both new linker files reserve this four-word retention mailbox. It is a
 * request to remain in recovery, never permission to execute an image. */
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
#if defined(F28_APPLICATION)
    boot_request[0] = BOOT_REQUEST_MAGIC;
    boot_request[1] = BOOT_REQUEST_MAGIC ^ 0xFFFFFFFFUL;
    reset_device();
#endif
    /* In the bootloader, INIT->BOOT already entered the FoE service. */
}

#if defined(F28_BOOTLOADER)
static bool force_recovery;
static uint16_t read_word(uint32_t address)
{
    return *(const volatile uint16_t *)address;
}
static bool program_words(const uint16_t *data, uint32_t address, uint32_t count)
{
    return Program_dataFlashSector(data, address, count) == Fapi_Status_Success;
}
static bool erase_words(uint32_t address, uint32_t count)
{
    return Erase_dataFlashSector(address, count) == Fapi_Status_Success;
}
static const boot_flash_io_t flash_io = {read_word, program_words, erase_words};

void boot_capture_request(void)
{
    uint32_t cause = SysCtl_getResetCause();
    force_recovery = ((cause & SYSCTL_CAUSE_WDRS) != 0UL) &&
                     ((cause & SYSCTL_CAUSE_POR) == 0UL) &&
                     boot_request[0] == BOOT_REQUEST_MAGIC &&
                     boot_request[1] == (BOOT_REQUEST_MAGIC ^ 0xFFFFFFFFUL);
    boot_request[0] = 0UL;
    boot_request[1] = 0UL;
    SysCtl_clearResetCause(cause);
}

uint32_t foe_update_prepare(void)
{
    return boot_image_prepare(&flash_io) ? 0UL : 1UL;
}

uint32_t foe_update_validate_and_stage(uint32_t address, uint32_t octets)
{
    return (address == BOOT_APP_WORD_ADDR &&
            boot_image_finish(&flash_io, octets)) ? 0UL : 1UL;
}

void boot_try_start_application(void)
{
    if (!force_recovery && boot_image_valid(&flash_io, true)) {
        /* No timer/ESC interrupts have been started yet. C28 codestart enters
         * _c_int00 and sets up the application's C runtime and stack anew. */
        DINT;
        IER = 0U;
        IFR = 0U;
        asm(" LB 0x0A0000");
        for (;;) {}
    }
}

void boot_post_state_change(uint8_t *as, uint8_t *an)
{
    /* Activation is explicit: the master leaves BOOT only after receiving
     * final ACK. Revalidation happens on reset, including interrupted writes. */
    if (*as == BOOT_TO_INIT && *an == ESCinit) { reset_device(); }
}
#endif
#endif
