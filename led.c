#include "common.h"
#include <avr/cpufunc.h>

/**
 * The state machine works like so:
 * 
 * 1) LED_STATE_IDLE: Inactive
 *  - The state will be LED_STATE_IDLE when no pending actions are desired.
 * 
 * 2) LED_STATE_IDLE/LED_STATE_CHANGED: Changing/Updating
 *  - LED_STATE_CHANGED denotes that the LED buffers have been modified.
 *  - The LEDs may be explicitly updated by a call to #Led_Update
 *  - Alternatively, LED_Task will automatically flush the updated LEDs.
 * 
 * 3) LED_STATE_CHANGED/LED_STATE_IDLE: #Led_Update called
 *    - State is set to LED_STATE_RESET to generate a self-timed reset
 *      pulse.
 *    - Ensure that CCL, TCB are disabled
 *    - Enable USART TX & Data Register Empty interrupt.
 * 
 * 4) LED_STATE_RESET: Data Register Empty Interrupt
 *    - Write data register with 0 (or any value, doesn't matter)
 *    - Increment index.
 *    - When index is LED_RESET_LENGTH, Data Register Empty
 *      is inhibited and TX complete interrupt is enabled.
 *    - Change state to LED_STATE_RESET_DONE
 * 
 * 5) LED_STATE_RESET_DONE: (Resetting) TX complete interrupt arrives:
 *    - CCL, TCB are enabled
 *    - TX complete interrupt inhibited
 *    - Data Register Empty is enabled
 *    - (Data Register Empty will then immediately activate)
 *    - State is set to LED_STATE_STREAM_PIXEL.
 * 
 * 6) LED_STATE_STREAM_PIXEL: Data Register Empty interrupt arrives:
 *    - Using index, load LedBuffer.raw[Led.index] and assign to data register
 *    - Increment the count
 *    - If index == sizeof(LedBuffer.raw):
 *        - Inhibit data register empty and enabled TX complete interrupt.
 *        - Change state to LED_STATE_DONE
 * 
 * 7) LED_STATE_STREAM_PIXEL_DONE: When TX complete interrupt:
 *     - Set the state to LED_CMD_IDLE
 *     - Disable CCL, TCB, USART
 */

#define LED_BAUD(BAUD_RATE) (((float)(F_CPU * 64) / (2 * (float)BAUD_RATE)) + 0.5)
#define LED_RESET_LENGTH 100

#define LED_STATE_IDLE                  0
#define LED_STATE_RESET                 1
#define LED_STATE_RESET_DONE            2
#define LED_STATE_STREAM_PIXEL          3
#define LED_STATE_STREAM_PIXEL_DONE     4

typedef struct {
    uint8_t     state;
    uint8_t     index;
    LedCount    total;
} LedState;

#define Led (*((LedState*) &_SFR_MEM8(0x001C)))

const LedColor BuiltinPallet[BuiltInPallet_MAX] = {
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

#if CONFIG_LED_OPTIMIZE_SIZE
// While less efficient than the run of the mill polled implementation,
// when interrupts are disabled, we can jump in to the middle of the IRQ handler
// and use the interrupt controller to see if we need to RET.
//
// It's highly questionable especially since it's entirely possible it may break
// after a change.
void SoftISR_Led_TXC ();
void SoftISR_Led_DRE ();
#endif

ISR(USART0_TXC_vect) {
#if CONFIG_LED_OPTIMIZE_SIZE
    asm volatile(
        ".global SoftISR_Led_TXC \n"
        "SoftISR_Led_TXC:"
    );
#endif
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
            Bus_RegisterFile.led_config.busy = false;
            break;
            
        default:
            DEBUG_BREAKPOINT();
            break;
    }
#if CONFIG_LED_IRQ_PERF
    CONFIG_LED_IRQ_PORT.OUT &= ~(1<<CONFIG_LED_IRQ_PIN);
#endif
#if CONFIG_LED_OPTIMIZE_SIZE
    if (!(CPUINT.STATUS&0x3)) {
        asm volatile("ret");
    }
#endif
}

// Note: Based on testing, DRE takes approx 1uSec to execute.
ISR(USART0_DRE_vect) {
#if CONFIG_LED_OPTIMIZE_SIZE
    asm volatile(
        ".global SoftISR_Led_DRE \n"
        "SoftISR_Led_DRE:"
    );
#endif
#if CONFIG_LED_IRQ_PERF
    CONFIG_LED_IRQ_PORT.OUT |= (1<<CONFIG_LED_IRQ_PIN);
#endif
    switch (Led.state) {
        case LED_STATE_STREAM_PIXEL:
            USART0.TXDATAL = Bus_RegisterFile.led_data.raw[Led.index];
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
#if CONFIG_LED_OPTIMIZE_SIZE
    if (!(CPUINT.STATUS&0x3)) {
        asm volatile("ret");
    }
#endif
}

void Led_Init() {
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
    Led_SetAll(&BuiltinPallet[BuiltInPallet_White]);
    Bus_RegisterFile.led_count          = CONFIG_LED_COUNT;
    Bus_RegisterFile.led_config.busy    = false;
    Bus_RegisterFile.led_config.update  = true;
    
#if CONFIG_LED_IRQ_PERF
    CONFIG_LED_IRQ_PORT.DIR |= (1<<CONFIG_LED_IRQ_PIN);
    CONFIG_LED_IRQ_PORT.OUT &= ~(1<<CONFIG_LED_IRQ_PIN);
#endif
    
}

void Led_SetAll(const LedColor* color){
    for (int i = 0; i < CONFIG_LED_COUNT; i ++){
        Bus_RegisterFile.led_data.colors[i] = *color;
    }
    Bus_RegisterFile.led_config.update = true;
}

void Led_SetMasked(uint8_t index, const LedColor* colorA, const LedColor* colorB, LedColorMask mask) {
    if (index < CONFIG_LED_COUNT) {
        if (mask & LedColorMask_RGB) {
            mask = 0xF;
        }
        Bus_RegisterFile.led_data.colors[index].r = mask & 1 ? colorB->r : colorA->r;
        Bus_RegisterFile.led_data.colors[index].g = mask & 2 ? colorB->g : colorA->g;
        Bus_RegisterFile.led_data.colors[index].b = mask & 4 ? colorB->b : colorA->b;
    }
    Bus_RegisterFile.led_config.update = true;
}

void Led_Set(uint8_t index, const LedColor* color) {
    if (index < CONFIG_LED_COUNT) {
        Bus_RegisterFile.led_data.colors[index] = *color;
    }
    Bus_RegisterFile.led_config.update = true;
}

bool Led_IsBusy() {
    return Led.state > LED_STATE_IDLE;
}

void Led_Update() {
    if (Led_IsBusy()) {
        return;
    }
    
    Bus_RegisterFile.led_config.busy        = true;
    Bus_RegisterFile.led_config.update      = false;
    if ( Bus_RegisterFile.led_count == 0 ) {
        // nothing to do...
        Bus_RegisterFile.led_config.busy    = false;
        return;
    }
    Led.total = Bus_RegisterFile.led_count * sizeof(LedColor);
    Led.state = LED_STATE_RESET;
    
    // Disable CCL Output, Timer, and clear USART Transmit Done Flag
    CCL.LUT0CTRLA   &= ~CCL_ENABLE_bm;
    TCB0.CTRLA      &= ~TCB_ENABLE_bm;
    USART0.STATUS   = USART_TXCIF_bm;
    USART0.CTRLB    |= USART_TXEN_bm;
    Led.index       = 0;
    
    if (SREG & CPU_I_bm) {
            USART0.CTRLA = USART_DREIE_bm;
    } else {
#if CONFIG_LED_OPTIMIZE_SIZE
        // Simulate Interrupts through polling...
        USART0.CTRLA = USART_DREIE_bm;
        while (USART0.CTRLA & (USART_TXCIE_bm | USART_DREIE_bm)) {
            if ((USART0.CTRLA & USART_DREIE_bm) && (USART0.STATUS & USART_DREIF_bm)) {
                SoftISR_Led_DRE();
            } else if ((USART0.CTRLA & USART_TXCIE_bm) && (USART0.STATUS & USART_TXCIF_bm)) {
                SoftISR_Led_TXC();
            }
        }
#else
        for (Led.index = 0; Led.index < LED_RESET_LENGTH; Led.index ++) {
            while (!(USART0.STATUS & USART_DREIF_bm));
            USART0.TXDATAL = 0;
        }

        Led.state = LED_STATE_STREAM_PIXEL;
        
        // Wait for TX to complete:
        while (!(USART0.STATUS & USART_TXCIF_bm));
        
        // Enable CCL Output, Timer, and clear USART Transmit Done Flag
        USART0.STATUS = USART_TXCIF_bm;
        CCL.LUT0CTRLA |= CCL_ENABLE_bm;
        TCB0.CTRLA |= TCB_ENABLE_bm;

        // Iterate through each pixel and send when buffered register is set.
        uint8_t* buf = Bus_RegisterFile.led_data.raw;
        for (Led.index = 0; Led.index < Led.total; Led.index++) {
            while (!(USART0.STATUS & USART_DREIF_bm));
            USART0.TXDATAL = buf[Led.index];
        }
        
        // Ensure that transmit is completed.
        while (!(USART0.STATUS & USART_TXCIF_bm));
        
        CCL.LUT0CTRLA   &= ~CCL_ENABLE_bm;
        TCB0.CTRLA      &= ~TCB_ENABLE_bm;
        USART0.STATUS   = USART_TXCIF_bm;
        USART0.CTRLB    &= ~USART_TXEN_bm;
        Led.state       = LED_STATE_IDLE;

        Led.state = LED_STATE_IDLE;
        Bus_RegisterFile.led_config.busy = false;
#endif
    }
}

void Led_Task() {
    if (Bus_RegisterFile.led_config.update) {
        Led_Update();
    }
}
