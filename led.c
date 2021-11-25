#include "led.h"
#include "bus.h"
#include "led_internal.h"
#include <avr/cpufunc.h>



static void led_init();
static void led_finit();
void led_task (message_t message, MessageData data);

PRIVATE_TASK_DEFINE(led_task, 4);

typedef struct {
    uint8_t     state;
    uint8_t     index;
    uint8_t     total;
} LedState;

Led_RegisterFile led_registerfile;
BUS_REGISTER_FILE_TASK(led_registerfile, 1, Bus_FlagReadWrite, GET_TASK_ID_WIDE(led_task));

#define Led (*((LedState*) &_SFR_MEM8(0x001C)))

const Led_Color Led_ColorPallet[Led_ColorIndexMax] = {
    //Pallet_Black
    {0,0,0},
    //Pallet_White
    {255,255,255},
    //Pallet_Red
    {.r = CONFIG_LED_R_INTENSITY, .g = 0, .b = 0},
    //Pallet_Green
    {.r = 0, .g = CONFIG_LED_G_INTENSITY, .b = 0},
    //Pallet_Blue
    {.r = 0, .g = 0, .b = CONFIG_LED_B_INTENSITY}
};

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
	EVSYS.ASYNCCH0 = 0x0D;
	EVSYS.ASYNCCH1 = 0x02;
	EVSYS.ASYNCUSER0 = 0x03;
	EVSYS.ASYNCUSER1 = 0x00;
	EVSYS.ASYNCUSER2 = 0x01;
	EVSYS.ASYNCUSER3 = 0x01;
	EVSYS.ASYNCUSER4 = 0x03;
	EVSYS.ASYNCUSER5 = 0x03;
	EVSYS.ASYNCUSER6 = 0x00;
	EVSYS.ASYNCUSER7 = 0x00;
	EVSYS.ASYNCUSER8 = 0x00;
	EVSYS.ASYNCUSER9 = 0x00;
	EVSYS.ASYNCUSER10 = 0x00;
	EVSYS.SYNCCH0 = 0x01;
	EVSYS.SYNCCH1 = 0x00;
	EVSYS.SYNCUSER0 = 0x00;
    
    // *************************************************************************
    // **** CCL Configuration **************************************************
    
	// Enable Protected register, peripheral must be disabled (ENABLE=0, in CCL.LUT0CTRLA).

    //SEQSEL RS; 
	CCL.SEQCTRL0 = 0x04;

    //Truth 0
	CCL.TRUTH0 = 0x08;

    //INSEL2 EVENT0; 
	CCL.LUT0CTRLC = 0x03;

    //INSEL1 TCB0; INSEL0 USART0; 
	CCL.LUT0CTRLB = 0x7A;

    //Truth 1
	CCL.TRUTH1 = 0x70;

    //INSEL2 EVENT0; 
	CCL.LUT1CTRLC = 0x03;

    //INSEL1 USART0; INSEL0 USART0; 
	CCL.LUT1CTRLB = 0xAA;
	
    //EDGEDET DIS; CLKSRC disabled; FILTSEL DISABLE; OUTEN enabled; ENABLE enabled; 
	CCL.LUT0CTRLA = 0x09;

    //EDGEDET DIS; CLKSRC disabled; FILTSEL DISABLE; OUTEN enabled; ENABLE enabled; 
	CCL.LUT1CTRLA = 0x09;

    //RUNSTDBY disabled; ENABLE disable; 
	CCL.CTRLA = 0x09;
    
    
    // *************************************************************************
    // **** TCB0 Configuration *************************************************
    
    TCB0.CCMP = 0x03;
    TCB0.CNT = 0x00;
    TCB0.CTRLB = 0x16;
    TCB0.DBGCTRL = 0x00;
    //FILTER disabled; EDGE enabled; CAPTEI enabled; 
    TCB0.EVCTRL = 0x11;
    TCB0.INTCTRL = 0x00;
    TCB0.INTFLAGS = 0x00;
    TCB0.TEMP = 0x00;

    //RUNSTDBY disabled; SYNCUPD disabled; CLKSEL CLKDIV1; ENABLE disabled; 
    TCB0.CTRLA = 0x00;

    // *************************************************************************
    // **** UART Configuration *************************************************
    
    //set baud rate register
    USART0.CTRLB = 0;
    USART0.BAUD = (uint16_t)LED_BAUD(500000);
    USART0.CTRLA = 0x00;
    USART0.CTRLB = USART_TXEN_bm;
    USART0.CTRLC = USART_CMODE_MSPI_gc;
    USART0.DBGCTRL = 0x00;
    USART0.EVCTRL = 0x00;
    USART0.RXPLCTRL = 0x00;
    USART0.TXPLCTRL = 0x00;
    
    // *************************************************************************
    // **** Misc Initialization ************************************************
    
    Led.state = LED_STATE_IDLE;
    Led.index = 0;
    Led.total = 0;
    
    led_setAll(&Led_ColorPallet[Led_ColorBlackIndex]);
    
    led_registerfile.data.count = CONFIG_LED_COUNT;
    led_registerfile.ctrla      = Led_CtrlA_Update;
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

void led_setMasked(uint8_t index, const Led_Color* colorA, const Led_Color* colorB, Led_ColorMask mask) {
    if (index < CONFIG_LED_COUNT) {
        if (mask & Led_ColorMaskAll) {
            mask = 0xF;
        }
        led_registerfile.data.colors[index].r = mask & 1 ? colorB->r : colorA->r;
        led_registerfile.data.colors[index].g = mask & 2 ? colorB->g : colorA->g;
        led_registerfile.data.colors[index].b = mask & 4 ? colorB->b : colorA->b;
    }
}

void led_set(uint8_t index, const Led_Color* color) {
    if (index < CONFIG_LED_COUNT) {
        led_registerfile.data.colors[index] = *color;
    }
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

void led_task (message_t message, MessageData data) {
    switch (message) {
        case SystemMessage_Abort:
        case SystemMessage_Init:
            led_init();
            break;
        case SystemMessage_Finit:
            led_finit();
            break;
        case SystemMessage_Loop:
            if (led_registerfile.ctrla & Led_CtrlA_Update) {
                led_update();
            }
            break;
#if CONFIG_BUS_SIGNAL
        case BusMessage_Signal(1):
            led_update();
            break;
#endif
        default: break;
    }
}