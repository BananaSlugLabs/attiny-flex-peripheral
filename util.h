/* 
 * File:   util.h
 * Author: fosterb
 *
 * Created on November 25, 2021, 1:26 AM
 */

#ifndef UTIL_H
#define	UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/builtins.h>
#include <avr/interrupt.h>
//#include "mcc_generated_files/mcc.h"
#include <util/atomic.h>
#include <avr/cpufunc.h>
#include "device_config.h"
#include "firmware.h"

#ifdef	__cplusplus
extern "C" {
#endif
    
#define _UTIL_STRY(s)               # s
#define UTIL_STRY(s)                _UTIL_STRY(s)
#define _UTIL_PASTE(a,b)            a ## b
#define UTIL_PASTE(a,b)             _UTIL_PASTE(a,b)

#define DISABLE_INTERRUPTS()    cli()
#define ENABLE_INTERRUPTS()     sei()
#define DEBUG_BREAKPOINT()      asm("BREAK")
#define ATTRIBUTES(x...)        __attribute__ ((x))
#define POSSIBLY_UNUSED         ATTRIBUTES(unused)
/**
 * Read a register even though we do not use the value.
 * This is to support peripherals which has side effects following
 * register reads.
 */
#define FORCE_READ(v)           asm volatile ("" : : "r" (&v))

extern const uint8_t util_bitmask [8];
// NO LONGER USED!
#if 0
#define LINKER_DESCRIPTOR_ID_NAME(name)         name ## _id
#define LINKER_DESCRIPTOR_DATA_NAME(name)       name ## _desc
#define LINKER_DESCRIPTOR_DATA(sec_type, section_name, name, pri)           \
    sec_type LINKER_DESCRIPTOR_DATA_NAME(name)                              \
        ATTRIBUTES(section(section_name ".descriptor" # pri))
#define LINKER_DESCRIPTOR_ID(sec_type, section_name, name, pri)             \
    const uint8_t LINKER_DESCRIPTOR_ID_NAME(name)                           \
        ATTRIBUTES(section(section_name ".id" # pri)) = 0xAA
    /* WORKAROUD: 0xAA is needed because...microchip f***ing sucks.
     * Don't try to make sense of it. The debugger will program the device
     * incorrectly.
     */
#define LINKER_DESCRIPTOR_ID_NOATTR(sec_type, section_name, name, pri)      \
    const uint8_t LINKER_DESCRIPTOR_ID_NAME(name) 
#endif
#ifdef	__cplusplus
}
#endif

#endif	/* UTIL_H */

