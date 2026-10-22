#ifndef BOOT_F28P65X_H
#define BOOT_F28P65X_H
#include <stdint.h>
/* Called after base/board/flash API setup, before EtherCAT init or IRQ enable. */
void boot_try_start_application(void);
/* Capture retention request BEFORE Board_init can initialize RAM. */
void boot_capture_request(void);
void boot_post_state_change(uint8_t *as, uint8_t *an);
#endif
