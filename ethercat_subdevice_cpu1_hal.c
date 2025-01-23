//###########################################################################
//
// FILE:   ethercat_subdevice_cpu1_hal.c
//
// TITLE:  C28.CPU1 EtherCAT HAL Driver.
//
//###########################################################################
// $TI Release: F28P65x EtherCAT Software v1.01.00.00 $
// $Release Date: Tue Oct 29 00:56:43 IST 2024 $
// $Copyright:
// Copyright (C) 2021 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//###########################################################################

//
// Included Files
//
#include "ethercat_subdevice_cpu1_hal.h"

#ifdef PDI_HAL_TEST

//
// HAL Test Application - Register Debug Structure
//
ESC_DebugRegisters ESC_escRegs[ESC_HAL_TEST_DEBUG_REGS_LENGTH];

//
// HAL Block Data Arrays
//
static uint32_t ESC_writeBlockData[ESC_HAL_TEST_BLOCK_LENGTH / 4U];
static uint32_t ESC_readBlockData[ESC_HAL_TEST_BLOCK_LENGTH / 4U];

#endif // PDI_HAL_TEST

//*****************************************************************************
//
// ESC_getTimer
//
//*****************************************************************************
uint32_t
ESC_getTimer(void)
{
    //
    // Return 1's compliment of the CPU timer
    //
    return(~((uint32_t)CPUTimer_getTimerCount(CPUTIMER0_BASE)));
}

//*****************************************************************************
//
// ESC_clearTimer
//
//*****************************************************************************
void
ESC_clearTimer(void)
{
    //
    // Set the timer period count
    //
    CPUTimer_setPeriod(CPUTIMER0_BASE, 0xFFFFFFFFUL);

    //
    // Reload timer with the period count value
    //
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
}

//*****************************************************************************
//
// ESC_timerIncPerMilliSec
//
//*****************************************************************************
uint32_t
ESC_timerIncPerMilliSec(void)
{
    //
    // Returns value based on core frequency of 200MHz.
    //
    return((uint32_t)200000UL);
}

//*****************************************************************************
//
// ESC_readBlockISR
//
//*****************************************************************************
void
ESC_readBlockISR(ESCMEM_ADDR *pData, uint16_t address, uint16_t len)
{
    //
    // Create pointer to ESC Address
    //
    const void * escSource = (void *)(ESC_BASE + (uint32_t)(address / 2U));

    //
    // Copy ESC data to buffer. Convert lengths to words.
    //
    memcpy(pData, escSource, (size_t)((len + 1U) >> 1U));

    //
    // Determine if length (in bytes) is even or odd
    // (No action on even)
    //
    if((len & 0x1U) == 0x1U)
    {
        //
        // Odd Length
        //

        //
        // Clear extra byte in buffer
        //
        pData[((len - 1U) / 2U)] = (pData[((len - 1U) / 2U)] & ESC_M_LSB);
    }
}

//*****************************************************************************
//
// ESC_readBlock
//
//*****************************************************************************
void
ESC_readBlock(ESCMEM_ADDR *pData, uint16_t address, uint16_t len)
{
    //
    // Disables interrupts
    //
    DINT;

    //
    // Perform copy of ESC data to buffer
    //
    ESC_readBlockISR(pData, address, len);

    //
    // Enable interrupts
    //
    EINT;
}

//*****************************************************************************
//
// ESC_readDWordISR
//
//*****************************************************************************
uint32_t
ESC_readDWordISR(uint16_t address)
{
    //
    // Read 32-bit address from ESC memory
    //
    return(HWREG(ESC_BASE + (address / 2U)));
}

//*****************************************************************************
//
// ESC_readDWord
//
//*****************************************************************************
uint32_t
ESC_readDWord(uint16_t address)
{
    uint32_t dWordValue;

    //
    // Disables interrupts
    //
    DINT;

    //
    // Read 32-bit address from ESC memory
    //
    dWordValue = ESC_readDWordISR(address);

    //
    // Enable interrupts
    //
    EINT;

    //
    // Return value
    //
    return(dWordValue);
}

//*****************************************************************************
//
// ESC_readWordISR
//
//*****************************************************************************
uint16_t
ESC_readWordISR(uint16_t address)
{
    //
    // Read 16-bit address from ESC memory
    //
    return(HWREGH(ESC_BASE + (address / 2U)));
}

//*****************************************************************************
//
// ESC_readWord
//
//*****************************************************************************
uint16_t
ESC_readWord(uint16_t address)
{
    uint16_t wordValue;

    //
    // Disables interrupts
    //
    DINT;

    //
    // Read 16-bit address from ESC memory
    //
    wordValue = ESC_readWordISR(address);

    //
    // Enable interrupts
    //
    EINT;

    //
    // Return value
    //
    return(wordValue);
}

//*****************************************************************************
//
// ESC_writeBlockISR
//
//*****************************************************************************
void
ESC_writeBlockISR(ESCMEM_ADDR *pData, uint16_t address, uint16_t len)
{
    uint16_t wordValue;

    //
    // Create pointer to ESC Address
    //
    void * escDest = (void *)(ESC_BASE + (uint32_t)(address / 2U));

    //
    // Determine if length (in bytes) is even or odd
    //
    if((len & 0x1U) == 0x1U)
    {
        //
        // Odd Length
        //

        //
        // Copy even bytes of buffer data to ESC memory.
        // Convert length to words.
        //
        memcpy(escDest, pData, (size_t)((len - 1U) >> 1U));

        //
        // Read last 16-bit word
        //
        wordValue = ESC_readWordISR(address + (len - 1U));

        //
        // Modify word data and set new data
        //
        wordValue &= ESC_M_MSB;
        wordValue |= (pData[((len - 1U) / 2U)] & ESC_M_LSB);

        //
        // Write modified value to ESC destination
        //
        ESC_writeWordISR(wordValue, (address + (len - 1U)));
    }
    else
    {
        //
        // Even Length
        //

        //
        // Copy buffer data to ESC memory. Convert length to words.
        //
        memcpy(escDest, pData, (size_t)(len >> 1U));
    }
}

//*****************************************************************************
//
// ESC_writeBlock
//
//*****************************************************************************
void
ESC_writeBlock(ESCMEM_ADDR *pData, uint16_t address, uint16_t len)
{
    //
    // Disables interrupts
    //
    DINT;

    //
    // Write data buffer into ESC memory
    //
    ESC_writeBlockISR(pData, address, len);

    //
    // Enable interrupts
    //
    EINT;
}

//*****************************************************************************
//
// ESC_writeDWordISR
//
//*****************************************************************************
void
ESC_writeDWordISR(uint32_t dWordValue, uint16_t address)
{
    //
    // Write two 16-bit words to ESC memory
    //
    HWREG(ESC_BASE + (address / 2U)) = dWordValue;
}

//*****************************************************************************
//
// ESC_writeDWord
//
//*****************************************************************************
void
ESC_writeDWord(uint32_t dWordValue, uint16_t address)
{
    //
    // Disables interrupts
    //
    DINT;

    //
    // Write two 16-words into ESC memory
    //
    ESC_writeDWordISR(dWordValue, address);

    //
    // Enable interrupts
    //
    EINT;
}

//*****************************************************************************
//
// ESC_writeWordISR
//
//*****************************************************************************
void
ESC_writeWordISR(uint16_t wordValue, uint16_t address)
{
    //
    // Write 16-bit word into ESC memory
    //
    HWREGH(ESC_BASE + (address / 2U)) = wordValue;
}

//*****************************************************************************
//
// ESC_writeWord
//
//*****************************************************************************
void
ESC_writeWord(uint16_t wordValue, uint16_t address)
{
    //
    // Disables interrupts
    //
    DINT;

    //
    // Write one 16-word into ESC memory
    //
    ESC_writeWordISR(wordValue, address);

    //
    // Enable interrupts
    //
    EINT;
}

//*****************************************************************************
//
// ESC_setLed
//
//*****************************************************************************
void
ESC_setLed(uint16_t runLed, uint16_t errLed)
{
    //
    // Set/Clear Run LED
    //
    GPIO_writePin(ESC_RUN_LED_GPIO, (uint32_t)(runLed));

    //
    // Set/Clear Error LED
    //
    GPIO_writePin(ESC_ERR_LED_GPIO, (uint32_t)(errLed));
}

#ifdef PDI_HAL_TEST
//*****************************************************************************
//
// HAL_initWriteBlockData
//
//*****************************************************************************
void HAL_initWriteBlockData(void)
{
    uint16_t i;

    //
    // Initialize write block data array
    //
    for(i = 0U; i < (ESC_HAL_TEST_BLOCK_LENGTH / 4U); i++)
    {
        ESC_writeBlockData[i] = 0x1234ABCDUL;
    }
}

//*****************************************************************************
//
// HAL_clearReadBlockData
//
//*****************************************************************************
void HAL_clearReadBlockData(void)
{
    uint16_t i;

    //
    // Clear read block data array
    //
    for(i = 0U; i < (ESC_HAL_TEST_BLOCK_LENGTH / 4U); i++)
    {
        ESC_readBlockData[i] = 0x0UL;
    }
}

//*****************************************************************************
//
// HAL_testBlockData
//
//*****************************************************************************
uint16_t HAL_testBlockData(uint16_t length)
{
    uint16_t i;
    uint16_t * writeData = (uint16_t *)(&ESC_writeBlockData[0]);
    uint16_t * readData = (uint16_t *)(&ESC_readBlockData[0]);

    //
    // Check if odd length of bytes
    //
    if((length & 0x1U) == 0x1U)
    {
        //
        // Odd Length of Bytes
        //

        //
        // Check that data written and data read match
        //
        for(i = 0U; i < ((length - 1U) / 2U); i++)
        {
            if(*writeData != *readData)
            {
                return(ESC_HAL_BLOCK_TEST_FAIL);
            }

            writeData++;
            readData++;
        }

        //
        // Check last byte
        //
        if(((*writeData) & ESC_M_LSB) != ((*readData) & ESC_M_LSB))
        {
            return(ESC_HAL_BLOCK_TEST_FAIL);
        }
    }
    else
    {
        //
        // Even Length of Bytes
        //

        //
        // Check that data written and data read match
        //
        for(i = 0U; i < (length / 2U); i++)
        {
            if(*writeData != *readData)
            {
                return(ESC_HAL_BLOCK_TEST_FAIL);
            }

            writeData++;
            readData++;
        }
    }

    return(ESC_HAL_BLOCK_TEST_PASS);
}

//*****************************************************************************
//
// ESC_setupPDITestInterface
//
//*****************************************************************************
void
ESC_setupPDITestInterface(void)
{
    uint16_t currentAddress = 0x0U;
    uint32_t doubleWordData = 0x0UL;
    uint16_t wordData = 0x0U;

    //
    // Setup and add various ESC register byte addresses to monitor
    //
    ESC_debugInitESCRegLogs();
    ESC_debugAddESCRegsAddress(0x0U);     // Type, Revision
    ESC_debugAddESCRegsAddress(0x2U);     // Build
    ESC_debugAddESCRegsAddress(0x4U);     // FMMUs, SyncManagers Supported
    ESC_debugAddESCRegsAddress(0x8U);     // ESC Supported Features
    ESC_debugAddESCRegsAddress(0x100U);   // DL Control
    ESC_debugAddESCRegsAddress(0x102U);   // DL Control (Extended)
    ESC_debugAddESCRegsAddress(0x110U);   // DL Status
    ESC_debugAddESCRegsAddress(0x310U);   // Lost Link Counter 1
    ESC_debugAddESCRegsAddress(0x312U);   // Lost Link Counter 2
    ESC_debugAddESCRegsAddress(0x510U);   // MII Control/Status
    ESC_debugAddESCRegsAddress(0x512U);   // PHY Address
    ESC_debugAddESCRegsAddress(0x514U);   // PHY Data
    ESC_debugAddESCRegsAddress(0x516U);   // MII ECAT Access State
    ESC_debugAddESCRegsAddress(0x518U);   // PHY Port Status (1)
    ESC_debugAddESCRegsAddress(0x51AU);   // PHY Port Status (2)
    ESC_debugAddESCRegsAddress(0x1000U);  // First data word of ESC RAM

    //
    // Check that correct PDI type (ASYNC16) is configured
    //
    if((ESC_readWord(ESC_O_PDI_CONTROL) & ESC_PDI_CONTROL_ASYNC16) !=
       ESC_PDI_CONTROL_ASYNC16)
    {
        //
        // PDI not operational or incorrect
        //
        while(1)
        {
            //
            // Toggle Error
            //
            ESC_signalFail();
        }
    }

    //
    // Perform PDI read and write tests for entire ESC PDI RAM range
    //
    for(currentAddress = ESC_PDI_RAM_START_ADDRESS_OFFSET;
        currentAddress <= ESC_PDI_RAM_END_ADDRESS_OFFSET;
        currentAddress += 4U)
    {
        //
        // Read DWord and Write DWord API Tests
        //
        doubleWordData = ESC_readDWord(currentAddress);
        ESC_writeDWord(0xABCD1234UL, currentAddress);
        doubleWordData = ESC_readDWord(currentAddress);

        if(doubleWordData != 0xABCD1234UL)
        {
            while(1)
            {
                //
                // Toggle Error
                //
                ESC_signalFail();
            }
        }

        doubleWordData = ESC_readDWordISR(currentAddress);
        ESC_writeDWordISR(0x1A2B3C4DUL, currentAddress);
        doubleWordData = ESC_readDWordISR(currentAddress);

        if(doubleWordData != 0x1A2B3C4DUL)
        {
            while(1)
            {
                //
                // Toggle Error
                //
                ESC_signalFail();
            }
        }

        //
        // Read Word and Write Word API Tests
        //
        wordData = ESC_readWord(currentAddress);
        ESC_writeWord(0x6789U, currentAddress);
        wordData = ESC_readWord(currentAddress);

        if(wordData != 0x6789U)
        {
            while(1)
            {
                //
                // Toggle Error
                //
                ESC_signalFail();
            }
        }

        wordData = ESC_readWordISR(currentAddress);
        ESC_writeWordISR(0x5A5AU, currentAddress);
        wordData = ESC_readWordISR(currentAddress);

        if(wordData != 0x5A5AU)
        {
            while(1)
            {
                //
                // Toggle Error
                //
                ESC_signalFail();
            }
        }
    }

    //
    // Setup for read/write block API Tests
    //
    HAL_initWriteBlockData();

    //
    // Perform even length write and read block (non-ISR) API Test
    //
    ESC_writeBlock((uint16_t *)(&ESC_writeBlockData[0]),
                   ESC_PDI_RAM_START_ADDRESS_OFFSET,
                   ESC_HAL_TEST_BLOCK_LENGTH);
    ESC_readBlock((uint16_t *)(&ESC_readBlockData[0]),
                  ESC_PDI_RAM_START_ADDRESS_OFFSET,
                  ESC_HAL_TEST_BLOCK_LENGTH);

    if(HAL_testBlockData(ESC_HAL_TEST_BLOCK_LENGTH) != ESC_HAL_BLOCK_TEST_PASS)
    {
        while(1)
        {
            //
            // Toggle Error
            //
            ESC_signalFail();
        }
    }

    HAL_clearReadBlockData();

    //
    // Perform even length write and read block API Test
    //
    ESC_writeBlockISR((uint16_t *)(&ESC_writeBlockData[0]),
                      (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x1000U),
                      ESC_HAL_TEST_BLOCK_LENGTH);
    ESC_readBlockISR((uint16_t *)(&ESC_readBlockData[0]),
                     (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x1000U),
                     ESC_HAL_TEST_BLOCK_LENGTH);

    if(HAL_testBlockData(ESC_HAL_TEST_BLOCK_LENGTH) != ESC_HAL_BLOCK_TEST_PASS)
    {
        while(1)
        {
            //
            // Toggle Error
            //
            ESC_signalFail();
        }
    }

    HAL_clearReadBlockData();

    //
    // Perform odd length write and read block (non-ISR) API Test
    //
    ESC_writeBlock((uint16_t *)(&ESC_writeBlockData[0]),
                   (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x2000U),
                   (ESC_HAL_TEST_BLOCK_LENGTH - 1U));
    ESC_readBlock((uint16_t *)(&ESC_readBlockData[0]),
                  (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x2000U),
                  (ESC_HAL_TEST_BLOCK_LENGTH - 1U));

    if(HAL_testBlockData((ESC_HAL_TEST_BLOCK_LENGTH - 1U)) !=
       ESC_HAL_BLOCK_TEST_PASS)
    {
        while(1)
        {
            //
            // Toggle Error
            //
            ESC_signalFail();
        }
    }

    HAL_clearReadBlockData();

    //
    // Perform odd length write and read block API Test
    //
    ESC_writeBlockISR((uint16_t *)(&ESC_writeBlockData[0]),
                      (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x3000U),
                      (ESC_HAL_TEST_BLOCK_LENGTH - 1U));
    ESC_readBlockISR((uint16_t *)(&ESC_readBlockData[0]),
                     (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x3000U),
                     (ESC_HAL_TEST_BLOCK_LENGTH - 1U));

    if(HAL_testBlockData((ESC_HAL_TEST_BLOCK_LENGTH - 1U)) !=
       ESC_HAL_BLOCK_TEST_PASS)
    {
        while(1)
        {
            //
            // Toggle Error
            //
            ESC_signalFail();
        }
    }

    //
    // Pass
    //
    ESC_signalPass();
}

//*****************************************************************************
//
// ESC_debugUpdateESCRegLogs
//
//*****************************************************************************
void
ESC_debugUpdateESCRegLogs(void)
{
    uint16_t i = 0U;

    //
    // Update ESC register data into debug array
    //
    while(ESC_escRegs[i].address != 0xFFFFU)
    {
        ESC_escRegs[i].data = ESC_readWord(ESC_escRegs[i].address);
        i++;
    }
}

//*****************************************************************************
//
// ESC_debugAddESCRegsAddress
//
//*****************************************************************************
void
ESC_debugAddESCRegsAddress(uint16_t address)
{
    uint16_t i = 0U;

    //
    // Add ESC Register Address to debug array
    //
    for(i = 0U; i < ESC_HAL_TEST_DEBUG_REGS_LENGTH; i++)
    {
        if(ESC_escRegs[i].address == 0xFFFFU)
        {
            //
            // Assign byte address to struct
            //
            ESC_escRegs[i].address = address;
            return;
        }
    }
}

//*****************************************************************************
//
// ESC_debugInitESCRegLogs
//
//*****************************************************************************
void
ESC_debugInitESCRegLogs(void)
{
    uint16_t i = 0U;

    //
    // Initialize ESC register debug array
    //
    for(i = 0U; i < ESC_HAL_TEST_DEBUG_REGS_LENGTH; i++)
    {
        ESC_escRegs[i].address = 0xFFFFU;
        ESC_escRegs[i].data = 0xFFFFU;
    }
}

//*****************************************************************************
//
// ESC_signalPass
//
//*****************************************************************************
void
ESC_signalPass(void)
{
    //
    // Turn on controlCARD LEDs
    //
    GPIO_writePin(CCARD_LED_1_GPIO, 0UL);
    GPIO_writePin(CCARD_LED_2_GPIO, 0UL);
}

//*****************************************************************************
//
// ESC_signalFail
//
//*****************************************************************************
void
ESC_signalFail(void)
{
    //
    // Toggle controlCARD LEDs and delay
    //
    GPIO_togglePin(CCARD_LED_1_GPIO);
    GPIO_togglePin(CCARD_LED_2_GPIO);

    DEVICE_DELAY_US((uint32_t)(500000));
}

//*****************************************************************************
//
// ESC_passFailSignalSetup
//
//*****************************************************************************
void
ESC_passFailSignalSetup(void)
{
    //
    // Set LED GPIOs to output mode
    //
    GPIO_setDirectionMode(CCARD_LED_1_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(CCARD_LED_2_GPIO, GPIO_DIR_MODE_OUT);

    //
    // Turn off controlCARD LEDs
    //
    GPIO_writePin(CCARD_LED_1_GPIO, 1UL);
    GPIO_writePin(CCARD_LED_2_GPIO, 1UL);
}
#endif // PDI_HAL_TEST

//*****************************************************************************
//
// ESC_loadedCheckEEPROM
//
//*****************************************************************************
uint16_t
ESC_loadedCheckEEPROM(void)
{
    uint16_t status;

    //
    // Get ESC DL Status register
    //
    status = ESC_readWord(ESC_O_DL_STATUS);

    //
    // Check if EEPROM is loaded
    //
    if((status & ESC_DL_STATUS_EEPROMLOAD_M) == ESC_DL_STATUS_EEPROMLOAD_M)
    {
        //
        // Get ESC EEPROM Status register
        //
        status = ESC_readWord(ESC_O_EEPROM_STATUS);

        //
        // Check if EEPROM is loaded and device information is okay
        //
        if((status & ESC_EEPROM_STATUS_EEPROMLOAD_M) == ESC_EEPROM_SUCCESS)
        {
            return(ESC_EEPROM_SUCCESS);
        }
        else
        {
            return(ESC_EEPROM_LOAD_ERROR);
        }
    }
    else
    {
        //
        // DL Status indicated EEPROM is not loaded
        //
        return(ESC_EEPROM_NOT_LOADED);
    }
}

//*****************************************************************************
//
// ESC_resetESC
//
//*****************************************************************************
void
ESC_resetESC(void)
{
    SysCtl_resetPeripheral(SYSCTL_PERIPH_RES_ECAT);
}

//*****************************************************************************
//
// ESC_holdESCInReset
//
//*****************************************************************************
void
ESC_holdESCInReset(void)
{
    //
    // Put ESC in reset
    //
    EALLOW;
    HWREGH(DEVCFG_BASE + SYSCTL_O_SOFTPRES23) |= SYSCTL_SOFTPRES23_ETHERCAT;
    EDIS;
}

//*****************************************************************************
//
// ESC_releaseESCReset
//
//*****************************************************************************
void
ESC_releaseESCReset(void)
{
    //
    // Release from reset
    //
    EALLOW;
    HWREGH(DEVCFG_BASE + SYSCTL_O_SOFTPRES23) &= (~SYSCTL_SOFTPRES23_ETHERCAT);
    EDIS;
}

//*****************************************************************************
//
// ESC_initHW
//
//*****************************************************************************
uint16_t
ESC_initHW(void)
{
    //
    // Set application-specific timeout for waiting for EEPROM to load
    // and one for waiting for memory initialization
    // (End user can adjust as needed)
    //
    uint16_t eepromTimeOut = 0x1000U;
    uint16_t memoryTimeOut = 0x300U;

    //
    // Disable the watchdog
    //
    SysCtl_disableWatchdog();

#ifdef _FLASH
    //
    // Copy time critical code and flash setup code to RAM. This includes the
    // following functions: Flash_initModule();
    //
    // The RamfuncsLoadStart, RamfuncsLoadSize, and RamfuncsRunStart symbols
    // are created by the linker. Refer to the device .cmd file.
    //
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);

    //
    // Call Flash Initialization to setup flash waitstates. This function must
    // reside in RAM.
    //
    Flash_initModule(FLASH0CTRL_BASE, FLASH0ECC_BASE, DEVICE_FLASH_WAITSTATES);
#endif

    //
    // Verify the XTAL crystal frequency.
    //
#warn "IMPORTANT: F2838x EtherCAT software is now built for XTAL source of 25MHz. Refer to the EtherCAT Software User Guide for more information"
    if(!Device_verifyXTAL(DEVICE_OSCSRC_FREQ / 1000000))
    {
        //
        // The actual XTAL frequency does not match DEVICE_OSCSRC_FREQ!!
        // Please check the XTAL frequency used.
        //
        // Note that the latest F2838x controlCARDs (Rev.B and later) have been
        // updated to use 25MHz XTAL by default. If you have an older 20MHz XTAL
        // controlCARD (E1, E2, or Rev.A), refer to the controlCARD
        // documentation on steps to reconfigure the controlCARD from 20MHz to
        // 25MHz.
        //
        ESTOP0;

        //
        // Set LED GPIOs to output mode
        //
        GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);
        GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);

        while(1)
        {
            //
            // Toggle controlCARD LEDs and delay
            //
            GPIO_togglePin(DEVICE_GPIO_PIN_LED1);
            GPIO_togglePin(DEVICE_GPIO_PIN_LED2);

            DEVICE_DELAY_US((uint32_t)(50000));
        }
    }

    //
    // Set up device clock
    //
    SysCtl_setClock(DEVICE_SETCLOCK_CFG);

    //
    // Make sure the LSPCLK divider is set to the default (divide by 4)
    //
    SysCtl_setLowSpeedClock(SYSCTL_LSPCLK_PRESCALE_4);

    //
    // Turn on all peripherals and initialize GPIOs
    //
    Device_enableAllPeripherals();
    Device_initGPIO();

    //
    // Configure GPIOs for ECAT
    //

    #ifdef _LAUNCHXL_F28P65X

    //
    // PHY Reset
    //
    GPIO_setPinConfig(GPIO_76_ESC_PHY_RESETN);
    GPIO_setQualificationMode(76,GPIO_QUAL_ASYNC);

    //
    // I2C for EEPROM
    //
    GPIO_setPinConfig(GPIO_40_ESC_I2C_SDA);
    GPIO_setQualificationMode(40,GPIO_QUAL_ASYNC);
    GPIO_setPadConfig(40,GPIO_PIN_TYPE_PULLUP);

    GPIO_setPinConfig(GPIO_30_ESC_I2C_SCL);
    GPIO_setQualificationMode(30,GPIO_QUAL_ASYNC);
    GPIO_setPadConfig(30,GPIO_PIN_TYPE_PULLUP);

    //
    // P0 TX and RX DATA
    //
    GPIO_setPinConfig(GPIO_87_ESC_TX0_DATA0);
    GPIO_setQualificationMode(87,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_88_ESC_TX0_DATA1);
    GPIO_setQualificationMode(88,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_89_ESC_TX0_DATA2);
    GPIO_setQualificationMode(89,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_90_ESC_TX0_DATA3);
    GPIO_setQualificationMode(90,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_80_ESC_RX0_DATA0);
    GPIO_setQualificationMode(80,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_28_ESC_RX0_DATA1);
    GPIO_setQualificationMode(28,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_29_ESC_RX0_DATA2);
    GPIO_setQualificationMode(29,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_83_ESC_RX0_DATA3);
    GPIO_setQualificationMode(83,GPIO_QUAL_ASYNC);

    //
    // P0 TX Enable, RX DV, RX ERR
    //
    GPIO_setPinConfig(GPIO_56_ESC_TX0_ENA);
    GPIO_setQualificationMode(56,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_78_ESC_RX0_DV);
    GPIO_setQualificationMode(78,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_26_ESC_RX0_ERR);
    GPIO_setPadConfig(26,GPIO_PIN_TYPE_STD);

    //
    // P0 TX and RX Clk
    //
    GPIO_setPinConfig(GPIO_85_ESC_TX0_CLK);
    GPIO_setQualificationMode(85,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_77_ESC_RX0_CLK);
    GPIO_setQualificationMode(77,GPIO_QUAL_ASYNC);

    //
    // P0 Linkstatus and Link Active LED
    //
    GPIO_setPinConfig(GPIO_86_ESC_PHY0_LINKSTATUS);
    GPIO_setPadConfig(86,GPIO_PIN_TYPE_INVERT);
    GPIO_setQualificationMode(86,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_58_ESC_LED_LINK0_ACTIVE);
    GPIO_setQualificationMode(58, GPIO_QUAL_ASYNC);

    //
    // P0+P1 MDIO CLK and Data
    //
    GPIO_setPinConfig(GPIO_62_ESC_MDIO_CLK);
    GPIO_setQualificationMode(62,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_27_ESC_MDIO_DATA);
    GPIO_setQualificationMode(27,GPIO_QUAL_ASYNC);

    //
    // P1 TX and RX DATA
    //
    GPIO_setPinConfig(GPIO_22_ESC_TX1_DATA0);
    GPIO_setQualificationMode(22,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_74_ESC_TX1_DATA1);
    GPIO_setQualificationMode(74,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_73_ESC_TX1_DATA2);
    GPIO_setQualificationMode(73,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_19_ESC_TX1_DATA3);
    GPIO_setQualificationMode(19,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_63_ESC_RX1_DATA0);
    GPIO_setQualificationMode(63,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_64_ESC_RX1_DATA1);
    GPIO_setQualificationMode(64,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_65_ESC_RX1_DATA2);
    GPIO_setQualificationMode(65,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_66_ESC_RX1_DATA3);
    GPIO_setQualificationMode(66,GPIO_QUAL_ASYNC);

    //
    // P1 TX Enable, RX DV, RX ERR
    //
    GPIO_setPinConfig(GPIO_45_ESC_TX1_ENA);
    GPIO_setQualificationMode(45,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_70_ESC_RX1_DV);
    GPIO_setQualificationMode(70,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_71_ESC_RX1_ERR);
    GPIO_setPadConfig(71,GPIO_PIN_TYPE_STD);

    //
    // P1 TX and RX Clk
    //
    GPIO_setPinConfig(GPIO_44_ESC_TX1_CLK);
    GPIO_setQualificationMode(44,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_69_ESC_RX1_CLK);
    GPIO_setQualificationMode(69,GPIO_QUAL_ASYNC);

    //
    // P1 Linkstatus and Link Active LED
    //
    GPIO_setPinConfig(GPIO_68_ESC_PHY1_LINKSTATUS);
    GPIO_setPadConfig(68,GPIO_PIN_TYPE_INVERT);
    GPIO_setQualificationMode(68,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_59_ESC_LED_LINK1_ACTIVE);
    GPIO_setQualificationMode(59, GPIO_QUAL_ASYNC);

    //
    // Sync and Latch
    //
    // Note these GPIOs aren't required for EtherCAT
    // operation and are configured here for demonstration,
    // analysis, and debug purposes
    //
    GPIO_setPinConfig(GPIO_34_ESC_SYNC0);
    GPIO_setQualificationMode(34,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_35_ESC_SYNC1);
    GPIO_setQualificationMode(35,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_60_ESC_LATCH0);
    GPIO_setQualificationMode(60,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_51_ESC_LATCH1);
    GPIO_setQualificationMode(51,GPIO_QUAL_ASYNC);

    //
    // Set GPIO direction for ECAT RUN/ERR LEDs
    //

    GPIO_setPinConfig(GPIO_33_ESC_LED_ERR);
    GPIO_setQualificationMode(33,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_61_ESC_LED_RUN);
    GPIO_setQualificationMode(61,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_0_ESC_GPI0);
    GPIO_setQualificationMode(0,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_8_ESC_GPO0);
    GPIO_setQualificationMode(8,GPIO_QUAL_ASYNC);

    #else 

    //
    // PHY Reset
    //
    GPIO_setPinConfig(GPIO_155_ESC_PHY_RESETN);
    GPIO_setQualificationMode(155,GPIO_QUAL_ASYNC);

    //
    // I2C for EEPROM
    //
    GPIO_setPinConfig(GPIO_150_ESC_I2C_SDA);
    GPIO_setQualificationMode(150,GPIO_QUAL_ASYNC);
    GPIO_setPadConfig(150,GPIO_PIN_TYPE_PULLUP);

    GPIO_setPinConfig(GPIO_151_ESC_I2C_SCL);
    GPIO_setQualificationMode(151,GPIO_QUAL_ASYNC);
    GPIO_setPadConfig(151,GPIO_PIN_TYPE_PULLUP);

    //
    // P0 TX and RX DATA
    //
    GPIO_setPinConfig(GPIO_158_ESC_TX0_DATA0);
    GPIO_setQualificationMode(158,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_159_ESC_TX0_DATA1);
    GPIO_setQualificationMode(159,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_160_ESC_TX0_DATA2);
    GPIO_setQualificationMode(160,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_161_ESC_TX0_DATA3);
    GPIO_setQualificationMode(161,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_165_ESC_RX0_DATA0);
    GPIO_setQualificationMode(165,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_166_ESC_RX0_DATA1);
    GPIO_setQualificationMode(166,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_167_ESC_RX0_DATA2);
    GPIO_setQualificationMode(167,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_168_ESC_RX0_DATA3);
    GPIO_setQualificationMode(168,GPIO_QUAL_ASYNC);

    //
    // P0 TX Enable, RX DV, RX ERR
    //
    GPIO_setPinConfig(GPIO_156_ESC_TX0_ENA);
    GPIO_setQualificationMode(156,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_162_ESC_RX0_DV);
    GPIO_setQualificationMode(162,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_164_ESC_RX0_ERR);
    GPIO_setPadConfig(164,GPIO_PIN_TYPE_STD);

    //
    // P0 TX and RX Clk
    //
    GPIO_setPinConfig(GPIO_157_ESC_TX0_CLK);
    GPIO_setQualificationMode(157,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_163_ESC_RX0_CLK);
    GPIO_setQualificationMode(163,GPIO_QUAL_ASYNC);

    //
    // P0 Linkstatus and Link Active LED
    //
    GPIO_setPinConfig(GPIO_148_ESC_PHY0_LINKSTATUS);
    GPIO_setPadConfig(148,GPIO_PIN_TYPE_INVERT);
    GPIO_setQualificationMode(146,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_58_ESC_LED_LINK0_ACTIVE);
    GPIO_setQualificationMode(58, GPIO_QUAL_ASYNC);

    //
    // P0+P1 MDIO CLK and Data
    //
    GPIO_setPinConfig(GPIO_152_ESC_MDIO_CLK);
    GPIO_setQualificationMode(152,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_153_ESC_MDIO_DATA);
    GPIO_setQualificationMode(153,GPIO_QUAL_ASYNC);

    //
    // P1 TX and RX DATA
    //
    GPIO_setPinConfig(GPIO_131_ESC_TX1_DATA0);
    GPIO_setQualificationMode(131,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_132_ESC_TX1_DATA1);
    GPIO_setQualificationMode(132,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_134_ESC_TX1_DATA2);
    GPIO_setQualificationMode(134,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_19_ESC_TX1_DATA3);
    GPIO_setQualificationMode(19,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_63_ESC_RX1_DATA0);
    GPIO_setQualificationMode(63,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_64_ESC_RX1_DATA1);
    GPIO_setQualificationMode(64,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_141_ESC_RX1_DATA2);
    GPIO_setQualificationMode(141,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_142_ESC_RX1_DATA3);
    GPIO_setQualificationMode(142,GPIO_QUAL_ASYNC);

    //
    // P1 TX Enable, RX DV, RX ERR
    //
    GPIO_setPinConfig(GPIO_129_ESC_TX1_ENA);
    GPIO_setQualificationMode(129,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_70_ESC_RX1_DV);
    GPIO_setQualificationMode(70,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_71_ESC_RX1_ERR);
    GPIO_setPadConfig(71,GPIO_PIN_TYPE_STD);

    //
    // P1 TX and RX Clk
    //
    GPIO_setPinConfig(GPIO_130_ESC_TX1_CLK);
    GPIO_setQualificationMode(130,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_69_ESC_RX1_CLK);
    GPIO_setQualificationMode(69,GPIO_QUAL_ASYNC);

    //
    // P1 Linkstatus and Link Active LED
    //
    GPIO_setPinConfig(GPIO_149_ESC_PHY1_LINKSTATUS);
    GPIO_setPadConfig(149,GPIO_PIN_TYPE_INVERT);
    GPIO_setQualificationMode(149,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_59_ESC_LED_LINK1_ACTIVE);
    GPIO_setQualificationMode(59, GPIO_QUAL_ASYNC);

    //
    // Sync and Latch
    //
    // Note these GPIOs aren't required for EtherCAT
    // operation and are configured here for demonstration,
    // analysis, and debug purposes
    //
    GPIO_setPinConfig(GPIO_127_ESC_SYNC0);
    GPIO_setQualificationMode(127,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_35_ESC_SYNC1);
    GPIO_setQualificationMode(35,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_50_ESC_LATCH0);
    GPIO_setQualificationMode(50,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_30_ESC_LATCH1);
    GPIO_setQualificationMode(30,GPIO_QUAL_ASYNC);

    //
    // Set GPIO direction for ECAT RUN/ERR LEDs
    //

    GPIO_setPinConfig(GPIO_145_ESC_LED_ERR);
    GPIO_setQualificationMode(145,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_146_ESC_LED_RUN);
    GPIO_setQualificationMode(146,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_0_ESC_GPI0);
    GPIO_setQualificationMode(0,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_8_ESC_GPO0);
    GPIO_setQualificationMode(8,GPIO_QUAL_ASYNC);

    #endif 

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // Register EtherCAT Interrupt Handlers
    //
    Interrupt_register(INT_ECAT, &ESC_applicationLayerHandler);
    Interrupt_register(INT_ECATSYNC0, &ESC_applicationSync0Handler);
    Interrupt_register(INT_ECATSYNC1, &ESC_applicationSync1Handler);

    //
    // Enable ECAT/PDI, Sync0, and Sync1 interrupts in CPU
    //
    Interrupt_enable(INT_ECAT);
    Interrupt_enable(INT_ECATSYNC0);
    Interrupt_enable(INT_ECATSYNC1);

    //
    // Configure and Start timer
    //
    CPUTimer_setPeriod(CPUTIMER0_BASE, 0xFFFFFFFFUL);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0U);
    CPUTimer_startTimer(CPUTIMER0_BASE);

#ifdef PDI_HAL_TEST
    //
    // Enable Pass, fail signals
    //
    ESC_passFailSignalSetup();
#endif // PDI_HAL_TEST

    //
    // Sys PLL = 200MHz and use /2 to get 100MHz for ECAT IP
    // (There is a built in /4 to get 25MHz for PHY when using
    //  internal clocking for PHY)
    //
    SysCtl_setECatClk(SYSCTL_ECATCLKOUT_DIV_2, SYSCTL_SOURCE_SYSPLL,
                      ESC_DISABLE_INT_PHY_CLK);

    //
    // Configure EEPROM Size for 16K bits or less
    //
    ESCSS_configureEEPROMSize(ESC_SS_CONFIG_BASE, ESCSS_LESS_THAN_16K);

    //
    // Reset ESC
    //
    ESC_resetESC();

#ifdef PDI_HAL_TEST
    //
    // Enable the debug access
    //
    while((HWREGH(ESC_SS_BASE + ESCSS_O_ACCESS_CTRL) &
           ESCSS_ACCESS_CTRL_ENABLE_DEBUG_ACCESS) !=
          ESCSS_ACCESS_CTRL_ENABLE_DEBUG_ACCESS)
    {
        ESCSS_enableDebugAccess(ESC_SS_BASE);
    }
#endif // PDI_HAL_TEST

    //
    // Initialize ESCSS Memory
    //
    ESCSS_initMemory(ESC_SS_BASE);

    //
    // Wait for ESCSS memory initialization to complete
    //
    if(ESCSS_getMemoryInitDoneStatusBlocking(ESC_SS_BASE, memoryTimeOut) !=
       ESCSS_API_SUCCESS)
    {
        return(ESC_HW_INIT_FAIL);
    }

    //
    // Wait (with time out) for EEPROM to be loaded
    //
    while(ESC_loadedCheckEEPROM() != ESC_EEPROM_SUCCESS)
    {
        eepromTimeOut--;

        //
        // On time out, return fail
        //
        if(eepromTimeOut == 0U)
        {
            return(ESC_HW_INIT_FAIL);
        }
    }

    //
    // Initialize AL Event Mask to zero to prevent ESC interrupts until
    // set during stack state transition to SAFEOP
    //
    ESC_writeDWord(0UL, ESC_O_AL_EVENTMASK);

    //
    // Enable ECAT/PDI, Sync0, and Sync1 interrupts in ESCSS
    //
    ESCSS_setMaskedInterruptStatus(ESC_SS_BASE,
                                   (ESCSS_INTR_MASK_IRQ_MASK |
                                    ESCSS_INTR_MASK_SYNC0_MASK |
                                    ESCSS_INTR_MASK_SYNC1_MASK));

    //
    // Connect ESCSS Sync0/1 signals to respective interrupts in PIE
    //
    ESCSS_configureSync0Connections(ESC_SS_BASE, ESCSS_SYNC0_CONFIG_C28X_PIE_EN,
                                    ESCSS_VALID_KEY_VALUE);
    ESCSS_configureSync1Connections(ESC_SS_BASE, ESCSS_SYNC1_CONFIG_C28X_PIE_EN,
                                    ESCSS_VALID_KEY_VALUE);

    //
    // Enable interrupts to CPU
    //
    EINT;

    return(ESC_HW_INIT_SUCCESS);
}

//*****************************************************************************
//
// ESC_releaseHW
//
//*****************************************************************************
void
ESC_releaseHW(void)
{
    //
    // Intentionally empty - Implementation is left to the end user
    //
}

//*****************************************************************************
//
// ESC_applicationLayerHandler
//
//*****************************************************************************
__interrupt void
ESC_applicationLayerHandler(void)
{
#ifdef ETHERCAT_STACK
#if AL_EVENT_ENABLED
    //
    // When stack is included and application event enabled, call stack PDI ISR
    //
    PDI_Isr();
#endif  // AL_EVENT_ENABLED
#endif  // ETHERCAT_STACK

    //
    // Acknowledge and clear interrupt in ESCSS
    //
    ESCSS_clearRawInterruptStatus(ESC_SS_BASE, ESCSS_INTR_CLR_IRQ_CLR);

    //
    // Acknowledge this interrupt located in PIE group 1
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

//*****************************************************************************
//
// ESC_applicationSync0Handler
//
//*****************************************************************************
__interrupt void
ESC_applicationSync0Handler(void)
{
#ifdef ETHERCAT_STACK
#if DC_SUPPORTED
    //
    // When stack is included and DC is enabled, call stack Sync0 ISR
    //
    Sync0_Isr();
#endif  // DC_SUPPORTED
#endif  // ETHERCAT_STACK

    //
    // Acknowledge and clear interrupt in ESCSS
    //
    ESCSS_clearRawInterruptStatus(ESC_SS_BASE, ESCSS_INTR_CLR_SYNC0_CLR);

    //
    // Acknowledge this interrupt located in PIE group 1
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

//*****************************************************************************
//
// ESC_applicationSync1Handler
//
//*****************************************************************************
__interrupt void
ESC_applicationSync1Handler(void)
{
#ifdef ETHERCAT_STACK
#if DC_SUPPORTED
    //
    // When stack is included and DC is enabled, call stack Sync1 ISR
    //
    Sync1_Isr();
#endif  // DC_SUPPORTED
#endif  // ETHERCAT_STACK

    //
    // Acknowledge and clear interrupt in ESCSS
    //
    ESCSS_clearRawInterruptStatus(ESC_SS_BASE, ESCSS_INTR_CLR_SYNC1_CLR);

    //
    // Acknowledge this interrupt located in PIE group 5
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP5);
}

//
// End of File
//
