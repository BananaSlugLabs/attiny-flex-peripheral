#include "common.h"
#include "led.h"
#include "sys.h"

#if !defined(CONFIG_TEST_PATTERN_TIMESTEP) || CONFIG_TEST_PATTERN_TIMESTEP <= 0
#define _CONFIG_TEST_PATTERN_TIMESTEP 25
#else
#define _CONFIG_TEST_PATTERN_TIMESTEP CONFIG_TEST_PATTERN_TIMESTEP
#endif

#if CONFIG_TEST_PATTERN
void app_task (message_t message, MessageData data);

PRIVATE_TASK_DEFINE(app_task, 5);

void app_task (message_t message, MessageData data) {
    static uint8_t i;
    static uint8_t seq;
    
    if (message == SystemMessage_Loop) {
        if (!led_isBusy()) {

            if (i == 0) {
                seq = (seq+1) & 0x7;
                if (seq == 0) {
                    seq = 1;
                } 
            }

            Led_Color c = {};
            c.r=(seq & 1) ? i : 0;
            c.g=(seq & 2) ? i : 0;
            c.b=(seq & 4) ? i : 0;
#if CONFIG_TEST_PATTERN == DEF_TEST_PATTERN_TYPE_SINGLE_FADE
            Led_Set(2, &c);
            Led_Set(1, &BuiltinPallet[BuiltInPallet_Blue]);
#elif CONFIG_TEST_PATTERN == DEF_TEST_PATTERN_TYPE_UNIFORM_FADE
            led_setAll(&c);
#else
#error "Unknown test pattern type."
#endif

            time_sleep(_CONFIG_TEST_PATTERN_TIMESTEP);
            i++;
            led_update();
#if CONFIG_TEST_ABORT
            if (seq == 3) {
                sys_abort(SysAbortAssertion);
            }
#endif
        }
    }
}
#endif