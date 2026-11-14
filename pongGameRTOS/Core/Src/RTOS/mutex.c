#include "common.h"

//inclusion de librerias .h

//declaracion funciones privadas

//definicion y tipos privados

//definicion funciones privadas

//definicion funciones publicas

u16 MutexInit(MutexHandler_t mutex){
    if(mutex != NULL){
        mutex->busy = 0;
        return queueInit(&mutex->waitQueue);
    }
    return MUTEX_ERROR_NULL_PARAM;
}

u16 MutexLock(MutexHandler_t mutex){//take
    u16 res = MUTEX_ERROR_NULL_PARAM;
    if(mutex != NULL){
        u8 callScheduler = FALSE;
        portableDisableInterrupts();
        if(mutex->busy){
            TCBHandle_t currentTask = schedulerGetCurrentTask();
            if(currentTask != NULL){
                currentTask->taskStatus = TASK_BLOCKED;
                currentTask->taskQueueElement.data = (pv)currentTask;
                res = enqueue(&mutex->waitQueue, &currentTask->taskQueueElement);
                if(res != QUEUE_OK){
                    currentTask->taskStatus = TASK_RUNNING;
                } else {
                    callScheduler = TRUE;
                }
            } else {
                res = MUTEX_ERROR_OPERATION_FAILED; 
            }
        }else{
            mutex->busy = TRUE;
            res = MUTEX_OK;
        }
        portableEnableInterrupts();
        if(callScheduler){
            portableCallScheduller();
        }
    }
    return res;
}

u16 MutexUnlock(MutexHandler_t mutex){//release
u16 res = MUTEX_ERROR_NULL_PARAM;
    if(mutex != NULL){
        u8 callScheduler = FALSE;
        portableDisableInterrupts();
        if(mutex->busy){
            if(getQueueSize(&mutex->waitQueue) > 0){
                queueElementHandle_t element = NULL;
                res = dequeue(&mutex->waitQueue, &element);
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
                            res = MUTEX_ERROR_OPERATION_FAILED; 
                        }
                    } else {
                        res = MUTEX_ERROR_OPERATION_FAILED; 
                    }
                }
            } else{
                mutex->busy = FALSE;
                res = MUTEX_OK;
            }
        }else{
            res = MUTEX_OK;
        }
        portableEnableInterrupts();
        if(callScheduler){
            portableCallScheduller();
        }
    }
    return res;
}