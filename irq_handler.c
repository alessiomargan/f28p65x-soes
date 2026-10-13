#include "board.h"
#include "irq_handler.h"

#include "soes/ecat_slv.h"
#include "globals.h"

#define DC_SYNC_LED_TOGGLE_TICKS    125U

#define CAN_RX_MSG_OBJ  1U
#define CAN_TX_MSG_OBJ  2U

#define CAN_RTR_0x100_MSG_OBJ       3U
#define CAN_RTR_0x101_MSG_OBJ       4U
#define CAN_RTR_0x102_MSG_OBJ       5U
#define CAN_RTR_0x103_MSG_OBJ       6U
#define CAN_RTR_0x104_MSG_OBJ       7U
#define CAN_RTR_0x105_MSG_OBJ       8U

#define CAN_RTR_RESPONSE_COUNT      6U
#define CAN_RTR_RESPONSE_LENGTH     8U

// Initialized data in RAMGS0 for DMA; separate from the NOINIT EtherCAT buffers.
__attribute__((section(".data:can_dma"), aligned(2)))
uint16_t txMsgData[8] = { 0xF0, 0xCA, 0xCC, 0x1A, 0x00, 0x00, 0x00, 0x00 };
__attribute__((section(".data:can_dma"), aligned(2)))
uint16_t rxMsgData[8] = {0};

__attribute__((section(".data:can_rtr"), aligned(2)))
uint16_t rtrMsgData[CAN_RTR_RESPONSE_COUNT][CAN_RTR_RESPONSE_LENGTH] = {
    { 0xF0, 0xCA, 0xCC, 0x1A, 0xBE, 0xEF, 0x00, 0x00 },
    { 0xF0, 0xCA, 0xCC, 0x1A, 0xBE, 0xEF, 0x00, 0x00 },
    { 0xF0, 0xCA, 0xCC, 0x1A, 0xBE, 0xEF, 0x00, 0x00 },
    { 0xF0, 0xCA, 0xCC, 0x1A, 0xBE, 0xEF, 0x00, 0x00 },
    { 0xF0, 0xCA, 0xCC, 0x1A, 0xBE, 0xEF, 0x00, 0x00 },
    { 0xF0, 0xCA, 0xCC, 0x1A, 0xBE, 0xEF, 0x00, 0x00 },
};

static uint16_t msgCount;

/*
 * The safest general solution is a small lock/critical section around every complete IF-register transaction.
 * DriverLib itself does not provide locking or reentrancy.
 * This protects against maskable CPU1 interrupts.
 * It does not protect against DMA, CLA, CPU2, or another execution context accessing the same CAN interface registers;
 * those contexts would need separate ownership or synchronization.
 */
static inline void CAN_loadRTRResponseLocked(uint32_t messageObject,
                                      const uint16_t payload[8])
{
    bool interruptsWereDisabled;

    // Prevent an ISR containing CAN_sendMessage() from using IF1
    // during this complete transaction.
    interruptsWereDisabled = Interrupt_disableGlobal();

    // Wait until any previous IF1 transaction has completed.
    while((HWREGH(CANA_BASE + CAN_O_IF1CMD) &
           CAN_IF1CMD_BUSY) != 0U) { }

    // Load the IF1 staging data registers.
    CAN_writeDataReg(payload,
                     CANA_BASE + CAN_O_IF1DATA,
                     8U);
    
    // Copy data bytes into message RAM without TXRQST.
    // The message ID, mask, DLC and RTR configuration remain unchanged.
    HWREG_BP(CANA_BASE + CAN_O_IF1CMD) =
        (uint32_t)CAN_IF1CMD_DIR |
        (uint32_t)CAN_IF1CMD_DATA_A |
        (uint32_t)CAN_IF1CMD_DATA_B |
        (messageObject & CAN_IF1CMD_MSG_NUM_M);

    // Keep the lock until message RAM has received the data.
    while((HWREGH(CANA_BASE + CAN_O_IF1CMD) &
           CAN_IF1CMD_BUSY) != 0U) { }

    // Restore interrupts only if they were enabled before this call.
    if(!interruptsWereDisabled) {
        (void)Interrupt_enableGlobal();
    }
}

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
    
    //CAN_sendMessage(CANA_BASE, CAN_TX_MSG_OBJ, 8, txMsgData);
    for (int i=0; i<CAN_RTR_RESPONSE_COUNT; i++) {
        msgCount ++;
        rtrMsgData[i][7] ++;
        CAN_loadRTRResponseLocked(CAN_RTR_0x100_MSG_OBJ+i, rtrMsgData[i]);
    }
    
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
    } else if((status >= 1U) && (status <= 32U)) {
        // A mailbox number (status) does not say whether the event was RX or TX.
        // Its configured type tells you that.
        // For example, cause CAN_RTR_0x100_MSG_OBJ on an RXTX_REMOTE object
        // with transmit interrupts enabled means its automatic response completed.
        switch (status) {
            case CAN_TX_MSG_OBJ:
                // Getting to this point means that the TX interrupt occurred on
                // message object CAN_TX_MSG_OBJ, and the message TX is complete.
                break;
            case CAN_RX_MSG_OBJ :
                // Getting to this point means that the RX interrupt occurred on
                // message object CAN_RX_MSG_OBJ, and the message RX is complete.
                (void)CAN_readMessage(CANA_BASE,
                          CAN_RX_MSG_OBJ,
                          rxMsgData);
                break;
            case CAN_RTR_0x100_MSG_OBJ :      
            case CAN_RTR_0x101_MSG_OBJ :      
            case CAN_RTR_0x102_MSG_OBJ :      
            case CAN_RTR_0x103_MSG_OBJ :      
            case CAN_RTR_0x104_MSG_OBJ :      
            case CAN_RTR_0x105_MSG_OBJ :
                // Automatic RTR response completed.
                break;
            default:
                // other NOT configured msg obj
                break;      
        }
    
        // Clear the message object interrupt.
        CAN_clearInterruptStatus(CANA_BASE, status);
        
    } else {
        // If something unexpected caused the interrupt, this would handle it.
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
