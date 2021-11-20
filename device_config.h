/* 
 * File:   device_config.h
 * Author: fosterb
 *
 * Created on November 20, 2021, 4:58 AM
 */

#ifndef DEVICE_CONFIG_H
#define	DEVICE_CONFIG_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#define DEF_ENABLE                              1
#define DEF_DISABLE                             0

// *****************************************************************************
// **** System *****************************************************************
    
/**
 * Reserve exclusive stack.
 */
#define CONFIG_RESERVE_STACK                    32
    
// *****************************************************************************
// **** GPIO Configuration *****************************************************
    
/**
 * Maximum LEDs to support.
 */
#define CONFIG_LED_COUNT                        4
#define CONFIG_LED_OPTIMIZE_SIZE                DEF_ENABLE
    
#define CONFIG_LED_R_INTENSITY                  0x20
#define CONFIG_LED_G_INTENSITY                  0x20
#define CONFIG_LED_B_INTENSITY                  0x20
    
// *****************************************************************************
// **** Command Bus (TWI) ******************************************************
    
/**
 * Default I2C device address. When modified at runtime, the value will be stored
 * in the EEPROM.
 */
#define CONFIG_TWI_ADDR_DEFAULT                 82

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
 * Never restart after an abort.
 */
#define DEF_ABORT_FLAGS_HALT                    0x0000
/**
 * After an abort, restart the device automatically.
 */
#define DEF_ABORT_FLAGS_RESTART                 0x8000
/**
 * Flashes the LEDs with the error code. (Red-Blue error codes)
 */
#define DEF_ABORT_FLAGS_LEDCODE_LONG            0x4000
/**
 * Flashes the LEDs (red-black flashing) without an error code. Reduces code space.
 */
#define DEF_ABORT_FLAGS_LEDCODE_SHORT           0x2000
/**
 * Upon entry to Sys_Abort, execute a soft breakpoint.
 */
#define DEF_ABORT_FLAGS_BREAKPOINT              0x1000
#define DEF_ABORT_FLAGS_LEDCODE_COUNT_bm        0x00FF
/**
 * If > 0, cycle the LEDCODE N times and then restart; Otherwise display LEDCODE
 * indefinitely. Supports upt to 255 cycles.
 */
#define DEF_ABORT_FLAGS_LEDCODE_COUNT(n)        ((n)&DEF_ABORT_FLAGS_LEDCODE_COUNT_bm)
    
#define CONFIG_ABORT_FLAGS                      (DEF_ABORT_FLAGS_BREAKPOINT         | \
                                                 DEF_ABORT_FLAGS_LEDCODE_SHORT      | \
                                                 DEF_ABORT_FLAGS_RESTART            | \
                                                 DEF_ABORT_FLAGS_LEDCODE_COUNT(4))

/**
 * Time in msec to hold the LEDCODE display on. Specify 0 for default.
 */
#define CONFIG_ABORT_LEDCODE_TIMER_HI           0
/**
 * Time in msec to hold the LEDCODE display off. Specify 0 for default.
 */
#define CONFIG_ABORT_LEDCODE_TIMER_LO           0

#define _CONFIG_ABORT_GET_LEDCODE_COUNT         ((CONFIG_ABORT_FLAGS)  & DEF_ABORT_FLAGS_LEDCODE_COUNT_bm)
#define _CONFIG_ABORT_GET_LEDCODE_LONG_EN       (((CONFIG_ABORT_FLAGS) & DEF_ABORT_FLAGS_LEDCODE_LONG)      == DEF_ABORT_FLAGS_LEDCODE_LONG)
#define _CONFIG_ABORT_GET_LEDCODE_RESTART       (((CONFIG_ABORT_FLAGS) & DEF_ABORT_FLAGS_RESTART)           == DEF_ABORT_FLAGS_RESTART)
#define _CONFIG_ABORT_GET_LEDCODE_BREAKPOINT    (((CONFIG_ABORT_FLAGS) & DEF_ABORT_FLAGS_BREAKPOINT)        == DEF_ABORT_FLAGS_BREAKPOINT)
#define _CONFIG_ABORT_GET_LEDCODE_SHORT_EN      (((CONFIG_ABORT_FLAGS) & DEF_ABORT_FLAGS_LEDCODE_SHORT)     == DEF_ABORT_FLAGS_LEDCODE_SHORT)

/**
 * When CONFIG_SRAM_INIT is DEF_SRAM_INIT_STRIPE/DEF_SRAM_INIT_ZERO, the SRAM is
 * fully initialized after boot. This is primarily a debugging tool to gather
 * stack utilization information.
 */
#define DEF_SRAM_INIT_NONE                      0
#define DEF_SRAM_INIT_ZERO                      1
#define DEF_SRAM_INIT_STRIPE                    2
    
#define CONFIG_SRAM_INIT                        DEF_SRAM_INIT_STRIPE

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
    
#define CONFIG_TEST_PATTERN                     DEF_TEST_PATTERN_TYPE_UNIFORM_FADE
#define CONFIG_TEST_PATTERN_TIMESTEP            0

/**
 * Generate a fake "abort" signal. Requires CONFIG_TEST_PATTERN.
 */
#define CONFIG_TEST_ABORT                       DEF_ENABLE


#ifdef	__cplusplus
}
#endif

#endif	/* DEVICE_CONFIG_H */

