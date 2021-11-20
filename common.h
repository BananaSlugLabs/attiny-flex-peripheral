/* 
 * File:   common.h
 * Author: fosterb
 *
 * Created on November 14, 2021, 2:35 PM
 */

#ifndef COMMON_H
#define	COMMON_H

#include "mcc_generated_files/mcc.h"
#include <stdbool.h>
#include <util/atomic.h>

#define CONFIG_LED_COUNT            4
#define CONFIG_TWI_ADDR_DEFAULT     82
    
#ifndef DISABLE_INTERRUPTS
#define DISABLE_INTERRUPTS()   __disable_interrupt();
#endif
#ifndef ENABLE_INTERRUPTS
#define ENABLE_INTERRUPTS()    __enable_interrupt();
#endif

#ifdef	__cplusplus
extern "C" {
#endif
    
    typedef uint8_t TimerHandle;
    typedef uint16_t TimerInterval; // in msec
    typedef uint16_t Time16; // in msec
    typedef uint32_t Time32; // in msec
    
    #define DEBUG_BREAKPOINT() asm("BREAK")
    
    void App_Task();
    
    // **** Core System Functions **********************************************
    typedef enum SysAbortCodeTag {
        SysAbortNone,
        SysAbortBadIRQ     = 0B0001,
        SysAbortAssertion  = 0B1010
    } SysAbortCode;
    
    void Sys_EarlyInit();
    void Sys_Init();
    void Sys_Loop();
    void Sys_Abort(SysAbortCode code);
    bool Sys_IsFaultMode();
    
    // **** Timer Functions ****************************************************
    TimerHandle Timer_Create(TimerInterval interval, bool repeat);
    void Timer_Delete(TimerHandle handle);
    bool Timer_Asserted(TimerHandle handle);
    bool Timer_Wait(TimerHandle handle);
    bool Timer_SetInterval(TimerHandle handle, TimerInterval interval, bool repeat);
    /**
     * Updates the interval for the current or next period. This should be used on
     * active timers.
     * 
     * @param handle
     * @param interval
     * @param immediate
     * @return 
     */
    bool Timer_UpdateInterval(TimerHandle handle, TimerInterval interval, bool immediate);
    void Timer_Stop(TimerHandle handle);
    bool Timer_Start(TimerHandle handle);
    bool Timer_IsActive(TimerHandle handle);
    
    Time32 Time_Get();
    Time32 Time_Sleep(TimerInterval interval);
    void Time_Init();
    void Time_Task();
    
    // **** LED Control ********************************************************
    typedef struct LedColorTag {
        uint8_t g;
        uint8_t r;
        uint8_t b;
    } LedColor;
    
    void Led_Init();
    void Led_FaultInit();
    void Led_Set(uint16_t index, LedColor color);
    void Led_SetAll(LedColor color);
    bool Led_IsBusy();
    void Led_Update();
    void Led_Task();
    
    // **** Device Control & Configuration *************************************
    void Command_Init();
    void Command_Task();
    
    // **** Device Control & Configuration *************************************
    typedef enum {
        Persist_Provisioning = 1,
        Persist_LEDDefault = 2,
    } PersistRecord;
    
    typedef struct ProvisionInfoTag {
        uint8_t deviceAddress;
    } ProvisionInfo;
    
    void Persist_Get(PersistRecord rec, void* info, size_t size);
    void Persist_Set(PersistRecord rec, void* info, size_t size);

    
    // **** TWI Register File ******************************************************
    typedef struct LedControlRegisterTag {
        bool    update : 1;
        bool    busy   : 1;
    } LedControlRegister;
    
    typedef struct RegisterFileTag {
        LedControlRegister led_config;
        uint8_t            _reserved[3];
        union {
            LedColor colors [CONFIG_LED_COUNT];
            uint8_t  raw[CONFIG_LED_COUNT*sizeof(LedColor)];
        } led_data;
    } RegisterFile;
    
    extern RegisterFile Bus_RegisterFile;
    
#ifdef	__cplusplus
}
#endif

#endif	/* COMMON_H */

