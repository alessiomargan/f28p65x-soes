
#include "ethercat_subdevice_cpu1_hal.h"
#include "irq_handler.h"
#include "globals.h"

extern void FatalError_Handler(uint16_t errorCode);

static inline uint16_t Config_ESCSS(void) {

    uint16_t eepromTimeOut = 0x1000U;
    uint16_t memoryTimeOut = 0x300U;

    // Sys PLL = 200MHz and use /2 to get 100MHz for ECAT IP
    // (There is a built in /4 to get 25MHz for PHY when using
    //  internal clocking for PHY)
    SysCtl_setECatClk(SYSCTL_ECATCLKOUT_DIV_2, SYSCTL_SOURCE_SYSPLL,
                      ESC_DISABLE_INT_PHY_CLK);

    // Configure EEPROM Size for 16K bits or less
    ESCSS_configureEEPROMSize(ESC_SS_CONFIG_BASE, ESCSS_LESS_THAN_16K);

    // Reset ESC
    ESC_resetESC();

    // Initialize ESCSS Memory
    ESCSS_initMemory(ESC_SS_BASE);

    // Wait for ESCSS memory initialization to complete
    if(ESCSS_getMemoryInitDoneStatusBlocking(ESC_SS_BASE, memoryTimeOut) !=
       ESCSS_API_SUCCESS) {
        return(ESC_HW_INIT_FAIL);
    }

    // Wait (with time out) for EEPROM to be loaded
    while(ESC_loadedCheckEEPROM() != ESC_EEPROM_SUCCESS) {
        eepromTimeOut--;
        // On time out, return fail
        if(eepromTimeOut == 0U) {
            return(ESC_HW_INIT_FAIL);
        }
    }

    // Initialize AL Event Mask to zero to prevent ESC interrupts until
    // set during stack state transition to SAFEOP
    ESC_writeDWord(0UL, ESC_O_AL_EVENTMASK);

    // Enable ECAT/PDI, Sync0, and Sync1 interrupts in ESCSS
    ESCSS_setMaskedInterruptStatus(ESC_SS_BASE,
                                   (ESCSS_INTR_MASK_IRQ_MASK |
                                    ESCSS_INTR_MASK_SYNC0_MASK |
                                    ESCSS_INTR_MASK_SYNC1_MASK));

    // Connect ESCSS Sync0/1 signals to respective interrupts in PIE
    ESCSS_configureSync0Connections(ESC_SS_BASE, ESCSS_SYNC0_CONFIG_C28X_PIE_EN,
                                    ESCSS_VALID_KEY_VALUE);
    ESCSS_configureSync1Connections(ESC_SS_BASE, ESCSS_SYNC1_CONFIG_C28X_PIE_EN,
                                    ESCSS_VALID_KEY_VALUE);


    return(ESC_HW_INIT_SUCCESS);
}

static inline void Config_ESCSS_GPIOs(void) {

    // Configure GPIOs for ECAT

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
    GPIO_setQualificationMode(148,GPIO_QUAL_ASYNC);

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
}

void EtherCAT_init(void) {

    Config_ESCSS_GPIOs();
    if ( Config_ESCSS() != ESC_HW_INIT_SUCCESS) {
        PRINTLN("FAIL Configure_ESCSS");
        FatalError_Handler(FATAL_ERROR_ESCSS_HW_INIT);
    }

    // Register EtherCAT Interrupt Handlers
    Interrupt_register(INT_ECAT, &ESC_applicationLayerHandler);
    //Interrupt_register(INT_ECATSYNC0, &ESC_applicationSync0Handler);
    //Interrupt_register(INT_ECATSYNC1, &ESC_applicationSync1Handler);
    Interrupt_register(INT_ECATSYNC0, &Sync0_Isr);
    Interrupt_register(INT_ECATSYNC1, &Sync1_Isr);
    
    // Enable ECAT/PDI, Sync0, and Sync1 interrupts in CPU
    Interrupt_enable(INT_ECAT);
    Interrupt_enable(INT_ECATSYNC0);
    Interrupt_enable(INT_ECATSYNC1);

}
