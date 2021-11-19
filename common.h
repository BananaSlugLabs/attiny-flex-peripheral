/* 
 * File:   common.h
 * Author: fosterb
 *
 * Created on November 14, 2021, 2:35 PM
 */

#ifndef COMMON_H
#define	COMMON_H

#include "mcc_generated_files/mcc.h"
#include "stdbool.h"

#define CONFIG_LED_COUNT            4
#define CONFIG_LED_PALLET           16
#define CONFIG_TIMERS_COUNT         8

#ifdef	__cplusplus
extern "C" {
#endif
    
    typedef uint8_t TimerHandle;
    typedef uint16_t TimerInterval; // in msec
    typedef uint16_t Time16; // in msec
    typedef uint32_t Time32; // in msec
    
    void App_Task();
    
    // **** Core System Functions **********************************************
    void Sys_EarlyInit();
    void Sys_Init();
    void Sys_Loop();
    
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
    
#ifdef	__cplusplus
}
#endif

#endif	/* COMMON_H */

