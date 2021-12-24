#include "keypad_internal.h"
#include "string.h"

/*
 * KeyPad Driver Information
 * 
 * This driver implements a keypad using a current mirror. This method provides
 * a roughly linear response. See:
 * 
 * https://github.com/sgmne/AnalogKeypad
 * 
 * See readme.md for further details on how this is implemented.
 */

#if CONFIG_KP_ENABLE

static void keypad_init ();
static void keypad_finit ();
static void keypad_worker ();
#define keypad_cal (keypad_state.cal)

static KeyPad_State keypad_state;

Bus_DefineMemoryMap     (keypad_state,         BUS_PRIORITY_003);
SysInit_Subscribe       (keypad_init,          Signal_Normal);
SysFinit_Subscribe      (keypad_finit,         Signal_nNormal);
SysLoop_Subscribe       (keypad_worker,        Signal_Normal);

ISR(ADC0_RESRDY_vect) {
    uint8_t raw = ADC0.RES;// >> 2;
    keypad_state.raw = raw;
    ADC0.INTFLAGS = ADC_RESRDY_bm | ADC_WCMP_bm;
    
    sys_signal(Sys_SignalWakeLock | Sys_SignalWorkerPending);
    
    if (raw > keypad_cal.threshold) {
        int8_t delta = keypad_state.raw - keypad_state.candidate;
        if ( keypad_state.candidate == 0 || delta > keypad_cal.steadyStateWindow || delta > keypad_cal.steadyStateWindow) {
            keypad_state.state = KeyPad_StateSteady + keypad_cal.filter;
            keypad_state.candidate = raw;
        } else if (keypad_state.state > KeyPad_StateLatch) {
            keypad_state.state --;
            keypad_state.candidate = raw;
        } else {
            // Stop processing new data...
            // This is an optimization that would also let us go to sleep.
            uint8_t lo, hi;
            lo = keypad_state.candidate - keypad_cal.steadyStateWindow;
            hi = keypad_state.candidate + keypad_cal.steadyStateWindow;
            if (lo > keypad_state.candidate) {
                lo = 0;
            }
            if (hi < keypad_state.candidate) {
                hi = 0xFF;
            }
            ADC0.WINHT = hi;
            ADC0.WINLT = lo;
            ADC0.INTCTRL = ADC_WCMP_bm;
        }
    } else {
        keypad_state.state = KeyPad_StateIdle;
        ADC0.WINHT = keypad_cal.threshold;
        ADC0.WINLT = 0;
        ADC0.INTCTRL = ADC_WCMP_bm;
    }
}

ISR(ADC0_WCOMP_vect) {
    ADC0.INTCTRL = ADC_RESRDY_bm;
    ADC0.INTFLAGS = ADC_WCMP_bm;
}

#define CONFIG_KP_CAL_THRESHOLD     0.9
#define CONFIG_KP_CAL_OFFSET        1.1
#define CONFIG_KP_CAL_STEP          0.26
#define CONFIG_KP_CAL_STREADY       (CONFIG_KP_CAL_STEP/8)
#define CONFIG_KP_CAL_FILTER        64
#define CONFIG_KP_CAL_VDD           5
#define adc_code(V)                 (uint8_t)(((255./((float)CONFIG_KP_CAL_VDD)) * (V)) + 0.5)

static void keypad_init () {
    // *************************************************************************
    // **** VREF Initialization ************************************************
    // *************************************************************************
    VREF.CTRLA      = VREF_ADC0REFSEL_1V1_gc | VREF_DAC0REFSEL_0V55_gc;         // VREF for ADC is 1.1V and VREF for AC is 0.55V.
    VREF.CTRLB      = 0x00;

    // *************************************************************************
    // **** ADC Initialization *************************************************
    // *************************************************************************
	ADC0.CTRLA      = 0;                                                        // Disable ADC
	ADC0.CTRLB      = 0;
	ADC0.CTRLC      = ADC_REFSEL_VDDREF_gc  |                                   // 5pF cap, clock / 16, use VDD as reference.
                      ADC_PRESC_DIV16_gc    |
                      ADC_SAMPCAP_bm;
	ADC0.CTRLD      = 0x00;                                                     // No init delay, sample delay variation, or sample delay
	ADC0.CTRLE      = ADC_WINCM_OUTSIDE_gc;
	ADC0.SAMPCTRL   = CONFIG_KP_TUNING_SAMPLE_LENGTH;                           // Improved impedance response
    ADC0.MUXPOS     = CONFIG_KP_ADC_PIN;
	ADC0.EVCTRL     = 0x00;                                                     // No events
    ADC0.INTCTRL    = ADC_RESRDY_bm;//|ADC_WCMP_bm;
	ADC0.DBGCTRL    = 0x00;                                                     // Disable during debug
	ADC0.CALIB      = ADC_DUTYCYC_DUTY25_gc;                                    // CLK is less than 1.5MHz therefore use DUTYCYC
    ADC0.CTRLA      = ADC_ENABLE_bm | ADC_RUNSTBY_bm | ADC_FREERUN_bm | ADC_RESSEL_8BIT_gc;
    ADC0.COMMAND    = ADC_STCONV_bm;
    
    keypad_state.cal.threshold          = adc_code(CONFIG_KP_CAL_THRESHOLD);    // 0.9 * 1V
    keypad_state.cal.offset             = adc_code(CONFIG_KP_CAL_OFFSET);       // 1V
    keypad_state.cal.vstep              = adc_code(CONFIG_KP_CAL_STEP);         // 270mV
    keypad_state.cal.steadyStateWindow  = adc_code(CONFIG_KP_CAL_STREADY);      // steady state window
    keypad_state.cal.filter             = CONFIG_KP_CAL_FILTER;                 // number of steady steps before acceptance
}

static void keypad_finit () {
	ADC0.CTRLA      = 0;
}

#if CONFIG_KP_HISTORY
uint8_t keyHistoryIndex = 0;
static inline void keypad_add_history() {
    if (keyHistoryIndex == 0) {
        memset(keypad_state.keyHistory, 0, sizeof(keypad_state.keyHistory));
    }
    keypad_state.keyHistory[keyHistoryIndex] = keypad_state.key;
    keyHistoryIndex = (keyHistoryIndex+1) & 0xF;
}
#endif

static void keypad_worker () {
    // Disable ADC Interrupt
    uint8_t irq = ADC0.INTCTRL;
    ADC0.INTCTRL = 0;
    // Handle state transitions
    if (keypad_state.state == KeyPad_StateIdle) {
        keypad_state.key = 0;
        keypad_state.candidate = 0;
        // This is not needed, but when reading diagnostics it will make more sense
        if (irq & ADC_WCMP_bm) {
            keypad_state.raw = 0;
        }
    } else if (keypad_state.state == KeyPad_StateLatch) {
        // The math for converting keys is off loaded to the idle loop
        keypad_state.state = KeyPad_StateSteady;
        uint8_t raw = (keypad_state.candidate > keypad_cal.offset) ? keypad_state.candidate - keypad_cal.offset : 0;
        keypad_state.key = rounduhk((unsigned short accum)(raw) / keypad_cal.vstep, 0) + 1;
#if CONFIG_KP_HISTORY
        keypad_add_history();
#endif
    }
    // Enable ADC Interrupt
    ADC0.INTCTRL = irq;
}
#endif