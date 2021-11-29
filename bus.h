/* 
 * File:   bus.h
 * Author: fosterb
 *
 * Created on November 24, 2021, 10:43 PM
 */

#ifndef BUS_H
#define	BUS_H

#include "common.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct Bus_MemMapTag {
    uint8_t*            data;
    uint8_t             length;
} Bus_MemMap;

#define BUS_PRIORITY_000 000
#define BUS_PRIORITY_001 001
#define BUS_PRIORITY_002 002
#define BUS_PRIORITY_003 003
#define BUS_PRIORITY_004 004
#define BUS_PRIORITY_005 005
#define BUS_PRIORITY_006 006
#define BUS_PRIORITY_007 007
#define BUS_PRIORITY_008 008
#define BUS_PRIORITY_009 009
#define BUS_PRIORITY_010 010
#define BUS_PRIORITY_011 011
#define BUS_PRIORITY_012 012
#define BUS_PRIORITY_013 013
#define BUS_PRIORITY_014 014
#define BUS_PRIORITY_015 015

#define _Bus_Attrs(n, pri) ATTRIBUTES(section( "iomem." n # pri ))

#define Bus_DefineMemoryMap(regs, pri)                                                      \
    const Bus_MemMap _iomem_ ## regs _Bus_Attrs("data", pri) = {                            \
            .data           = (uint8_t*)&regs,                                              \
            .length         = sizeof(regs),                                                 \
    };                                                                                      \
    const uint8_t _iomap_ ## regs _Bus_Attrs("id", pri) = 0xAA; // See comment below for why we set to 0xAA

/*
 * For some reason if this is not initialized, the MPLAB debugger will place NOPs at ADDR 0.
 * The actual build itself is fine... just whatever the debugger is doing is wrong. It does not
 * seem to respect the DISCARD section.
 */

#ifdef	__cplusplus
}
#endif

#endif	/* BUS_H */

