#include "common.h"

//inclusion de librerias .h
extern TCBHandle_t taskGetIdleTask(void);//accedemos a la funcion definida en task.c, oara ser usada no puede ser definida como static
//declaracion funciones privadas


//definicion y tipos privados
static queue_t schedulerQueues[RTOS_MAX_TASK_PRIORITIES];
static TCBHandle_t currentTask = NULL;

//definicion funciones privadas


//definicion funciones publicas
u16 schedulerInit(void){
	portableDisableInterrupts();
	u16 result = SCHEDULER_OK;
	for(u16 i=0; i<RTOS_MAX_TASK_PRIORITIES; i++){
		result = queueInit(&schedulerQueues[i]);
		if(result != QUEUE_OK) break;
	}
	portableEnableInterrupts();
	
	return result;
}

u16 schedulerAddTaskToReadyQueue(TCBHandle_t task){
	u16 result = SCHEDULER_OK;
	portableDisableInterrupts();
	if(task == NULL){
		portableEnableInterrupts();
		return SCHEDULER_ERROR_NULL_PARAM;
	}
	if(task->taskPriority >= RTOS_MAX_TASK_PRIORITIES){
		portableEnableInterrupts();
		return SCHEDULER_ERROR_INVALID_PARAM;
	}
	
	task->taskStatus = TASK_READY;
	result = enqueue(&schedulerQueues[task->taskPriority], &task->taskQueueElement);
	portableEnableInterrupts();
	return result;
}

//devuelve el stack pointer de la tarea nueva para restaurar el contexto
u32 schedulerGetNextTask(u32 currentTaskStackPointer){
	if((void *)currentTaskStackPointer != NULL){
		if(currentTask != NULL){
			currentTask->taskStackPointer = currentTaskStackPointer;
			if(currentTask->taskPriority < RTOS_MAX_TASK_PRIORITIES){//cuando no es la idle task, se agrega a la cola de tareas listas
				if(currentTask->taskStatus == TASK_RUNNING){
					schedulerAddTaskToReadyQueue(currentTask);
				}
			} else
				currentTask->taskStatus = TASK_READY; //es cuando la tarea es la idle task, que no se agrega a la cola de tareas listas, sino que se mantiene en ejecución hasta que llegue una tarea de mayor prioridad
		}
	}
	currentTask = NULL;
	for(u16 i = 0; i < RTOS_MAX_TASK_PRIORITIES; i++){
		if(getQueueSize(&schedulerQueues[i]) != 0){
			queueElementHandle_t element = NULL;
			if(dequeue(&schedulerQueues[i], &element) == QUEUE_OK){//tuve que quitar el & por que era como un puntero triple
				if(element == NULL) continue;
				currentTask = (TCBHandle_t)(element->data);//faltaba dererenciar el puntero para obtener el TCB de la tarea
				break;
			}
		}		
	}
	if(currentTask == NULL) //si no hay tareas por lanzar, el scheduler lanza una default mientras llega una tarea, la tarea inaxtiva (conocida en rtos como idle task)
		currentTask = taskGetIdleTask();
	
	currentTask->taskStatus = TASK_RUNNING;
	return currentTask->taskStackPointer;			
}

TCBHandle_t schedulerGetCurrentTask(void){
	return currentTask;
}