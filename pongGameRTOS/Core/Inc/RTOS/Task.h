#ifndef _TASK_H_
#define _TASK_H_

//inclusion de librerias .h
#include "myTypes.h"
#include "queue.h"
#include "list.h"

//definiciones publicas y errores
#ifndef TASK_BASE_ERROR
#define TASK_BASE_ERROR               0X300
#endif

#define TASK_OK                        ERROR_OK
#define TASK_ERROR_FAIL                      TASK_BASE_ERROR | ERROR_FAIL
#define TASK_ERROR_UNKOWN                    TASK_BASE_ERROR | ERROR_UNKOWN
#define TASK_ERROR_NULL_PARAM                TASK_BASE_ERROR | ERROR_NULL_PARAM
#define TASK_ERROR_OUT_OF_MEMORY             TASK_BASE_ERROR | ERROR_OUT_OF_MEMORY
#define TASK_ERROR_TIMEOUT                   TASK_BASE_ERROR | ERROR_TIMEOUT
#define TASK_ERROR_NOT_FOUND                 TASK_BASE_ERROR | ERROR_NOT_FOUND
#define TASK_ERROR_INVALID_PARAM             TASK_BASE_ERROR | ERROR_INVALID_PARAM
#define TASK_ERROR_ALREADY_EXISTS            TASK_BASE_ERROR | ERROR_ALREADY_EXISTS
#define TASK_ERROR_NOT_IMPLEMENTED           TASK_BASE_ERROR | ERROR_NOT_IMPLEMENTED
#define TASK_ERROR_BUSY                      TASK_BASE_ERROR | ERROR_BUSY
#define TASK_ERROR_NOT_SUPPORTED             TASK_BASE_ERROR | ERROR_NOT_SUPPORTED
#define TASK_ERROR_HARDWARE_FAILURE          TASK_BASE_ERROR | ERROR_HARDWARE_FAILURE
#define TASK_ERROR_SOFTWARE_FAILURE          TASK_BASE_ERROR | ERROR_SOFTWARE_FAILURE
#define TASK_ERROR_COMMUNICATION_FAILURE     TASK_BASE_ERROR | ERROR_COMMUNICATION_FAILURE
#define TASK_ERROR_SECURITY_FAILURE          TASK_BASE_ERROR | ERROR_SECURITY_FAILURE
#define TASK_ERROR_EMPTY                     TASK_BASE_ERROR | ERROR_EMPTY
#define TASK_ERROR_INVALID_STATE             TASK_BASE_ERROR | ERROR_INVALID_STATE
#define TASK_ERROR_NOT_READY                 TASK_BASE_ERROR | ERROR_NOT_READY
#define TASK_ERROR_INVALID_OPERATION         TASK_BASE_ERROR | ERROR_INVALID_OPERATION
#define TASK_ERROR_RESOURCE_UNAVAILABLE      TASK_BASE_ERROR | ERROR_RESOURCE_UNAVAILABLE
#define TASK_ERROR_OPERATION_FAILED          TASK_BASE_ERROR | ERROR_OPERATION_FAILED
#define TASK_ERROR_DATA_CORRUPTION           TASK_BASE_ERROR | ERROR_DATA_CORRUPTION
#define TASK_ERROR_FULL                        TASK_BASE_ERROR | ERROR_FULL

//tipos publicos
typedef void (*taskFunctionCallback)(void);

typedef enum{
	TASK_READY=0, 
	TASK_RUNNING, 
	TASK_BLOCKED, 
	TASK_LAST
} taskStatus_t;

typedef struct TCB{
	u16 taskId;
	const char *taskName;
	taskStatus_t taskStatus;
	u8 taskPriority;
	pu8 taskStack;
	u16 taskStackSize;
	u32 taskStackPointer;
	u32 taskSleepTime;
	u16 taskEvents;
	queueElement_t taskQueueElement;
	listElement_t taskListElement;
	taskFunctionCallback taskFunction;
	
}TCB_t, * TCBHandle_t;

//funciones publicas
u16 taskInit(void);// para inicializar la lista
u16 taskCreate(TCB_t *taskControlBlock, taskFunctionCallback taskFunction, u8 taskPriority, pu8 taskStack, u16 taskStackSize, const char *taskName);
u16 getTaskIdByName(const char *taskName);
u16 taskDelete(TCB_t *taskControlBlock);
u16 taskStart(TCB_t *taskControlBlock);
u16 taskStop(TCB_t *taskControlBlock);
u16 taskSuspend(TCB_t *taskControlBlock);
u16 taskResume(TCB_t *taskControlBlock);
u16 taskYield(TCB_t *taskControlBlock);
u16 taskDelay(u32 ticks);

#endif