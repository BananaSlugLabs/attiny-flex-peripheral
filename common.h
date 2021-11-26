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
#include <avr/io.h>
#include <avr/builtins.h>
#include <avr/interrupt.h>
//#include "mcc_generated_files/mcc.h"
#include <util/atomic.h>
#include <avr/cpufunc.h>
#include "device_config.h"
#include "firmware.h"
#include "util.h"
#include "tasks.h"

/*init_priority (101)*/

#ifdef	__cplusplus
extern "C" {
#endif

typedef uint8_t     timer_handle_t;
typedef uint16_t    timer_interval_t; // in msec
typedef uint16_t    time16_t; // in msec
typedef uint32_t    time32_t; // in msec

#ifdef	__cplusplus
}
#endif

#endif	/* COMMON_H */

