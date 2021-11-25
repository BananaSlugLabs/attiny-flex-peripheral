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

enum {
    Bus_FlagReadOnly        = 0,
    Bus_FlagReadWrite       = 1<<0,
};
typedef uint8_t Bus_Flags;

typedef struct BusBufferTag {
    uint8_t*                data;
    uint8_t                 size;
    Bus_Flags               flags;
#if CONFIG_BUS_SIGNAL
    uintptr_t               task;
#endif
} Bus_EndPoint;
#if 0
#define BUS_REGISTER_FILE(registerfile, pri, regflags)                                      \
    LINKER_DESCRIPTOR_DATA(const Bus_EndPoint, "registerfile", registerfile, pri) =         \
        {                                                                                   \
            .data           = (uint8_t*)&registerfile,                                      \
            .size           = sizeof(registerfile),                                         \
            .flags          = regflags                                                      \
        };                                                                                  \
    LINKER_DESCRIPTOR_ID(const Bus_EndPoint, "registerfile", registerfile, pri);
#endif

#if CONFIG_BUS_SIGNAL
#define _BUS_REGISTER_FILE_SET_TASK(mtask) .task = mtask
#else
#define _BUS_REGISTER_FILE_SET_TASK(mtask)
#endif

#define BUS_REGISTER_FILE(registerfile, pri, regflags)                                      \
    BUS_REGISTER_FILE_TASK(registerfile, pri, regflags, 0)

#define BUS_REGISTER_FILE_TASK(registerfile, pri, regflags, mtask)                          \
    LINKER_DESCRIPTOR_DATA(const Bus_EndPoint, "registerfile", registerfile, pri) =         \
        {                                                                                   \
            .data           = (uint8_t*)&registerfile,                                      \
            .size           = sizeof(registerfile),                                         \
            .flags          = regflags,                                                     \
            _BUS_REGISTER_FILE_SET_TASK(mtask)                                              \
        };                                                                                  \
    LINKER_DESCRIPTOR_ID(const Bus_EndPoint, "registerfile", registerfile, pri);
#ifdef	__cplusplus
}
#endif

#endif	/* BUS_H */

