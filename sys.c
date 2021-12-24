#include "sys.h"
#include "led.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "signal.h"

static void sys_initIO();
static void sys_init();

#if CONFIG_SLEEP >= DEF_SLEEP_STANDBY
#if CONFIG_STANDBY_SLOWCLOCK
static inline void sys_slowClock();
static inline void sys_fastClock();
#endif
static inline void time_resetStabdbyTimer();
static inline void time_enableStabdbyTimer();
static inline void time_disableStabdbyTimer();
#endif

Signal_Publisher(sys_init_io);
Signal_Publisher(sys_init_early);
Signal_Publisher(sys_init);
Signal_Publisher(sys_start);
Signal_Publisher(sys_finit);
Signal_Publisher(sys_abort);
Signal_Publisher(sys_loop);
Signal_Publisher(sys_standby_enter);
Signal_Publisher(sys_standby_exit);

static volatile Sys_AbortCode   sys_fault;
volatile Sys_Signal             sys_signalValue;

ISR(BADISR_vect) {
    sys_abort(SysAbortBadIRQ);
}

#define _CONFIG_ABORT_COUNT                     ((CONFIG_ABORT_FLAGS)  & DEF_ABORT_FLAGS_COUNT_bm)
#define _CONFIG_ABORT_RESTART                   (((CONFIG_ABORT_FLAGS) & DEF_ABORT_FLAGS_RESTART)           == DEF_ABORT_FLAGS_RESTART)
#define _CONFIG_ABORT_BREAKPOINT                (((CONFIG_ABORT_FLAGS) & DEF_ABORT_FLAGS_BREAKPOINT)        == DEF_ABORT_FLAGS_BREAKPOINT)

#ifndef CONFIG_ABORT_COLOR1
#define CONFIG_ABORT_COLOR1                     DEF_ABORT_COLOR_RED
#endif
#ifndef CONFIG_ABORT_COLOR2
#define CONFIG_ABORT_COLOR2                     DEF_ABORT_COLOR_BLACK
#endif

#if CONFIG_ABORT_COLOR1 == DEF_ABORT_COLOR_BLACK
#define _CONFIG_ABORT_COLOR1 led_color_black
#elif CONFIG_ABORT_COLOR1 == DEF_ABORT_COLOR_RED
#define _CONFIG_ABORT_COLOR1 led_color_red
#elif CONFIG_ABORT_COLOR1 == DEF_ABORT_COLOR_GREEN
#define _CONFIG_ABORT_COLOR1 led_color_green
#elif CONFIG_ABORT_COLOR1 == DEF_ABORT_COLOR_BLUE
#define _CONFIG_ABORT_COLOR1 led_color_blue
#elif CONFIG_ABORT_COLOR1 == DEF_ABORT_COLOR_WHITE
#define _CONFIG_ABORT_COLOR1 led_color_white
#else
#error "Invalid CONFIG_ABORT_COLOR1 selected."
#endif

#if CONFIG_ABORT_COLOR2 == DEF_ABORT_COLOR_BLACK
#define _CONFIG_ABORT_COLOR2 led_color_black
#elif CONFIG_ABORT_COLOR2 == DEF_ABORT_COLOR_RED
#define _CONFIG_ABORT_COLOR2 led_color_red
#elif CONFIG_ABORT_COLOR2 == DEF_ABORT_COLOR_GREEN
#define _CONFIG_ABORT_COLOR2 led_color_green
#elif CONFIG_ABORT_COLOR2 == DEF_ABORT_COLOR_BLUE
#define _CONFIG_ABORT_COLOR2 led_color_blue
#elif CONFIG_ABORT_COLOR2 == DEF_ABORT_COLOR_WHITE
#define _CONFIG_ABORT_COLOR2 led_color_white
#else
#error "Invalid CONFIG_ABORT_COLOR1 selected."
#endif

void sys_abort(Sys_AbortCode code) {
    DISABLE_INTERRUPTS();
    
    sys_fault = code;
    
#if _CONFIG_ABORT_BREAKPOINT
    DEBUG_BREAKPOINT();
#endif
    
    wdt_reset();
    wdt_disable();
    
    signal_dispatch(sys_finit);
    signal_dispatch(sys_abort);
    
#if _CONFIG_ABORT_COUNT > 0 && CONFIG_LED_ENABLE
    for (uint8_t count = 0; count < _CONFIG_ABORT_COUNT; count ++) {
        led_setAll(&_CONFIG_ABORT_COLOR1);
        led_update();
        time_sleep(CONFIG_ABORT_TIMER_HI);
        led_setAll(&_CONFIG_ABORT_COLOR2);
        led_update();
        time_sleep(CONFIG_ABORT_TIMER_LO);
    }
#endif
    
#if _CONFIG_ABORT_RESTART
    sys_restart();
#else
    for (;;) {}
#endif
}

void sys_restart() {
    for (;;) {
        ccp_write_io((void*)&(RSTCTRL.SWRR),RSTCTRL_SWRE_bm);
    }
}

int main () {
    DEBUG_BREAKPOINT();
    RSTCTRL.RSTFR = 0xFF;
    signal_dispatch(sys_init_io);
    signal_dispatch(sys_init_early);
    signal_dispatch(sys_init);
    
    //wdt_reset();
    sys_signalValue |= Sys_SignalWakeLock | Sys_SignalWorkerPending; // always do at least one loop at startup
    ENABLE_INTERRUPTS();
    signal_dispatch(sys_start);
    while(1) {
        wdt_reset();
        
        Sys_Signal sigs;
        
        DISABLE_INTERRUPTS();
        
        sigs = sys_signalValue;
        sys_signalValue = 0;
#if CONFIG_SLEEP
        
#if CONFIG_SLEEP >= DEF_SLEEP_STANDBY
        if (sigs == Sys_SignalEnterStandby) {
            signal_dispatch(sys_standby_enter);
            time_disableStabdbyTimer();
#if CONFIG_SLEEP == DEF_SLEEP_STANDBY
            set_sleep_mode(SLPCTRL_SMODE_STDBY_gc);
#elif CONFIG_SLEEP == DEF_SLEEP_POWERDOWN
            set_sleep_mode(SLPCTRL_SMODE_PDOWN_gc);
#endif
#if CONFIG_STANDBY_SLOWCLOCK
            sys_slowClock();
#endif
            ENABLE_INTERRUPTS();
#if CONFIG_SLEEP != DEF_SLEEP_PRETEND // Enter standby unless we're faking it...
            sleep_mode();
#endif
            DISABLE_INTERRUPTS();
#if CONFIG_STANDBY_SLOWCLOCK
            sys_fastClock();
#endif
            time_enableStabdbyTimer();
            signal_dispatch(sys_standby_exit);
            ENABLE_INTERRUPTS();
            continue;
        }
#endif
        if (!sigs) {
            set_sleep_mode(SLPCTRL_SMODE_IDLE_gc);
            ENABLE_INTERRUPTS();
            sleep_mode();
            continue;
        }
#endif
        ENABLE_INTERRUPTS();
        
#if CONFIG_SLEEP >= DEF_SLEEP_STANDBY
        if (sigs & Sys_SignalWakeLock) {
            time_resetStabdbyTimer();
        }
#endif
        
        if (sigs & Sys_SignalWorkerPending) {
            signal_dispatch(sys_loop);
        }
    }
    signal_dispatch(sys_finit);
}

SysInitIO_Subscribe(sys_initIO, Signal_Highest);
SysInitEarly_Subscribe(sys_init, Signal_Highest);

static void sys_initIO() {
#if CONFIG_HAS_PORT_A
    for (uint8_t i = 0; i < 8; i++) {
        *((uint8_t *)&PORTA + 0x10 + i) |= 1 << PORT_PULLUPEN_bp;
    }
#endif
#if CONFIG_HAS_PORT_B
    for (uint8_t i = 0; i < 8; i++) {
        *((uint8_t *)&PORTB + 0x10 + i) |= 1 << PORT_PULLUPEN_bp;
    }
#endif
#if CONFIG_HAS_PORT_C
    for (uint8_t i = 0; i < 8; i++) {
        *((uint8_t *)&PORTC + 0x10 + i) |= 1 << PORT_PULLUPEN_bp;
    }
#endif
    
#if CONFIG_HAS_PORT_A
    PORTA.DIR = CONFIG_PORT_A_DIR;
    PORTA.OUT = CONFIG_PORT_A_OUT;
    
#if defined(CONFIG_PORT_A_PIN0)
    PORTA.PIN0CTRL = CONFIG_PORT_A_PIN0;
#endif
#if defined(CONFIG_PORT_A_PIN1)
    PORTA.PIN1CTRL = CONFIG_PORT_A_PIN1;
#endif
#if defined(CONFIG_PORT_A_PIN2)
    PORTA.PIN2CTRL = CONFIG_PORT_A_PIN2;
#endif
#if defined(CONFIG_PORT_A_PIN3)
    PORTA.PIN3CTRL = CONFIG_PORT_A_PIN3;
#endif    
#if defined(CONFIG_PORT_A_PIN4)
    PORTA.PIN4CTRL = CONFIG_PORT_A_PIN4;
#endif
#if defined(CONFIG_PORT_A_PIN5)
    PORTA.PIN5CTRL = CONFIG_PORT_A_PIN5;
#endif    
#if defined(CONFIG_PORT_A_PIN6)
    PORTA.PIN6CTRL = CONFIG_PORT_A_PIN6;
#endif
#if defined(CONFIG_PORT_A_PIN7)
    PORTA.PIN7CTRL = CONFIG_PORT_A_PIN7;
#endif
#endif
    
#if CONFIG_HAS_PORT_B
    PORTB.DIR = CONFIG_PORT_B_DIR;
    PORTB.OUT = CONFIG_PORT_B_OUT;
#if defined(CONFIG_PORT_B_PIN0)
    PORTB.PIN0CTRL = CONFIG_PORT_A_PIN0;
#endif
#if defined(CONFIG_PORT_B_PIN1)
    PORTB.PIN1CTRL = CONFIG_PORT_B_PIN1;
#endif
#if defined(CONFIG_PORT_B_PIN2)
    PORTB.PIN2CTRL = CONFIG_PORT_B_PIN2;
#endif
#if defined(CONFIG_PORT_B_PIN3)
    PORTB.PIN3CTRL = CONFIG_PORT_B_PIN3;
#endif    
#if defined(CONFIG_PORT_B_PIN4)
    PORTB.PIN4CTRL = CONFIG_PORT_B_PIN4;
#endif
#if defined(CONFIG_PORT_B_PIN5)
    PORTB.PIN5CTRL = CONFIG_PORT_B_PIN5;
#endif    
#if defined(CONFIG_PORT_B_PIN6)
    PORTB.PIN6CTRL = CONFIG_PORT_B_PIN6;
#endif
#if defined(CONFIG_PORT_B_PIN7)
    PORTB.PIN7CTRL = CONFIG_PORT_B_PIN7;
#endif
#endif

#if CONFIG_HAS_PORT_C
    PORTC.DIR = CONFIG_PORT_C_DIR;
    PORTC.OUT = CONFIG_PORT_C_OUT;
    
#if defined(CONFIG_PORT_C_PIN0)
    PORTC.PIN0CTRL = CONFIG_PORT_C_PIN0;
#endif
#if defined(CONFIG_PORT_C_PIN1)
    PORTC.PIN1CTRL = CONFIG_PORT_C_PIN1;
#endif
#if defined(CONFIG_PORT_C_PIN2)
    PORTC.PIN2CTRL = CONFIG_PORT_C_PIN2;
#endif
#if defined(CONFIG_PORT_C_PIN3)
    PORTC.PIN3CTRL = CONFIG_PORT_C_PIN3;
#endif    
#if defined(CONFIG_PORT_C_PIN4)
    PORTC.PIN4CTRL = CONFIG_PORT_C_PIN4;
#endif
#if defined(CONFIG_PORT_C_PIN5)
    PORTC.PIN5CTRL = CONFIG_PORT_C_PIN5;
#endif    
#if defined(CONFIG_PORT_C_PIN6)
    PORTC.PIN6CTRL = CONFIG_PORT_C_PIN6;
#endif
#if defined(CONFIG_PORT_C_PIN7)
    PORTC.PIN7CTRL = CONFIG_PORT_C_PIN7;
#endif
#endif
    
    /* PORTMUX Initialization */
#if defined(CONFIG_PINMUX_A)
    PORTMUX.CTRLA = CONFIG_PINMUX_A;
#endif
#if defined(CONFIG_PINMUX_B)
    PORTMUX.CTRLB = CONFIG_PINMUX_B;
#endif
#if defined(CONFIG_PINMUX_C)
    PORTMUX.CTRLC = CONFIG_PINMUX_C;
#endif
#if defined(CONFIG_PINMUX_D)
    PORTMUX.CTRLD = CONFIG_PINMUX_D;
#endif
}

static void sys_init() {
    // *************************************************************************
    // ******** Watchdog Timer *************************************************
    //wdt_enable(WDTO_2S);
    
    // *************************************************************************
    // ******** Brown Out Detector *********************************************
#if 0
    ccp_write_io((void*)&(BOD.CTRLA),0x00);

    //VLMCFG BELOW; VLMIE disabled; 
	BOD.INTCTRL = 0x00;

    //VLMLVL 5ABOVE; 
	BOD.VLMCTRLA = 0x00;
#endif
    
    // *************************************************************************
    // ******** Sleep Controller ***********************************************
    
    ccp_write_io((void*)&(SLPCTRL.CTRLA),0x00);
    
    // *************************************************************************
    // ******** Clock Init *****************************************************
    
    //RUNSTDBY disabled; 
    ccp_write_io((void*)&(CLKCTRL.OSC32KCTRLA),0x00);

    //RUNSTDBY disabled; 
    ccp_write_io((void*)&(CLKCTRL.OSC20MCTRLA),0x00);

    //PDIV 2X; PEN disabled; 
    ccp_write_io((void*)&(CLKCTRL.MCLKCTRLB),0x00);

    //CLKOUT disabled; CLKSEL OSC20M; 
    ccp_write_io((void*)&(CLKCTRL.MCLKCTRLA),0x00);

    //LOCKEN disabled; 
    ccp_write_io((void*)&(CLKCTRL.MCLKLOCK),0x00);
    
    // *************************************************************************
    // ******** Interrupt Init *************************************************
    
    //IVSEL disabled; CVT disabled; LVL0RR disabled; 
    ccp_write_io((void*)&(CPUINT.CTRLA),0x00);
    
    //LVL0PRI 0; 
    CPUINT.LVL0PRI = 0x00;
    
    //LVL1VEC 23; 
    CPUINT.LVL1VEC = 0x17;
    
    // *************************************************************************
    // ******** Time Init ******************************************************
    
    while (RTC.STATUS > 0) {}
    
#if CONFIG_SLEEP >= DEF_SLEEP_STANDBY
    time_enableStabdbyTimer();
#else
    RTC.CMP = 0x01;
    RTC.INTCTRL = 0x00;
#endif

    //Count
    RTC.CNT = 0x00;

    //Period
    RTC.PER = 0xFFFF;


    //Clock selection
    RTC.CLKSEL = 0x00;

    //RUNSTDBY disabled; PRESCALER DIV16; RTCEN enabled; 
    RTC.CTRLA = RTC_PRESCALER_DIV16_gc | RTC_RTCEN_bm;//0x21;/**/

    // Wait for all register to be synchronized
    while (RTC.PITSTATUS > 0) {} 

    //PI disabled; 
    RTC.PITINTCTRL = 0;
}




time16_t time_sleep(timer_interval_t interval) {
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

time16_t time_get16() {
    uint16_t time;
    
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        time =  RTC.CNT;
    }
    
    return time;
}

#if CONFIG_STANDBY_SLOWCLOCK && CONFIG_SLEEP >= DEF_SLEEP_STANDBY
static inline void sys_slowClock() {
    // Reduce CLK to 2MHz
    ccp_write_io((void*)&(CLKCTRL.MCLKCTRLB),CLKCTRL_PDIV_24X_gc | CLKCTRL_PEN_bm);
}
static inline void sys_fastClock() {
    ccp_write_io((void*)&(CLKCTRL.MCLKCTRLB),0x00);
}
#endif

#if CONFIG_SLEEP >= DEF_SLEEP_STANDBY
ISR(RTC_CNT_vect) {
    DEBUG_BREAKPOINT();
    RTC.INTFLAGS = RTC_CMP_bm;
    sys_signalIRQ(Sys_SignalEnterStandby);
}

static inline void time_resetStabdbyTimer() {
    // if it's busy, don't wait
    if (RTC.STATUS & RTC_CMPBUSY_bm) {
        return;
    }
    RTC.CMP = RTC.CNT + CONFIG_SLEEP_TIMEOUT;
}

static inline void time_enableStabdbyTimer() {
    while (RTC.STATUS & RTC_CMPBUSY_bm) {}
    RTC.CMP = RTC.CNT + CONFIG_SLEEP_TIMEOUT;
    RTC.INTFLAGS = RTC_CMP_bm;
    RTC.INTCTRL = RTC_CMP_bm;
}

static inline void time_disableStabdbyTimer() {
    RTC.INTCTRL = 0x00;
}
#endif