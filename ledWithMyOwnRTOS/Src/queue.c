#include "common.h"
#include "portable.h"

//inclusion de librerias .h

//declaracion funciones privadas

//definicion y tipos privados

//definicion funciones privadas

//definicion funciones publicas
u16 queueInit(queueHandle_t queue){
	portableDisableInterrupts();//se debe proteger la edicion de la cola para que una interrupcion no la corrompa
	if(queue == NULL)
		return QUEUE_ERROR_NULL_PARAM;
		
	queue->head = NULL;
	queue->tail = NULL;
	queue->size = 0;
	portableEnableInterrupts();
	return QUEUE_OK;
}

u16 dequeue(queueHandle_t queue, queueElementHandle_t *element){
	portableDisableInterrupts();//se debe proteger la edicion de la cola para que una interrupcion no la corrompa
	if(queue == NULL || element == NULL){
		portableEnableInterrupts();
		return QUEUE_ERROR_NULL_PARAM;
	}
	
	if(queue->size == 0 || queue->head == NULL){
		portableEnableInterrupts();
		return QUEUE_ERROR_EMPTY;
	}
	
	*element = queue->head;
	queue->head = queue->head->next;
	queue->size--;
	if(queue->size == 0 || queue->head == NULL){
		queue->tail = NULL;
		queue->head = NULL;
		queue->size = 0;
	}
	portableEnableInterrupts();

	return QUEUE_OK;
}

u16 enqueue(queueHandle_t queue, queueElementHandle_t element){
	portableDisableInterrupts();//se debe proteger la edicion de la cola para que una interrupcion no la corrompa
	if(queue == NULL || element == NULL){
		portableEnableInterrupts();
		return QUEUE_ERROR_NULL_PARAM;
	}
	
	element->next = NULL;
	if(queue->tail == NULL){
		queue->head = element;
		queue->tail = element;
	} else {
		queue->tail->next = element;//TODO: no seria mas bien el actual tail? ya que llega uno nuevo, ek next no puede ser él mismo
		//queue->tail->next = queue->tail;//pienso yo
		queue->tail = element;
	}
	queue->size++;
	portableEnableInterrupts();

	return QUEUE_OK;
}

u16 getQueueSize(queueHandle_t queue){
	portableDisableInterrupts();//se debe proteger la edicion de la cola para que una interrupcion no la corrompa
	if(queue == NULL){
		portableEnableInterrupts();
		return 0;
	}
	u16 size = queue->size;
	portableEnableInterrupts();
	return size;
}

u16 queuePeek(queueHandle_t queue, queueElementHandle_t element){
	portableDisableInterrupts();//se debe proteger la edicion de la cola para que una interrupcion no la corrompa
	if(queue == NULL || element == NULL){
		portableEnableInterrupts();
		return QUEUE_ERROR_NULL_PARAM;
	}
	
	if(queue->size == 0 || queue->head == NULL){
		portableEnableInterrupts();
		return QUEUE_ERROR_EMPTY;
	}
	//guardamos una copia
	element->data = queue->head->data;
	element->next = queue->head->next;
	portableEnableInterrupts();
	return QUEUE_OK;
}