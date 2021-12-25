/*
 * File:   led_regs.h
 * Author: fosterb
 *
 * Created on November 25, 2021, 11:34 AM
 */

#ifndef LED_REGS_H
#define	LED_REGS_H

#ifdef	__cplusplus
extern "C" {
#endif

enum {
    Led_CtrlA_Update            = 1<<0,
    Led_CtrlA_Busy              = 1<<1,
    Led_CtrlA_PersistKey        = 0x35<<2,
    Led_CtrlA_PersistKeyMask    = 0x1F<<2
};

typedef struct Led_RegisterData {
    uint8_t             count;
    Led_Color           colors [CONFIG_LED_COUNT];
} Led_RegisterData;

typedef struct RegisterFileTag {
    uint8_t             ctrla;
    uint8_t             ctrlb;
    Led_RegisterData    data;
} Led_RegisterFile;

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


#define LED_BAUD(BAUD_RATE)             (((float)(F_CPU * 64) / (2 * (float)BAUD_RATE)) + 0.5)
#define LED_RESET_LENGTH                15

#define LED_STATE_IDLE                  0
#define LED_STATE_RESET                 1
#define LED_STATE_RESET_DONE            2
#define LED_STATE_STREAM_PIXEL          3
#define LED_STATE_STREAM_PIXEL_DONE     4

#ifdef	__cplusplus
}
#endif

#endif	/* LED_REGS_H */

