#include "board.h"
#include "irq_handler.h"

#include "soes/ecat_slv.h"
#include "globals.h"

#define DC_SYNC_LED_TOGGLE_TICKS    125U


__attribute__((section(".TI.ramfunc")))
__interrupt void Sync0_Isr(void) {

    GPIO_writePin(dbg_2, 1);
    ecat_slv();
    GPIO_writePin(dbg_2, 0);
    // Acknowledge and clear interrupt in ESCSS
    ESCSS_clearRawInterruptStatus(ESC_SS_BASE, ESCSS_INTR_CLR_SYNC0_CLR);
    // Acknowledge this interrupt located in PIE group 1
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
   
__attribute__((section(".TI.ramfunc")))
__interrupt void Sync1_Isr(void) {

    // Acknowledge and clear interrupt in ESCSS
    ESCSS_clearRawInterruptStatus(ESC_SS_BASE, ESCSS_INTR_CLR_SYNC1_CLR);
    // Acknowledge this interrupt located in PIE group 5
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP5);
}

__attribute__((section(".TI.ramfunc")))
__interrupt void INT_myCPUTIMER2_ISR(void) {
    
    static uint16_t dcSyncLedTicks = 0U;
    uint8_t syncActivation;

    GPIO_writePin(dbg_1, 1);

    syncActivation = ESC_SYNCactivation();
    if (syncActivation == 0U) {
		ecat_slv();
	}

    if ((syncActivation & (ESCREG_SYNC_ACT_ACTIVATED |
                           ESCREG_SYNC_AUTO_ACTIVATED)) != 0U) {
        dcSyncLedTicks++;
        if (dcSyncLedTicks >= DC_SYNC_LED_TOGGLE_TICKS) {
            dcSyncLedTicks = 0U;
            GPIO_togglePin(DEVICE_LED2_GPIO);
        }
    }
    else {
        dcSyncLedTicks = 0U;
        GPIO_writePin(DEVICE_LED2_GPIO, 1U);
    }

    GPIO_writePin(dbg_1, 0);
}
