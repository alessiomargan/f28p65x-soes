//
// Included Files
//

#include "ethercat_subdevice_cpu1_hal.h"
#include "board.h"

#include <soes/ecat_slv.h>
#include <pins.h>

extern esc_cfg_t config;


void Sync0_Isr(void) {

    GPIO_writePin(dbg_2, 1);
    ecat_slv();
    GPIO_writePin(dbg_2, 0);
}
    

__interrupt
void INT_myCPUTIMER2_ISR(void) {

    GPIO_writePin(dbg_1, 1);
    if ( ! ESC_SYNCactivation() ) {
		ecat_slv();
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
            printf("FAIL ESC_initHW\n");
            ESTOP0;
        }
    }

    // Syscfg generate initialization
    Board_init();
    // Redirect printf/DPRINT to SCIA
    sci_stdio_init();

    // Setup and perform PDI Test
    //ESC_setupPDITestInterface();
    // Init soes
    ecat_slv_init(&config);

    // start timer 
    CPUTimer_startTimer(myCPUTIMER2_BASE);

    // Update local RAM with ESC register values for debugging
    while(1)
    {
        //ESC_debugUpdateESCRegLogs();
        DEVICE_DELAY_US((uint32_t)(500000));
        GPIO_togglePin(DEVICE_GPIO_PIN_LED2);
    }
}

//
// End of File
//
