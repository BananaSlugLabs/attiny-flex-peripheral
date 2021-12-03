/* 
 * File:   umap.h
 * Author: fosterb
 *
 * The ?MemoryMap module was created to address challenges around building
 * modular software for AVR Tiny0/1/2 microcontrollers. The goal was to make
 * it less painful to write software for.
 *  
 * A set of utilities to help improve the modularity of the building blocks
 * found in this program.
 */

#ifndef UMAP_H
#define	UMAP_H

#include "util.h"

#define _UMAP_Attrs(section_name, additional...) \
    ATTRIBUTES(section( section_name ), additional)

#define _UMAP_NakedAttrs(section_name, additional...) \
    _UMAP_Attrs(section_name, naked, noinline, used, additional)

/* FOR DISCARD SECTIONS in linker script...
 * 
 * MPLab X's debugger has a software defect where it treats uninitialized values
 * in discard sections of the linker script to be set to 0s when loading
 * firmware. Thus will begin to clear the memory at 0x8000 (0x0). It does not
 * impact the hex files as this is handled with objcopy. 
 * 
 * For some reason setting a random value causes the debugger to properly handle
 * the discard section.
 */
#define _MPLABX_WORKAROUD = 0xAA 

#define Priority_Highest            100
#define Priority_High               300
#define Priority_Normal             500
#define Priority_Low                700
#define Priority_Lowest             900

#define Priority_nHighest           900
#define Priority_nHigh              700
#define Priority_nNormal            500
#define Priority_nLow               300
#define Priority_nLowest            100

// *****************************************************************************
// **** Signals
// *****************************************************************************

/**
 * Signalling System
 * 
 * The signal system provides a low effort mechanism for publishing signals that
 * other modules can register to receive notifications (signals) without hard
 * coding function calls, using pointers, iteration, and other techniques. 
 * Instead of the usual approach, this method uses naked functions containing a  
 * single RCALL instruction that, when linked together, generates a single
 * function of RCALLs to the relevant subscribers (consumers of signals).
 * 
 * Like so:
 * 
 * signal_XXXX_dispatch:
 *      rcall XYZ
 *      rcall ABC
 *      rcall ...
 *      ret
 * 
 * It's that simple. Compared to other techniques I use, this method proved to
 * reduce program size, improved performance, and offers a reasonable robust &
 * safe approach.
 * 
 * This technique relies on linker scripts to ensure the order of the variables
 * is correct:
 * 
 *    .text {
 *      ...
 *      KEEP (*(SORT_BY_NAME(dispatch.*.[pxs][0-9][0-9][0-9])))
 *      ...
 *    }
 * 
 * The main weakness in this system is the priorities don't reverse for say a
 * shutdown sequence. Thus Signal_nXXXXXXX priorities are provided that flip
 * the order (unless somebody offers me a better idea). Any 3 digit priority
 * may be used for precise ordering.
 */


// Use Priority_XXX going forward
#define Signal_Highest              Priority_Highest
#define Signal_High                 Priority_High
#define Signal_Normal               Priority_Normal
#define Signal_Low                  Priority_Low
#define Signal_Lowest               Priority_Lowest

#define Signal_nHighest             Priority_nHighest
#define Signal_nHigh                Priority_nHigh
#define Signal_nNormal              Priority_nNormal
#define Signal_nLow                 Priority_nLow
#define Signal_nLowest              Priority_nLowest

#define _SIG_Attrs(pub, pri)        _UMAP_NakedAttrs( "dispatch." UTIL_STRY(pub) "." UTIL_STRY(pri) )
#define _SIG_NamePub(pub,postfix)   signal_  ## pub ## _ ## postfix
#define _SIG_NamePrv(pub,postfix)   _signal_ ## pub ## _ ## postfix

typedef void (*signal_handler_t) ();

#define signal_dispatch_name(pub)                                                           \
    _SIG_NamePub(pub, dispatch)

#define signal_dispatch(pub)                                                                \
    signal_dispatch_name(pub) ()

#define Signal_Publisher_Export(pub)                                                        \
    extern void signal_dispatch_name (pub) ()

#define Signal_Publisher(pub)                                                               \
    Signal_Publisher_Export(pub);                                                           \
    extern void _SIG_NamePrv(pub, start)       () _SIG_Attrs(pub, p000);                    \
    extern void _SIG_NamePrv(pub, end)         () _SIG_Attrs(pub, x000);                    \
    void _SIG_NamePrv(pub,start) () {                                                       \
        asm(                                                                                \
            ".global " UTIL_STRY(_SIG_NamePub(pub,dispatch)) " \n"                          \
            UTIL_STRY(_SIG_NamePub(pub,dispatch)) ":"                                       \
        );                                                                                  \
    }                                                                                       \
    void _SIG_NamePrv(pub,end) ()  {                                                        \
        asm("ret");                                                                         \
    }

#define Signal_Subscriber(pub, function, pri)                                               \
    extern void _SIG_NamePrv(pub,function) () _SIG_Attrs(pub, UTIL_PASTE(s, pri));          \
    void _SIG_NamePrv(pub,function)     () {                                                \
        asm(                                                                                \
            "rcall " #function " \n"                                                        \
            : : "i" (function)                                                              \
        );                                                                                  \
    }

// *****************************************************************************
// **** Events
// *****************************************************************************

/**
 * Event subsystem provides a straight forward way to signal events across modules
 * and is used to 
 * 
 * This also provides a rudimentary priority mechanism such that lower priority events
 * are preempted by higher priority events.
 */

typedef uint8_t event_t;
//typedef uint8_t event_flags_t;
typedef void (*event_handler_t) ();

#define _EVT_REGISTER               GPIO_GPIOR3

#define _EVT_NakedAttrs(evt)        _UMAP_NakedAttrs( "event." UTIL_STRY(evt) )
#define _EVT_Attrs(evt)             _UMAP_Attrs( "event." UTIL_STRY(evt) )
#define _EVT_NamePub(evt,postfix)   event_ ## evt ## _ ## postfix
#define _EVT_NamePrv(evt,postfix)   _event_ ## evt ## _ ## postfix

#define event_assert_name(evt)      _EVT_NamePub(evt, assert)
#define event_assert(evt)           event_assert_name(evt) ()
#define event_clear_name(evt)       _EVT_NamePub(evt, clear)
#define event_clear(evt)            event_clear_name(evt) ()
#define event_id_name(evt)          _EVT_NamePub(evt, id)
#define event_id(evt)               event_id_name(evt)

#define Event_Declare(evt, slotIndex, callback)                                               \
    static inline void event_assert_name(evt) () ATTRIBUTES(unused);                                \
    static inline void event_clear_name(evt) () ATTRIBUTES(unused);                                 \
    static inline void event_assert_name(evt) () {                                                  \
        asm(                                                                                        \
        "SBI %0, " #slotIndex " \n"                                                                 \
        : : "I" (_SFR_IO_ADDR(_EVT_REGISTER)));                                                     \
    }                                                                                               \
                                                                                                    \
    static inline void event_clear_name(evt) () {                                                   \
        asm(                                                                                        \
        "CBI %0, " #slotIndex " \n"                                                                 \
        : : "I" (_SFR_IO_ADDR(_EVT_REGISTER)));                                                     \
    }                                                                                               \
                                                                                                    \
    static const event_t event_id_name(evt) POSSIBLY_UNUSED = slotIndex;

#define Event_Define(evt, slotIndex, callback)                                                \
    extern const event_handler_t _event_slot_ ## slotIndex ATTRIBUTES(section("events.slot" # slotIndex)); \
    const event_handler_t _event_slot_ ## slotIndex = callback;

static inline void event_reset() {
    _EVT_REGISTER = 0;
}
void event_process();
bool event_assert_index(event_t id);

#endif	/* EMMAP_H */

