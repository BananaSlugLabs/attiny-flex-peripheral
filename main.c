#include "mcc_generated_files/mcc.h"
#include "common.h"

void App_Init() {}

void App_InitIO() {}

void App_Task () {
#if 0
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
        
        Led_Set(2, &c);
        Led_Set(1, &BuiltinPallet[BuiltInPallet_Blue]);
        
        Time_Sleep(10);
        i++;
        
        if (seq == 3) {
            Sys_Abort(SysAbortAssertion);
        }
    }
#endif
}