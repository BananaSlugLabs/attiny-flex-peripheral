/*
 * File:   keypad_internal.h
 * Author: fosterb
 *
 * Created on December 6, 2021, 12:04 PM
 */

#ifndef KEYPAD_INTERNAL_H
#define	KEYPAD_INTERNAL_H
#include "common.h"
#include "sys.h"
#include "bus.h"
#include "stdfix.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
    KeyPad_StateIdle,
    KeyPad_StateSteady,
    KeyPad_StateLatch,
} keypad_state_t;

typedef struct KeyPad_AnalogCalibration {
    uint8_t                 threshold;
    uint8_t                 offset;
    uint8_t                 vstep;
    uint8_t                 filter;
    uint8_t                 steadyStateWindow;
} KeyPad_AnalogCalibration;

typedef struct ADC_StateTag {
    uint8_t                 status;
    uint8_t                 ctrl;
    uint8_t                 raw;
    uint8_t                 key;
    keypad_state_t          state;
    uint8_t                 candidate;
#if CONFIG_KP_HISTORY
    uint8_t                 keyHistory[16];
#endif
    KeyPad_AnalogCalibration cal;
} KeyPad_State;

#ifdef	__cplusplus
}
#endif

// *****************************************************************************
// ***** Validation of configuration parameters ********************************
// *****************************************************************************

#if CONFIG_KP_ENABLE

    // **** Currently only analog keypads are supported so we must know what channel the ADC is mapped to.
    #if !defined(CONFIG_KP_ADC_PIN)
    #error "KeyPad: CONFIG_KP_ADC_PIN must be set to the input channel."
    #endif

    // **** Ensure CONFIG_KP_TUNING_SAMPLE_LENGTH is within a valid range (0 to 31).
    #if CONFIG_KP_TUNING_SAMPLE_LENGTH < 0 ||  CONFIG_KP_TUNING_SAMPLE_LENGTH > 31
        #error "Sample Length must be between 0 and 31."
    #endif

#endif

#endif	/* KEYPAD_INTERNAL_H */

