/* 
 * File:   attiny402.h
 * Author: fosterb
 *
 * Created on December 5, 2021, 2:31 PM
 */

#ifndef ATTINY402_H
#define	ATTINY402_H

#ifdef	__cplusplus
extern "C" {
#endif

#define CONFIG_DEVICE                           DEF_DEVICE_ATTINY402
    
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

// When CONFIG_LED_IRQ_PERF is set use this port for toggling...
#if 0
#define CONFIG_LED_IRQ_PERF                     DEF_ENABLE
#define CONFIG_LED_IRQ_PORT                     VPORTA
#define CONFIG_LED_IRQ_PIN                      2               // Reuses SCL pin
#define CONFIG_BUS_ENABLE                       DEF_DISABLE     // Can't use I2C with this configuration
#endif

#if defined(CONFIG_LED_IRQ_PERF) && CONFIG_LED_IRQ_PERF
// Forces test mode when performance mode is selected.
#define CONFIG_TEST_PATTERN                     DEF_TEST_PATTERN_TYPE_SINGLE_FADE
//#define CONFIG_TEST_ABORT                       8
#endif
    
#ifdef	__cplusplus
}
#endif

#endif	/* ATTINY402_H */

