//
// Included Files
//

#include "ethercat_subdevice_cpu1_hal.h"
#include "board.h"

#include "soes/ecat_slv.h"
#include "flash_utils.h"
#include "globals.h"
#include "params.h"
#include "sci_io_driverlib.h"

extern void EtherCAT_init(void);

extern esc_cfg_t config;

static volatile uint16_t fatalErrorCode = 0U;

__attribute__((section(".TI.ramfunc")))
void FatalError_Handler(uint16_t errorCode)
{
    /* Prevent interrupts from changing application state or debug outputs. */
    DINT;

    /* Preserve the error for the debugger and expose its low two bits. */
    fatalErrorCode = errorCode;
    GPIO_writePin(dbg_1, (uint32_t)(errorCode & 1U));
    GPIO_writePin(dbg_2, (uint32_t)((errorCode >> 1U) & 1U));
    GPIO_writePin(dbg_3, (uint32_t)((errorCode >> 2U) & 1U));

    /* The controlCARD LEDs are active-low. */
    GPIO_writePin(DEVICE_LED1_GPIO, 0U);
    GPIO_writePin(DEVICE_LED2_GPIO, 0U);

    /* Halt under a debugger and never resume normal execution. */
    ESTOP0;
    for(;;) {}
}

/*  Initialize device clock and peripherals
    Copy the Flash initialization code from Flash to RAM
    Configure Flash wait-states, fall back power mode, performance features
    GPIO unlock
    PIE/vector table
*/
static void Device_BaseInit(void) {

    // Disable the watchdog
    SysCtl_disableWatchdog();

#ifdef _FLASH
    // Copy time critical code and flash setup code to RAM. This includes the
    // following functions: Flash_initModule();
    // The RamfuncsLoadStart, RamfuncsLoadSize, and RamfuncsRunStart symbols
    // are created by the linker. Refer to the device .cmd file.
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
    // Call Flash Initialization to setup flash waitstates. This function must
    // reside in RAM.
    Flash_initModule(FLASH0CTRL_BASE, FLASH0ECC_BASE, DEVICE_FLASH_WAITSTATES);
#endif
    
    // Verify the XTAL crystal frequency.
    if(!Device_verifyXTAL(DEVICE_OSCSRC_FREQ / 1000000)) {
        ESTOP0;
        for(;;) {}
    }

    // Set up device clock
    SysCtl_setClock(DEVICE_SETCLOCK_CFG);

    // Make sure the LSPCLK divider is set to the default (divide by 4)
    SysCtl_setLowSpeedClock(SYSCTL_LSPCLK_PRESCALE_4);

    // Turn on all peripherals and initialize GPIOs
    Device_enableAllPeripherals();
    Device_initGPIO();

    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    Interrupt_initModule();

    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    Interrupt_initVectorTable();

}

extern uint16_t txMsgData[];
//
// Main
//
void main()
{
    // clocks, Flash, GPIO unlock, PIE/vector table
    Device_BaseInit();
    // Syscfg generate initialization
    Board_init();
    // Configure EtherCAT after SysConfig so its pin mux and interrupts remain active
    EtherCAT_init();
    // Redirect printf/DPRINT to SCIA
    sci_stdio_init();
    // 
    print_build_info();

    //
    if ( Configure_flashAPI() != Fapi_Status_Success ) {
        PRINTLN("FAIL Configure_flashAPI");
        FatalError_Handler(FATAL_ERROR_FLASH_API);
    }
    if (Read_Flash_Params() == PARAMS_CMD_ERROR) {
        //
        //glob_fault.bit.warn_read_flash = 1;
        PRINTLN("FAIL Read_Flash_Params");
        if (Load_Default_Params() == PARAMS_CMD_ERROR) {
            FatalError_Handler(FATAL_ERROR_DEFAULT_PARAMS);
        }
        PRINTLN("Load_Default_Params");
    }

    PRINTLN("sdo.ram.fw_ver=%s", sdo.ram.fw_ver);
    PRINTLN("FLASH_SDO");
    print_sdo(&flash_sdo);
    PRINTLN("DFLT_FLASH_SDO");
    print_sdo(&dflt_flash_sdo);
    PRINTLN("SDO");
    print_sdo(&sdo.flash);
    
    // Init soes
    ecat_slv_init(&config);

    // start timer 
    CPUTimer_startTimer(myCPUTIMER2_BASE);

    // Enable interrupts to CPU
    EINT;

    while(1)
    {
        DEVICE_DELAY_US((uint32_t)(500000));
        GPIO_togglePin(DEVICE_LED1_GPIO);
        CAN_sendMessage(CANA_BASE, 2, 8, txMsgData);

    }
}

//
// End of File
//
