#include "common.h"


int main () {
    Sys_EarlyInit();
    Sys_Init();
    while(1) {
        Sys_Loop();
    }
}

void Sys_EarlyInit() {
    SYSTEM_Initialize();
    Time_Init();
    Led_Init();
    Command_Init();
}

void Sys_Init() {
    
}

void Sys_Loop() {
    Time_Task();
    App_Task();
    Led_Task();
    Command_Task();
}
