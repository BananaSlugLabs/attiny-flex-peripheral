#include "mcc_generated_files/mcc.h"
#include "common.h"

void App_Task () {
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

        Led_SetAll(c);
        //Time_Sleep(10);
        i++;
    }
}