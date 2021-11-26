/* 
 * File:   tasks.h
 * Author: fosterb
 *
 * Created on November 24, 2021, 11:14 PM
 */

#ifndef TASKS_H
#define	TASKS_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include "util.h"

#define TaskNone        0xFF
typedef uint8_t         task_t;
    
enum {
    SystemMessage_InitIo    = 0x0,
    SystemMessage_EarlyInit = 0x1,
    SystemMessage_Init      = 0x2,
    SystemMessage_Start     = 0x3,
    SystemMessage_Finit     = 0x4,
    SystemMessage_Loop      = 0x5,
    SystemMessage_Abort     = 0x6,
    
    BusMessage_SignalBase   = 0x20, // Strictly speaking 0x20 will never be used because Signal 0 is a no-op
    BusMessage_SignalLast   = BusMessage_SignalBase + 7,
};
typedef uint8_t message_t;

typedef union MessageDataTag {
    uint8_t             u8;
    uint8_t             u8a[2];
    uint16_t            u16;
    int8_t              i8;
    int8_t              i8a[2];
    int16_t             i16;
    void*               ptr;
} MessageData;

#define MessageData_Empty       (MessageData)((uint8_t)0)
#define MessageData_U16(u)      (MessageData)((uint16_t)u)
#define MessageData_U8(u)       (MessageData)((uint8_t)u)
#define MessageData_I16(i)      (MessageData)((uint16_t)i)
#define MessageData_I8(i)       (MessageData)((uint8_t)i)
#define MessageData_PTR(i)      (MessageData)((void*)ptr)

#define BusMessage_Signal(index) (BusMessage_SignalBase+((index)&0x7))


typedef struct TaskTag {
    void (*entryPoint) (message_t message, MessageData data);
} Task;

#define PUBLIC_TASK_DECLARE(taskEntryPoint, pri)                            \
    extern LINKER_DESCRIPTOR_ID(const Task, "task", taskEntryPoint, pri);

#define PUBLIC_TASK_DEFINE(taskEntryPoint, pri)                             \
    LINKER_DESCRIPTOR_DATA(const Task, "task", taskEntryPoint, pri) =       \
        {                                                                   \
            .entryPoint           = taskEntryPoint,                         \
        };                                                                  \
    LINKER_DESCRIPTOR_ID_NOATTR(const Task, "task", taskEntryPoint, pri);

#define PRIVATE_TASK_DEFINE(taskEntryPoint, pri)                            \
    LINKER_DESCRIPTOR_DATA(const Task, "task", taskEntryPoint, pri) =       \
        {                                                                   \
            .entryPoint           = taskEntryPoint,                         \
        };                                                                  \
    LINKER_DESCRIPTOR_ID(const Task, "task", taskEntryPoint, pri);

#define GET_TASK_ID(taskEntryPoint)         ((uint8_t)((uintptr_t)&LINKER_DESCRIPTOR_ID_NAME(taskEntryPoint)))
#define GET_TASK_ID_WIDE(taskEntryPoint)    ((uintptr_t)&LINKER_DESCRIPTOR_ID_NAME(taskEntryPoint))
#define GET_TASK_ID_RAW(taskEntryPoint)     LINKER_DESCRIPTOR_ID_NAME(taskEntryPoint)

/**
 * Sends a message to all tasks. May not be called during IRQ.
 * 
 * @param message
 * @param data
 */
void message_broadcastNow (message_t message, MessageData data);

/**
 * Sends a message to a specific task.
 * 
 * @param task
 * @param message
 * @param data
 */
void message_send (task_t task, message_t message, MessageData data);

static inline bool task_valid (task_t task) {
    extern const uint8_t _TaskDescriptorsCount;
    return task < ((uint8_t)((uintptr_t)&_TaskDescriptorsCount));
}

#if CONFIG_MESSAGE_QUEUE > 0
/**
 * Queues a message for a task.
 * 
 * @param task
 * @param message
 * @param data
 */
void message_queue (task_t task, message_t message, MessageData data);

/**
 * Queues a message for a task from an IRQ.
 * 
 * @param task
 * @param message
 * @param data
 */
void message_queueFromIrq (task_t task, message_t message, MessageData data);

/**
 * Dispatch all queued messages.
 */
void message_dequeueAll ();
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* TASKS_H */

