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

#define _UTIL_SectionAttrs(section_name, additional...) \
    ATTRIBUTES(section( section_name ), additional)

#define _UTIL_NakedSectionAttrs(section_name, additional...) \
    _UTIL_SectionAttrs(section_name, naked, noinline, used, additional)

#define Priority_Highest            100
#define Priority_High               300
#define Priority_Normal             500
#define Priority_Low                700
#define Priority_Lowest             900

#define Priority_nHighest           900
#define Priority_nHigh              700
#define Priority_nNormal            500
#define Priority_nLow               300
#define Priority_nLowest            100

/**
 * Read a register even though we do not use the value.
 * This is to support peripherals which has side effects following
 * register reads.
 */
#define FORCE_READ(v)           asm volatile ("" : : "r" (&v))

/* FOR DISCARD SECTIONS in linker script...
 * 
 * MPLab X's debugger has a software defect where it treats uninitialized values
 * in discard sections of the linker script to be set to 0s when loading
 * firmware. Thus will begin to clear the memory at 0x8000 (0x0). It does not
 * impact the hex files as this is handled with objcopy. 
 * 
 * For some reason setting a random value causes the debugger to properly handle
 * the discard section.
 */
#define _MPLABX_WORKAROUD = 0xAA 

// Not used currently...
extern const uint8_t util_bitmask [8];

typedef uint8_t     timer_handle_t;
typedef uint16_t    timer_interval_t; // in msec
typedef uint16_t    time16_t; // in msec
typedef uint32_t    time32_t; // in msec

#ifdef	__cplusplus
}
#endif

#endif	/* UTIL_H */

