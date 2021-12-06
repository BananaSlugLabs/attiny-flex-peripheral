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

typedef struct Led_ColorTag {
    uint8_t g;
    uint8_t r;
    uint8_t b;
} Led_Color;

extern const Led_Color led_color_black;
extern const Led_Color led_color_white;
extern const Led_Color led_color_red;
extern const Led_Color led_color_green;
extern const Led_Color led_color_blue;

void led_set(uint8_t index, const Led_Color* color);
void led_setAll(const Led_Color* color);
bool led_isBusy();
void led_update();


#ifdef	__cplusplus
}
#endif

#endif	/* LED_H */

