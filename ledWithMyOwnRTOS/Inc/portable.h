#ifndef _PORTABLE_H_
#define _PORTABLE_H_
 
/*******************************************************
 *             INCLUSION LIBRERIAS .h                  *
 *******************************************************/
 
#include "myTypes.h"
 
/*******************************************************
 *          DEFINICION DE ERRORES DE PORTABLE_H_       *
 *******************************************************/
 
#ifndef PORTABLE_BASE_ERROR
#define PORTABLE_BASE_ERROR                     0x0500
#endif
 
#define PORTABLE_OK                             ERROR_OK
#define PORTABLE_ERROR_FAIL                     PORTABLE_BASE_ERROR | ERROR_FAIL
#define PORTABLE_ERROR_UNKNOWN                  PORTABLE_BASE_ERROR | ERROR_UNKOWN
#define PORTABLE_ERROR_NULL_PARAM               PORTABLE_BASE_ERROR | ERROR_NULL_PARAM
#define PORTABLE_ERROR_OUT_OF_MEMORY            PORTABLE_BASE_ERROR | ERROR_OUT_OF_MEMORY
#define PORTABLE_ERROR_TIMEOUT                  PORTABLE_BASE_ERROR | ERROR_TIMEOUT
#define PORTABLE_ERROR_NOT_FOUND                PORTABLE_BASE_ERROR | ERROR_NOT_FOUND
#define PORTABLE_ERROR_INVALID_PARAM            PORTABLE_BASE_ERROR | ERROR_INVALID_PARAM
#define PORTABLE_ERROR_ALREADY_EXISTS           PORTABLE_BASE_ERROR | ERROR_ALREADY_EXISTS
#define PORTABLE_ERROR_NOT_IMPLEMENTED          PORTABLE_BASE_ERROR | ERROR_NOT_IMPLEMENTED
#define PORTABLE_ERROR_BUSY                     PORTABLE_BASE_ERROR | ERROR_BUSY
#define PORTABLE_ERROR_UNSUPPORTED              PORTABLE_BASE_ERROR | ERROR_NOT_SUPPORTED
#define PORTABLE_ERROR_HARDWARE_FAILURE         PORTABLE_BASE_ERROR | ERROR_HARDWARE_FAILURE
#define PORTABLE_ERROR_SOFTWARE_FAILURE         PORTABLE_BASE_ERROR | ERROR_SOFTWARE_FAILURE
#define PORTABLE_ERROR_COMMUNICATION_FAILURE    PORTABLE_BASE_ERROR | ERROR_COMMUNICATION_FAILURE
#define PORTABLE_ERROR_SECURITY_FAILURE         PORTABLE_BASE_ERROR | ERROR_SECURITY_FAILURE
#define PORTABLE_ERROR_EMPTY                    PORTABLE_BASE_ERROR | ERROR_EMPTY
#define PORTABLE_ERROR_INVALID_STATE            PORTABLE_BASE_ERROR | ERROR_INVALID_STATE
#define PORTABLE_ERROR_NOT_READY                PORTABLE_BASE_ERROR | ERROR_NOT_READY
#define PORTABLE_ERROR_INVALID_OPERATION        PORTABLE_BASE_ERROR | ERROR_INVALID_OPERATION
#define PORTABLE_ERROR_RESOURCE_UNAVAILABLE     PORTABLE_BASE_ERROR | ERROR_RESOURCE_UNAVAILABLE
#define PORTABLE_ERROR_OPERATION_FAILED         PORTABLE_BASE_ERROR | ERROR_OPERATION_FAILED
#define PORTABLE_ERROR_DATA_CORRUPTION          PORTABLE_BASE_ERROR | ERROR_DATA_CORRUPTION
#define PORTABLE_ERROR_FULL                     PORTABLE_BASE_ERROR | ERROR_FULL
 
/*******************************************************
 *       DEFINICIONES PUBLICAS DEL _PORTABLE_H_        *
 *******************************************************/
 
//#define PORTABLE_RTOS_SYSTEM_CLOCK              16000000U
#define PORTABLE_RTOS_SYSTEM_CLOCK  4000000UL   /* MSI por defecto */
 
/*******************************************************
 *           TIPOS PUBLICOS DEL _PORTABLE_H_           *
 *******************************************************/
 
/*******************************************************
 *         FUNCIONES PUBLICAS DEL _PORTABLE_H_         *
 *******************************************************/
 
void portableEnableInterrupts(void);
void portableDisableInterrupts(void);
void portableInitTickTimer(void);
u64 portableGetSystemTicks(void);
void portableRestoreFirstContext(u32 firstTaskStackPointer);
u32 portableCreateContext(pu8 StackPointer, u16 StackSize, u32 Function); 
void portableCallScheduller(void);


#endif //_PORTABLE_H_[cite: 22]
 