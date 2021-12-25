/* 
 * File:   attiny402.h
 * Author: fosterb
 *
 * Created on December 5, 2021, 2:31 PM
 */

#ifndef ATTINY402_H
#define	ATTINY402_H

/*
 * Pinout:
 * | Pin | Port | Function    | Usage                                             |
 * | --- | ---- | ----------- | --------------------------------------------------|
 * | 1   |      | VCC         | 5V                                                |
 * | 2   | PA6  | LUT0.OUT    | WS2812 Serial Waveform                            |
 * | 3   | PA7  | AIN7/AINP0  | Analog Keypad                                     |
 * | 4   | PA1  | TWI.SDA     | TWI (I2C) Data Signal                             |
 * | 5   | PA2  | TWI.SCL     | TWI (I2C) Clock Signal                            |
 * | 6   | PA0  | UPDI        | Debug interface.                                  |
 * | 7   | PA3  | USART.XCK   | USART XCK routed to pin to trigger evsys.         |
 * | 8   |      | GND         |                                                   |
 */

#define CONFIG_HAS_PORT_A                       DEF_ENABLE
#define CONFIG_PORT_A_OUT                       0x00
#define CONFIG_PORT_A_DIR                       0x08
#define CONFIG_PORT_A_PIN6                      0x04
#define CONFIG_PORT_A_PIN7                      0x04

#define CONFIG_PINMUX_A                         0x00
#define CONFIG_PINMUX_B                         PORTMUX_USART0_ALTERNATE_gc
#define CONFIG_PINMUX_C                         0x00
#define CONFIG_PINMUX_D                         0x00

// *****************************************************************************
// **** LED Hardware Configuration *********************************************
// *****************************************************************************

#define CONFIG_HW_EVSYS_GENERATOR_ASYNC0        EVSYS_ASYNCCH0_PORTA_PIN3_gc    // XCK
#define CONFIG_HW_EVSYS_GENERATOR_SYNC0         EVSYS_SYNCCH0_TCB0_gc           // TCB Timing Pulses

#define CONFIG_HW_EVSYS_USER_ASYNC0             DEF_HW_EVSYS_CHANNEL_ASYNC0     // XCK -> TCB0
//#define CONFIG_HW_EVSYS_USER_ASYNC1             DEF_HW_EVSYS_CHANNEL_OFF
#define CONFIG_HW_EVSYS_USER_ASYNC2             DEF_HW_EVSYS_CHANNEL_SYNC0      // TCB.WO -> CCL.LUT0_EVENT0
#define CONFIG_HW_EVSYS_USER_ASYNC3             DEF_HW_EVSYS_CHANNEL_SYNC0      // TCB.WO -> CCL.LUT1_EVENT0
//#define CONFIG_HW_EVSYS_USER_ASYNC4             DEF_HW_EVSYS_CHANNEL_OFF
//#define CONFIG_HW_EVSYS_USER_ASYNC5             DEF_HW_EVSYS_CHANNEL_OFF
//#define CONFIG_HW_EVSYS_USER_ASYNC6             DEF_HW_EVSYS_CHANNEL_OFF
//#define CONFIG_HW_EVSYS_USER_ASYNC7             DEF_HW_EVSYS_CHANNEL_OFF
//#define CONFIG_HW_EVSYS_USER_ASYNC8             DEF_HW_EVSYS_CHANNEL_OFF
//#define CONFIG_HW_EVSYS_USER_ASYNC9             DEF_HW_EVSYS_CHANNEL_OFF
//#define CONFIG_HW_EVSYS_USER_ASYNC10            DEF_HW_EVSYS_CHANNEL_OFF
//#define CONFIG_HW_EVSYS_USER_SYNC0              DEF_HW_EVSYS_CHANNEL_OFF
//#define CONFIG_HW_EVSYS_USER_SYNC1              DEF_HW_EVSYS_CHANNEL_OFF

// *****************************************************************************
// **** KeyPad Hardware Configuration ******************************************
// *****************************************************************************

#define CONFIG_KP_ADC_PIN                       7

// *****************************************************************************
// ********* Used for testing & validation (see docs/test-validation.md)
//#define TEST_GROUP                              4
//#define TEST_SEQ                                1
//#define CONFIG_SLEEP_TIMEOUT                    4000
#if defined(TEST_GROUP) && defined(TEST_SEQ)
#if TEST_GROUP == 1
#define CONFIG_SLEEP                            DEF_DISABLE
#define CONFIG_STANDBY_SLOWCLOCK                DEF_DISABLE
#elif TEST_GROUP == 2
#define CONFIG_SLEEP                            DEF_SLEEP_IDLE
#define CONFIG_STANDBY_SLOWCLOCK                DEF_DISABLE
#elif TEST_GROUP == 3
#define CONFIG_SLEEP                            DEF_SLEEP_STANDBY 
#define CONFIG_STANDBY_SLOWCLOCK                DEF_DISABLE
#elif TEST_GROUP == 4
#define CONFIG_SLEEP                            DEF_SLEEP_STANDBY 
#define CONFIG_STANDBY_SLOWCLOCK                DEF_ENABLE
#elif TEST_GROUP == 5
#define CONFIG_SLEEP                            DEF_SLEEP_POWERDOWN 
#define CONFIG_STANDBY_SLOWCLOCK                DEF_DISABLE
#elif TEST_GROUP == 6
#define CONFIG_SLEEP                            DEF_SLEEP_POWERDOWN 
#define CONFIG_STANDBY_SLOWCLOCK                DEF_ENABLE
#else
#error "Invalid test group."
#endif
#if TEST_SEQ == 1
// Uses the same defaults for the first 3 tests.
#elif TEST_SEQ == 2
#define CONFIG_KP_ENABLE                        DEF_DISABLE
#elif TEST_SEQ == 3
#define CONFIG_LED_ENABLE                       DEF_DISABLE
#define CONFIG_KP_ENABLE                        DEF_DISABLE
#define CONFIG_BUS_ENABLE                       DEF_DISABLE
#else
#error "Bad test case selected"
#endif
#endif



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

#endif	/* ATTINY402_H */

