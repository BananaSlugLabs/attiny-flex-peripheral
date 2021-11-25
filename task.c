#include "sys.h"
#include "device_config.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>

extern const Task                       _TaskDescriptors;
extern const uint8_t                    _TaskDescriptorsCount;
#define Task_GetDescriptor(index)       (((Task*)&_TaskDescriptors)[index])
#define Task_Count()                    ((uint8_t)((uintptr_t)&_TaskDescriptorsCount))

#if CONFIG_MESSAGE_QUEUE > 0
#define MQ_MAX                          (1<<CONFIG_MESSAGE_QUEUE)
#define MQ_MASK                         ((1<<CONFIG_MESSAGE_QUEUE)-1)
volatile uint8_t mq_head;
volatile uint8_t mq_count;
struct {
    task_t          task;
    message_t       message;
    MessageData     data;
} mq_queue [CONFIG_MESSAGE_QUEUE];
#endif

void message_broadcastNow (message_t message, MessageData data) {
    for (uint8_t task = 0; task < Task_Count(); task ++) {
        Task_GetDescriptor(task).entryPoint(message, data);
    }
}

void message_send (task_t task, message_t message, MessageData data) {
    if (task_valid(task)) {
        Task_GetDescriptor(task).entryPoint(message, data);
    }
}

#if CONFIG_MESSAGE_QUEUE > 0
#error Untested
void message_queue (task_t task, message_t message, MessageData data) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        message_queueFromIrq(task, message, data);
    }
}

void message_queueFromIrq (task_t task, message_t message, MessageData data) {
    if (mq_count == MQ_MAX) {
        sys_abort(SysAbortMessageQueueExhausted);
    }
    
    mq_queue[mq_head].task      = task;
    mq_queue[mq_head].message   = message;
    mq_queue[mq_head].data      = data;
    mq_head                     = (mq_head + 1) & MQ_MASK;
    mq_count ++;
}

void message_dequeueAll () {
    for (;;) {
        uint8_t cnt;
        uint8_t index;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            cnt = mq_count;
            if (cnt == 0) {
                return;
            }
            index = (mq_head - cnt);
            cnt --;
            mq_count = cnt;
        }
        index &= MQ_MASK;
        if (mq_queue[index].task <= Task_Count()) {
            Task_GetDescriptor(mq_queue[index].task).entryPoint(mq_queue[index].message, mq_queue[index].data);
        } else {
            sys_abort(SysAbortBadTask);
        }
    }
}
#endif