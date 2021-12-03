#include "sys.h"
#include "led.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "umap.h"
static void sys_initIO();
static void sys_init();

Signal_Publisher(sys_init_io);
Signal_Publisher(sys_init_early);
Signal_Publisher(sys_init);
Signal_Publisher(sys_start);
Signal_Publisher(sys_finit);
Signal_Publisher(sys_abort);
Signal_Publisher(sys_loop);

static volatile Sys_AbortCode   sys_fault;

ISR(BADISR_vect) {
    sys_abort(SysAbortBadIRQ);
}

#if !defined(CONFIG_ABORT_TIMER_HI) || CONFIG_ABORT_TIMER_HI <= 0
#define _CONFIG_ABORT_TIMER_HI 1000
#else
#define _CONFIG_ABORT_TIMER_HI CONFIG_ABORT_TIMER_HI
#endif
#if !defined(CONFIG_ABORT_TIMER_LO) || CONFIG_ABORT_TIMER_LO <= 0
#define _CONFIG_ABORT_TIMER_LO 500
#else
#define _CONFIG_ABORT_TIMER_LO CONFIG_ABORT_TIMER_LO
#endif

#define _CONFIG_ABORT_COUNT                     ((CONFIG_ABORT_FLAGS)  & DEF_ABORT_FLAGS_COUNT_bm)
#define _CONFIG_ABORT_RESTART                   (((CONFIG_ABORT_FLAGS) & DEF_ABORT_FLAGS_RESTART)           == DEF_ABORT_FLAGS_RESTART)
#define _CONFIG_ABORT_BREAKPOINT                (((CONFIG_ABORT_FLAGS) & DEF_ABORT_FLAGS_BREAKPOINT)        == DEF_ABORT_FLAGS_BREAKPOINT)

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
    
#if _CONFIG_ABORT_COUNT > 0
    for (uint8_t count = 0; count < _CONFIG_ABORT_COUNT; count ++) {
        led_setAll(&led_color_red);
        led_update();
        time_sleep(_CONFIG_ABORT_TIMER_HI);
        led_setAll(&led_color_black);
        led_update();
        time_sleep(_CONFIG_ABORT_TIMER_LO);
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
    event_reset();
    signal_dispatch(sys_init_io);
    signal_dispatch(sys_init_early);
    signal_dispatch(sys_init);
    
    wdt_reset();
    ENABLE_INTERRUPTS();
    signal_dispatch(sys_start);
    while(1) {
        event_process();
        signal_dispatch(sys_loop);
        wdt_reset();
    }
    signal_dispatch(sys_finit);
}

static void sys_initIO();
static void sys_init();

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
#endif
    
#if CONFIG_HAS_PORT_B
    PORTB.DIR = CONFIG_PORT_B_DIR;
    PORTB.OUT = CONFIG_PORT_B_OUT;
#endif

#if CONFIG_HAS_PORT_C
    PORTC.DIR = CONFIG_PORT_C_DIR;
    PORTC.OUT = CONFIG_PORT_C_OUT;
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
    wdt_enable(WDTO_2S);
    
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
}