//
// Included Files
//

#include "ethercat_subdevice_cpu1_hal.h"
#include <soes/ecat_slv.h>
#include <pins.h>
#include <peripherals.h>

extern esc_cfg_t config;


void Sync0_Isr(void) {
    // off
    GPIO_writePin(CCARD_LED_1_GPIO, 1UL);
    ecat_slv();
    // on
    GPIO_writePin(CCARD_LED_1_GPIO, 0UL);
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
    //
    gpio_conf();
    scia_init();

    // Setup and perform PDI Test
    //ESC_setupPDITestInterface();
    // Init soes
    ecat_slv_init(&config);

    // Update local RAM with ESC register values for debugging
    while(1)
    {
        if ( ! ESC_SYNCactivation() ) {
		    ecat_slv();
	    }
        //ESC_debugUpdateESCRegLogs();
        //DEVICE_DELAY_US((uint32_t)(50000));
        //GPIO_togglePin(DEVICE_GPIO_PIN_LED2);
    }
}

//
// End of File
//
