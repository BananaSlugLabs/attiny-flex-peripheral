#include <avr/io.h>

/**
 * \Configures Fuse bits
 */
FUSES = 
{
	.APPEND = 0,
	.BODCFG = ACTIVE_DIS_gc | LVL_BODLEVEL0_gc | SAMPFREQ_1KHz_gc | SLEEP_DIS_gc,
	.BOOTEND = 0,
	.OSCCFG = FREQSEL_20MHZ_gc,
	.SYSCFG0 = CRCSRC_NOCRC_gc | RSTPINCFG_UPDI_gc,
	.SYSCFG1 = SUT_64MS_gc,
	.WDTCFG = PERIOD_OFF_gc | WINDOW_OFF_gc,
};
