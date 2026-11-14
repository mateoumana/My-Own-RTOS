#ifndef _LIST_H_
#define _LIST_H_

//inclusion de librerias .h
#include "myTypes.h"

//definiciones publicas y errores
#ifndef LIST_BASE_ERROR
#define LIST_BASE_ERROR               0X200
#endif

#define LIST_OK                        	ERROR_OK
#define LIST_ERROR_FAIL                      LIST_BASE_ERROR | ERROR_FAIL
#define LIST_ERROR_UNKOWN                    LIST_BASE_ERROR | ERROR_UNKOWN
#define LIST_ERROR_NULL_PARAM                LIST_BASE_ERROR | ERROR_NULL_PARAM
#define LIST_ERROR_OUT_OF_MEMORY             LIST_BASE_ERROR | ERROR_OUT_OF_MEMORY
#define LIST_ERROR_TIMEOUT                   LIST_BASE_ERROR | ERROR_TIMEOUT
#define LIST_ERROR_NOT_FOUND                 LIST_BASE_ERROR | ERROR_NOT_FOUND
#define LIST_ERROR_INVALID_PARAM             LIST_BASE_ERROR | ERROR_INVALID_PARAM
#define LIST_ERROR_ALREADY_EXISTS            LIST_BASE_ERROR | ERROR_ALREADY_EXISTS
#define LIST_ERROR_NOT_IMPLEMENTED           LIST_BASE_ERROR | ERROR_NOT_IMPLEMENTED
#define LIST_ERROR_BUSY                      LIST_BASE_ERROR | ERROR_BUSY
#define LIST_ERROR_NOT_SUPPORTED             LIST_BASE_ERROR | ERROR_NOT_SUPPORTED
#define LIST_ERROR_HARDWARE_FAILURE          LIST_BASE_ERROR | ERROR_HARDWARE_FAILURE
#define LIST_ERROR_SOFTWARE_FAILURE          LIST_BASE_ERROR | ERROR_SOFTWARE_FAILURE
#define LIST_ERROR_COMMUNICATION_FAILURE     LIST_BASE_ERROR | ERROR_COMMUNICATION_FAILURE
#define LIST_ERROR_SECURITY_FAILURE          LIST_BASE_ERROR | ERROR_SECURITY_FAILURE
#define LIST_ERROR_EMPTY                     LIST_BASE_ERROR | ERROR_EMPTY
#define LIST_ERROR_INVALID_STATE             LIST_BASE_ERROR | ERROR_INVALID_STATE
#define LIST_ERROR_NOT_READY                 LIST_BASE_ERROR | ERROR_NOT_READY
#define LIST_ERROR_INVALID_OPERATION         LIST_BASE_ERROR | ERROR_INVALID_OPERATION
#define LIST_ERROR_RESOURCE_UNAVAILABLE      LIST_BASE_ERROR | ERROR_RESOURCE_UNAVAILABLE
#define LIST_ERROR_OPERATION_FAILED          LIST_BASE_ERROR | ERROR_OPERATION_FAILED
#define LIST_ERROR_DATA_CORRUPTION           LIST_BASE_ERROR | ERROR_DATA_CORRUPTION
#define LIST_ERROR_FULL                        LIST_BASE_ERROR | ERROR_FULL

//tipos publicos
typedef struct listElement {
	pv data;//aqui va la lista de todas las tareas que se van creando
	struct listElement *next;
	struct listElement *prev;
} listElement_t, * listElementHandle_t;

typedef struct list{
	listElement_t *current;//para recorrer la lista
	listElement_t *head;//primero en entrar, primero en salir
	listElement_t *tail;//ultimo en entrar, ultimo en salir
	u16 size;
} list_t, * listHandle_t;

//funciones publicas
u16 listInit(listHandle_t list);
u16 listRemove(listHandle_t list, listElementHandle_t element);
u16 listAddTail(listHandle_t list, listElementHandle_t element);
u16 listAddHead(listHandle_t list, listElementHandle_t element);
u16 listAddAfter(listHandle_t list, listElementHandle_t element, listElementHandle_t newElement);
u16 listAddBefore(listHandle_t list, listElementHandle_t element, listElementHandle_t newElement);
u16 listGetNext(listHandle_t list, listElementHandle_t *element);
u16 listGetPrev(listHandle_t list, listElementHandle_t *element);
u16 listGetCurrent(listHandle_t list, listElementHandle_t *element);
u16 listMoveToHead(listHandle_t list);
u16 listMoveToTail(listHandle_t list);
u16 listMoveNext(listHandle_t list);
u16 listMovePrev(listHandle_t list);
u16 getListSize(listHandle_t list);
u16 listPeek(listHandle_t list, listElementHandle_t element);//para observar el head
u16 listClear(listHandle_t list);

#endif