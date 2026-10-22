#ifndef FOE_UPDATE_POLICY_H
#define FOE_UPDATE_POLICY_H

/* Fail closed until the target's bootloader contract has been implemented.
 * Enabling this requires BOTH hooks below and the SOES error-propagation patch.
 */
#ifndef FOE_UPDATE_INTEGRATED
#if defined(F28_BOOTLOADER)
#define FOE_UPDATE_INTEGRATED 1
#else
#define FOE_UPDATE_INTEGRATED 0
#endif
#endif

#if FOE_UPDATE_INTEGRATED
/* Return zero only after durably invalidating the staging image BEFORE erase.
 * Must preserve a known-good executable/recovery path across any reset.
 */
uint32_t foe_update_prepare(void);
/* Validate the stored image against its own declared length, target, entry
 * point and integrity/authenticity metadata; publish pending metadata last.
 * octets excludes transport padding. Do not reset/jump before the final ACK.
 * The bootloader must independently validate again before execution.
 */
uint32_t foe_update_validate_and_stage(uint32_t address, uint32_t octets);
#endif

#endif
