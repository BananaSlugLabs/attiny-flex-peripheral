/* 
 * File:   bus.h
 * Author: fosterb
 *
 * Created on November 24, 2021, 10:43 PM
 */

#ifndef BUS_H
#define	BUS_H

#include "common.h"
#include "umap.h"

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

#if 0
typedef enum device_protocol_t {
    Device_Protocol_V1             = 0xAF01,
} device_protocol_t;

typedef uint16_t        device_id_t;
enum {
    Device_ClassLED     = 0x01,
    Device_ClassKeyPad  = 0x02,
    Device_ClassGPIO    = 0x03,
    Device_ClassAnalog  = 0x04,
};
typedef uint8_t         device_class_t;

enum {
    Device_Magic_V1     = 0xA0,
};
typedef uint8_t         device_magic_t;

typedef uint8_t         bus_command_t;
typedef uint16_t        bus_ref_t;
typedef uint8_t         bus_size_t;
typedef uint8_t         device_fieldtype_t;

enum {
    Device_Version_bp           = 4,
    Device_Version_bm           = 0xF << Device_Version_bp,
    
    Device_Version1             = 1 << Device_Version_bp,
    
    Device_MappedCount_bp       = 4,
    Device_MappedCount_bm       = 0xF << Device_MappedCount_bp,
    
    Device_InterruptIndex_bp    = 8,
    Device_InterruptIndex_bm    = 0xF << Device_InterruptIndex_bp,
};

typedef uint16_t device_header_t;
typedef struct Device_SectionTag {
    device_header_t     header;
    device_id_t         id;
    device_class_t      class;
    bus_ref_t           mapped[];
} Device_Section;
#endif

/*
 * For some reason if this is not initialized, the MPLAB debugger will place NOPs at ADDR 0.
 * The actual build itself is fine... just whatever the debugger is doing is wrong. It does not
 * seem to respect the DISCARD section.
 */


#ifdef	__cplusplus
}
#endif

#endif	/* BUS_H */

