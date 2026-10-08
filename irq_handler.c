#include "board.h"
#include "irq_handler.h"

#include "soes/ecat_slv.h"
#include "globals.h"

#define DC_SYNC_LED_TOGGLE_TICKS    125U

#define RX_MSG_OBJ_ID 1

__attribute__((section("ramgs0")))
uint16_t txMsgData[8] = { 0xF0, 0xCA, 0xCC, 0x1A, 0x00, 0x00, 0x00, 0x00 };
__attribute__((section("ramgs0")))
uint16_t rxMsgData[8] = {0};

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
    
    //CAN_sendMessage(CANA_BASE, 2, 8, txMsgData);

    GPIO_writePin(dbg_1, 0);
}

__attribute__((section(".TI.ramfunc")))
__interrupt void INT_myCAN0_0_ISR(void) {
    
    uint32_t status;

    // Read the CAN interrupt status to find the cause of the interrupt
    status = CAN_getInterruptCause(CANA_BASE);

    // If the cause is a controller status interrupt, then get the status
    if(status == CAN_INT_INT0ID_STATUS) {
        // Read the controller status.  This will return a field of status
        // error bits that can indicate various errors.  Error processing
        // is not done in this example for simplicity.  Refer to the
        // API documentation for details about the error status bits.
        // The act of reading this status will clear the interrupt.
        status = CAN_getStatus(CANA_BASE);
        // Check to see if an error occurred.
        if(((status  & ~(CAN_STATUS_TXOK | CAN_STATUS_RXOK)) != 7) &&
           ((status  & ~(CAN_STATUS_TXOK | CAN_STATUS_RXOK)) != 0)) {
            // Something went wrong. rData doesn't contain expected data.
            ESTOP0;
        }
    } else if(status == 2U) {
        // Getting to this point means that the TX interrupt occurred on
        // message object 2, and the message TX is complete.  Clear the
        // message object interrupt.
        CAN_clearInterruptStatus(CANA_BASE, 2U);
    } else if(status == 1U) {
        // Getting to this point means that the RX interrupt occurred on
        // message object 1, and the message RX is complete.  Clear the
        // message object interrupt.
        CAN_clearInterruptStatus(CANA_BASE, 1U);
    } // If something unexpected caused the interrupt, this would handle it.
    else {
        // Spurious interrupt handling can go here.
    }

    // Clear the global interrupt flag for the CAN interrupt line
    CAN_clearGlobalInterruptStatus(CANA_BASE, CAN_GLOBAL_INT_CANINT0);
    // Acknowledge this interrupt located in group 9
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}
    

__attribute__((section(".TI.ramfunc")))
__interrupt void INT_myCAN0_1_ISR(void) {
    // NOT USED 
}
