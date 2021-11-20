#include <avr/io.h>
#include "device_config.h"
    
#if CONFIG_SRAM_INIT == DEF_SRAM_INIT_ZERO || CONFIG_SRAM_INIT == DEF_SRAM_INIT_STRIPE
/**
 * When CONFIG_SRAM_INIT is DEF_SRAM_INIT_STRIPE/DEF_SRAM_INIT_ZERO, the SRAM is
 * fully initialized after boot. This is primarily a debugging tool to gather
 * stack utilization information.
 */
.section .init1,"ax",@progbits
.global __boot_sram_init
.func __boot_sram_init
__boot_sram_init:
    ldi r26, lo8(RAMSTART)
    ldi r27, hi8(RAMSTART)
    ldi r28, lo8(RAMEND)
    ldi r29, hi8(RAMEND)
#if CONFIG_SRAM_INIT == DEF_SRAM_INIT_STRIPE
    ldi r16, 0xFF
    ldi r17, 0xAA
    ldi r18, 0x55
    ldi r19, 0x00
#elif CONFIG_SRAM_INIT == DEF_SRAM_INIT_ZERO 
    eor	r16, r16
    eor	r17, r17
    eor	r18, r18
    eor	r19, r19
#endif
    .Lnext_byte:
    ST X+, r16
    ST X+, r17
    ST X+, r18
    ST X+, r19
    cp R26, R28
    cpc R27, R29  
    brlt .Lnext_byte
    nop
#elif CONFIG_SRAM_INIT != DEF_SRAM_INIT_NONE
#error "CONFIG_SRAM_INIT must be one of DEF_SRAM_INIT_STRIPE, DEF_SRAM_INIT_ZERO, or DEF_SRAM_INIT_NONE."
#endif

#if CONFIG_RESERVE_STACK
    
#endif