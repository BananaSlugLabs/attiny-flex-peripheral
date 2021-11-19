#include "common.h"

static const uint16_t       rtc_overflow_inc = 32768;
static volatile uint32_t    rtc_boottime;

void Time_Init() {
    while (RTC.STATUS > 0) {}
    //Compare 
    RTC.CMP = 0x01;

    //Count
    RTC.CNT = 0x00;

    //Period
    RTC.PER = 0xFFFF;

    //CMP disabled; OVF enabled; 
    RTC.INTCTRL = 0x01;

    //Clock selection
    RTC.CLKSEL = 0x00;

    //RUNSTDBY disabled; PRESCALER DIV16; RTCEN enabled; 
    RTC.CTRLA = 0x21;
    
    // Wait for all register to be synchronized
    while (RTC.PITSTATUS > 0) {} 
    //PI disabled; 
    RTC.PITINTCTRL = 0x00;
}

/**
 * Gets the current counter and ensures that the overflow is handled since
 * we are not using the interrupt.
 * 
 * @param timeOutput Current time (without adjustment)
 * @return Current ticks.
 */
static inline Time32 Time_GetTime32 () {
    uint16_t c;
    uint32_t time;
    
    do {
        time = rtc_boottime;
        if (RTC.INTFLAGS & RTC_OVF_bm) {
            time += rtc_overflow_inc;
            rtc_boottime = time;
            RTC.INTFLAGS = RTC_OVF_bm;
        }
        { // TODO: Critical Section
            c = RTC.CNT;
        }
        // Repeat until no overflow is detected. Should run twice worst case.
    } while (RTC.INTFLAGS & RTC_OVF_bm); 
    
    return time + c;
}

static inline Time16 Time_GetRtcTime16 () {
    uint16_t c;
    
    do {
        if (RTC.INTFLAGS & RTC_OVF_bm) {
            rtc_boottime += rtc_overflow_inc;
            RTC.INTFLAGS = RTC_OVF_bm;
        }
        { // TODO: Critical Section
            c = RTC.CNT;
        }
    } while (RTC.INTFLAGS & RTC_OVF_bm);
    
    return c;
}

Time32 Time_Get() {
    return Time_GetTime32();
}

void Time_Task() {
    Time_GetRtcTime16();
}

Time32 Time_Sleep(TimerInterval interval) {
    uint16_t start, now, delta;
    
    start = Time_GetRtcTime16();
    
    for (;;) {
        now = Time_GetRtcTime16();
        delta = now-start;
        if (delta>interval) {
            return delta;
        }
    }
}

