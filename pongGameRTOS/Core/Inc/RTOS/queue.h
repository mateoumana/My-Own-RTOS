#ifndef _QUEUE_H_
#define _QUEUE_H_

//inclusion de librerias .h
#include "myTypes.h"

//definiciones publicas y errores
#ifndef QUEUE_BASE_ERROR
#define QUEUE_BASE_ERROR               0X100
#endif

#define QUEUE_OK 							ERROR_OK
#define QUEUE_ERROR_FAIL                      QUEUE_BASE_ERROR | ERROR_FAIL
#define QUEUE_ERROR_UNKOWN                    QUEUE_BASE_ERROR | ERROR_UNKOWN
#define QUEUE_ERROR_NULL_PARAM                QUEUE_BASE_ERROR | ERROR_NULL_PARAM
#define QUEUE_ERROR_OUT_OF_MEMORY             QUEUE_BASE_ERROR | ERROR_OUT_OF_MEMORY
#define QUEUE_ERROR_TIMEOUT                   QUEUE_BASE_ERROR | ERROR_TIMEOUT
#define QUEUE_ERROR_NOT_FOUND                 QUEUE_BASE_ERROR | ERROR_NOT_FOUND
#define QUEUE_ERROR_INVALID_PARAM             QUEUE_BASE_ERROR | ERROR_INVALID_PARAM
#define QUEUE_ERROR_ALREADY_EXISTS            QUEUE_BASE_ERROR | ERROR_ALREADY_EXISTS
#define QUEUE_ERROR_NOT_IMPLEMENTED           QUEUE_BASE_ERROR | ERROR_NOT_IMPLEMENTED
#define QUEUE_ERROR_BUSY                      QUEUE_BASE_ERROR | ERROR_BUSY
#define QUEUE_ERROR_NOT_SUPPORTED             QUEUE_BASE_ERROR | ERROR_NOT_SUPPORTED
#define QUEUE_ERROR_HARDWARE_FAILURE          QUEUE_BASE_ERROR | ERROR_HARDWARE_FAILURE
#define QUEUE_ERROR_SOFTWARE_FAILURE          QUEUE_BASE_ERROR | ERROR_SOFTWARE_FAILURE
#define QUEUE_ERROR_COMMUNICATION_FAILURE     QUEUE_BASE_ERROR | ERROR_COMMUNICATION_FAILURE
#define QUEUE_ERROR_SECURITY_FAILURE          QUEUE_BASE_ERROR | ERROR_SECURITY_FAILURE
#define QUEUE_ERROR_EMPTY                     QUEUE_BASE_ERROR | ERROR_EMPTY
#define QUEUE_ERROR_INVALID_STATE             QUEUE_BASE_ERROR | ERROR_INVALID_STATE
#define QUEUE_ERROR_NOT_READY                 QUEUE_BASE_ERROR | ERROR_NOT_READY
#define QUEUE_ERROR_INVALID_OPERATION         QUEUE_BASE_ERROR | ERROR_INVALID_OPERATION
#define QUEUE_ERROR_RESOURCE_UNAVAILABLE      QUEUE_BASE_ERROR | ERROR_RESOURCE_UNAVAILABLE
#define QUEUE_ERROR_OPERATION_FAILED          QUEUE_BASE_ERROR | ERROR_OPERATION_FAILED
#define QUEUE_ERROR_DATA_CORRUPTION           QUEUE_BASE_ERROR | ERROR_DATA_CORRUPTION
#define QUEUE_ERROR_FULL                        QUEUE_BASE_ERROR | ERROR_FULL


//tipos publicos
typedef struct queueElement {
	pv data;//aqui va el puntero a la task que se va a crear, el TCB.
	struct queueElement *next;
} queueElement_t, * queueElementHandle_t;

typedef struct queue{
	queueElement_t *head;//primero en entrar, primero en salir
	queueElement_t *tail;//ultimo en entrar, ultimo en salir
	u16 size;
} queue_t, * queueHandle_t;

//funciones publicas
u16 queueInit(queueHandle_t queue);
u16 dequeue(queueHandle_t queue, queueElementHandle_t *element);
u16 enqueue(queueHandle_t queue, queueElementHandle_t element);
u16 getQueueSize(queueHandle_t queue);
u16 queuePeek(queueHandle_t queue, queueElementHandle_t element);//para observar el head
u16 queueCopy(queueHandle_t queueSource, queueHandle_t queueDest);//copia el contenido de una cola a otra

#endif