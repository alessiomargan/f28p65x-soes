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

extern esc_cfg_t config;

#define DC_SYNC_LED_TOGGLE_TICKS    125U

__attribute__((section(".TI.ramfunc")))
void Sync0_Isr(void) {

    GPIO_writePin(dbg_2, 1);
    ecat_slv();
    GPIO_writePin(dbg_2, 0);
}
    

__interrupt
void INT_myCPUTIMER2_ISR(void) {
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
            GPIO_togglePin(DEVICE_GPIO_PIN_LED2);
        }
    }
    else {
        dcSyncLedTicks = 0U;
        GPIO_writePin(DEVICE_GPIO_PIN_LED2, 1U);
    }

    GPIO_writePin(dbg_1, 0);
}

//
// Main
//
void main()
{
    uint16_t initStatus;

    // Initialize CPU1 and HAL interface
    initStatus = ESC_initHW();

    // Loop and signal error if initHW returns failure
    if(initStatus == ESC_HW_INIT_FAIL)
    {
        while(1)
        {
            // Toggle Error
            PRINTLN("FAIL ESC_initHW");
            ESTOP0;
        }
    }

    // Syscfg generate initialization
    Board_init();
    // Redirect printf/DPRINT to SCIA
    sci_stdio_init();
    // 
    print_build_info();
    // Setup and perform PDI Test
    //ESC_setupPDITestInterface();

    //
    if ( Configure_flashAPI() != Fapi_Status_Success ) {
        PRINTLN("FAIL Configure_flashAPI");
    }
    if (Read_Flash_Params() == PARAMS_CMD_ERROR) {
        //
        //glob_fault.bit.warn_read_flash = 1;
        PRINTLN("FAIL Read_Flash_Params");
        if (Load_Default_Params() == PARAMS_CMD_ERROR) {
            // FATAL ERROR
            //Error_Handler();
        }
        PRINTLN("Load_Default_Params");
    }

    if ( Erase_dataFlashSector((uint32_t)&flash_sdo, sizeof(flash_sdo)) != Fapi_Status_Success ) {
        PRINTLN("FAIL erase data sector !!!");
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

    // Update local RAM with ESC register values for debugging
    while(1)
    {
        //ESC_debugUpdateESCRegLogs();
        DEVICE_DELAY_US((uint32_t)(500000));
        GPIO_togglePin(DEVICE_GPIO_PIN_LED1);
        
    }
}

//
// End of File
//
