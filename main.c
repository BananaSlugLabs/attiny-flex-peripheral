#include "common.h"
#include "led.h"
#include "sys.h"


#if CONFIG_TEST_PATTERN
static void app_loop ();
SysLoop_Subscribe(app_loop, Signal_Low);

static void app_loop () {
    static uint8_t i;
    static uint8_t seq;
#if CONFIG_TEST_ABORT
    static uint8_t cycles;
#endif


    if (!led_isBusy()) {

        if (i == 0) {
            seq = (seq+1) & 0x7;
            if (seq == 0) {
                seq = 1;
            }
#if CONFIG_TEST_ABORT
            cycles ++;
#endif
        }

        Led_Color c = {};
        c.r=(seq & 1) ? i : 0;
        c.g=(seq & 2) ? i : 0;
        c.b=(seq & 4) ? i : 0;
#if CONFIG_TEST_PATTERN == DEF_TEST_PATTERN_TYPE_SINGLE_FADE
        led_set(0, &led_color_black);
        led_set(1, &c);
        led_set(2, &led_color_white);
        led_set(3, &led_color_green);
#elif CONFIG_TEST_PATTERN == DEF_TEST_PATTERN_TYPE_UNIFORM_FADE
        led_setAll(&c);
#else
#error "Unknown test pattern type."
#endif

        time_sleep(CONFIG_TEST_PATTERN_TIMESTEP);
        i++;
        led_update();
#if CONFIG_TEST_ABORT
        if (cycles >= CONFIG_TEST_ABORT) {
            sys_abort(SysAbortAssertion);
        }
#endif
    }
}
#endif