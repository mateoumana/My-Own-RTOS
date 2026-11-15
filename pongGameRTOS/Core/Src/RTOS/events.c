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
                                if(taskToWakeUp->taskEvents | event->events){
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


/*u16 eventsClearEvent(eventHandler_t event, u16 bits){}


u16 eventsWaitAny(eventHandler_t event, u16 bits){}


u16 eventsWaitAll(eventHandler_t event, u16 bits){}*/