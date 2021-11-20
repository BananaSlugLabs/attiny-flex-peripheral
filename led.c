#include "common.h"

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
#define LED_STATE_CHANGED               1
#define LED_STATE_RESET                 2
#define LED_STATE_RESET_DONE            3
#define LED_STATE_STREAM_PIXEL          3
#define LED_STATE_STREAM_PIXEL_DONE     4

typedef union LedBufferTag {
    LedColor colors [CONFIG_LED_COUNT];
    uint8_t  raw[CONFIG_LED_COUNT*sizeof(LedColor)];
} LedBuffer;

typedef struct {
    uint8_t     state;
    uint8_t     index;
} LedState;

static LedBuffer Led_Buffer;
#define Led (*((LedState*) &_SFR_MEM8(0x001C)))


ISR(USART0_TXC_vect) {
    switch (Led.state) {
        case LED_STATE_RESET_DONE:
            Led.index       = 0;
            USART0.STATUS   = USART_TXCIF_bm;
            USART0.CTRLA    = USART_DREIE_bm; // Switch to DRE IRQ
            CCL.LUT0CTRLA   |= CCL_ENABLE_bm;
            TCB0.CTRLA      |= TCB_ENABLE_bm;
            Led.state       = LED_STATE_STREAM_PIXEL;
            return;
            
        case LED_STATE_STREAM_PIXEL_DONE:
            Led.index       = 0;
            CCL.LUT0CTRLA   &= ~CCL_ENABLE_bm;
            TCB0.CTRLA      &= ~TCB_ENABLE_bm;
            USART0.STATUS   = USART_TXCIF_bm;
            USART0.CTRLA    = 0;
            USART0.CTRLB    &= ~USART_TXEN_bm;
            Led.state       = LED_STATE_IDLE;
            return;
        default:
            DEBUG_BREAKPOINT();
            return;
    }
}

ISR(USART0_DRE_vect) {
    switch (Led.state) {
        case LED_STATE_STREAM_PIXEL:
            USART0.TXDATAL = Led_Buffer.raw[Led.index];
            Led.index ++;
            if (Led.index == sizeof(Led_Buffer.raw)) {
                USART0.CTRLA    = USART_TXCIE_bm; // switch to TXC IRQ
                Led.state       = LED_STATE_STREAM_PIXEL_DONE;
            }
            return;
        case LED_STATE_RESET:
            USART0.TXDATAL = 0;
            Led.index ++;   
            if (Led.index == LED_RESET_LENGTH) {
                USART0.CTRLA    = USART_TXCIE_bm;  // switch to TXC IRQ
                Led.state       = LED_STATE_RESET_DONE;
            }
            return;
            
        default:
            DEBUG_BREAKPOINT();
            return;
    }
}

void Led_Init() {
    USART0.CTRLB = 0;
    
    // **** IO Configuration ***************************************************
    
    // TODO: This code belongs somewhere else, but MCC would not allow me to
    // configure as I wished.
    
    // Ensure XCK is enabled for output
    PORTA_DIRSET = 1<<3;
    
    // Ensure USART uses the alternate port.
    PORTMUX.CTRLB |= PORTMUX_USART0_ALTERNATE_gc;
    
    
    // **** UART Configuration *************************************************
    
    //set baud rate register
    USART0.BAUD = (uint16_t)LED_BAUD(500000);
    USART0.CTRLA = 0x00;
    USART0.CTRLB = USART_TXEN_bm;
    USART0.CTRLC = USART_CMODE_MSPI_gc;
    USART0.DBGCTRL = 0x00;
    USART0.EVCTRL = 0x00;
    USART0.RXPLCTRL = 0x00;
    USART0.TXPLCTRL = 0x00;
    
    Led.state = LED_STATE_CHANGED;
    Led.index = 0;
    
    // **** Misc Initialization ************************************************
    
}

void Led_SetAll(LedColor color){
    for (int i = 0; i < CONFIG_LED_COUNT; i ++){
        Led_Buffer.colors[i] = color;
    }
    if (Led.state == LED_STATE_IDLE) {
        Led.state = LED_STATE_CHANGED;
    }
}

void Led_Set(uint16_t index, LedColor color) {
    if (index < CONFIG_LED_COUNT) {
        Led_Buffer.colors[index] = color;
    }
    if (Led.state == LED_STATE_IDLE) {
        Led.state = LED_STATE_CHANGED;
    }
}

bool Led_IsBusy() {
    return Led.state > LED_STATE_CHANGED;
}

void Led_Update() {
    if (Led_IsBusy()) {
        return;
    }
    
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
        uint8_t* buf = Led_Buffer.raw;
        for (Led.index = 0; Led.index < sizeof(Led_Buffer.raw); Led.index++) {
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
    }
}

void Led_Task() {
    if (Led.state == LED_STATE_CHANGED) {
        Led_Update();
    }
}
