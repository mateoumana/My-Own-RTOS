#include "common.h"


//inclusion de librerias .h

//declaracion funciones privadas

//definicion y tipos privados

//definicion funciones privadas

//definicion funciones publicas

u16 semaphoreInit(semaphoreHandler_t semaphore, u16 initialCount){
    if(semaphore != NULL){
        semaphore->busy = 0;
        return queueInit(&semaphore->waitQueue);
    }
    return SEMAPHORE_ERROR_NULL_PARAM;
}

u16 semaphoreLock(semaphoreHandler_t semaphore){//take
    u16 res = SEMAPHORE_ERROR_NULL_PARAM;
    if(semaphore != NULL){
        u8 callScheduler = FALSE;
        portableDisableInterrupts();
        if(semaphore->count > semaphore->maxCount){
            TCBHandle_t currentTask = schedulerGetCurrentTask();
            if(currentTask != NULL){
                currentTask->taskStatus = TASK_BLOCKED;
                currentTask->taskQueueElement.data = (pv)currentTask;
                res = enqueue(&semaphore->waitQueue, &currentTask->taskQueueElement);
                if(res != QUEUE_OK){
                    currentTask->taskStatus = TASK_RUNNING;
                } else {
                    callScheduler = TRUE;
                }
            } else {
                res = SEMAPHORE_ERROR_NULL_PARAM; 
            }
        }else{
            semaphore->count++;
            res = SEMAPHORE_OK;
        }
        portableEnableInterrupts();
        if(callScheduler){
            portableCallScheduller();
        }
    }
    return res;
}

u16 semaphoreUnlock(semaphoreHandler_t semaphore){//release
    u16 res = SEMAPHORE_ERROR_NULL_PARAM;
    if(semaphore != NULL){
        u8 callScheduler = FALSE;
        portableDisableInterrupts();
        if(semaphore->count > 0){
            semaphore->count--;
            if(getQueueSize(&semaphore->waitQueue) > 0){
                queueElementHandle_t element = NULL;
                res = dequeue(&semaphore->waitQueue, &element);
                if(res == QUEUE_OK){
                    if(element != NULL){
                        TCBHandle_t taskToWakeUp = (TCBHandle_t)element->data;
                        if(taskToWakeUp != NULL){
                            res = schedulerAddTaskToReadyQueue(taskToWakeUp);
                            if(res == SCHEDULER_OK){
                                TCBHandle_t currentTask = schedulerGetCurrentTask();
                                if(currentTask != NULL){
                                    if(taskToWakeUp->taskPriority < currentTask->taskPriority){
                                        callScheduler = TRUE;
                                    }
                                }
                            }
                        } else {
                            res = SEMAPHORE_ERROR_OPERATION_FAILED; 
                        }
                    } else {
                        res = SEMAPHORE_ERROR_OPERATION_FAILED; 
                    }
                }
            } else{
                semaphore->busy = FALSE;
                res = SEMAPHORE_OK;
            }
        }else{
            res = SEMAPHORE_OK;
        }
        portableEnableInterrupts();
        if(callScheduler){
            portableCallScheduller();
        }
    }
    return res;
}


u16 semaphoreGetCount(semaphoreHandler_t semaphore){
 u16 res = 0;
 portableDisableInterrupts();
    if(semaphore != NULL){
        res = semaphore->count;
    }
    portableEnableInterrupts();
    return res;
}


u16 semaphoreGetLimit(semaphoreHandler_t semaphore){
    u16 res = 0;
    portableDisableInterrupts();
    if(semaphore != NULL){
        res = semaphore->maxCount;
    }
    portableEnableInterrupts();
    return res;
}