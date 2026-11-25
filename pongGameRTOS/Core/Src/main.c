/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "ssd1306_fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  GAME_IDLE = 0,      // pantalla de inicio
  GAME_PLAYING,       // partida en curso
  GAME_OVER           // partida terminada (por puntos o por el botón)
} gameState_t;

// Estado del juego compartido entre GameTask (escribe) y RenderTask (lee).
// Protegido por gameMutex.
typedef struct {
  gameState_t state;
  s32 ballX, ballY;          // posición de la bola en Q8 (pixeles * 256)
  s32 paddleY[2];            // posición superior de cada paleta en Q8
  u8  score[2];              // marcador [jugador 1, jugador 2]
  u8  winner;                // 0 = empate/terminado con botón, 1 o 2 = ganador
  u8  ballVisible;           // la bola se oculta durante el saque
  u32 frame;                 // contador de frames (para parpadeos en pantalla)
} game_t;

// Lecturas de entrada compartidas entre InputTask (escribe) y GameTask (lee).
// Protegido por inputMutex.
typedef struct {
  s16 joy[2];                // desviación de cada joystick respecto al centro (-2048..2047)
} input_t;

typedef struct {
  u16 freqHz;                // 0 = silencio
  u16 ms;
} note_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// ---------------- Hardware ----------------
// Botón START/STOP (activo en bajo). Configurado en CubeMX:
// PA8 (D7) -> GPIO_Input, Pull-up, User Label = BTN_START
#define BUTTON_PORT             BTN_START_GPIO_Port
#define BUTTON_PIN              BTN_START_Pin
#define BUTTON_DEBOUNCE_SAMPLES 3U            // lecturas iguales seguidas

// Joysticks (solo eje Y) -> ADC1
#define JOY1_ADC_CHANNEL        ADC_CHANNEL_5 // PA0 - A0
#define JOY2_ADC_CHANNEL        ADC_CHANNEL_6 // PA1 - A1
#define JOY_CENTER              2048          // centro teórico del ADC de 12 bits
#define JOY_MARGIN              600           // margen alrededor del centro
#define JOY_INVERT_Y            1             // 0/1 si la paleta se mueve al revés

// Buzzer -> TIM2 CH3 en PB10 (D6), PWM al 50 %
#define BUZZER_TIMER            htim2         //htim2 es el handler del TIMER2, este define es un alias
#define BUZZER_CHANNEL          TIM_CHANNEL_3
#define BUZZER_COUNTER_HZ       1000000U      // contador del timer a 1 MHz

// ---------------- Tiempos ----------------
#define GAME_FPS                30U     //30 frames por segundo
#define INPUT_PERIOD_MS         10U     //cada 10ms se lee el adc de cada joystick

// ---------------- Pantalla y juego ----------------
#define SCREEN_W                SSD1306_WIDTH   // 128
#define SCREEN_H                SSD1306_HEIGHT  // 64

#define PADDLE_W                2                           // width of the paddle in pixels
#define PADDLE_H                14                          // height of the paddle in pixels
#define PADDLE_LEFT_X           2                           // initial x position of the left paddle
#define PADDLE_RIGHT_X          (SCREEN_W - 2 - PADDLE_W)   // initial x position of the right paddle
#define PADDLE_SPEED            2   // px por frame al mover el joystick

#define BALL_SIZE               2   // bola de 2x2 px
#define BALL_SPEED_X            2   // px por frame en horizontal (constante)
#define BALL_SPEED_Y            1   // px por frame en vertical (+1 o -1)

#define SERVE_DELAY_FRAMES      30U // después de cada punto, la bola desaparece durante 30 frames. A 30 FPS eso es 30 / 30 = 1 segundo para que los jugadores se preparen
#define WIN_SCORE               5U  // el primero que llega a 5 gana y se pasa a la pantalla de fin del juego.s

// ---------------- Mascaras de Eventos ----------------
// Grupo inputEvents (InputTask -> GameTask)
#define EV_BTN_PRESSED          (1U << 0)
// Grupo frameEvents (GameTask -> RenderTask)
#define EV_FRAME_READY          (1U << 0)
// Grupo soundEvents (GameTask -> SoundTask)
#define EV_SND_WALL             (1U << 0)
#define EV_SND_PADDLE           (1U << 1)
#define EV_SND_START            (1U << 2)
#define EV_SND_SCORE            (1U << 3)
#define EV_SND_GAMEOVER         (1U << 4)
#define EV_SND_ALL              (EV_SND_WALL | EV_SND_PADDLE | EV_SND_START | EV_SND_SCORE | EV_SND_GAMEOVER)

// ---------------- Tareas ----------------
// En este RTOS: 0 = prioridad más alta
#define PRIO_SOUND              1
#define PRIO_INPUT              2
#define PRIO_GAME               3
#define PRIO_RENDER             4

#define STACK_SOUND             768
#define STACK_INPUT             1024
#define STACK_GAME              1024
#define STACK_RENDER            1024
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MS_TO_TICKS(ms)  ((u32)(ms) * RTOS_TICK_RATE_HZ / 1000U)
#define ARRAY_LEN(a)     (sizeof(a) / sizeof((a)[0]))
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
// ---- Tareas ----
TCB_t soundTaskTCB, inputTaskTCB, gameTaskTCB, renderTaskTCB;
u8 soundStack[STACK_SOUND]   __attribute__((aligned(8)));
u8 inputStack[STACK_INPUT]   __attribute__((aligned(8)));
u8 gameStack[STACK_GAME]     __attribute__((aligned(8)));
u8 renderStack[STACK_RENDER] __attribute__((aligned(8)));

// ---- Sincronización ----
static Mutex_t gameMutex;      // protege 'game'
static Mutex_t inputMutex;     // protege 'input'
static event_t inputEvents;    // botón
static event_t frameEvents;    // frame listo para dibujar
static event_t soundEvents;    // efectos de sonido

// ---- Datos compartidos ---- (acceder solo con Mutex)
static game_t  game;           
static input_t input;   


// ---- Efectos de sonido PWM en buzzer ---- {frecuencia, duración en ms}
static const note_t sndPaddle[]   = { {1000, 25} };                        // "tic" agudo
static const note_t sndWall[]     = { {600, 20} };                         // "tic" más grave
static const note_t sndStart[]    = { {523, 80}, {659, 80}, {784, 120} };  // Do-Mi-Sol subiendo
static const note_t sndScore[]    = { {300, 80}, {0, 20}, {200, 150} };    // dos tonos bajando
static const note_t sndGameOver[] = { {784, 120}, {659, 120}, {523, 120}, {392, 300} }; // Sol-Mi-Do-Sol bajando
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void SoundTask(void);
void InputTask(void);
void GameTask(void);
void RenderTask(void);

// Utilidades de eventos
static u16  eventsWaitAndTake(eventHandler_t ev, u16 bits);
static u16  eventsTake(eventHandler_t ev, u16 bits);

// Hardware
static u32  buttonIsPressed(void);
static u16  joystickReadRaw(u32 channel);
static void buzzerInit(void);
static void buzzerTone(u16 freqHz);
static void buzzerOff(void);

// Juego
static void gameReset(game_t *g);
static void gameServe(game_t *g, s32 *vx, s32 *vy, s8 direction);
static u32  randomNext(void);
static void drawFrame(const game_t *g);
static void drawCenteredText(char *text, u8 y, SSD1306_Font_t font);
static void u8ToString(u8 value, char *out);
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  // El botón (PA8) ya lo configura MX_GPIO_Init()
  buzzerInit();

  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

  ssd1306_Init();                    // usa HAL_Delay internamente
  ssd1306_Fill(Black);
  ssd1306_UpdateScreen();

  /* ---------- Prioridades de las interrupciones del RTOS ----------
   * SysTick y PendSV hacen el cambio de contexto: deben tener la prioridad MÁS BAJA
   * (15), igual que TIM6 (base de tiempo de HAL). Así nunca interrumpen a otra ISR
   * y el cambio de contexto siempre ocurre desde una tarea. */
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);

  // ---------- RTOS ----------
  rtosInit(); //Initialize the RTOS

  MutexInit(&gameMutex);
  MutexInit(&inputMutex);
  eventsInit(&inputEvents, 0);
  eventsInit(&frameEvents, 0);
  eventsInit(&soundEvents, 0);

  gameReset(&game);
  game.state = GAME_IDLE;

  taskCreate(&soundTaskTCB,  SoundTask,  PRIO_SOUND,  soundStack,  STACK_SOUND,  "SoundTask");
  taskCreate(&inputTaskTCB,  InputTask,  PRIO_INPUT,  inputStack,  STACK_INPUT,  "InputTask");
  taskCreate(&gameTaskTCB,   GameTask,   PRIO_GAME,   gameStack,   STACK_GAME,   "GameTask");
  taskCreate(&renderTaskTCB, RenderTask, PRIO_RENDER, renderStack, STACK_RENDER, "RenderTask");
	
  rtosStart(); //Starting the RTOS scheduler
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00F12981;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : BTN_START_Pin */
  GPIO_InitStruct.Pin = BTN_START_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_START_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* ==========================================================================
 *                          UTILIDADES DE EVENTOS
 * ========================================================================== */

/**
 * @brief Espera a que se active alguno de 'bits' y los consume (los borra).
 * @details eventsWaitAny() siempre bloquea, aunque el bit ya esté activo. Para no
 * perder un aviso, se revisa el bit y se llama a eventsWaitAny() dentro de una
 * sección crítica: el bloqueo real (PendSV) ocurre al habilitar interrupciones,
 * cuando la tarea ya está en la cola de espera. Así ningún eventsSetEvent()
 * puede colarse entre la comprobación y la espera.
 * @return bits que se activaron (y que fueron borrados)
 */
static u16 eventsWaitAndTake(eventHandler_t ev, u16 bits)
{
  u16 got = 0;
  while (got == 0) {
    portableDisableInterrupts();
    if ((ev->events & bits) == 0) {
      eventsWaitAny(ev, bits);          // encola la tarea; el cambio de contexto queda pendiente
    }
    portableEnableInterrupts();         // aquí se bloquea la tarea si no había bits
    got = eventsTake(ev, bits);         // al despertar: leer y borrar
  }
  return got;
}

/**
 * @brief Lee y borra de forma atómica los bits indicados, sin bloquear.
 * @return bits que estaban activos
 */
static u16 eventsTake(eventHandler_t ev, u16 bits)
{
  portableDisableInterrupts();
  u16 got = ev->events & bits;
  if (got) {
    eventsClearEvent(ev, got);
  }
  portableEnableInterrupts();
  return got;
}

/* ==========================================================================
 *                               HARDWARE
 * ========================================================================== */

static u32 buttonIsPressed(void)
{
  return (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET);   // activo en bajo
}


/**
 * @brief Una conversión del ADC1 en el canal indicado (polling).
 * @note  Solo la usa InputTask, por eso el ADC no necesita mutex.
 */
static u16 joystickReadRaw(u32 channel)
{
  ADC_ChannelConfTypeDef cfg = {0};
  u16 value = 2048;

  cfg.Channel      = channel;
  cfg.Rank         = ADC_REGULAR_RANK_1;
  cfg.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;   // potenciómetros de 10k: más tiempo de muestreo
  cfg.SingleDiff   = ADC_SINGLE_ENDED;
  cfg.OffsetNumber = ADC_OFFSET_NONE;
  cfg.Offset       = 0;

  if (HAL_ADC_ConfigChannel(&hadc1, &cfg) != HAL_OK) {
    return value;
  }

  // Dos conversiones seguidas: la primera tras cambiar de canal puede arrastrar
  // carga del canal anterior, así que se descarta y se usa la segunda.
  for (u8 i = 0; i < 2; i++) {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK) {
      value = (u16)HAL_ADC_GetValue(&hadc1);
    }
  }
  HAL_ADC_Stop(&hadc1);
  return value;
}

/** @brief Configura el contador del timer del buzzer a 1 MHz y arranca el PWM en silencio. */
static void buzzerInit(void)
{
  // Reloj del timer: TIM2 está en APB1. Si el prescaler de APB1 es > 1, el timer va al doble.
  u32 timClk = HAL_RCC_GetPCLK1Freq();
  if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
    timClk *= 2U;
  }
  __HAL_TIM_SET_PRESCALER(&BUZZER_TIMER, (timClk / BUZZER_COUNTER_HZ) - 1U);
  __HAL_TIM_SET_AUTORELOAD(&BUZZER_TIMER, 1000U - 1U);
  __HAL_TIM_SET_COMPARE(&BUZZER_TIMER, BUZZER_CHANNEL, 0);        // 0 % = silencio
  HAL_TIM_GenerateEvent(&BUZZER_TIMER, TIM_EVENTSOURCE_UPDATE);   // cargar el prescaler ya
  HAL_TIM_PWM_Start(&BUZZER_TIMER, BUZZER_CHANNEL);
}

/** @brief Genera un tono cuadrado al 50 % (para buzzer pasivo). */
static void buzzerTone(u16 freqHz)
{
  if (freqHz == 0U) {
    buzzerOff();
    return;
  }
  u32 arr = (BUZZER_COUNTER_HZ / freqHz) - 1U;
  __HAL_TIM_SET_AUTORELOAD(&BUZZER_TIMER, arr);
  __HAL_TIM_SET_COMPARE(&BUZZER_TIMER, BUZZER_CHANNEL, arr / 2U);
  __HAL_TIM_SET_COUNTER(&BUZZER_TIMER, 0);   // evita que el contador quede por encima del nuevo ARR
}

static void buzzerOff(void)
{
  __HAL_TIM_SET_COMPARE(&BUZZER_TIMER, BUZZER_CHANNEL, 0);
}

/* ==========================================================================
 *                             LÓGICA DEL JUEGO
 * ========================================================================== */

/** @brief Generador pseudoaleatorio simple (xorshift32). */
static u32 randomNext(void)
{
  static u32 seed = 0;
  if (seed == 0U) {
    seed = (u32)portableGetSystemTicks() ^ 0xA5A5F00DU;
    if (seed == 0U) seed = 1U;
  }
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}

/** @brief Deja el marcador en 0 y las paletas centradas. */
static void gameReset(game_t *g)
{
  g->score[0] = 0;
  g->score[1] = 0;
  g->winner   = 0;
  g->paddleY[0] = (SCREEN_H - PADDLE_H) / 2;
  g->paddleY[1] = (SCREEN_H - PADDLE_H) / 2;
  g->ballX = (SCREEN_W - BALL_SIZE) / 2;
  g->ballY = (SCREEN_H - BALL_SIZE) / 2;
  g->ballVisible = 0;
}


/**
 * @brief Coloca la bola en el centro y le da velocidad hacia 'direction'.
 * @param direction -1 = hacia el jugador 1 (izquierda), +1 = hacia el jugador 2 (derecha)
 */
static void gameServe(game_t *g, s32 *vx, s32 *vy, s8 direction)
{
  g->ballX = (SCREEN_W - BALL_SIZE) / 2;                    // centro horizontal
  g->ballY = 8 + (s32)(randomNext() % (SCREEN_H - 16));     // altura al azar (8..55)
  *vx = (direction < 0) ? -BALL_SPEED_X : BALL_SPEED_X;
  *vy = (randomNext() & 1U) ? -BALL_SPEED_Y : BALL_SPEED_Y; // sube o baja al azar
}
/* ==========================================================================
 *                                 TAREAS
 * ========================================================================== */

/**
 * @brief InputTask: lee joysticks y botón cada 10 ms.
 *  - Joysticks -> input.joy[] (protegido con inputMutex).
 *  - Flanco de pulsación del botón (con antirrebote) -> evento EV_BTN_PRESSED.
 */
void InputTask(void)
{
  const u32 channels[2] = {JOY1_ADC_CHANNEL, JOY2_ADC_CHANNEL};
  u32 btnState = 0;      // estado confirmado: 1 = presionado
  u32 btnCount = 0;      // lecturas consecutivas distintas al estado confirmado

  for (;;) {
    /* ---- Joysticks: solo importa la dirección ---- */
    s8 dir[2];
    for (u8 j = 0; j < 2; j++) {
      u16 raw = joystickReadRaw(channels[j]);

      if (raw < JOY_CENTER - JOY_MARGIN)      dir[j] = -1;   // inclinado hacia un lado
      else if (raw > JOY_CENTER + JOY_MARGIN) dir[j] = +1;   // inclinado hacia el otro
      else                                    dir[j] = 0;    // dentro del margen: quieto
#if JOY_INVERT_Y
      dir[j] = (s8)-dir[j];
#endif
    }

    MutexLock(&inputMutex);
    input.joy[0] = dir[0];
    input.joy[1] = dir[1];
    MutexUnlock(&inputMutex);

    /* ---- Botón START/STOP con antirrebote ---- */
    u32 raw = buttonIsPressed();
    if (raw != btnState) {
      if (++btnCount >= BUTTON_DEBOUNCE_SAMPLES) {
        btnState = raw;
        btnCount = 0;
        if (btnState) {                                 // solo al presionar, no al soltar
          eventsSetEvent(&inputEvents, EV_BTN_PRESSED);
        }
      }
    } else {
      btnCount = 0;
    }

    taskDelay(MS_TO_TICKS(INPUT_PERIOD_MS));
  }
}

/**
 * @brief GameTask: bucle principal a 30 FPS.
 * Trabaja sobre una copia privada 'g' y al final de cada frame la publica en
 * 'game' con gameMutex (sección crítica corta). Luego avisa a RenderTask.
 */
void GameTask(void)
{
  game_t g;
  s32 vx = 0, vy = 0;                  // velocidad de la bola (px/frame)
  u32 serveCountdown = 0;
  s8  serveDir = 1;
  u64 startTicks = portableGetSystemTicks();
  u64 frameIndex = 0;

  MutexLock(&gameMutex);
  g = game;
  MutexUnlock(&gameMutex);

  for (;;) {
    u16 sounds = 0;

    /* ---------- 1. Botón START/STOP ---------- */
    if (eventsTake(&inputEvents, EV_BTN_PRESSED)) {
      if (g.state == GAME_PLAYING) {
        // Terminar la partida manualmente
        g.state  = GAME_OVER;
        g.winner = (g.score[0] > g.score[1]) ? 1 : (g.score[1] > g.score[0]) ? 2 : 0;
        g.ballVisible = 0;
        buzzerOff();
        sounds |= EV_SND_GAMEOVER;
      } else {
        // Empezar una partida nueva (desde IDLE o GAME_OVER)
        gameReset(&g);
        g.state = GAME_PLAYING;
        serveDir = (randomNext() & 1U) ? 1 : -1;
        serveCountdown = SERVE_DELAY_FRAMES;
        sounds |= EV_SND_START;
      }
    }

    /* ---------- 2. Paletas (también se mueven en IDLE para probar los mandos) ---------- */
    s16 joy[2];
    MutexLock(&inputMutex);
    joy[0] = input.joy[0];
    joy[1] = input.joy[1];
    MutexUnlock(&inputMutex);

    // Fuera de la zona muerta, la paleta se mueve PADDLE_SPEED px en la dirección del joystick
    for (u8 p = 0; p < 2; p++) {
      if (joy[p] < 0)      g.paddleY[p] -= PADDLE_SPEED;   // baja
      else if (joy[p] > 0) g.paddleY[p] += PADDLE_SPEED;   // sube

      if (g.paddleY[p] < 0) g.paddleY[p] = 0;              // limites en el techo y en el piso
      if (g.paddleY[p] > SCREEN_H - PADDLE_H) g.paddleY[p] = SCREEN_H - PADDLE_H;
    }

    /* ---------- 3. Bola ---------- */
    if (g.state == GAME_PLAYING) {
      if (serveCountdown > 0U) {
        // Pausa antes del saque
        serveCountdown--;
        g.ballVisible = 0;
        if (serveCountdown == 0U) {
          gameServe(&g, &vx, &vy, serveDir);
          g.ballVisible = 1;
        }
      } else {
        s32 oldX = g.ballX;
        g.ballX += vx;
        g.ballY += vy;

        // Paredes superior e inferior: la bola rebota invirtiendo vy
        if (g.ballY < 0) {
          g.ballY = -g.ballY;
          vy = -vy;
          sounds |= EV_SND_WALL;
        } else if (g.ballY > SCREEN_H - BALL_SIZE) {
          g.ballY = 2 * (SCREEN_H - BALL_SIZE) - g.ballY;
          vy = -vy;
          sounds |= EV_SND_WALL;
        }

        // Paletas: se comprueba si la bola cruzó la cara de la paleta en este frame
        const s32 leftFace  = PADDLE_LEFT_X + PADDLE_W;      // x = 4  (cara derecha de la paleta 1)
        const s32 rightFace = PADDLE_RIGHT_X - BALL_SIZE;    // x = 122 (bola pegada a la paleta 2)

        s32 ballTop = g.ballY;
        s32 ballBot = g.ballY + BALL_SIZE - 1;

        for (u8 p = 0; p < 2; p++) {
          u8 crossed = (p == 0) ? (vx < 0 && oldX >= leftFace  && g.ballX < leftFace)
                                : (vx > 0 && oldX <= rightFace && g.ballX > rightFace);
          if (!crossed) continue;

          s32 padTop = g.paddleY[p];
          s32 padBot = padTop + PADDLE_H - 1;
          if (ballBot >= padTop && ballTop <= padBot) {
            // Rebote: misma velocidad, sentido contrario
            vx = -vx;
            g.ballX = (p == 0) ? leftFace : rightFace;

            // Si pega en la mitad de arriba de la paleta sale hacia arriba; abajo, hacia abajo
            s32 ballCenter   = ballTop + BALL_SIZE / 2;
            s32 paddleCenter = padTop + PADDLE_H / 2;
            vy = (ballCenter < paddleCenter) ? -BALL_SPEED_Y : BALL_SPEED_Y;

            sounds |= EV_SND_PADDLE;
          }
        }

        // Punto: la bola salió por un lado
        s8 scorer = -1;
        if (g.ballX < -BALL_SIZE)        scorer = 1;   // salió por la izquierda: punto para el jugador 2
        else if (g.ballX > SCREEN_W)     scorer = 0;   // salió por la derecha: punto para el jugador 1

        if (scorer >= 0) {
          g.score[scorer]++;
          g.ballVisible = 0;
          serveDir = (scorer == 0) ? 1 : -1;                  // saca hacia el que perdió el punto
          if (g.score[scorer] >= WIN_SCORE) {
            g.state  = GAME_OVER;
            g.winner = (u8)(scorer + 1);
            sounds |= EV_SND_GAMEOVER;
          } else {
            serveCountdown = SERVE_DELAY_FRAMES;
            sounds |= EV_SND_SCORE;
          }
        }
      }
    }

    g.frame++;

    /* ---------- 4. Publicar el frame y avisar ---------- */
    MutexLock(&gameMutex);
    game = g;
    MutexUnlock(&gameMutex);

    eventsSetEvent(&frameEvents, EV_FRAME_READY);
    if (sounds) {
      eventsSetEvent(&soundEvents, sounds);
    }

    /* ---------- 5. Esperar al siguiente frame (30 FPS exactos en promedio) ---------- */
    frameIndex++;
    u64 nextFrame = startTicks + (frameIndex * RTOS_TICK_RATE_HZ) / GAME_FPS;
    u64 now = portableGetSystemTicks();
    if (nextFrame > now) {
      taskDelay((u32)(nextFrame - now));
    } else {
      // Vamos atrasados (el dibujo tardó más que un frame): no acumular retraso
      startTicks = now;
      frameIndex = 0;
      taskDelay(1);
    }
  }
}

/**
 * @brief RenderTask: espera EV_FRAME_READY, copia el estado y dibuja.
 * Es la ÚNICA tarea que usa la pantalla/I2C, así que el bus no necesita mutex.
 * Tiene la prioridad más baja porque ssd1306_UpdateScreen() tarda ~25 ms.
 */
void RenderTask(void)
{
  game_t snapshot;

  for (;;) {
    eventsWaitAndTake(&frameEvents, EV_FRAME_READY);

    MutexLock(&gameMutex);
    snapshot = game;                 // copia rápida; el dibujo se hace sin el mutex
    MutexUnlock(&gameMutex);

    drawFrame(&snapshot);
    ssd1306_UpdateScreen();
  }
}

/**
 * @brief SoundTask: espera eventos de sonido y toca el efecto más importante.
 * Si llegan varios a la vez, gana el de mayor prioridad (fin > punto > inicio > paleta > pared).
 */
void SoundTask(void)
{
  for (;;) {
    u16 ev = eventsWaitAndTake(&soundEvents, EV_SND_ALL);

    const note_t *melody;
    u8 len;
    if      (ev & EV_SND_GAMEOVER) { melody = sndGameOver; len = ARRAY_LEN(sndGameOver); }
    else if (ev & EV_SND_SCORE)    { melody = sndScore;    len = ARRAY_LEN(sndScore); }
    else if (ev & EV_SND_START)    { melody = sndStart;    len = ARRAY_LEN(sndStart); }
    else if (ev & EV_SND_PADDLE)   { melody = sndPaddle;   len = ARRAY_LEN(sndPaddle); }
    else                           { melody = sndWall;     len = ARRAY_LEN(sndWall); }

    for (u8 i = 0; i < len; i++) {
      buzzerTone(melody[i].freqHz);
      taskDelay(MS_TO_TICKS(melody[i].ms));
    }
    buzzerOff();
  }
}

/* ==========================================================================
 *                                 DIBUJO
 * ========================================================================== */
/***
 * @brief Convierte un número de 8 bits a cadena de caracteres.
 * @details Solo para valores 0..99. No usa sprintf() ni malloc(). Si value >= 10, se generan dos dígitos; si es < 10, un solo dígito. La cadena resultante siempre termina en '\0'.
 * @param value Número a convertir.
 * @param out Cadena de caracteres donde se almacenará el resultado.
 */
static void u8ToString(u8 value, char *out)
{
  if (value >= 10U) {
    out[0] = (char)('0' + value / 10U);
    out[1] = (char)('0' + value % 10U);
    out[2] = '\0';
  } else {
    out[0] = (char)('0' + value);
    out[1] = '\0';
  }
}

static void drawCenteredText(char *text, u8 y, SSD1306_Font_t font)
{
  u8 len = 0;
  while (text[len] != '\0') len++;
  s32 x = ((s32)SCREEN_W - (s32)len * font.width) / 2;
  if (x < 0) x = 0;
  ssd1306_SetCursor((u8)x, y);
  ssd1306_WriteString(text, font, White);
}

static void drawFrame(const game_t *g)
{
  char buf[4];

  ssd1306_Fill(Black);

  /* Paletas (siempre visibles, así se pueden probar los joysticks en cualquier pantalla) */
  for (u8 p = 0; p < 2; p++) {
    u8 x = (p == 0) ? PADDLE_LEFT_X : PADDLE_RIGHT_X;
    u8 y = (u8)g->paddleY[p];
    ssd1306_FillRectangle(x, y, x + PADDLE_W - 1, y + PADDLE_H - 1, White);
  }

  switch (g->state) {
  case GAME_IDLE:
    drawCenteredText("PONG", 8, Font_11x18);
    drawCenteredText("My-Own-RTOS", 30, Font_6x8);
    if ((g->frame / 15U) % 2U == 0U) {                 // parpadeo de 0.5 s
      drawCenteredText("Presiona START", 50, Font_6x8);
    }
    break;

  case GAME_PLAYING:
    /* Red central discontinua */
    for (u8 y = 0; y < SCREEN_H; y += 4) {
      ssd1306_DrawPixel(SCREEN_W / 2 - 1, y, White);
      ssd1306_DrawPixel(SCREEN_W / 2 - 1, y + 1, White);
    }
    /* Marcador */
    u8ToString(g->score[0], buf);
    ssd1306_SetCursor(SCREEN_W / 2 - 16, 1);
    ssd1306_WriteString(buf, Font_7x10, White);
    u8ToString(g->score[1], buf);
    ssd1306_SetCursor(SCREEN_W / 2 + 9, 1);
    ssd1306_WriteString(buf, Font_7x10, White);
    /* Bola */
    if (g->ballVisible) {
      s32 bx = g->ballX;
      s32 by = g->ballY;
      if (bx >= 0 && bx <= SCREEN_W - BALL_SIZE && by >= 0 && by <= SCREEN_H - BALL_SIZE) {
        ssd1306_FillRectangle((u8)bx, (u8)by, (u8)(bx + BALL_SIZE - 1), (u8)(by + BALL_SIZE - 1), White);
      }
    }
    break;

  case GAME_OVER:
  default: {
    char score[8];
    u8 i = 0;
    drawCenteredText("FIN DEL JUEGO", 2, Font_7x10);
    u8ToString(g->score[0], buf);
    for (u8 k = 0; buf[k]; k++) score[i++] = buf[k];
    score[i++] = '-';
    u8ToString(g->score[1], buf);
    for (u8 k = 0; buf[k]; k++) score[i++] = buf[k];
    score[i] = '\0';
    drawCenteredText(score, 16, Font_11x18);
    if (g->winner == 1)      drawCenteredText("GANA JUGADOR 1", 38, Font_6x8);
    else if (g->winner == 2) drawCenteredText("GANA JUGADOR 2", 38, Font_6x8);
    else                     drawCenteredText("EMPATE", 38, Font_6x8);
    if ((g->frame / 15U) % 2U == 0U) {
      drawCenteredText("START: jugar", 52, Font_6x8);
    }
    break;
  }
  }
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
