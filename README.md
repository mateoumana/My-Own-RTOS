\# My-Own-RTOS



Un sistema operativo de tiempo real (RTOS) escrito desde cero en C para microcontroladores \*\*ARM Cortex-M4\*\*, probado sobre una placa \*\*NUCLEO-L476RG\*\* (STM32L476RG).



El objetivo del proyecto es entender por dentro cómo funciona un RTOS: el cambio de contexto en ensamblador, el planificador por prioridades, los retardos por ticks y los mecanismos de sincronización entre tareas. No usa FreeRTOS ni ningún otro kernel.



\---



\## Contenido del repositorio



El repositorio contiene dos versiones del sistema operativo, que muestran su evolución:



| Carpeta | Versión | Descripción |

|---|---|---|

| \[`ledWithMyOwnRTOS/`](ledWithMyOwnRTOS) | \*\*RTOS básico\*\* | El núcleo mínimo de un sistema operativo: creación de tareas, planificador preemptivo por prioridades, cambio de contexto y retardos con `taskDelay()`. Proyecto \*\*bare-metal\*\* (sin HAL), con acceso directo a registros, startup y linker script propios. Demo: 3 LEDs parpadeando a distinta frecuencia y un botón con antirrebote, cada uno en su propia tarea. |

| \[`pongGameRTOS/`](pongGameRTOS) | \*\*RTOS completo\*\* | El sistema operativo ampliado con \*\*mecanismos para comunicar tareas y proteger la memoria compartida\*\*: mutex, semáforos, eventos y colas de mensajes. Integrado con \*\*STM32CubeMX + HAL\*\* y con I2C1 configurado para una pantalla \*\*OLED SSD1306\*\*, como base para un juego de Pong sobre el RTOS. |



\### ¿Qué agrega la versión completa?



| Funcionalidad | `ledWithMyOwnRTOS` (básico) | `pongGameRTOS` (completo) |

|---|:---:|:---:|

| Tareas con prioridad y planificador preemptivo | ✅ | ✅ |

| Cambio de contexto (SysTick) | ✅ | ✅ |

| `taskDelay()` y tarea Idle | ✅ | ✅ |

| Cesión inmediata de la CPU (PendSV) | — | ✅ |

| \*\*Mutex\*\*: exclusión mutua para proteger memoria y periféricos compartidos | — | ✅ |

| \*\*Semáforos\*\* contadores: sincronizar tareas y controlar recursos limitados | — | ✅ |

| \*\*Eventos\*\* (banderas de bits): avisar a una o varias tareas de que algo ocurrió | — | ✅ |

| \*\*Colas de mensajes\*\*: enviar datos de una tarea a otra | — | ✅ |

| HAL + CubeMX (`.ioc`) | — | ✅ |

| Pantalla OLED SSD1306 por I2C | — | ✅ |



\---



\## Características del kernel



\- \*\*Planificador preemptivo por prioridades\*\*

&#x20; - 16 niveles de prioridad (`0` = la más alta, `15` = la más baja).

&#x20; - Una cola FIFO por prioridad: las tareas con la misma prioridad se reparten la CPU por turnos (\*round-robin\*).

&#x20; - Siempre se ejecuta la tarea lista de mayor prioridad.

\- \*\*Tick del sistema con SysTick\*\* a 1 kHz (1 tick = 1 ms, configurable con `RTOS\_TICK\_RATE\_HZ`).

\- \*\*Cambio de contexto en ensamblador\*\* mediante los manejadores `SysTick\_Handler` y `PendSV\_Handler` (funciones `naked`):

&#x20; - el hardware apila automáticamente `R0-R3, R12, LR, PC, xPSR`;

&#x20; - el kernel guarda manualmente `R4-R11` y `EXC\_RETURN`;

&#x20; - PendSV permite que una tarea ceda la CPU sin esperar al siguiente tick (por ejemplo, al bloquearse en un mutex).

\- \*\*Tarea Idle\*\* integrada: se ejecuta cuando no hay ninguna tarea lista y se encarga de despertar a las tareas dormidas cuando vence su `taskDelay()`.

\- \*\*Memoria 100 % estática\*\*: el usuario reserva el TCB y el stack de cada tarea; el kernel no usa `malloc`.

\- \*\*Estructuras propias\*\*: lista doblemente enlazada (`list`) y cola FIFO (`queue`).

\- \*\*Comunicación entre tareas y protección de memoria\*\* \*(solo en la versión completa, `pongGameRTOS`)\*:

&#x20; - \*\*Mutex\*\* con cola de espera: protege una zona de memoria o un periférico compartido para que solo una tarea lo use a la vez. La tarea que no obtiene el mutex se bloquea y no consume CPU.

&#x20; - \*\*Semáforo contador\*\* con cola de espera: sincroniza tareas (una avisa, otra espera) y limita el acceso a recursos con varias unidades disponibles.

&#x20; - \*\*Eventos\*\* (banderas de bits): una tarea activa bits y otras esperan a que se cumpla una o varias condiciones.

&#x20; - \*\*Colas de mensajes\*\*: paso de datos entre tareas sin compartir variables globales.

\- \*\*Códigos de error por módulo\*\*: cada módulo tiene su rango (`0x100` queue, `0x200` list, `0x300` task, `0x400` scheduler, `0x500` portable, `0x600` rtos, `0x700` mutex, `0x800` semaphore, `0x900` events), lo que permite saber de dónde viene un error con solo mirar el código devuelto.



\### Funcionamiento general



```mermaid

flowchart LR

&#x20;   A\[SysTick cada 1 ms<br/>o PendSV] --> B\[Guardar contexto<br/>R4-R11 + EXC\_RETURN]

&#x20;   B --> C\[schedulerGetNextTask]

&#x20;   C --> D{¿Hay tareas listas?}

&#x20;   D -- Sí --> E\[Tarea de mayor prioridad<br/>primera de su cola FIFO]

&#x20;   D -- No --> F\[Tarea Idle]

&#x20;   E --> G\[Restaurar contexto]

&#x20;   F --> G

&#x20;   G --> H\[Retorno de excepción<br/>a la nueva tarea]

```



\---



\## Estructura del kernel



```

RTOS/

├── common.h      Configuración global (prioridades, tamaño de stack, tick)

├── myTypes.h     Tipos base (u8, u16, u32, u64, pv...) y códigos de error genéricos

├── rtos.c/h      rtosInit() / rtosStart()

├── Task.c/h      TCB, creación de tareas, taskDelay(), tarea Idle

├── Scheduler.c/h Colas de listos por prioridad y selección de la siguiente tarea

├── portable.c/h  Código dependiente del Cortex-M: SysTick, PendSV, cambio de contexto,

│                 habilitar/deshabilitar interrupciones (con anidamiento)

├── list.c/h      Lista doblemente enlazada

├── queue.c/h     Cola FIFO

├── mutex.c/h     Mutex                    (solo versión completa)

├── semaphore.c/h Semáforo contador        (solo versión completa)

└── events.c/h    Grupos de eventos        (solo versión completa)

```



`ledWithMyOwnRTOS` contiene solo los módulos del núcleo básico (`rtos`, `Task`, `Scheduler`, `portable`, `list`, `queue`). `pongGameRTOS` incluye todos.



\### Configuración (`common.h` y `portable.h`)



| Parámetro | Valor | Significado |

|---|---|---|

| `RTOS\_MAX\_TASK\_PRIORITIES` | `16` | Número de niveles de prioridad |

| `RTOS\_MIN\_STACK\_SIZE` | `512` | Tamaño mínimo de stack por tarea (bytes) |

| `RTOS\_TICK\_RATE\_HZ` | `1000` | Frecuencia del tick (Hz) |

| `PORTABLE\_RTOS\_SYSTEM\_CLOCK` | `4000000` | Frecuencia del núcleo en Hz (MSI a 4 MHz). \*\*Debe coincidir con el reloj configurado en el micro.\*\* |



\---



\## Uso básico



```c

\#include "common.h"



\#define MS\_TO\_TICKS(ms)  ((u64)(ms) \* RTOS\_TICK\_RATE\_HZ / 1000U)



TCB\_t myTaskLed;

u8    stackLed\[RTOS\_MIN\_STACK\_SIZE] \_\_attribute\_\_((aligned(4)));



void LedTask(void){

&#x20;   for(;;){

&#x20;       HAL\_GPIO\_TogglePin(GPIOB, GPIO\_PIN\_4);

&#x20;       taskDelay(MS\_TO\_TICKS(500));   // bloquea la tarea y cede la CPU

&#x20;   }

}



int main(void){

&#x20;   HAL\_Init();

&#x20;   SystemClock\_Config();

&#x20;   // ... inicialización de periféricos ...



&#x20;   rtosInit();

&#x20;   taskCreate(\&myTaskLed, LedTask, 0, stackLed, RTOS\_MIN\_STACK\_SIZE, "LedTask");

&#x20;   rtosStart();   // no retorna



&#x20;   while(1){}

}

```



Uso de un mutex para proteger un recurso compartido (por ejemplo, el bus I2C):



```c

Mutex\_t i2cMutex;



MutexInit(\&i2cMutex);      // antes de rtosStart()



// dentro de una tarea

MutexLock(\&i2cMutex);

ssd1306\_UpdateScreen();

MutexUnlock(\&i2cMutex);

```



> Dentro de las tareas usa `taskDelay()` en lugar de `HAL\_Delay()`: `HAL\_Delay()` es una espera activa que mantiene ocupada la CPU, mientras que `taskDelay()` bloquea la tarea y deja correr a las demás.



\---



\## Hardware



\- \*\*Placa:\*\* NUCLEO-L476RG (STM32L476RG, Cortex-M4F, 1 MB Flash, 128 KB RAM)

\- \*\*Pantalla:\*\* OLED SSD1306 128×64 por I2C (dirección `0x3C`), solo en `pongGameRTOS`



\### Conexiones



| Señal | Pin MCU | Conector Arduino | Proyecto |

|---|---|---|---|

| LED 1 (0.5 Hz) | PB4 | D5 | ambos |

| LED 2 (1 Hz) | PB5 | D4 | ambos |

| LED 3 (5 Hz) | PB3 | D3 | ambos |

| LED 4 (sigue al botón) | PB10 | D6 | ambos |

| Botón (activo en bajo, pull-up interno) | PA8 | D7 | ambos |

| SSD1306 SCL | PB6 | D10 | pongGameRTOS |

| SSD1306 SDA | PB7 | — (morpho CN7) | pongGameRTOS |



\---



\## Compilación y carga



\### Requisitos



\- \[VS Code](https://code.visualstudio.com/) con la extensión \*\*STM32Cube for Visual Studio Code\*\*

&#x20; (instala automáticamente CMake, Ninja y la toolchain `arm-none-eabi-gcc` como \*bundles\*).

\- \[STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) (solo para modificar la configuración de `pongGameRTOS.ioc`).

\- Drivers ST-Link.



\### Pasos



1\. Clona el repositorio:

&#x20;  ```bash

&#x20;  git clone https://github.com/mateoumana/My-Own-RTOS.git

&#x20;  ```

2\. En VS Code, abre la carpeta del proyecto que quieras (`ledWithMyOwnRTOS` o `pongGameRTOS`).

3\. Si la extensión lo pide, usa \*\*STM32Cube: Set Up STM32Cube project(s)\*\* para que configure las herramientas.

4\. Selecciona el preset \*\*Debug\*\* y compila (\*\*CMake: Build\*\* o `F7`).

5\. Conecta la Nucleo por USB y depura / carga con \*\*F5\*\*.



\### Notas sobre `pongGameRTOS` y CubeMX



El RTOS necesita para sí mismo los manejadores `SysTick\_Handler` y `PendSV\_Handler`, así que el proyecto `.ioc` está configurado de esta forma:



\- \*\*SYS → Timebase Source = TIM6\*\*: HAL usa TIM6 para `HAL\_GetTick()` / `HAL\_Delay()`, y SysTick queda reservado para el RTOS.

\- \*\*NVIC → Code generation\*\*: \*Generate IRQ handler\* \*\*desactivado\*\* para \*System tick timer\* y \*Pendable request for system service\*.



Si se regenera código con CubeMX, hay que mantener esas opciones; de lo contrario aparecerán errores de \*multiple definition\* al enlazar. El código propio en `main.c` debe ir dentro de los bloques `/\* USER CODE BEGIN ... \*/ /\* USER CODE END ... \*/` para que CubeMX no lo borre.



\---



\## Limitaciones conocidas



\- Todas las tareas usan el \*\*MSP\*\* (\*Main Stack Pointer\*); no hay separación entre stack de tareas (PSP) y stack del kernel.

\- \*\*No se guarda el contexto de la FPU\*\*: las tareas no deberían usar operaciones en coma flotante.

\- No hay herencia de prioridad en el mutex (posible inversión de prioridad).

\- El juego de Pong está en desarrollo; por ahora `pongGameRTOS` ejecuta la demo de LEDs y botón sobre HAL.



\---



\## Hoja de ruta



\- \[ ] Juego de Pong sobre el SSD1306 (tareas de entrada, lógica y dibujo)

\- \[ ] Uso de PSP para las tareas y MSP para el kernel

\- \[ ] Guardado del contexto de la FPU

\- \[ ] Herencia de prioridad en el mutex



\---



\## Créditos



\- Librería del SSD1306: \[afiskon/stm32-ssd1306](https://github.com/afiskon/stm32-ssd1306) (licencia MIT).

\- HAL y CMSIS: STMicroelectronics / ARM.



Proyecto desarrollado para la asignatura de \*\*Sistemas Operativos de Tiempo Real (SOTR)\*\*.

