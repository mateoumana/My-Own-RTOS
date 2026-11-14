#ifndef _event_H_
#define _event_H_

//inclusion de librerias .h
#include "myTypes.h"

//definiciones publicas y errores
#ifndef EVENT_BASE_ERROR
#define EVENT_BASE_ERROR               0X900
#endif

#define EVENT_OK                              ERROR_OK
#define EVENT_ERROR_FAIL                      EVENT_BASE_ERROR | ERROR_FAIL
#define EVENT_ERROR_UNKOWN                    EVENT_BASE_ERROR | ERROR_UNKOWN
#define EVENT_ERROR_NULL_PARAM                EVENT_BASE_ERROR | ERROR_NULL_PARAM
#define EVENT_ERROR_OUT_OF_MEMORY             EVENT_BASE_ERROR | ERROR_OUT_OF_MEMORY
#define EVENT_ERROR_TIMEOUT                   EVENT_BASE_ERROR | ERROR_TIMEOUT
#define EVENT_ERROR_NOT_FOUND                 EVENT_BASE_ERROR | ERROR_NOT_FOUND
#define EVENT_ERROR_INVALID_PARAM             EVENT_BASE_ERROR | ERROR_INVALID_PARAM
#define EVENT_ERROR_ALREADY_EXISTS            EVENT_BASE_ERROR | ERROR_ALREADY_EXISTS
#define EVENT_ERROR_NOT_IMPLEMENTED           EVENT_BASE_ERROR | ERROR_NOT_IMPLEMENTED
#define EVENT_ERROR_BUSY                      EVENT_BASE_ERROR | ERROR_BUSY
#define EVENT_ERROR_NOT_SUPPORTED             EVENT_BASE_ERROR | ERROR_NOT_SUPPORTED
#define EVENT_ERROR_HARDWARE_FAILURE          EVENT_BASE_ERROR | ERROR_HARDWARE_FAILURE
#define EVENT_ERROR_SOFTWARE_FAILURE          EVENT_BASE_ERROR | ERROR_SOFTWARE_FAILURE
#define EVENT_ERROR_COMMUNICATION_FAILURE     EVENT_BASE_ERROR | ERROR_COMMUNICATION_FAILURE
#define EVENT_ERROR_SECURITY_FAILURE          EVENT_BASE_ERROR | ERROR_SECURITY_FAILURE
#define EVENT_ERROR_EMPTY                     EVENT_BASE_ERROR | ERROR_EMPTY
#define EVENT_ERROR_INVALID_STATE             EVENT_BASE_ERROR | ERROR_INVALID_STATE
#define EVENT_ERROR_NOT_READY                 EVENT_BASE_ERROR | ERROR_NOT_READY
#define EVENT_ERROR_INVALID_OPERATION         EVENT_BASE_ERROR | ERROR_INVALID_OPERATION
#define EVENT_ERROR_RESOURCE_UNAVAILABLE      EVENT_BASE_ERROR | ERROR_RESOURCE_UNAVAILABLE
#define EVENT_ERROR_OPERATION_FAILED          EVENT_BASE_ERROR | ERROR_OPERATION_FAILED
#define EVENT_ERROR_DATA_CORRUPTION           EVENT_BASE_ERROR | ERROR_DATA_CORRUPTION
#define EVENT_ERROR_FULL                        EVENT_BASE_ERROR | ERROR_FULL

//tipos publicos
typedef struct event{
    u16 events;
    queue_t waitQueue;
}event_t, * eventHandler_t;

//funciones publicas
u16 eventsInit(eventHandler_t event, u16 init);
u16 eventsSetEvent(eventHandler_t event, u16 bits);
u16 eventsClearEvent(eventHandler_t event, u16 bits);
u16 eventsWaitAny(eventHandler_t event, u16 bits);
u16 eventsWaitAll(eventHandler_t event, u16 bits);


#endif