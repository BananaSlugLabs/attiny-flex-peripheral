/* 
 * File:   led.h
 * Author: fosterb
 *
 * Created on November 24, 2021, 10:48 PM
 */

#ifndef LED_H
#define	LED_H

#include "common.h"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * ColorCode:
 *  Led_Color[16] ram_pallet
 *  Led_Color[16] rom_pallet
 *  Led_Color[16] ram_pallet
 */

typedef struct Led_ColorTag {
    uint8_t g;
    uint8_t r;
    uint8_t b;
} Led_Color;

typedef enum Led_ColorMaskTag {
    Led_ColorMaskRed    = 1<<0,
    Led_ColorMaskGreen  = 1<<1,
    Led_ColorMaskBlue   = 1<<2,
    Led_ColorMaskAll    = 1<<3,
} Led_ColorMask;

enum LED_BuiltInColors {
    Led_ColorBlackIndex, Led_ColorWhiteIndex, Led_ColorRedIndex, Led_ColorGreenIndex, Led_ColorBlueIndex, Led_ColorIndexMax
};

extern const Led_Color Led_ColorPallet[Led_ColorIndexMax];

void led_set(uint8_t index, const Led_Color* color);
void led_setMasked(uint8_t index, const Led_Color* colorA, const Led_Color* colorB, Led_ColorMask mask);
void led_setAll(const Led_Color* color);
bool led_isBusy();
void led_update();

#ifdef	__cplusplus
}
#endif

#endif	/* LED_H */

