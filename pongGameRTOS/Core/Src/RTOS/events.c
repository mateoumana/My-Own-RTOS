#include "common.h"
//inclusion de librerias .h

//declaracion funciones privadas

//definicion y tipos privados
#define EVENTS_WAIT_TYPE_BIT            0X8000
#define EVENTS_WAIT_TYPE_MASK           0X7FFF

//definicion funciones privadas

//definicion funciones publicas
u16 eventsInit(eventHandler_t event, u16 init){
    if(event == NULL){
        return EVENT_ERROR_NULL_PARAM;
    }
    init &= EVENTS_WAIT_TYPE_MASK; //limitar a 15 bits
    event->events = init;
    return queueInit(&event->waitQueue);
}


u16 eventsSetEvent(eventHandler_t event, u16 bits){
    u16 res = EVENT_ERROR_NULL_PARAM;
    if(event != NULL){
        u8 callScheduler = FALSE;
        portableDisableInterrupts();
        bits &= EVENTS_WAIT_TYPE_MASK; //limitar a 15 bits
        event->events |= bits;
        queue_t temp;
        res = queueInit(&temp);
        if(res == QUEUE_OK){
            TCBHandle_t currentTask = schedulerGetCurrentTask();
            while(getQueueSize(&event->waitQueue) > 0){
                queueElementHandle_t element = NULL;
                res = dequeue(&event->waitQueue, &element);
                if(res == QUEUE_OK){
                    if(element != NULL){
                        TCBHandle_t taskToWakeUp = (TCBHandle_t)element->data;
                        if(taskToWakeUp != NULL){
                            if(taskToWakeUp->taskEvents & EVENTS_WAIT_TYPE_BIT){
                                u16 mask = taskToWakeUp->taskEvents & EVENTS_WAIT_TYPE_MASK;
                                if((mask & event->events) == mask){
                                    res = schedulerAddTaskToReadyQueue(taskToWakeUp);
                                    if(currentTask != NULL){
                                        if(taskToWakeUp->taskPriority < currentTask->taskPriority){
                                            callScheduler = TRUE;
                                        }
                                    }
                                    taskToWakeUp = NULL;
                                }
                            } else {
                                if(taskToWakeUp->taskEvents & event->events){
                                    res = schedulerAddTaskToReadyQueue(taskToWakeUp);
                                    if(currentTask != NULL){
                                        if(taskToWakeUp->taskPriority < currentTask->taskPriority){
                                            callScheduler = TRUE;
                                        }
                                    }
                                    taskToWakeUp = NULL;
                                }
                            }
                        } else {
                            res = SEMAPHORE_ERROR_OPERATION_FAILED; 
                        }
                        if(taskToWakeUp != NULL){
                            res = enqueue(&temp, &taskToWakeUp->taskQueueElement);
                        }
                    }
                }
            }
            res = queueCopy(&temp, &event->waitQueue);
        }
        portableEnableInterrupts();
        if(callScheduler){
            portableCallScheduller();
        }
    }
    
    return res;
}


u16 eventsClearEvent(eventHandler_t event, u16 bits){
    u16 res = EVENT_ERROR_NULL_PARAM;
    if(event != NULL){
        portableDisableInterrupts();
        bits = ~bits;
        event->events &= bits;
        event->events &= EVENTS_WAIT_TYPE_MASK; //limitar a 15 bits
        portableEnableInterrupts();
        res = EVENT_OK;
    }
    return res;
}


/***
 * @brief Waits for any of the specified events to occur, for example, if the event handler has bits 0 and 1 set, 
 * and the task is waiting for either bit 0 or bit 1, it will be unblocked when either of those events occurs.
 * @details This function blocks the current task until any of the specified events occur.
 * @param event Event handler
 * @param bits Bits representing the events to wait for
 * @return Result of the operation
 */
u16 eventsWaitAny(eventHandler_t event, u16 bits){
    u16 res = EVENT_ERROR_NULL_PARAM;
    if(event != NULL){
        bits &= EVENTS_WAIT_TYPE_MASK; //limitar a 15 bits
        if(bits){
            portableDisableInterrupts();
            TCBHandle_t currentTask = schedulerGetCurrentTask();
            if(currentTask != NULL){
                currentTask->taskEvents = bits;
                currentTask->taskStatus = TASK_BLOCKED;
                res = enqueue(&event->waitQueue, &currentTask->taskQueueElement);
                if(res != QUEUE_OK){
                    currentTask->taskEvents = 0;
                    currentTask->taskStatus = TASK_RUNNING;
                }
            } else {
                res = EVENT_ERROR_INVALID_STATE;
            }
            portableEnableInterrupts();
            if(res == QUEUE_OK){
                portableCallScheduller();
            }
        } else {
            res = EVENT_ERROR_INVALID_PARAM;
        }
    }
    return res;
}


/***
 * @brief Waits for all of the specified events to occur. For example, if the event handler has bits 0 and 1 set, 
 * the task has to wait for both bit 0 and bit 1. It will be unblocked only when both events occur.
 * @details This function blocks the current task until all of the specified events occur.
 * @param event Event handler
 * @param bits Bits representing the events to wait for
 * @return Result of the operation
 */
u16 eventsWaitAll(eventHandler_t event, u16 bits){
    u16 res = EVENT_ERROR_NULL_PARAM;
    if(event != NULL){
        bits &= EVENTS_WAIT_TYPE_MASK; //limitar a 15 bits
        if(bits){
            portableDisableInterrupts();
            TCBHandle_t currentTask = schedulerGetCurrentTask();
            if(currentTask != NULL){
                currentTask->taskEvents = bits;
                currentTask->taskEvents |= EVENTS_WAIT_TYPE_BIT; //indicar que se espera a todos los bits
                currentTask->taskStatus = TASK_BLOCKED;
                res = enqueue(&event->waitQueue, &currentTask->taskQueueElement);
                if(res != QUEUE_OK){
                    currentTask->taskEvents = 0;
                    currentTask->taskStatus = TASK_RUNNING;
                }
            } else {
                res = EVENT_ERROR_INVALID_STATE;
            }
            portableEnableInterrupts();
            if(res == QUEUE_OK){
                portableCallScheduller();
            }
        } else {
            res = EVENT_ERROR_INVALID_PARAM;
        }
    }
    return res;
}