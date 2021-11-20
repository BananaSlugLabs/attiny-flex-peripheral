#include "common.h"
#include <avr/interrupt.h>

static volatile SysAbortCode sys_fault;

ISR(BADISR_vect) {
    Sys_Abort(SysAbortBadIRQ);
}

void Sys_Abort(SysAbortCode code) {
    sys_fault = code;
    DISABLE_INTERRUPTS();
    DEBUG_BREAKPOINT();
    Led_Init();
    LedColor c;
    for (;;) {
        c.r = 0x20;
        c.g = 0;
        c.b = 0;
        Led_Set(0, c);
        
        if (code & (1<<3)) {
            c.g = (code & (1<<0)) ? 0x20 : 0;
            Led_Set(1, c);
            c.g = (code & (1<<1)) ? 0x20 : 0;
            Led_Set(2, c);
            c.g = (code & (1<<2)) ? 0x20 : 0;
            Led_Set(3, c);
        } else {
            c.b = (code & (1<<0)) ? 0x20 : 0;
            Led_Set(1, c);
            c.b = (code & (1<<1)) ? 0x20 : 0;
            Led_Set(2, c);
            c.b = (code & (1<<2)) ? 0x20 : 0;
            Led_Set(3, c);
        }
        
        Led_Update();
        Time_Sleep(500);
        
        c.r = 0;
        c.g = 0;
        c.b = 0;
        Led_SetAll(c);
        Led_Update();
        Time_Sleep(500);
    }
}

int main () {
    Sys_Init();
    ENABLE_INTERRUPTS();
    while(1) {
        Sys_Loop();
    }
}

void Sys_Init() {
    SYSTEM_Initialize();
    Time_Init();
    Led_Init();
    Command_Init();
}

void Sys_Loop() {
    Time_Task();
    App_Task();
    Led_Task();
    Command_Task();
}
