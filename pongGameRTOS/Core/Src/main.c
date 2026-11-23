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
// Botón START/STOP (activo en bajo, pull-up interno)
#define BUTTON_PORT             GPIOA
#define BUTTON_PIN              GPIO_PIN_8    // PA8 - D7
#define BUTTON_DEBOUNCE_SAMPLES 3U            // lecturas iguales seguidas

// Joysticks (solo eje Y) -> ADC1
#define JOY1_ADC_CHANNEL        ADC_CHANNEL_5 // PA0 - A0
#define JOY2_ADC_CHANNEL        ADC_CHANNEL_6 // PA1 - A1
#define JOY_INVERT_Y            0             // pon 1 si la paleta se mueve al revés
#define JOY_DEADZONE            300           // zona muerta alrededor del centro (cuentas ADC)

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

#define FP_SHIFT                8               // tamaño de la parte fraccionaria (Q8)
#define TO_FP(px)               ((s32)(px) << FP_SHIFT)
#define FROM_FP(v)              ((s32)(v) >> FP_SHIFT)

#define PADDLE_W                2                           // width of the paddle in pixels
#define PADDLE_H                14                          // height of the paddle in pixels
#define PADDLE_LEFT_X           2                           // initial x position of the left paddle
#define PADDLE_RIGHT_X          (SCREEN_W - 2 - PADDLE_W)   // initial x position of the right paddle
#define PADDLE_MAX_SPEED        TO_FP(3)                    // 3 px por frame como maximo de velocidad (Q8)

#define BALL_SIZE               2
#define BALL_START_SPEED        384                         // 1.5 px/frame (Q8)
#define BALL_SPEED_STEP         24                          // aceleración en cada golpe
#define BALL_MAX_SPEED          896                         // 3.5 px/frame (Q8)
#define BALL_MAX_VY             512                         // 2 px/frame (Q8)

#define SERVE_DELAY_FRAMES      30U                         // después de cada punto, la bola desaparece durante 30 frames. A 30 FPS eso es 30 / 30 = 1 segundo para que los jugadores se preparen
#define WIN_SCORE               5U                          // el primero que llega a 5 gana y se pasa a la pantalla de fin del juego.s

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

// ---- Calibración de joysticks (se escribe antes de arrancar el RTOS) ----
static u16 joyCenter[2] = {2048, 2048};

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
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void SoundTask(void);
void InputTask(void);
void GameTask(void);
void RenderTask(void);

/* Utilidades de eventos */
static u16  eventsWaitAndTake(eventHandler_t ev, u16 bits);
static u16  eventsTake(eventHandler_t ev, u16 bits);

/* Hardware */
static u32  buttonIsPressed(void);
static u16  joystickReadRaw(u32 channel);
static void joystickCalibrate(void);
static void buzzerInit(void);
static void buzzerTone(u16 freqHz);
static void buzzerOff(void);

/* Juego */
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
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  // El botón (PA8) ya lo configura MX_GPIO_Init()
  buzzerInit();

  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
  joystickCalibrate();               // ¡no tocar los joysticks al encender!

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
