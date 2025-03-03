//
// Included Files
//
#include <stdint.h>
#include <stdio.h>
#include <file.h>

#include "ethercat_subdevice_cpu1_hal.h"
#include <soes/ecat_slv.h>
#include <pins.h>
#include <sci_io_driverlib.h>
#include <ti_ascii.h>

extern esc_cfg_t config;
//
// Main
//
void main()
{
    uint16_t initStatus;
    volatile FILE *fid;
    volatile int status = 0;


    // Initialize CPU1 and HAL interface
    initStatus = ESC_initHW();

    // Loop and signal error if initHW returns failure
    if(initStatus == ESC_HW_INIT_FAIL)
    {
        while(1)
        {
            // Toggle Error
            ESC_signalFail();
        }
    }
    //
    gpio_conf();
    scia_init();
    // Redirect STDOUT to SCI
    status = add_device("scia", _SSA, SCI_open, SCI_close, SCI_read, SCI_write,
                        SCI_lseek, SCI_unlink, SCI_rename);
    fid = fopen("scia","w");
    freopen("scia:", "w", stdout);
    setvbuf(stdout, NULL, _IONBF, 0);
    print_ascii_banner();

    printf("%s %d\n",__FUNCTION__, 1234);
    DPRINT("Hello world 0x%04X !!\n", 0xBEEF);
    float pi = 3.14159265;
    DPRINT("int(%d) float(%f)\n", 0xBEEF, pi);

    // Setup and perform PDI Test
    ESC_setupPDITestInterface();
    // Init soes
    ecat_slv_init(&config);

    // Update local RAM with ESC register values for debugging
    while(1)
    {
        ecat_slv();
        ESC_debugUpdateESCRegLogs();
        DEVICE_DELAY_US((uint32_t)(50000));
    }
}

//
// End of File
//
