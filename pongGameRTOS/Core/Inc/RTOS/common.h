#ifndef _COMMON_H_
#define _COMMON_H_

//ESTA VA EN LOS .c PARA EVITAR INCLUSIONES CIRCULARES, SOLO SE INCLUYE UNA VEZ EN EL .c

//inclusion de librerias
#include <stdint.h>
#include <stdio.h>
#include "myTypes.h"
#include "list.h"
#include "Task.h"
#include "queue.h"
#include "Scheduler.h"
#include "portable.h"
#include "rtos.h"
#include "mutex.h"
#include "semaphore.h"
#include "events.h"

//definiciones publicas
#define RTOS_MIN_STACK_SIZE                 512//128
#define RTOS_MAX_TASK_PRIORITIES            16
#define RTOS_MIN_PRIORITY                   RTOS_MAX_TASK_PRIORITIES - 1
#define RTOS_TICK_RATE_HZ                   1000

#endif