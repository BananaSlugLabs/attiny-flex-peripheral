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


typedef enum {
    Bus_StatusBusy          = (1<<0),
    Bus_StatusBusError      = (1<<1),
            
    Bus_StatusCode_bp       = (4),
    Bus_StatusCode_bm       = (0xF<<Bus_StatusCode_bp),
            
    Bus_StatusSuccess       = (0<<Bus_StatusCode_bp),
    Bus_StatusInProgress    = (1<<Bus_StatusCode_bp),
    Bus_StatusErrorBusy     = (2<<Bus_StatusCode_bp),
    Bus_StatusErrorAccess   = (3<<Bus_StatusCode_bp),
    Bus_StatusErrorCommand  = (4<<Bus_StatusCode_bp),
    Bus_StatusErrorArgument = (5<<Bus_StatusCode_bp),
} bus_status_t;

typedef uint8_t bus_command_t;
typedef bus_status_t (*bus_handler_t) ();

typedef struct Bus_MemMapTag {
    uint8_t*            data;
    uint8_t             length;
} Bus_MemMap;

typedef struct Bus_CommandTag {
    bus_handler_t       handler;
    bus_command_t       mask;
    bus_command_t       match;
} Bus_Command;

typedef struct Bus_CommandContextTag {
    bus_command_t       command;
    bus_command_t       lastCommand;
    void*               lastAddress;
} Bus_CommandContext;

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
    const uint8_t _iomap_ ## regs _Bus_Attrs("id", pri) _MPLABX_WORKAROUD;

#define Bus_DefineCommand(callback, commandMask, commandMatch)                              \
    const Bus_Command _bus_command_ ## callback _Bus_Attrs("commands", 0) = {               \
            .handler        = callback,                                                     \
            .mask           = commandMask,                                                  \
            .match          = commandMatch,                                                 \
    };

#define bus_getActiveCommand() bus_commandContext.command

extern Bus_CommandContext bus_commandContext;
/**
 * For asynchronous completion of commands.
 * 
 * @param status Return Bus_StatusInProgress if command is async.
 */
void bus_commandUpdateStatus (uint8_t status);
void bus_commandUpdateStatusIRQ (uint8_t status);

#ifdef	__cplusplus
}
#endif

#endif	/* BUS_H */


