/* 
 * File:   util.h
 * Author: fosterb
 *
 * Created on November 25, 2021, 1:26 AM
 */

#ifndef UTIL_H
#define	UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

#define DISABLE_INTERRUPTS()    cli()
#define ENABLE_INTERRUPTS()     sei()
#define DEBUG_BREAKPOINT()      asm("BREAK")
#define ATTRIBUTES(x...)        __attribute__ ((x))

#define LINKER_DESCRIPTOR_ID_NAME(name)         name ## _id
#define LINKER_DESCRIPTOR_DATA_NAME(name)       name ## _desc
#define LINKER_DESCRIPTOR_DATA(sec_type, section_name, name, pri)           \
    sec_type LINKER_DESCRIPTOR_DATA_NAME(name)                        \
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

#ifdef	__cplusplus
}
#endif

#endif	/* UTIL_H */

