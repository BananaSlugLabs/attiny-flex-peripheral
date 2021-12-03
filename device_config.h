/* 
 * File:   device_config.h
 * Author: fosterb
 *
 * Created on November 20, 2021, 4:58 AM
 */

#ifndef DEVICE_CONFIG_H
#define	DEVICE_CONFIG_H
    
#define DEF_ENABLE                              1
#define DEF_DISABLE                             0

// *****************************************************************************
// **** System *****************************************************************

#define CONFIG_CLOCK                            20000000
#define F_CPU                                   CONFIG_CLOCK

/**
 * Number of messages to queue. The count of messages is (1<<value). I.e. power of 2.
 */
#define CONFIG_MESSAGE_QUEUE                    0

#define CONFIG_BREAK_ON_START                   DEF_DISABLE
// *****************************************************************************
// **** GPIO Configuration *****************************************************
    
/**
 * Maximum LEDs to support.
 */
#define CONFIG_LED_COUNT                        48 // Note: Should ensure that there is about 32 bytes free
    
#define CONFIG_LED_R_INTENSITY                  0x20
#define CONFIG_LED_G_INTENSITY                  0x20
#define CONFIG_LED_B_INTENSITY                  0x20
   
#define CONFIG_LED_IRQ_PERF                     DEF_DISABLE // Ensure CONFIG_TEST_PATTERN is set to something (otherwise this test will do nothing)
#define CONFIG_LED_IRQ_PORT                     VPORTA
#define CONFIG_LED_IRQ_PIN                      2               // Reuses SCL pin
    
// *****************************************************************************
// **** Command Bus (TWI) ******************************************************

#define CONFIG_BUS_DEFAULT_ADDRESS              0x52
#define CONFIG_BUS_ENABLE                       DEF_ENABLE // disable if CONFIG_LED_IRQ_PERF

// *****************************************************************************
// **** GPIO Configuration *****************************************************
#define CONFIG_HAS_PORT_A                       DEF_ENABLE
#define CONFIG_PORT_A_OUT                       0x00
#define CONFIG_PORT_A_DIR                       0xC8
    
#define CONFIG_HAS_PORT_B                       0
#define CONFIG_PORT_B_OUT                       0x00
#define CONFIG_PORT_B_DIR                       0x00
    
#define CONFIG_HAS_PORT_C                       0
#define CONFIG_PORT_C_OUT                       0x00
#define CONFIG_PORT_C_DIR                       0x00

#define CONFIG_PINMUX_A                         0x00
#define CONFIG_PINMUX_B                         PORTMUX_USART0_ALTERNATE_gc
#define CONFIG_PINMUX_C                         0x00
#define CONFIG_PINMUX_D                         0x00

// *****************************************************************************
// **** Diagnostic Options *****************************************************

/**
 * Never restart after an abort, halt (i.e. infinite loop).
 */
#define DEF_ABORT_FLAGS_HALT                    0x0000

/**
 * After an abort, restart the device automatically.
 */
#define DEF_ABORT_FLAGS_RESTART                 0x8000

/**
 * Upon entry to Sys_Abort, execute a soft breakpoint.
 */
#define DEF_ABORT_FLAGS_BREAKPOINT              0x1000

#define DEF_ABORT_FLAGS_COUNT_bm                0x00FF

/**
 * If > 0, flash the LEDs RED/Black before restarting.
 */
#define DEF_ABORT_FLAGS_COUNT(n)                ((n)&DEF_ABORT_FLAGS_COUNT_bm)

#define CONFIG_ABORT_FLAGS                      (DEF_ABORT_FLAGS_BREAKPOINT         | \
                                                 DEF_ABORT_FLAGS_RESTART            | \
                                                 DEF_ABORT_FLAGS_COUNT(4))

/**
 * Time in msec to flash the FAULT led on. Specify 0 for default.
 */
#define CONFIG_ABORT_TIMER_HI                   0
 
/**
 * Time in msec to flash the FAULT led off. Specify 0 for default.
 */
#define CONFIG_ABORT_TIMER_LO                   0

/**
 * When CONFIG_SRAM_INIT is DEF_SRAM_INIT_STRIPE/DEF_SRAM_INIT_ZERO, the SRAM is
 * fully initialized after boot. This is primarily a debugging tool to gather
 * stack utilization information.
 */
#define DEF_SRAM_INIT_NONE                      0
#define DEF_SRAM_INIT_ZERO                      1
#define DEF_SRAM_INIT_STRIPE                    2

#define CONFIG_SRAM_INIT                        DEF_SRAM_INIT_NONE

// *****************************************************************************
// **** Test Patterns & Simulated Faults ***************************************
    
/**
 * Test Pattern: Fade a single LED in and out with fixed neighbours.
 */
#define DEF_TEST_PATTERN_TYPE_SINGLE_FADE       1
/**
 * Test Pattern: Fade a all LEDs uniformly.
 */
#define DEF_TEST_PATTERN_TYPE_UNIFORM_FADE      2
    
#define CONFIG_TEST_PATTERN                     DEF_DISABLE
#define CONFIG_TEST_PATTERN_TIMESTEP            0

/**
 * Generate a fake "abort" signal. Requires CONFIG_TEST_PATTERN.
 */
#define CONFIG_TEST_ABORT                       DEF_ENABLE

#if CONFIG_LED_IRQ_PERF && CONFIG_BUS_ENABLE
#error "Cannot enable TWI bus and IRQ performance monitor."
#endif
#if CONFIG_LED_IRQ_PERF && (!defined(CONFIG_LED_IRQ_PORT) || !defined(CONFIG_LED_IRQ_PIN))
#error "Missing IRQ Port/Pin configuration for performance measurements."
#endif

#endif	/* DEVICE_CONFIG_H */

