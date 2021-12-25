/*
 * File:   sys.h
 * Author: fosterb
 *
 * Created on November 24, 2021, 10:49 PM
 */

#ifndef SYS_H
#define	SYS_H

#include "common.h"
#include "signal.h"

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
#if CONFIG_SLEEP >= DEF_SLEEP_STANDBY
#define SysStandbyEnter_Subscribe(s,p)  Signal_Subscriber(sys_standby_enter,    s, p)
#define SysStandbyExit_Subscribe(s,p)   Signal_Subscriber(sys_standby_exit,     s, p)
#else
#define SysStandbyEnter_Subscribe(s,p)
#define SysStandbyExit_Subscribe(s,p)
#endif

typedef enum Sys_SignalTag {
    Sys_SignalWakeLock              = 1<<0,
    Sys_SignalWorkerPending         = 1<<1,
    Sys_SignalEnterStandby          = 1<<7,
} Sys_Signal;

typedef enum Sys_AbortCodeTag {
    SysAbortNone,
    SysAbortBadIRQ                  = 0B0001,
    SysAbortAssertion               = 0B0010,
    SysAbortMessageQueueExhausted   = 0B0110,
    SysAbortBadEvent                = 0B0101,
} Sys_AbortCode;

void sys_abort(Sys_AbortCode code) __attribute__((noreturn));
void sys_restart() __attribute__((noreturn));
bool sys_isFaultMode();

static inline void sys_signal(Sys_Signal sigs) {
    extern volatile Sys_Signal sys_signalValue;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        sys_signalValue |= sigs;
    }
}

static inline void sys_signalIRQ(Sys_Signal sigs) {
    extern volatile Sys_Signal sys_signalValue;
    sys_signalValue |= sigs;
}

#if !CONFIG_RTC_SIMPLIFIED_DRIVER
time32_t time_get();
#endif
time16_t time_sleep(timer_interval_t interval);
time16_t time_get16();

#ifdef	__cplusplus
}
#endif

#endif	/* SYS_H */

