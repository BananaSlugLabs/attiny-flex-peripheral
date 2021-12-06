#include "sys.h"

// Note: This code hasn't been fully tested as it hasn't been used extensively in
// the codebase. The most of this could could likely be removed.

static void time_init();

#if !CONFIG_RTC_SIMPLIFIED_DRIVER
static void time_loop();
static inline time32_t time_getTime32 ();
static inline time16_t time_getRtcTime16 ();

static const uint16_t rtc_overflow_inc = 32768;
static volatile uint32_t rtc_boottime;

SysLoop_Subscribe(time_loop,        Signal_Normal);
#endif

SysInit_Subscribe(time_init,        Signal_Normal);
static void time_init () {
    while (RTC.STATUS > 0) {}
    //Compare 
    RTC.CMP = 0x01;

    //Count
    RTC.CNT = 0x00;

    //Period
    RTC.PER = 0xFFFF;

    //CMP disabled; OVF enabled; 
    RTC.INTCTRL = 0x00;

    //Clock selection
    RTC.CLKSEL = 0x00;

    //RUNSTDBY disabled; PRESCALER DIV16; RTCEN enabled; 
    RTC.CTRLA = RTC_PRESCALER_DIV16_gc | RTC_RTCEN_bm;//0x21;/**/

    // Wait for all register to be synchronized
    while (RTC.PITSTATUS > 0) {} 

    //PI disabled; 
    RTC.PITINTCTRL = 0x00;
}
#if !CONFIG_RTC_SIMPLIFIED_DRIVER

static void time_loop() {
    time_getRtcTime16();
}

/**
 * Gets the current counter and ensures that the overflow is handled since
 * we are not using the interrupt.
 * 
 * @param timeOutput Current time (without adjustment)
 * @return Current ticks.
 */
static inline time32_t time_getTime32 () {
    uint16_t c;
    uint32_t time;
    
    do {
        time = rtc_boottime;
        if (RTC.INTFLAGS & RTC_OVF_bm) {
            time += rtc_overflow_inc;
            rtc_boottime = time;
            RTC.INTFLAGS = RTC_OVF_bm;
        }
        {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                c = RTC.CNT;
            }
        }
        // Repeat until no overflow is detected. Should run twice worst case.
    } while (RTC.INTFLAGS & RTC_OVF_bm); 
    
    return time + c;
}

static inline time16_t time_getRtcTime16 () {
    uint16_t c;
    
    do {
        if (RTC.INTFLAGS & RTC_OVF_bm) {
            rtc_boottime += rtc_overflow_inc;
            RTC.INTFLAGS = RTC_OVF_bm;
        }
        {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                c = RTC.CNT;
            }
        }
    } while (RTC.INTFLAGS & RTC_OVF_bm);
    
    return c;
}

time32_t time_get() {
    return time_getTime32();
}

time32_t time_sleep(timer_interval_t interval) {
    uint16_t start, now, delta;
    
    start = time_getRtcTime16();
    
    for (;;) {
        now = time_getRtcTime16();
        delta = now-start;
        if (delta>interval) {
            return delta;
        }
    }
}
#else

time32_t time_sleep(timer_interval_t interval) {
    uint16_t start, now, delta;
    
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        start =  RTC.CNT;
    }
    
    for (;;) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            now =  RTC.CNT;
        }
        delta = now-start;
        if (delta>interval) {
            return delta;
        }
    }
}

#endif
