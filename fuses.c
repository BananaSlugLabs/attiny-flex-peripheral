#include <avr/io.h>
#include "device_config.h"

#if CONFIG_DEVICE == DEF_DEVICE_ATTINY402
#define DFP_BOD_WORKAROUND_FREQ SAMPFREQ_1KHz_gc // case sensitive typo...
#else
#define DFP_BOD_WORKAROUND_FREQ SAMPFREQ_1KHZ_gc
#endif

/**
 * \Configures Fuse bits
 */
FUSES =
{
	.APPEND = 0,
	.BODCFG = ACTIVE_DIS_gc | LVL_BODLEVEL0_gc | DFP_BOD_WORKAROUND_FREQ | SLEEP_DIS_gc,
	.BOOTEND = 0,
	.OSCCFG = FREQSEL_20MHZ_gc,
	.SYSCFG0 = CRCSRC_NOCRC_gc | RSTPINCFG_UPDI_gc,
	.SYSCFG1 = SUT_64MS_gc,
	.WDTCFG = PERIOD_OFF_gc | WINDOW_OFF_gc,
};
