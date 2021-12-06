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
// **** System & Diagnostics ***************************************************
// *****************************************************************************

// ==== CONFIG_DEVICE ==========================================================
#define DEF_DEVICE_ATTINY402                    402

#ifdef __ATtiny402__
#define CONFIG_DEVICE                           DEF_DEVICE_ATTINY402
#define CONFIG_DEVICE_INCLUDE                   "configs/attiny402.h"
#else
#error "Unsupported device."
#endif

// ==== CONFIG_ABORT_FLAGS =====================================================
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
 * After an abort, restart the device automatically.
 */
#define DEF_ABORT_FLAGS_RESTART                 0x8000

/**
 * If > 0, flash the LEDs RED/Black before restarting.
 */
#define DEF_ABORT_FLAGS_COUNT(n)                ((n)&DEF_ABORT_FLAGS_COUNT_bm)

// ==== CONFIG_ABORT_COLOR1 & CONFIG_ABORT_COLOR2 ==============================
#define DEF_ABORT_COLOR_BLACK                   0x0
#define DEF_ABORT_COLOR_RED                     0x1
#define DEF_ABORT_COLOR_GREEN                   0x2
#define DEF_ABORT_COLOR_BLUE                    0x3
#define DEF_ABORT_COLOR_WHITE                   0x4

// ==== CONFIG_SRAM_INIT =======================================================
/**
 * Do not enable SRAM pattern.
 */
#define DEF_SRAM_INIT_NONE                      0
/**
 * Clear all SRAM.
 */
#define DEF_SRAM_INIT_ZERO                      1
/**
 * Writes a series of 0xFF 0xAA 0x55 0xA5 which is used to understand the real
 * world stack usage.
 */
#define DEF_SRAM_INIT_STRIPE                    2

// *****************************************************************************
// **** Test Application *******************************************************
// *****************************************************************************

// ==== CONFIG_TEST_PATTERN ====================================================
/**
 * Test Pattern: Fade a single LED in and out with fixed neighbours.
 */
#define DEF_TEST_PATTERN_TYPE_SINGLE_FADE       1
/**
 * Test Pattern: Fade a all LEDs uniformly.
 */
#define DEF_TEST_PATTERN_TYPE_UNIFORM_FADE      2
    
// *****************************************************************************
// **** Load Customizations ****************************************************
// *****************************************************************************

#include CONFIG_DEVICE_INCLUDE

// *****************************************************************************
// **** Defaults: System & Diagnostics *****************************************
// *****************************************************************************

// ==== CONFIG_DEVICE ==========================================================
// ... Check device is supported.
#ifndef CONFIG_DEVICE
#error "Unknown device."
#else
#if CONFIG_DEVICE != DEF_DEVICE_ATTINY402
#error "Device is not supported."
#endif
#endif

// ==== CONFIG_CLOCK ===========================================================
// ... Default to 20MHz
#ifdef F_CPU
#undef F_CPU
#endif
#ifndef CONFIG_CLOCK
#define CONFIG_CLOCK                            20000000
#endif
#define F_CPU                                   CONFIG_CLOCK

// ==== CONFIG_ABORT_FLAGS =====================================================
// ... Repeat a sequence of 
#ifndef CONFIG_ABORT_FLAGS
#define CONFIG_ABORT_FLAGS                      (DEF_ABORT_FLAGS_BREAKPOINT | \
                                                 DEF_ABORT_FLAGS_RESTART    | \
                                                 DEF_ABORT_FLAGS_COUNT(4))
#endif



// ==== CONFIG_RTC_SIMPLIFIED_DRIVER ===========================================
/**
 * Minimize the timer driver since most of the functionality is not used.
 */
#ifndef CONFIG_RTC_SIMPLIFIED_DRIVER
#define CONFIG_RTC_SIMPLIFIED_DRIVER            DEF_ENABLE
#endif

// ==== CONFIG_ABORT_TIMER_HI & CONFIG_ABORT_TIMER_LO ==========================
/**
 * Time in msec to flash the FAULT led on. Specify 0 for default.
 */
#ifndef CONFIG_ABORT_TIMER_HI
#define CONFIG_ABORT_TIMER_HI                   500
#endif

/**
 * Time in msec to flash the FAULT led off. Specify 0 for default.
 */
#ifndef CONFIG_ABORT_TIMER_LO
#define CONFIG_ABORT_TIMER_LO                   500
#endif

// ==== CONFIG_ABORT_COLOR1 & CONFIG_ABORT_COLOR2 ==============================
#ifndef CONFIG_ABORT_COLOR1
#define CONFIG_ABORT_COLOR1                     DEF_ABORT_COLOR_RED
#endif
#ifndef CONFIG_ABORT_COLOR2
#define CONFIG_ABORT_COLOR2                     DEF_ABORT_COLOR_WHITE
#endif

// ==== CONFIG_ABORT_COLOR1 & CONFIG_ABORT_COLOR2 ==============================
/**
 * When CONFIG_SRAM_INIT is DEF_SRAM_INIT_STRIPE/DEF_SRAM_INIT_ZERO, the SRAM is
 * fully initialized after boot. This is primarily a debugging tool to gather
 * stack utilization information.
 */
#ifndef CONFIG_SRAM_INIT
#define CONFIG_SRAM_INIT                        DEF_SRAM_INIT_NONE
#endif

// *****************************************************************************
// **** LED Configuration ******************************************************
// *****************************************************************************


// ==== CONFIG_DEVICE ==========================================================
#ifndef CONFIG_LED_ENABLE
#define CONFIG_LED_ENABLE                       DEF_ENABLE
#endif

// ==== CONFIG_DEVICE ==========================================================
#ifndef CONFIG_LED_COUNT
#define CONFIG_LED_COUNT                        48
#endif

// ==== CONFIG_DEVICE ==========================================================
#ifndef CONFIG_LED_R_INTENSITY
#define CONFIG_LED_R_INTENSITY                  0x20
#endif

// ==== CONFIG_DEVICE ==========================================================
#ifndef CONFIG_LED_G_INTENSITY
#define CONFIG_LED_G_INTENSITY                  0x20
#endif

// ==== CONFIG_DEVICE ==========================================================
#ifndef CONFIG_LED_B_INTENSITY
#define CONFIG_LED_B_INTENSITY                  0x20
#endif

// ==== CONFIG_LED_IRQ_PERF, CONFIG_LED_IRQ_PORT, CONFIG_LED_IRQ_PIN ===========
#ifndef CONFIG_LED_IRQ_PERF
// Ensure CONFIG_TEST_PATTERN is set to something
#define CONFIG_LED_IRQ_PERF                     DEF_DISABLE
#endif

#if CONFIG_LED_IRQ_PERF && !defined(CONFIG_LED_IRQ_PORT)
#error "Must define CONFIG_LED_IRQ_PORT to use CONFIG_LED_IRQ_PERF."
#endif

#if CONFIG_LED_IRQ_PERF && !defined(CONFIG_LED_IRQ_PIN)
#error "Must define CONFIG_LED_IRQ_PORT to use CONFIG_LED_IRQ_PIN."
#endif

// *****************************************************************************
// **** Command Bus (TWI) ******************************************************
// *****************************************************************************

// ==== CONFIG_DEVICE ==========================================================
#ifndef CONFIG_BUS_DEFAULT_ADDRESS
#define CONFIG_BUS_DEFAULT_ADDRESS              0x52
#endif

// ==== CONFIG_DEVICE ==========================================================
#ifndef CONFIG_BUS_ENABLE
#define CONFIG_BUS_ENABLE                       DEF_ENABLE // disable if CONFIG_LED_IRQ_PERF
#endif

// *****************************************************************************
// **** GPIO Configuration *****************************************************
// *****************************************************************************

#ifndef CONFIG_HAS_PORT_A
#define CONFIG_HAS_PORT_A DEF_DISABLE
#endif
#ifndef CONFIG_HAS_PORT_B
#define CONFIG_HAS_PORT_B DEF_DISABLE
#endif
#ifndef CONFIG_HAS_PORT_C
#define CONFIG_HAS_PORT_C DEF_DISABLE
#endif

#if CONFIG_HAS_PORT_A && !(defined(CONFIG_HAS_PORT_A) || defined(CONFIG_PORT_A_OUT) || defined(CONFIG_PORT_A_DIR))
#error "PORTA is enabled, but not defined. Need to set CONFIG_PORT_A_OUT, CONFIG_PORT_A_DIR."
#endif
#if CONFIG_HAS_PORT_B && !(defined(CONFIG_HAS_PORT_B) || defined(CONFIG_PORT_B_OUT) || defined(CONFIG_PORT_B_DIR))
#error "PORTB is enabled, but not defined. Need to set CONFIG_PORT_B_OUT, CONFIG_PORT_B_DIR."
#endif
#if CONFIG_HAS_PORT_C && !(defined(CONFIG_HAS_PORT_B) || defined(CONFIG_PORT_C_OUT) || defined(CONFIG_PORT_C_DIR))
#error "PORTC is enabled, but not defined. Need to set CONFIG_PORT_C_OUT, CONFIG_PORT_C_DIR."
#endif

#ifndef CONFIG_PINMUX_A
#define CONFIG_PINMUX_A                         0x00
#endif
#ifndef CONFIG_PINMUX_B
#define CONFIG_PINMUX_B                         0x00
#endif
#ifndef CONFIG_PINMUX_C
#define CONFIG_PINMUX_C                         0x00
#endif
#ifndef CONFIG_PINMUX_D
#define CONFIG_PINMUX_D                         0x00
#endif

// *****************************************************************************
// **** Test Patterns & Simulated Faults ***************************************
// *****************************************************************************
    
// ==== CONFIG_TEST_PATTERN ====================================================
#ifndef CONFIG_TEST_PATTERN
#define CONFIG_TEST_PATTERN                     DEF_DISABLE
#endif

// ==== CONFIG_TEST_PATTERN_TIMESTEP ===========================================
#ifndef CONFIG_TEST_PATTERN_TIMESTEP
#define CONFIG_TEST_PATTERN_TIMESTEP            25
#endif

// ==== CONFIG_TEST_ABORT ======================================================
/**
 * Generate a fake "abort" signal. Requires CONFIG_TEST_PATTERN. The value is
 * number of times to cycle through a test pattern before aborting. Disable
 * with DEF_DISABLE.
 */
#ifndef CONFIG_TEST_ABORT
#define CONFIG_TEST_ABORT                       DEF_DISABLE
#endif

// *****************************************************************************
// **** Firmware Information ***************************************************
// *****************************************************************************

// ==== CONFIG_FW_MFG, CONFIG_FW_IDENT, CONFIG_FW_VERSION ======================
#ifndef CONFIG_FW_MFG
#define CONFIG_FW_MFG                           0xBAAA
#endif
#ifndef CONFIG_FW_IDENT
#define CONFIG_FW_IDENT                         0x2812
#endif
#ifndef CONFIG_FW_VERSION
#define CONFIG_FW_VERSION                       0x01
#endif

#endif	/* DEVICE_CONFIG_H */

