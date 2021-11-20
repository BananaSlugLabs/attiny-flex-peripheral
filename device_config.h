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
    
#define CONFIG_LED_COUNT                        4
#define CONFIG_LED_R_INTENSITY                  0x20
#define CONFIG_LED_G_INTENSITY                  0x20
#define CONFIG_LED_B_INTENSITY                  0x20
    
#define CONFIG_TWI_ADDR_DEFAULT                 82
    
#define CONFIG_HAS_PORT_A                       1
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
    
#ifdef	__cplusplus
}
#endif

#endif	/* DEVICE_CONFIG_H */

