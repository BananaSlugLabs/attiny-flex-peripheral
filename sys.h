/* 
 * File:   sys.h
 * Author: fosterb
 *
 * Created on November 24, 2021, 10:49 PM
 */

#ifndef SYS_H
#define	SYS_H

#include "common.h"
#include "umap.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define SysInitIO_Subscribe(s,p)        Signal_Subscriber(sys_init_io,          s, p)
#define SysInitEarly_Subscribe(s,p)     Signal_Subscriber(sys_init_early,       s, p)
#define SysInit_Subscribe(s,p)          Signal_Subscriber(sys_init,             s, p)
#define SysStart_Subscribe(s,p)         Signal_Subscriber(sys_start,            s, p)
#define SysFinit_Subscribe(s,p)         Signal_Subscriber(sys_finit,            s, p)
#define SysAbort_Subscribe(s,p)         Signal_Subscriber(sys_abort,            s, p)
#define SysLoop_Subscribe(s,p)          Signal_Subscriber(sys_loop,             s, p)
    
typedef enum SysAbortCodeTag {
    SysAbortNone,
    SysAbortBadIRQ                  = 0B0001,
    SysAbortAssertion               = 0B0010,
    SysAbortMessageQueueExhausted   = 0B0110,
    SysAbortBadEvent                = 0B0101,
} Sys_AbortCode;

void sys_abort(Sys_AbortCode code) __attribute__((noreturn));
void sys_restart() __attribute__((noreturn));
bool sys_isFaultMode();
time32_t time_get();
time32_t time_sleep(timer_interval_t interval);

// **** Core System Functions **********************************************

// **** Timer Functions ****************************************************
#if 0
timer_handle_t Timer_Create(timer_interval_t interval, bool repeat);
void Timer_Delete(timer_handle_t handle);
bool Timer_Asserted(timer_handle_t handle);
bool Timer_Wait(timer_handle_t handle);
bool Timer_SetInterval(timer_handle_t handle, timer_interval_t interval, bool repeat);
/**
 * Updates the interval for the current or next period. This should be used on
 * active timers.
 * 
 * @param handle
 * @param interval
 * @param immediate
 * @return 
 */
bool Timer_UpdateInterval(timer_handle_t handle, timer_interval_t interval, bool immediate);
void Timer_Stop(timer_handle_t handle);
bool Timer_Start(timer_handle_t handle);
bool Timer_IsActive(timer_handle_t handle);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* SYS_H */

