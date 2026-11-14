/*
todos los M-Cortex tienen la misma estructura de portable, por lo que se puede usar el mismo portable para todos los M-Cortex
*/

/*******************************************************
 *             INCLUSION LIBRERIAS .h                  *
 *******************************************************/
 
#include "common.h"
 
/*******************************************************
 *         DEFINICIONES PRIVADAS DEL PORTABLE          *
 *******************************************************/
 //el datasheet indica que el EPSR debe ser inicializado en 0x01000000, para que el procesador use el set de instrucciones thumb, que es el set de instrucciones que usan los M-Cortex, 
 //si se inicializa en 0x00000000 usara el set de instrucciones ARM, que es el set de instrucciones que usan los procesadores ARM
#define PORTABLE_INITIAL_XPSR               (u32)( 0x01000000 ) //bit T del EPSR register
 
/* For strict compliance with the Cortex-M spec the task start address should
 * have bit-0 clear, as it is loaded into the PC on exit from an ISR. */
 //se setea en 0 para que use el set de instrucciones thumb, que es el set de instrucciones que usan los M-Cortex, 
 //si se setea en 1 usara el set de instrucciones ARM, que es el set de instrucciones que usan los procesadores ARM
#define PORTABLE_START_ADDRESS_MASK         ( (u32) 0xFFFFFFFEUL )
 
/*******************************************************
 *             TIPOS PRIVADOS DEL PORTABLE             *
 *******************************************************/
 
/*******************************************************
 *     DEFINICION DEFUNCIONES PRIVADAS DEL PORTABLE    *
 *******************************************************/
 
static inline void portableSaveContext(void);
static inline void portableRestoreContext(void);
 
/*******************************************************
 * DEFINICION VARIABLE GLOBALES PRIVADAS DEL PORTABLE *
 *******************************************************/
 
static u32 PortableCurrentSP = 0;
static u32 PortableNestedCount = 0; //para cuando se haga un llamado anidado de deshabilitacion de interrupciones, hacer un conteo de cuantas veces se han deshabilitado las interrupciones, para que cuando se habiliten, solo se habiliten cuando el conteo llegue a 0
static volatile u64 PortableTickCounter = 0;
 
/*******************************************************
 * IMPLEMENTACIÓN DE DEFUNCIONES PUBLICAS DEL PORTABLE * 
*******************************************************/

/**
 * @brief Saves the context of the current task
 * @details This function saves the context of the current task by pushing the registers onto the stack and saving the stack pointer to a global variable.
 * Inline para que cuando saltemos a esta funcion, se copie literal el codigo de la funcion en el lugar donde generar un salto a esta funcion y asi dejar quieto el stack pointer, 
 * ya que si hacemos un salto a esta funcion, el procesador cambiara el stack pointer al de esta funcion y no al de la tarea que estaba ejecutando.
 */
__attribute__((always_inline)) static inline void portableSaveContext(void){
    __asm volatile ("PUSH {LR}");       //to send the LR to the stack, so that when we restore the context, we can return to the correct address
    __asm volatile ("PUSH {R4-R11}");   //to save the registers that are not automatically saved by the processor when an interrupt occurs
    __asm volatile ("MRS R0, MSP");      //Move the SP register to R0
    __asm volatile ("MOV %0, R0" : "=r" (PortableCurrentSP) ); //to save the current stack pointer to the global variable PortableCurrentSP
    //PortableNestedCount++;
}

/**
 * @brief Restores the context of a task
 * @details This function restores the context of the task that was previously saved using portableSaveContext
 * Inline para que cuando saltemos a esta funcion, se copie literal el codigo de la funcion en el lugar donde generar un salto a esta funcion y asi dejar quieto el stack pointer, 
 * ya que si hacemos un salto a esta funcion, el procesador cambiara el stack pointer al de esta funcion y no al de la tarea que estaba ejecutando.
 */
__attribute__((always_inline)) static inline void portableRestoreContext(void){
    //PortableNestedCount--;
    __asm volatile ("MOV R0, %0" : : "r" (PortableCurrentSP)); //to restore the stack pointer from the global variable PortableCurrentSP
    __asm volatile ("MSR MSP, R0");      //Restore the SP register from R0
    __asm volatile ("POP {R4-R11}");    //to restore the registers that were saved by portableSaveContext
    __asm volatile ("POP {PC}");        //to restore the program counter from the stack, which will return to the address that was saved in LR when portableSaveContext was called
}

/**
 * @brief Enables interrupts
 * @details This function enables interrupts by clearing the interrupt disable bit in the CPS register (Change Processor State).
 * CPSIE is the command to clear the special purpose register. the flag 'I' set or clear PRIMASK, which is the interrupt mask register. 
 * When PRIMASK is set to 0, all interrupts are enabled.
*/
void portableEnableInterrupts(void)
{
    if(PortableNestedCount > 0){
        PortableNestedCount--;
    }
    if(PortableNestedCount == 0){//esta variable tiene que llegar a 0 para poder ejecutar la interrupcion donde voy a cambiar de tarea, luego no la necesitamos como contexto
        __asm volatile ("CPSIE I");
    }
}
 
/**
 * @brief Disables interrupts
 * @details This function disables interrupts by setting the interrupt disable bit in the CPS register (Change Processor State).
 * CPSID is the command to set the special purpose register. the flag 'I' set or clear PRIMASK, which is the interrupt mask register.
 * When PRIMASK is set to 1, all interrupts are disabled except for NMI and HardFault.
*/
void portableDisableInterrupts(void)
{
    __asm volatile ("CPSID I");
    PortableNestedCount++;
}

/***
 * @brief Initializes the tick timer
 * @details This function is called when the operating system starts
 * When the systick generates an interrupt, the SysTick_Handler function will be executed, which is responsible for saving 
 * the context of the current task, calling the scheduler to select the next task to execute, and restoring the 
 * context of the newly selected task.
 */
 void portableInitTickTimer(void){
    pu32 pSTK_CTRL = (pu32)0xE000E010; // Address of the SysTick Control and Status Register. To set the value
    //tiene el CLKSOURCE, que es el clock source del systick, que puede ser el clock del procesador o el clock externo, en este caso se usa el clock del procesador.
    //TICKINIT, que es el bit que habilita el systick, y TICKINT, que es el bit que habilita la interrupcion del systick.
    //ENABLE, que es el bit que habilita el conteo del systick. Cuenta desde pSTK_LOAD hasta 0, y cuando llega a 0, genera una interrupcion y se reinicia el conteo desde pSTK_LOAD.
    pu32 pSTK_LOAD = (pu32)0xE000E014; // Address of the SysTick Reload Value Register. To set the value the Systick timer counts down from.
    //El systick cuenta hasta el valor deseado de pSTK_LOAD - 1. para lanzar un clock, luego se reinicia y asi sucesivamente. 
    //Por lo que para que el systick cuente 1ms, se debe setear pSTK_LOAD en 16000 - 1 = 15999, ya que el clock del procesador es de 16MHz.
    pu32 pSTK_VAL = (pu32)0xE000E018; 

    *pSTK_LOAD &= 0xFF000000;//mascara para los bits resevados, dejarlos quietos y limpiar data basura
    *pSTK_LOAD |= (PORTABLE_RTOS_SYSTEM_CLOCK / RTOS_TICK_RATE_HZ) - 1U; // Set reload value
    *pSTK_VAL = *pSTK_LOAD;//inicializar el conteo
    *pSTK_CTRL |= 0x07; // Enable SysTick, enable SysTick interrupt, use processor clock
}
 
/***
 * @brief Gets the system ticks
 * @details This function returns the number of ticks since the system started, it could overflow after a long long time, 
 * but it is not a problem, because u64 is so much time.
 * @return Number of ticks
 */
u64 portableGetSystemTicks(void){
    //portableDisableInterrupts();
    u64 Ticks = PortableTickCounter;
    //portableEnableInterrupts();
    return Ticks;
}

/***
 * @brief Sets the context of the first task to be executed
 * @details This function is called when the operating system starts, it sets the context of the first task to be executed, 
 * which is the idle task. It sets the stack pointer to the top of the stack of the idle task, and sets the registers from the stack.
 * @param firstTaskStackPointer Pointer to the stack of the first task to be executed
*/
void portableRestoreFirstContext(u32 firstTaskStackPointer){
    PortableNestedCount = 0;
    PortableCurrentSP = firstTaskStackPointer;
    __asm volatile ("MOV R0, %0" : : "r" (PortableCurrentSP)); //to restore the stack pointer from the global variable PortableCurrentSP
    __asm volatile ("MSR MSP, R0");      //Restore the MSP=SP register from R0
    __asm volatile ("POP {R4-R11}");    //to restore the registers
    __asm volatile ("POP {LR}");        //
    __asm volatile ("POP {R0-R3}");     //to restore the registers
    __asm volatile ("POP {R12}");       //to restore the registers
    __asm volatile ("POP {LR}");        //
    __asm volatile ("POP {LR}");        //
    __asm volatile ("ORR LR, LR, #1");  //para que BX se pueda ejecutar sin lanzar excepción. Bit 0 debe ser 1, despues de ejecutar BX se setea de nuevo en 0
    __asm volatile ("POP {R10}");       //
    __asm volatile ("MSR APSR_nzcvq, R10");//("MSR PSR, R10");    //Restore the PSR register from R10
    __asm volatile ("MOV R10, #0");     //Move R0 to R10
    __asm volatile ("CPSIE I");         //to enable interrupts when all starts
    __asm volatile ("BX LR");           //salto a la rutina de la primera tarea, punto de no retorno
}

/***
 * @brief Creates a new context for a task
 * @param StackPointer Pointer to the stack memory
 * @param StackSize Size of the stack memory
 * @param Function Address of the function to execute
 * @return Address of the new context
 */
u32 portableCreateContext(pu8 StackPointer, u16 StackSize, u32 Function){//contexto de arranque de cada tarea
    // Implement the context creation logic here
    if(StackPointer == NULL || StackSize == 0 || Function == 0)
        return 0;
    pu32 TopOfStack = (pu32)((u32)StackPointer + StackSize); // Return the top of the stack
    TopOfStack--;
    *TopOfStack = PORTABLE_INITIAL_XPSR; // Store the function address at the top of the stack
    TopOfStack--;
    *TopOfStack = Function & PORTABLE_START_ADDRESS_MASK; // Store the function address at the top of the stack
    //*TopOfStack |= 0x00000001UL; //bit 0 debe ser 1, despues de ejecutar BX se setea de nuevo en 0
    TopOfStack--;
    *TopOfStack = Function & PORTABLE_START_ADDRESS_MASK; // Initialize the next stack position to 0
    //*TopOfStack |= 0x00000001UL; //bit 0 debe ser 1, despues de ejecutar BX se setea de nuevo en 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R12 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R3 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R2 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R1 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R0 to 0
    TopOfStack--;
    //Hasta aqui lo hace automaticamente el procesador al detectar una interrupcion.
    *TopOfStack = 0xFFFFFFF9; // here we modify EXEC_RETURN to Thread mode with MSP in LR (Link Register) (to not use PSP)
    TopOfStack--;
    //De aqui en adelante se hace manualmente para que el procesador pueda restaurar el contexto de la tarea cuando se haga un cambio de contexto.
    *TopOfStack = 0; // Initialize R11 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R10 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R9 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R8 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R7 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R6 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R5 to 0
    TopOfStack--;
    *TopOfStack = 0; // Initialize R4 to 0
    //TopOfStack--;
    return (u32)TopOfStack;
}

/*** 
 * @brief Calls the scheduler
 * @details This function triggers PendSV to call the scheduler to select the next task to execute.
 */
void portableCallScheduller(void){
    pu32 pSCB_ICSR = (pu32)0xE000ED04; // Address of the Interrupt Control and State Register. To set the value
    *pSCB_ICSR |= (1U << 28); // Set the PendSV bit to trigger a PendSV exception, which will call the scheduler to select the next task to execute
}
 
/*******************************************************
 *  IMPLEMENTACIÓN DE DEFUNCIONES PRIVADAS DEL PORTABLE *
 *******************************************************/

/***
 * @brief SysTick interrupt handler
 * @details This function is called when the SysTick interrupt occurs. It saves the context of the current task, 
 * increments the tick counter, calls the scheduler to select the next task to execute, and restores the context of the newly selected task.
 * The micro generates it as a "weak" default function, that means that it can be redefined in the user code. In this case, 
 * we redefine it to implement the context switching logic.
 * 
 * We use and Interruption to switch a task, because the microcontroller can save automatically almost the half of the context of the current task when an interruption occurs,
 * and we can restore the context of the new task when the interruption ends. This is a very efficient way to switch tasks, because we don't have to save and restore the context manually.
 * 
 * "naked" attribute is used to tell the compiler not to generate prologue and epilogue code for this function, so that we can implement the context switching logic manually.
 */
 __attribute__((naked)) void SysTick_Handler(void){
    // Implement the SysTick interrupt handler logic here
    // Primero Salvar Contexto Actual
    // Llamar al Scheduler para seleccionar la siguiente tarea a ejecutar
    // Restaurar el contexto de la nueva tarea seleccionada
    portableSaveContext();
    PortableTickCounter++;
    PortableCurrentSP = schedulerGetNextTask(PortableCurrentSP);
    portableRestoreContext();
}
//Clang no permite mezclar en funciones naked codigo assembler con codio C

/***
* @brief PendSV interrupt handler
 * @details This function is called when the PendSV interrupt (a software interruption) occurs. It saves the context of the current task, 
 * calls the scheduler to select the next task to execute, and restores the context of the newly selected task.
 * The micro generates it as a "weak" default function, that means that it can be redefined in the user code. In this case, 
 * we redefine it to implement the context switching logic.
 * 
 * We use and Interruption to switch a task, because the microcontroller can save automatically almost the half of the context of the current task when an interruption occurs,
 * and we can restore the context of the new task when the interruption ends. This is a very efficient way to switch tasks, because we don't have to save and restore the context manually.
 * 
 * "naked" attribute is used to tell the compiler not to generate prologue and epilogue code for this function, so that we can implement the context switching logic manually.
*/
__attribute__((naked)) void PendSV_Handler(void){
    portableSaveContext();
    PortableCurrentSP = schedulerGetNextTask(PortableCurrentSP);
    portableRestoreContext();
}   