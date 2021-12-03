/* 
 * File:   bus_internal.h
 * Author: fosterb
 *
 * Created on November 27, 2021, 3:55 PM
 */

#ifndef BUS_INTERNAL_H
#define	BUS_INTERNAL_H

#include "bus.h"

#ifdef	__cplusplus
extern "C" {
#endif
typedef enum {
    Bus_Idle                = 0,
    Bus_AddressOrRead       = 1,
    Bus_ReadOnly            = 2,
    Bus_ReadWrite0          = 3,
    Bus_ReadWrite           = 4,
    Bus_IoFinish            = 5,
    Bus_Error               = 6,
} bus_state_t;

typedef enum {
    Bus_StatusBusy          = (1<<0),
            
    Bus_StatusCode_bp       = (4),
    Bus_StatusCode_bm       = (0xF<<Bus_StatusCode_bp),
            
    Bus_StatusSuccess       = (0<<Bus_StatusCode_bp),
    Bus_StatusInProgress    = (1<<Bus_StatusCode_bp),
    Bus_StatusErrorBusy     = (2<<Bus_StatusCode_bp),
    Bus_StatusErrorAccess   = (3<<Bus_StatusCode_bp),
    Bus_StatusErrorCommand  = (4<<Bus_StatusCode_bp),
    Bus_StatusErrorArgument = (5<<Bus_StatusCode_bp),
} bus_status_t;

typedef enum {
    Bus_CommandSetPage      = 1,
    Bus_CommandSetDevAddr   = 2,
    Bus_CommandReset        = 3,
    Bus_CommandEventFirst   = 8,
    Bus_CommandEventLast    = 15,
} bus_command_t;

typedef struct Bus_IOControlTag {
    bus_status_t        status;
    bus_command_t       txCommand;
    bus_command_t       command;
    uint8_t             params[4];
} Bus_IOControl;

typedef struct Device_InfoTag {
    uint16_t            mfg;
    uint16_t            ident;
    uint8_t             version;
} Bus_IODeviceInfo;

typedef struct Bus_IONvmConfigTag {
    uint8_t             deviceAddress;
} Bus_IONvmConfig;

typedef struct Bus_BuildConfigTag {
    Bus_MemMap          map [4];
} Bus_BuildConfig;

typedef struct Bus_StateTag {
    bus_state_t         state;
    uint8_t             page;
    uint8_t             activePage;
    uint8_t             offset;
    bus_command_t       command;
} Bus_State;

#ifdef	__cplusplus
}
#endif

#endif	/* BUS_INTERNAL_H */

