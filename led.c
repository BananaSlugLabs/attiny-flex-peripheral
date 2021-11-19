#include "common.h"

#define LED_BAUD(BAUD_RATE) (((float)(F_CPU * 64) / (2 * (float)BAUD_RATE)) + 0.5)
#define LED_RESET_LENGTH 50

typedef union LedBufferTag {
    LedColor colors [CONFIG_LED_COUNT];
    uint8_t  raw[CONFIG_LED_COUNT*sizeof(LedColor)];
} LedBuffer;

static LedBuffer Led_Buffer;
static bool Led_IrqMode = true;

#define LED_CMD_IDLE            0xFF
#define LED_CMD_CHANGED         0xFE
#define LED_CMD_BEGIN_BREAK     0xFD
#define LED_CMD_END_BREAK       0xE8
#define LED_CMD_LAST_PIXEL     (CONFIG_LED_COUNT*sizeof(LedColor))-1
#define LED_CMD_FIRST_PIXEL    0

typedef struct {
    uint8_t     cmd;
} LedState;
#define Led (*((LedState*) &_SFR_MEM8(0x001C)))


ISR(USART0_DRE_vect) {
    /*
    
    if (Led.cmd & LED_FLAG_STREAM) {
        USART0.TXDATAL = Led_Buffer.raw[Led.co]
    } else if (Led.cmd & LED_FLAG_BREAK) {
        USART0.TXDATAL = 0;
        Led.count --;
        if (Led.count == 0) {
            Led.cmd |= 
        }
    }
    */
}

void Led_Init() {
    
    // **** UART Configuration *************************************************
    
    // Ensure XCK is enabled for output
    PORTA_DIRSET = 1<<3;
    
    // Ensure USART uses the alternate port.
    PORTMUX.CTRLB |= PORTMUX_USART0_ALTERNATE_gc;
    
    //set baud rate register
    USART0.BAUD = (uint16_t)LED_BAUD(500000);
	
    //RXCIE disabled; TXCIE disabled; DREIE disabled; RXSIE disabled; LBME disabled; ABEIE disabled; RS485 OFF; 
    USART0.CTRLA = 0x00;
	
    //RXEN enabled; TXEN enabled; SFDEN disabled; ODME disabled; RXMODE NORMAL; MPCM disabled; 
    USART0.CTRLB = 0x40;
	
    //CMODE MSPI 
    USART0.CTRLC = USART_CMODE_MSPI_gc;
	
    //DBGCTRL_DBGRUN
    USART0.DBGCTRL = 0x00;
	
    //EVCTRL_IREI
    USART0.EVCTRL = 0x00;
	
    //RXPLCTRL_RXPL
    USART0.RXPLCTRL = 0x00;
	
    //TXPLCTRL_TXPL
    USART0.TXPLCTRL = 0x00;
    
    Led.cmd = LED_CMD_CHANGED;
    
    // **** Misc Initialization ************************************************
    
}

void Led_SetAll(LedColor color){
    for (int i = 0; i < CONFIG_LED_COUNT; i ++){
        Led_Buffer.colors[i] = color;
    }
    if (Led.cmd == LED_CMD_IDLE) {
        Led.cmd = LED_CMD_CHANGED;
    }
}

void Led_Set(uint16_t index, LedColor color) {
    if (index < CONFIG_LED_COUNT) {
        Led_Buffer.colors[index] = color;
    }
    if (Led.cmd == LED_CMD_IDLE) {
        Led.cmd = LED_CMD_CHANGED;
    }
}

bool Led_IsBusy() {
    return Led.cmd < LED_CMD_CHANGED;
}



void Led_Update() {
    if (Led_IsBusy()) {
        return;
    }
    
    Led.cmd = LED_CMD_BEGIN_BREAK;
    
    // Disable CCL Output, Timer, and clear USART Transmit Done Flag
    CCL.LUT0CTRLA &= ~CCL_ENABLE_bm;
    TCB0.CTRLA &= ~TCB_ENABLE_bm;
    USART0.STATUS = USART_TXCIF_bm;
    
    
    //if (!Led_IrqMode) {
    //    USART0.CTRLA |= 
        
    //} else {
    
        for (; Led.cmd > LED_CMD_END_BREAK; Led.cmd --) {
            USART0.TXDATAL = 0;
            while (!(USART0.STATUS & USART_DREIF_bm));
        }

        Led.cmd = LED_CMD_FIRST_PIXEL;
        
        // Wait for TX to complete:
        while (!(USART0.STATUS & USART_TXCIF_bm));
        
        // Enable CCL Output, Timer, and clear USART Transmit Done Flag
        USART0.STATUS = USART_TXCIF_bm;
        CCL.LUT0CTRLA |= CCL_ENABLE_bm;
        TCB0.CTRLA |= TCB_ENABLE_bm;

        // Iterate through each pixel and send when buffered register is set.
        uint8_t* buf = Led_Buffer.raw;
        for (; Led.cmd < LED_CMD_LAST_PIXEL; Led.cmd++) {
            while (!(USART0.STATUS & USART_DREIF_bm));
            USART0.TXDATAL = buf[Led.cmd];
        }
        
        // Ensure that transmit is completed.
        while (!(USART0.STATUS & USART_TXCIF_bm));
        USART0.STATUS = USART_TXCIF_bm;

        Led.cmd = LED_CMD_IDLE;
    //}
}

void Led_Task() {
    if (Led.cmd == LED_CMD_CHANGED) {
        Led_Update();
    }
}
