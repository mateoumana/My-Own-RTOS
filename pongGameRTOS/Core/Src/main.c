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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Botón
#define BUTTON_PORT             GPIOA
#define BUTTON_PIN              GPIO_PIN_8    // PA8 - D7
#define BUTTON_POLL_MS          10U           // periodo de lectura
#define BUTTON_DEBOUNCE_SAMPLES 3U            // lecturas iguales seguidas

// LEDs (todos en el puerto B)
#define LED_PORT   GPIOB
//#define LED1_PIN   GPIO_PIN_5   // PA5 es el LED integrado de la Nucleo (puerto A)
#define LED1_PIN   GPIO_PIN_4     // PB4  - D5
#define LED2_PIN   GPIO_PIN_5     // PB5  - D4
#define LED3_PIN   GPIO_PIN_3     // PB3  - D3
#define LED4_PIN   GPIO_PIN_10    // PB10 - D6

// Medio periodo de cada LED en ms: T/2 = 1 / (2f)
#define LED1_HALF_PERIOD_MS   1000U  // 0.5 Hz
#define LED2_HALF_PERIOD_MS   500U   // 1 Hz  
#define LED3_HALF_PERIOD_MS   100U   // 5 Hz  

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MS_TO_TICKS(ms)  ((u64)(ms) * RTOS_TICK_RATE_HZ / 1000U)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
TCB_t myButtonTask;
TCB_t myTaskLed1;
TCB_t myTaskLed2;
TCB_t myTaskLed3;
u8 taskStack0[RTOS_MIN_STACK_SIZE] __attribute__((aligned(4)));
u8 taskStack1[RTOS_MIN_STACK_SIZE] __attribute__((aligned(4)));
u8 taskStack2[RTOS_MIN_STACK_SIZE] __attribute__((aligned(4)));
u8 taskStack3[RTOS_MIN_STACK_SIZE] __attribute__((aligned(4)));
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
static void LED_AllInit(void);
void LED_On(uint16_t pin);
void LED_Off(uint16_t pin);
static void LED_Blink(u32 pin, u32 halfPeriodMs);
static void buttonInit(void);
static inline u32 buttonIsPressed(void);
void LedTask1(void);
void LedTask2(void);
void LedTask3(void);
void ButtonTask(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  /* USER CODE BEGIN 2 */
  LED_AllInit();
  buttonInit();
  rtosInit(); //Initialize the RTOS

  //Creating the tasks
	taskCreate(&myButtonTask, ButtonTask, 0, taskStack0, RTOS_MIN_STACK_SIZE, "buttonTask");
  taskCreate(&myTaskLed1, LedTask1, 0, taskStack1, RTOS_MIN_STACK_SIZE, "LedTask1");
  taskCreate(&myTaskLed2, LedTask2, 0, taskStack2, RTOS_MIN_STACK_SIZE, "LedTask2");
  taskCreate(&myTaskLed3, LedTask3, 0, taskStack3, RTOS_MIN_STACK_SIZE, "LedTask3");
	
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
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
  hi2c1.Init.Timing = 0x00000003;
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static void LED_AllInit(void){
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();

  // Apagar antes de configurar como salida (evita un pulso al arrancar)
  HAL_GPIO_WritePin(LED_PORT, LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN, GPIO_PIN_RESET);

  gpio.Pin   = LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN;
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;      // salida push-pull (MODER = 01)
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &gpio);
}

void LED_On(uint16_t pin)  { HAL_GPIO_WritePin(LED_PORT, pin, GPIO_PIN_SET); }
void LED_Off(uint16_t pin) { HAL_GPIO_WritePin(LED_PORT, pin, GPIO_PIN_RESET); }

static void LED_Blink(u32 pin, u32 halfPeriodMs){
  const u32 half = MS_TO_TICKS(halfPeriodMs);

  for(;;){
    LED_On(pin);
    taskDelay(half);

    LED_Off(pin);
    taskDelay(half);
  }
}

static void buttonInit(void){
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  gpio.Pin  = BUTTON_PIN;
  gpio.Mode = GPIO_MODE_INPUT;           // MODER = 00
  gpio.Pull = GPIO_PULLUP;               // PUPDR = 01
  HAL_GPIO_Init(BUTTON_PORT, &gpio);
}

static inline u32 buttonIsPressed(void){
  return (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET);  // activo en bajo
}

void LedTask1(void){ LED_Blink(LED1_PIN, LED1_HALF_PERIOD_MS); }
void LedTask2(void){ LED_Blink(LED2_PIN, LED2_HALF_PERIOD_MS); }
void LedTask3(void){ LED_Blink(LED3_PIN, LED3_HALF_PERIOD_MS); }

void ButtonTask(void){
  const u64 period = MS_TO_TICKS(BUTTON_POLL_MS);
  u32 state = 0;      // estado confirmado: 1 = presionado
  u32 count = 0;      // lecturas consecutivas distintas

  for(;;){
    u32 raw = buttonIsPressed();

    if (raw != state){
      if (++count >= BUTTON_DEBOUNCE_SAMPLES){
        state = raw;
        count = 0;
      }
    } else {
      count = 0;
    }

    if (state) LED_On(LED4_PIN);
    else       LED_Off(LED4_PIN);

    taskDelay(period);
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
