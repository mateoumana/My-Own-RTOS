/*******************************************************
 *             INCLUSION LIBRERIAS .h                  *
 *******************************************************/
#include "common.h"
 
/*******************************************************
 *         DEFINICIONES PRIVADAS DEL RTOS          *
 *******************************************************/

 
/*******************************************************
 *             TIPOS PRIVADOS DEL RTOS             *
 *******************************************************/
 
/*******************************************************
 *     DEFINICION DEFUNCIONES PRIVADAS DEL RTOS    *
 *******************************************************/

 
/*******************************************************
 * DEFINICION VARIABLE GLOBALES PRIVADAS DEL RTOS *
 *******************************************************/

 
/*******************************************************
 * IMPLEMENTACIÓN DE DEFUNCIONES PUBLICAS DEL RTOS * 
*******************************************************/
u16 rtosInit(void){
    u16 result = RTOS_OK;
    portableDisableInterrupts();

    result = schedulerInit();
    if(result != RTOS_OK){
        portableEnableInterrupts();
        return result;
    }

    result = taskInit();
    if(result != RTOS_OK){
        portableEnableInterrupts();
        return result;
    }

    portableEnableInterrupts();
    return result;
}


void rtosStart(void){
    u32 firstSP = schedulerGetNextTask(0);//0 to get de IDLE task
    portableInitTickTimer();
    portableRestoreFirstContext(firstSP);
}

/*******************************************************
 *  IMPLEMENTACIÓN DE DEFUNCIONES PRIVADAS DEL RTOS *
 *******************************************************/

 