#include "Task.h"
#include "common.h"
#include "myTypes.h"

//inclusion de librerias .h

//declaracion funciones privadas
static u32 taskInitStack(pu8 taskStack, u16 taskStackSize, u32 taskFunction);
static u16 taskCreateIdleTask(void);
static void taskIdleTaskFunction(void);
TCBHandle_t taskGetIdleTask(void);

//definicion y tipos privados
static u16 taskIdCounter = 0;
static list_t taskList;
static TCB_t idleTask;// = NULL;
static u8 idleTaskStack[RTOS_MIN_STACK_SIZE] __attribute__((aligned(4)));
static const char *idleTaskName = "IdleTask";

//definicion funciones privadas
u32 taskInitStack(pu8 taskStack, u16 taskStackSize, u32 taskFunction){
	if(taskStack == NULL || taskStackSize < RTOS_MIN_STACK_SIZE || taskFunction == 0)
		return 0;
	
	u32 topOfStack = portableCreateContext(taskStack, taskStackSize, taskFunction);
	return topOfStack;//stack pointer
}

u16 taskCreateIdleTask(void){
	portableDisableInterrupts();
	/*if(idleTaskStack == NULL){
		portableEnableInterrupts();
		return TASK_ERROR_FAIL;
	}*/
	
	idleTask.taskFunction = taskIdleTaskFunction;
	idleTask.taskPriority = RTOS_MAX_TASK_PRIORITIES;
	idleTask.taskStack = idleTaskStack;
	idleTask.taskStackSize = RTOS_MIN_STACK_SIZE;
	idleTask.taskName = idleTaskName;
	idleTask.taskStatus = TASK_READY;
	idleTask.taskQueueElement.data = (void*) &idleTask;//TODO: revisar bien
	idleTask.taskListElement.data = (void*) &idleTask;
	idleTask.taskId = taskIdCounter++;
	idleTask.taskSleepTime = 0;
	idleTask.taskStackPointer = taskInitStack(idleTaskStack, RTOS_MIN_STACK_SIZE, (u32)taskIdleTaskFunction);
	
	if(idleTask.taskStackPointer == 0){
		portableEnableInterrupts();
		return TASK_ERROR_FAIL;
	}
	portableEnableInterrupts();

	return TASK_OK;
}

TCBHandle_t taskGetIdleTask(void){
	return &idleTask;
}

void taskIdleTaskFunction(void){
	u64 systemTicks = 0;
	while(1){
		if(systemTicks != portableGetSystemTicks()){
			u8 callScheduler = FALSE;
			portableDisableInterrupts();
			u64 ticksChange = (u64)(portableGetSystemTicks() - systemTicks);
			systemTicks = portableGetSystemTicks();
			listMoveToHead(&taskList);
			listElementHandle_t element = NULL;
			u16 res = LIST_OK;
			do{
				res = listGetCurrent(&taskList, &element);
				if(element != NULL){
					TCBHandle_t task = (TCBHandle_t)element->data;
					if(task != NULL){
						if(task->taskSleepTime){
							if(task->taskSleepTime > ticksChange){
								task->taskSleepTime -= ticksChange;
							} else {
								task->taskSleepTime = 0;
							}
							if(task->taskSleepTime == 0 /*&& task->taskStatus == TASK_BLOCKED*/){
								schedulerAddTaskToReadyQueue(task);
								callScheduler = TRUE;
							}
						}
					}
					res = listMoveNext(&taskList);
				}
			}while((res == LIST_OK) && (element != NULL) /*&& ((TCBHandle_t)element->data)->taskStatus != TASK_BLOCKED*/);
			portableEnableInterrupts();
			if(callScheduler){
				portableCallScheduller();
			}
		}
	}
}


//definicion funciones publicas
u16 taskInit(void){
	portableDisableInterrupts();
	u16 result = TASK_OK;
	result = listInit(&taskList);
	if(result != TASK_OK) {
		portableEnableInterrupts();
		return result;
	}
		
	result = taskCreateIdleTask();
	portableEnableInterrupts();
	return result;
}

u16 taskCreate(TCB_t *taskControlBlock, taskFunctionCallback taskFunction, u8 taskPriority, pu8 taskStack, u16 taskStackSize, const char *taskName){
	u16 result = TASK_OK;
	portableDisableInterrupts();
	if(taskControlBlock == NULL || taskFunction == NULL || taskStack == NULL || taskName == NULL){
		portableEnableInterrupts();
		return TASK_ERROR_NULL_PARAM;
	}

	if(taskStack == NULL){
		portableEnableInterrupts();
		return TASK_ERROR_FAIL;
	}
		
	if(taskStackSize < RTOS_MIN_STACK_SIZE){
		portableEnableInterrupts();
		return TASK_ERROR_INVALID_PARAM;
	}
		
	if(taskPriority > RTOS_MIN_PRIORITY)
		taskPriority = RTOS_MIN_PRIORITY;
	
	taskControlBlock->taskFunction = taskFunction;
	taskControlBlock->taskPriority = taskPriority;
	taskControlBlock->taskStack = taskStack;
	taskControlBlock->taskStackSize = taskStackSize;
	taskControlBlock->taskName = taskName;
	taskControlBlock->taskStatus = TASK_READY;
	taskControlBlock->taskQueueElement.data = (void*) taskControlBlock;
	taskControlBlock->taskListElement.data = (void*) taskControlBlock;
	taskControlBlock->taskId = taskIdCounter++;
	taskControlBlock->taskSleepTime = 0;
	taskControlBlock->taskStackPointer = taskInitStack(taskStack, taskStackSize, (u32)taskFunction);
	
	if(taskControlBlock->taskStackPointer == 0){
		portableEnableInterrupts();
		return TASK_ERROR_FAIL;
	}
	
	result = schedulerAddTaskToReadyQueue(taskControlBlock);//enviar señal al scheduler
	if(result != TASK_OK) {
		portableEnableInterrupts();
		return result;
	}
	
	result = listAddTail(&taskList, (listElementHandle_t)&taskControlBlock->taskListElement);
	portableEnableInterrupts();

	return result;
}

u16 getTaskIdByName(const char *taskName){
	portableDisableInterrupts();
	if(taskName == NULL){
		portableEnableInterrupts();
		return TASK_ERROR_NULL_PARAM;
	}
	portableEnableInterrupts();

	return TASK_OK;	
}

u16 taskDelete(TCB_t *taskControlBlock){
	portableDisableInterrupts();
	if(taskControlBlock == NULL){
		portableEnableInterrupts();
		return TASK_ERROR_NULL_PARAM;
	}
	portableEnableInterrupts();

	return TASK_OK;
}

u16 taskStart(TCB_t *taskControlBlock){
	portableDisableInterrupts();
	if(taskControlBlock == NULL){
		portableEnableInterrupts();
		return TASK_ERROR_NULL_PARAM;
	}
	portableEnableInterrupts();

	return TASK_OK;
}

u16 taskStop(TCB_t *taskControlBlock){
	portableDisableInterrupts();
	if(taskControlBlock == NULL){
		portableEnableInterrupts();
		return TASK_ERROR_NULL_PARAM;
	}
	portableEnableInterrupts();

	return TASK_OK;
}

u16 taskSuspend(TCB_t *taskControlBlock){
	portableDisableInterrupts();
	if(taskControlBlock == NULL){
		portableEnableInterrupts();
		return TASK_ERROR_NULL_PARAM;
	}
	portableEnableInterrupts();

	return TASK_OK;
}

u16 taskResume(TCB_t *taskControlBlock){
	portableDisableInterrupts();
	if(taskControlBlock == NULL){
		portableEnableInterrupts();
		return TASK_ERROR_NULL_PARAM;
	}
	portableEnableInterrupts();

	return TASK_OK;
}

u16 taskYield(TCB_t *taskControlBlock){
	portableDisableInterrupts();
	if(taskControlBlock == NULL){
		portableEnableInterrupts();
		return TASK_ERROR_NULL_PARAM;
	}
	portableEnableInterrupts();

	return TASK_OK;
}

/***
	* @brief Delays the execution of the current task for a specified number of ticks
	* @details This function blocks the current task for a specified number of ticks, allowing other tasks to run. If the number of ticks is zero, the task will yield the CPU to other tasks of equal priority.
	* @param ticks Number of ticks to delay the task, each tick is 1ms by default, but it can be configured in the portable.c file. The tick frequency is defined by the macro PORTABLE_RTOS_SYSTEM_CLOCK in portable.h.
	* @return TASK_OK if the task was successfully delayed
*/
u16 taskDelay(u32 ticks){
	u8 callScheduler = FALSE;
	u16 res = TASK_ERROR_NULL_PARAM;
	portableDisableInterrupts();
	if(ticks){
		TCBHandle_t task = schedulerGetCurrentTask();
		if(task != NULL){
			if(task->taskPriority < RTOS_MAX_TASK_PRIORITIES){
				task->taskSleepTime = ticks;
				task->taskStatus = TASK_BLOCKED;
				callScheduler = TRUE;
			}//si es la tarea idle, todo ok, no se bloquea
			res = TASK_OK;			
		}
	} else {
		callScheduler = TRUE;
		res = TASK_OK;
	}
	portableEnableInterrupts();
	if(callScheduler){//llamada al scheduler por interrupcion por software
		portableCallScheduller();
	}

	return res;
}