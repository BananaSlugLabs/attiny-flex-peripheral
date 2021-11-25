#include "sys.h"
#include "led.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>


static void sys_initIO();
static void sys_init();

static volatile Sys_AbortCode   sys_fault;

ISR(BADISR_vect) {
    sys_abort(SysAbortBadIRQ);
}

#if !defined(CONFIG_ABORT_LEDCODE_TIMER_HI) || CONFIG_ABORT_LEDCODE_TIMER_HI <= 0
#define _CONFIG_ABORT_LEDCODE_TIMER_HI 1000
#else
#define _CONFIG_ABORT_LEDCODE_TIMER_HI CONFIG_ABORT_LEDCODE_TIMER_HI
#endif
#if !defined(CONFIG_ABORT_LEDCODE_TIMER_LO) || CONFIG_ABORT_LEDCODE_TIMER_LO <= 0
#define _CONFIG_ABORT_LEDCODE_TIMER_LO 500
#else
#define _CONFIG_ABORT_LEDCODE_TIMER_LO CONFIG_ABORT_LEDCODE_TIMER_LO
#endif

void sys_abort(Sys_AbortCode code) {
    DISABLE_INTERRUPTS();
    
    sys_fault = code;
    
#if _CONFIG_ABORT_GET_LEDCODE_BREAKPOINT
    DEBUG_BREAKPOINT();
#endif
    
#if _CONFIG_ABORT_GET_LEDCODE_LONG_EN || _CONFIG_ABORT_GET_LEDCODE_SHORT_EN
    wdt_reset();
    wdt_disable();
    message_broadcastNow(SystemMessage_Finit, MessageData_Empty);
    message_broadcastNow(SystemMessage_Abort, MessageData_Empty);
    
#if _CONFIG_ABORT_GET_LEDCODE_COUNT > 0
    uint8_t count = 0;
#endif
    for (;;) {
#if _CONFIG_ABORT_GET_LEDCODE_LONG_EN
        led_setAll(&Led_ColorPallet[Led_ColorBlackIndex]);
        led_update();
        time_sleep(_CONFIG_ABORT_LEDCODE_TIMER_LO);
        led_setMasked(0, &Led_ColorPallet[Led_ColorRedIndex], &Led_ColorPallet[Led_ColorBlueIndex], (code<<3)&Led_ColorMaskAll);
        led_setMasked(1, &Led_ColorPallet[Led_ColorRedIndex], &Led_ColorPallet[Led_ColorBlueIndex], (code<<2)&Led_ColorMaskAll);
        led_setMasked(2, &Led_ColorPallet[Led_ColorRedIndex], &Led_ColorPallet[Led_ColorBlueIndex], (code<<1)&Led_ColorMaskAll);
        led_setMasked(3, &Led_ColorPallet[Led_ColorRedIndex], &Led_ColorPallet[Led_ColorBlueIndex], (code<<0)&Led_ColorMaskAll);
        led_update();
        time_sleep(CONFIG_ABORT_LEDCODE_TIMER_HI);
#elif _CONFIG_ABORT_GET_LEDCODE_SHORT_EN
        led_setAll(&Led_ColorPallet[Led_ColorBlackIndex]);
        led_update();
        time_sleep(_CONFIG_ABORT_LEDCODE_TIMER_LO);
        led_setAll(&Led_ColorPallet[Led_ColorRedIndex]);
        led_update();
        time_sleep(_CONFIG_ABORT_LEDCODE_TIMER_HI);
#endif
        
#if _CONFIG_ABORT_GET_LEDCODE_COUNT > 0
        if (count >= _CONFIG_ABORT_GET_LEDCODE_COUNT) {
            break;
        }
        count ++;
#endif
    }
#endif
    
#if _CONFIG_ABORT_GET_LEDCODE_RESTART
    ccp_write_io((void*)&(RSTCTRL.SWRR),RSTCTRL_SWRE_bm);
#endif
    for (;;);
}

void sys_restart() {
    ccp_write_io((void*)&(RSTCTRL.SWRR),RSTCTRL_SWRE_bm);
}

int main () {
    sys_initIO();
    message_broadcastNow(SystemMessage_InitIo,      MessageData_Empty);
    sys_init();
    message_broadcastNow(SystemMessage_EarlyInit,   MessageData_Empty);
    message_broadcastNow(SystemMessage_Init,        MessageData_Empty);
    wdt_reset();
    ENABLE_INTERRUPTS();
    message_broadcastNow(SystemMessage_Start,       MessageData_Empty);
    while(1) {
#if CONFIG_MESSAGE_QUEUE > 0
        message_dequeueAll();
#endif
        message_broadcastNow(SystemMessage_Loop,    MessageData_Empty);
        wdt_reset();
    }
}

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