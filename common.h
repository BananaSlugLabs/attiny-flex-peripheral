/* 
 * File:   common.h
 * Author: fosterb
 *
 * Created on November 14, 2021, 2:35 PM
 */

#ifndef COMMON_H
#define	COMMON_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "device_config.h"
#include <avr/io.h>
#include <avr/builtins.h>
//#include "mcc_generated_files/mcc.h"
#include <util/atomic.h>
#include <avr/cpufunc.h>
#include "firmware.h"

#define DISABLE_INTERRUPTS()   cli()
#define ENABLE_INTERRUPTS()    sei()

#include "mcc_generated_files/config/clock_config.h"

#ifdef	__cplusplus
extern "C" {
#endif
    
    typedef uint8_t TimerHandle;
    typedef uint16_t TimerInterval; // in msec
    typedef uint16_t Time16; // in msec
    typedef uint32_t Time32; // in msec
    
    #define DEBUG_BREAKPOINT() asm("BREAK")
    
    void App_Task();
    void App_Init();
    void App_InitIO();
    
    // **** Core System Functions **********************************************
    typedef enum SysAbortCodeTag {
        SysAbortNone,
        SysAbortBadIRQ      = 0B0001,
        SysNmiIRQ           = 0B0010,
        SysAbortAssertion   = 0B1010
    } SysAbortCode;
    
    void Sys_EarlyInit();
    void Sys_Init();
    void Sys_Loop();
    void Sys_Abort(SysAbortCode code) __attribute__((noreturn));
    void Sys_Restart();
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
    
    typedef enum LedColorMaskTag {
        LedColorMask_Red    = 1<<0,
        LedColorMask_Green  = 1<<1,
        LedColorMask_Blue   = 1<<2,
        LedColorMask_RGB    = 1<<3,
    } LedColorMask;
    
    typedef enum BuiltInPalletTag {
        BuiltInPallet_Black, BuiltInPallet_White, BuiltInPallet_Red, BuiltInPallet_Green, BuiltInPallet_Blue, BuiltInPallet_MAX
    } BuiltInPallet;
    
    extern const LedColor BuiltinPallet[BuiltInPallet_MAX];
    
    void Led_Init();
    void Led_Set(uint8_t index, const LedColor* color);
    void Led_SetMasked(uint8_t index, const LedColor* colorA, const LedColor* colorB, LedColorMask mask);
    void Led_SetAll(const LedColor* color);
    bool Led_IsBusy();
    void Led_Update();
    void Led_Task();
    
    // **** Device Control & Configuration *************************************
    void Command_Init();
    void Command_Task();
    void Command_Finit();
    
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
    enum PixelKindTag {
        LedKindWS2812         = 1,
    };
    typedef uint8_t LedKind;
    
    enum PixelFormatTag {
        LedFormatGRB888       = 1
    };
    typedef uint8_t LedFormat;
    typedef uint8_t LedCount;
    
    typedef struct LedRegisterControlATag {
        bool    update      : 1;
        bool    busy        : 1;
    } LedRegisterControlA;
    
    typedef struct FirmwareRegisterFileTag {
        uint16_t        ident;                                                  // 0..1
        uint8_t         version;                                                // 2
        uint8_t         led_maximum;                                            // 3
        LedKind         led_kind;                                               // 4
        LedFormat       led_format;                                             // 5
        uint16_t        _r1;                                                    // 6..7
    } FirmwareRegisterFile;
    
    typedef struct RegisterFileTag {
        LedCount                led_count;                                      // 8
        LedRegisterControlA     led_config;                                     // 9
        union {
            LedColor colors [CONFIG_LED_COUNT];                                 // 10+
            uint8_t  raw[CONFIG_LED_COUNT*sizeof(LedColor)];
        } led_data;
    } RegisterFile;
    
    extern RegisterFile                 Bus_RegisterFile;
    extern const FirmwareRegisterFile   Bus_FirmwareRegisterFile;
    
#ifdef	__cplusplus
}
#endif

#endif	/* COMMON_H */

