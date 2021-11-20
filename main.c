#include "mcc_generated_files/mcc.h"
#include "common.h"

void App_Init() {}

void App_InitIO() {}

#if !defined(CONFIG_TEST_PATTERN_TIMESTEP) || CONFIG_TEST_PATTERN_TIMESTEP <= 0
#define _CONFIG_TEST_PATTERN_TIMESTEP 25
#else
#define _CONFIG_TEST_PATTERN_TIMESTEP CONFIG_TEST_PATTERN_TIMESTEP
#endif

void App_Task () {
#if CONFIG_TEST_PATTERN
    static uint8_t i = 0;
    static uint8_t seq = 0;
    if (!Led_IsBusy()) {
    
        if (i == 0) {
            seq = (seq+1) & 0x7;
            if (seq == 0) {
                seq = 1;
            } 
        }

        LedColor c = {};
        c.r=(seq & 1) ? i : 0;
        c.g=(seq & 2) ? i : 0;
        c.b=(seq & 4) ? i : 0;
#if CONFIG_TEST_PATTERN == DEF_TEST_PATTERN_TYPE_SINGLE_FADE
        Led_Set(2, &c);
        Led_Set(1, &BuiltinPallet[BuiltInPallet_Blue]);
#elif CONFIG_TEST_PATTERN == DEF_TEST_PATTERN_TYPE_UNIFORM_FADE
        Led_SetAll(&c);
#else
#error "Unknown test pattern type."
#endif
        
        Time_Sleep(_CONFIG_TEST_PATTERN_TIMESTEP);
        i++;
#if CONFIG_TEST_ABORT
        if (seq == 3) {
            Sys_Abort(SysAbortAssertion);
        }
#endif
    }
#endif
}