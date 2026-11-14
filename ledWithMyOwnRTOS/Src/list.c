#include "common.h"

//inclusion de librerias .h

//declaracion funciones privadas

//definicion y tipos privados

//definicion funciones privadas

//definicion funciones publicas
u16 listInit(listHandle_t list){
	portableDisableInterrupts();
	if(list == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
	
	list->current = NULL;
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
	portableEnableInterrupts();

	return LIST_OK;
}

u16 listRemove(listHandle_t list, listElementHandle_t element){
	portableDisableInterrupts();
	if(list == NULL || element == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
	
	if(element->prev)
		element->prev->next = element->next;
	else
		list->head = element->next;
		
	if(element->next)
		element->next->prev = element->prev;
	else
		list->tail = element->prev;
		
	list->size--;
	portableEnableInterrupts();

	return LIST_OK;
}

u16 listAddTail(listHandle_t list, listElementHandle_t element){
	portableDisableInterrupts();
	if(list == NULL || element == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
		
	element->prev = list->tail;
	element->next = NULL;
	
	if(list->tail)
		list->tail->next = element;
	else{
		list->head = element;
		list->current = element;
	}
		
	list->tail = element;
	list->size++;
	portableEnableInterrupts();

	return LIST_OK;
}

u16 listAddHead(listHandle_t list, listElementHandle_t element){
	portableDisableInterrupts();
	if(list == NULL || element == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
		
	element->prev = list->head;
	element->next = NULL;
	
	if(list->head)
		list->head->prev = element;
	else{
		list->tail = element;
		list->current = element;
	}
		
	list->head = element;
	list->size++;
	portableEnableInterrupts();

	return LIST_OK;
}

u16 listAddAfter(listHandle_t list, listElementHandle_t element, listElementHandle_t newElement){
	portableDisableInterrupts();
	if(list == NULL || element == NULL || newElement == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
	
	newElement->prev = element;
	newElement->next = element->next;
	
	if(element->next)
		element->next->prev = newElement;
	else
		list->tail = newElement;
	
	element->next = newElement;	
	list->size++;
	portableEnableInterrupts();

	return LIST_OK;
}

u16 listAddBefore(listHandle_t list, listElementHandle_t element, listElementHandle_t newElement){
	portableDisableInterrupts();
	if(list == NULL || element == NULL || newElement == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
	
	newElement->next = element;
	newElement->prev = element->prev;
	
	if(element->prev)
		element->prev->next = newElement;
	else
		list->head = newElement;
	
	element->prev = newElement;	
	list->size++;
	portableEnableInterrupts();

	return LIST_OK;
}

u16 listGetNext(listHandle_t list, listElementHandle_t *element){
	portableDisableInterrupts();
	if(list == NULL || element == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
		
	if(list->current == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NOT_FOUND;
	}
	
	*element = list->current->next;
	portableEnableInterrupts();

	return LIST_OK;
}

u16 listGetPrev(listHandle_t list, listElementHandle_t *element){
	portableDisableInterrupts();
	if(list == NULL || element == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
		
	if(list->current == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NOT_FOUND;
	}
	
	*element = list->current->prev;
	portableEnableInterrupts();

	return LIST_OK;
}

u16 listGetCurrent(listHandle_t list, listElementHandle_t *element){
	portableDisableInterrupts();
	if(list == NULL || element == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
	if(list->current == NULL){
		*element = NULL;
		portableEnableInterrupts();
		return LIST_ERROR_NOT_FOUND;
	}
	
	*element = list->current;
	portableEnableInterrupts();
	
	return LIST_OK;
}

u16 listMoveToHead(listHandle_t list){
	portableDisableInterrupts();
	if(list == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
		
	if(list->head == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NOT_FOUND;
	}
		
	list->current = list->head;
	portableEnableInterrupts();
	
	return LIST_OK;
}

u16 listMoveToTail(listHandle_t list){
	portableDisableInterrupts();
	if(list == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
		
	if(list->tail == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NOT_FOUND;
	}
		
	list->current = list->tail;
	portableEnableInterrupts();
	
	return LIST_OK;
}

u16 listMoveNext(listHandle_t list){
	portableDisableInterrupts();
	if(list == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
		
	if(list->current == NULL || list->current->next == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NOT_FOUND;
	}
		
	list->current = list->current->next;
	portableEnableInterrupts();
	
	return LIST_OK;
}

u16 listMovePrev(listHandle_t list){
	portableDisableInterrupts();
	if(list == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
		
	if(list->current == NULL || list->current->prev == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NOT_FOUND;
	}
		
	list->current = list->current->prev;
	portableEnableInterrupts();
	
	return LIST_OK;
}

u16 getListSize(listHandle_t list){
	portableDisableInterrupts();
	if(list == NULL){
		portableEnableInterrupts();
		return 0;
	}
	u16 size = list->size;
	portableEnableInterrupts();

	return size;
}
	
u16 listPeek(listHandle_t list, listElementHandle_t element){
	portableDisableInterrupts();
	if(list == NULL || element == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
	
	element->data = list->head->data;
	element->prev = list->head->prev;
	element->next = list->head->next;
	portableEnableInterrupts();
	
	return LIST_OK;
}

u16 listClear(listHandle_t list){
	portableDisableInterrupts();
	if(list == NULL){
		portableEnableInterrupts();
		return LIST_ERROR_NULL_PARAM;
	}
		
	list->current = NULL;
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
	portableEnableInterrupts();

	return LIST_OK;
}