#ifndef MY_TYPES_H
#define MY_TYPES_H

/* *****************************************************
        Declaraciones publicas de la libreria MY_TYPES_H
***************************************************** */
#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!0)
#endif

/* *****************************************************
        Defiinicion de errores de la libreria MY_TYPES_H
***************************************************** */
#define ERROR_OK                        0X00
#define ERROR_FAIL                      0XFF
#define ERROR_UNKOWN                    0XFE
#define ERROR_NULL_PARAM                0XFD
#define ERROR_OUT_OF_MEMORY             0XFC
#define ERROR_TIMEOUT                   0XFB
#define ERROR_NOT_FOUND                 0XFA
#define ERROR_INVALID_PARAM             0XF9
#define ERROR_ALREADY_EXISTS            0XF8
#define ERROR_NOT_IMPLEMENTED           0XF7
#define ERROR_BUSY                      0XF6
#define ERROR_NOT_SUPPORTED             0XF5
#define ERROR_HARDWARE_FAILURE          0XF4
#define ERROR_SOFTWARE_FAILURE          0XF3
#define ERROR_COMMUNICATION_FAILURE     0XF2
#define ERROR_SECURITY_FAILURE          0XF1
#define ERROR_EMPTY                     0XF0
#define ERROR_INVALID_STATE             0XEF
#define ERROR_NOT_READY                 0XEE
#define ERROR_INVALID_OPERATION         0XED
#define ERROR_RESOURCE_UNAVAILABLE      0XEC
#define ERROR_OPERATION_FAILED          0XEB
#define ERROR_DATA_CORRUPTION           0XEA
#define ERROR_FULL                        0XE9


#define LIST_BASE_ERROR                0X100
#define QUEUE_BASE_ERROR               0X200
#define TASK_BASE_ERROR                0X300
#define SCHEDULER_BASE_ERROR           0X400
#define PORTABLE_BASE_ERROR            0X500
#define RTOS_BASE_ERROR                0X600
#define MUTEX_BASE_ERROR               0X700
#define SEMAPHORE_BASE_ERROR           0X800
#define EVENT_BASE_ERROR               0X900


/* *****************************************************
        Tipos de datos de la libreria MY_TYPES_H
***************************************************** */
typedef char c;
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;
typedef float f;
typedef double d;
typedef void v;

typedef u8 *pu8;
typedef s8 *ps8;
typedef u16 *pu16;
typedef s16 *ps16;
typedef u32 *pu32;
typedef s32 *ps32;
typedef u64 *pu64;
typedef s64 *ps64;
typedef f *pf;
typedef d *pd;
typedef v *pv; 

/* *****************************************************
        Definicion publicas de la libreria MY_TYPES_H
***************************************************** */

/* *****************************************************
        Variables globales de la libreria MY_TYPES_H
***************************************************** */

#endif // MY_TYPES_H