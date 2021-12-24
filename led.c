#include "led.h"
#include "bus.h"
#include "sys.h"
#include "led_internal.h"
#include <avr/cpufunc.h>
#if CONFIG_LED_ENABLE
static void led_init();
static void led_finit();
static bus_status_t led_update_cmd();

Bus_DefineCommand(led_update_cmd, 0xFF, 0x08);

SysInit_Subscribe(led_init,         Signal_Normal);
SysFinit_Subscribe(led_finit,       Signal_Normal);
SysAbort_Subscribe(led_init,        Signal_Normal);

typedef struct {
    uint8_t     state;
    uint8_t     index;
    uint8_t     total;
} Led_State;

Led_RegisterFile led_registerfile;
Bus_DefineMemoryMap(led_registerfile, BUS_PRIORITY_002);

const Led_Color led_color_black = {0,0,0};
const Led_Color led_color_white = {CONFIG_LED_R_INTENSITY,CONFIG_LED_G_INTENSITY,CONFIG_LED_B_INTENSITY};
const Led_Color led_color_red   = {.r = CONFIG_LED_R_INTENSITY};
const Led_Color led_color_green = {.g = CONFIG_LED_G_INTENSITY};
const Led_Color led_color_blue  = {.b = CONFIG_LED_B_INTENSITY};

// Reduces instruction count... also faster.
#define Led (*((Led_State*) &_SFR_MEM8(0x001C)))
//static Led_State Led;

// While less efficient than the run of the mill polled implementation,
// when interrupts are disabled, we can jump in to the middle of the IRQ handler
// and use the interrupt controller to see if we need to RET.
//
// It's highly questionable especially since it's entirely possible it may break
// after a change.
void SoftISR_Led_TXC ();
void SoftISR_Led_DRE ();

ISR(USART0_TXC_vect) {
    asm volatile(
        ".global SoftISR_Led_TXC \n"
        "SoftISR_Led_TXC:"
    );
#if CONFIG_LED_IRQ_PERF
    CONFIG_LED_IRQ_PORT.OUT |= (1<<CONFIG_LED_IRQ_PIN);
#endif
    switch (Led.state) {
        case LED_STATE_RESET_DONE:
            Led.index       = 0;
            USART0.STATUS   = USART_TXCIF_bm;
            CCL.LUT0CTRLA   |= CCL_ENABLE_bm;
            TCB0.CTRLA      |= TCB_ENABLE_bm;
            Led.state       = LED_STATE_STREAM_PIXEL;
            USART0.CTRLA    = USART_DREIE_bm; // Switch to DRE IRQ
            // Note: Annoyingly, the high priority DRE interrupt is fired immediately due to high priority
            // Need to see if there is a workaround to prevent this from happening -- could pre-fill USART0.TXDATAL buffers?
            break;
            
        case LED_STATE_STREAM_PIXEL_DONE:
            Led.index       = 0;
            CCL.LUT0CTRLA   &= ~CCL_ENABLE_bm;
            TCB0.CTRLA      &= ~TCB_ENABLE_bm;
            USART0.STATUS   = USART_TXCIF_bm;
            USART0.CTRLA    = 0;
            USART0.CTRLB    &= ~USART_TXEN_bm;
            Led.state       = LED_STATE_IDLE;
            led_registerfile.ctrla &= ~Led_CtrlA_Busy;
            break;
            
        default:
            DEBUG_BREAKPOINT();
            break;
    }
#if CONFIG_LED_IRQ_PERF
    CONFIG_LED_IRQ_PORT.OUT &= ~(1<<CONFIG_LED_IRQ_PIN);
#endif
    if (!(CPUINT.STATUS&0x3)) {
        asm volatile("ret");
    }
}

// Note: Based on testing, DRE takes approx 1uSec to execute.
ISR(USART0_DRE_vect) {
    asm volatile(
        ".global SoftISR_Led_DRE \n"
        "SoftISR_Led_DRE:"
    );
#if CONFIG_LED_IRQ_PERF
    CONFIG_LED_IRQ_PORT.OUT |= (1<<CONFIG_LED_IRQ_PIN);
#endif
    switch (Led.state) {
        case LED_STATE_STREAM_PIXEL:
            USART0.TXDATAL = ((uint8_t*)led_registerfile.data.colors)[Led.index];
            Led.index ++;
            if (Led.index == Led.total) {
                USART0.CTRLA    = USART_TXCIE_bm; // switch to TXC IRQ
                Led.state       = LED_STATE_STREAM_PIXEL_DONE;
            }
            sys_signal(Sys_SignalWakeLock);
            break;
            
        case LED_STATE_RESET:
            USART0.TXDATAL = 0;
            Led.index ++;   
            if (Led.index == LED_RESET_LENGTH) {
                USART0.CTRLA    = USART_TXCIE_bm;  // switch to TXC IRQ
                Led.state       = LED_STATE_RESET_DONE;
            }
            break;
            
        default:
            DEBUG_BREAKPOINT();
            break;
    }
#if CONFIG_LED_IRQ_PERF
    CONFIG_LED_IRQ_PORT.OUT &= ~(1<<CONFIG_LED_IRQ_PIN);
#endif
    if (!(CPUINT.STATUS&0x3)) {
        asm volatile("ret");
    }
}

static void led_init() {
    // *************************************************************************
    // **** Event System Configuration *****************************************
	EVSYS.ASYNCCH0      = 0x0D;
	EVSYS.ASYNCCH1      = 0x02;
	EVSYS.ASYNCUSER0    = 0x03;
	EVSYS.ASYNCUSER1    = 0x00;
	EVSYS.ASYNCUSER2    = 0x01;
	EVSYS.ASYNCUSER3    = 0x01;
	EVSYS.ASYNCUSER4    = 0x03;
	EVSYS.ASYNCUSER5    = 0x03;
	EVSYS.ASYNCUSER6    = 0x00;
	EVSYS.ASYNCUSER7    = 0x00;
	EVSYS.ASYNCUSER8    = 0x00;
	EVSYS.ASYNCUSER9    = 0x00;
	EVSYS.ASYNCUSER10   = 0x00;
	EVSYS.SYNCCH0       = 0x01;
	EVSYS.SYNCCH1       = 0x00;
	EVSYS.SYNCUSER0     = 0x00;
    
    // *************************************************************************
    // **** CCL Configuration **************************************************

	CCL.SEQCTRL0        = CCL_SEQSEL_RS_gc;
	CCL.TRUTH1          = 0x70;
	CCL.TRUTH0          = 0x08;
	CCL.LUT0CTRLC       = CCL_INSEL2_EVENT0_gc;
	CCL.LUT0CTRLB       = CCL_INSEL1_TCB0_gc | CCL_INSEL0_USART0_gc;
	CCL.LUT1CTRLC       = CCL_INSEL2_EVENT0_gc;
	CCL.LUT1CTRLB       = CCL_INSEL0_USART0_gc | CCL_INSEL1_USART0_gc;
	CCL.LUT0CTRLA       = CCL_OUTEN_bm | CCL_ENABLE_bm;
	CCL.LUT1CTRLA       = CCL_ENABLE_bm;
	CCL.CTRLA           = CCL_RUNSTDBY_bm | CCL_ENABLE_bm;
    
    // *************************************************************************
    // **** TCB0 Configuration *************************************************
    
    TCB0.CCMP           = 0x03;
    TCB0.CNT            = 0x00;
    TCB0.CTRLB          = 0x16;
    TCB0.DBGCTRL        = 0x00;
    TCB0.EVCTRL         = 0x11;
    TCB0.INTCTRL        = 0x00;
    TCB0.INTFLAGS       = 0x00;
    TCB0.TEMP           = 0x00;
    TCB0.CTRLA          = 0x00;

    // *************************************************************************
    // **** UART Configuration *************************************************
    
    //set baud rate register
    USART0.CTRLB        = 0;
    USART0.BAUD         = (uint16_t)LED_BAUD(500000);
    USART0.CTRLA        = 0x00;
    USART0.CTRLB        = USART_TXEN_bm;
    USART0.CTRLC        = USART_CMODE_MSPI_gc;
    USART0.DBGCTRL      = 0x00;
    USART0.EVCTRL       = 0x00;
    USART0.RXPLCTRL     = 0x00;
    USART0.TXPLCTRL     = 0x00;
    
    // *************************************************************************
    // **** Misc Initialization ************************************************
    
    Led.state           = LED_STATE_IDLE;
    Led.index           = 0;
    Led.total           = 0;
    
    led_setAll(&led_color_black);
    
    led_registerfile.data.count = CONFIG_LED_COUNT;
    led_registerfile.ctrla      = 0;
    led_registerfile.ctrlb      = 0;
    
#if CONFIG_LED_IRQ_PERF
    CONFIG_LED_IRQ_PORT.DIR |= (1<<CONFIG_LED_IRQ_PIN);
    CONFIG_LED_IRQ_PORT.OUT &= ~(1<<CONFIG_LED_IRQ_PIN);
#endif
}

static void led_finit() {
    CCL.LUT0CTRLA   = 0;
    TCB0.CTRLA      = 0;
    USART0.CTRLB    = 0;
}

void led_setAll(const Led_Color* color){
    for (int i = 0; i < CONFIG_LED_COUNT; i ++){
        led_registerfile.data.colors[i] = *color;
    }
}

void led_set(uint8_t index, const Led_Color* color) {
    led_registerfile.data.colors[index] = *color;
}

bool led_isBusy() {
    return Led.state > LED_STATE_IDLE;
}

void led_update() {
    if (led_isBusy()) {
        return;
    }
    
    if ( led_registerfile.data.count == 0 ) {
        // nothing to do...
        led_registerfile.ctrla = 0;
        return;
    }
    
    sys_signal(Sys_SignalWakeLock);
    
    if ( led_registerfile.data.count > CONFIG_LED_COUNT) {
        led_registerfile.data.count = CONFIG_LED_COUNT;
    }
    
    // Report busy & setup registers
    led_registerfile.ctrla  = Led_CtrlA_Busy;
    Led.total               = led_registerfile.data.count * sizeof(Led_Color);
    Led.state               = LED_STATE_RESET;
    Led.index               = 0;
    
    // Disable CCL Output, Timer, and clear USART Transmit Done Flag
    CCL.LUT0CTRLA           &= ~CCL_ENABLE_bm;
    TCB0.CTRLA              &= ~TCB_ENABLE_bm;
    USART0.STATUS           = USART_TXCIF_bm;
    USART0.CTRLB            |= USART_TXEN_bm;
    
    if (SREG & CPU_I_bm) {
        USART0.CTRLA = USART_DREIE_bm;
    } else {
        // Simulate Interrupts through polling...
        USART0.CTRLA = USART_DREIE_bm;
        while (USART0.CTRLA & (USART_TXCIE_bm | USART_DREIE_bm)) {
            if ((USART0.CTRLA & USART_DREIE_bm) && (USART0.STATUS & USART_DREIF_bm)) {
                SoftISR_Led_DRE();
            } else if ((USART0.CTRLA & USART_TXCIE_bm) && (USART0.STATUS & USART_TXCIF_bm)) {
                SoftISR_Led_TXC();
            }
        }
    }
}

static bus_status_t led_update_cmd() {
    led_update();
    return Bus_StatusSuccess; // could improve by only setting after LED chain updated.
}
#else
// STUB LED APIs when disabled.
void led_setAll(const Led_Color* color){}
void led_set(uint8_t index, const Led_Color* color) {}
bool led_isBusy() {return false;}
void led_update() {}
#endif