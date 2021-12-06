/* 
 * File:   signal.h
 * Author: fosterb
 *
 * Created on December 5, 2021, 2:01 PM
 */

#ifndef SIGNAL_H
#define	SIGNAL_H

#include "util.h"

#ifdef	__cplusplus
extern "C" {
#endif

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

#define _SIG_Attrs(pub, pri)        _UTIL_NakedSectionAttrs( "dispatch." UTIL_STRY(pub) "." UTIL_STRY(pri) )
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

#ifdef	__cplusplus
}
#endif

#endif	/* SIGNAL_H */

