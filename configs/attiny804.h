/* 
 * File:   attiny804.h
 * Author: fosterb
 *
 * Created on December 23, 2021, 7:17 PM
 */

#ifndef ATTINY804_H
#define	ATTINY804_H

/*
 * Pinout 1:
 * | Pin | Port | Function    | Usage                                                 |
 * | --- | ---- | ----------- | ----------------------------------------------------- |
 * | 1   |      | VCC         | 5V                                                    |
 * | 2   | PA4  | LUT0.OUT    | WS2812 Serial Waveform                                |
 * | 3   | PA5  | AIN5        | Analog Keypad                                         |
 * | 4   | PA6  |             | Unused.                                               |
 * | 5   | PA7  |             | Unused.                                               |
 * | 6   | PB3  |             | Unused.                                               |
 * | 7   | PB2  |             | Unused.                                               |
 * | 8   | PB1  | TWI.SDA     | TWI (I2C) Data Signal                                 |
 * | 9   | PB2  | TWI.SCL     | TWI (I2C) Clock Signal                                |
 * | 10  | PA0  | UPDI        | Debug interface.                                      |
 * | 11  | PA1  | USART.TxD   | Raw LED bitstream.                                    |
 * | 12  | PA2  | USART.RxD   | Unused.                                               |
 * | 13  | PA3  | USART.XCK   | Raw LED bitstream clock.                              |
 * | 14  |      | GND         |                                                        |
 */

#define CONFIG_HAS_PORT_A                       DEF_ENABLE
#define CONFIG_PORT_A_OUT                       0x00
#define CONFIG_PORT_A_DIR                       ((1<<4)|(1<<3)|(1<<1)|(1<<2))
#define CONFIG_PORT_A_PIN5                      0x04

#define CONFIG_HAS_PORT_B                       DEF_ENABLE
#define CONFIG_PORT_B_OUT                       0
#define CONFIG_PORT_B_DIR                       0x00

#define CONFIG_HAS_PORT_C                       DEF_DISABLE

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
#define CONFIG_KP_ADC_PIN                       5

// When CONFIG_LED_IRQ_PERF is set use this port for toggling...
#if 0
#define CONFIG_LED_IRQ_PERF                     DEF_ENABLE
#define CONFIG_LED_IRQ_PORT                     VPORTA
#define CONFIG_LED_IRQ_PIN                      2               // Reuses SCL pin
#define CONFIG_BUS_ENABLE                       DEF_DISABLE     // Can't use I2C with this configuration

#if defined(CONFIG_LED_IRQ_PERF) && CONFIG_LED_IRQ_PERF
// Forces test mode when performance mode is selected.
#define CONFIG_TEST_PATTERN                     DEF_TEST_PATTERN_TYPE_SINGLE_FADE
//#define CONFIG_TEST_ABORT                       8
#endif
#endif

//#define CONFIG_TEST_PATTERN                     DEF_TEST_PATTERN_TYPE_UNIFORM_FADE

#endif	/* ATTINY804_H */

