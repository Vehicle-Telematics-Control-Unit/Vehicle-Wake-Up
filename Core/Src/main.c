#define _WIZCHIP_ 5500
#include "main.h"
#include "stm32f1xx_hal.h"
#include "MQTTClient.h"

#define BUFFER_SIZE 2048
#define SOC_NUM 0

SPI_HandleTypeDef hspi1;

static unsigned char targetIP[4] = {192, 168, 1, 90}; // set your mqtt server IP
static int targetPort = 1883;                           // mqtt server port

static unsigned char tempBuffer[BUFFER_SIZE] = {};
TIM_HandleTypeDef htim3;
static uint8_t lock;

struct opts_struct
{
  char *clientid;
  int nodelimiter;
  char *delimiter;
  enum QoS qos;
  char *username;
  char *password;
  char *host;
  int port;
  int showtopics;
} opts;

static MQTTClient mqttClient;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);

/* User Defined functions */
void configureMQttClient();
void messageArrived(MessageData *md);

void schedule(uint32_t waitFor)
{
  static MQTTMessage message;
  static char payload[30];
  message.qos = 0;
  message.retained = 0;
  message.payload = payload;

  static uint32_t tickstart;
  int count = 0;
  if ((HAL_GetTick() - tickstart) > waitFor)
  {
    //	    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
    tickstart = HAL_GetTick();
    count++;
    sprintf(payload, "Publish Sensor value %d", count);
    message.payloadlen = strlen(payload);
    MQTTPublish(&mqttClient, "Publish/Topic", &message);
  }
}

// @brief messageArrived callback function
void messageArrived(MessageData *md)
{
  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
}

void configureMQttClient()
{
  opts.clientid = "subscriber";
  opts.nodelimiter = 0;
  opts.delimiter = "\n";
  opts.qos = QOS0;
//  opts.username = "bentur";
//  opts.password = "1234";
  opts.host = targetIP;
  opts.port = targetPort;
  opts.showtopics = 0;
  // opts.showtopics = 1;
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)//if time 0.3 make interrupt
  {

	  HAL_TIM_Base_Stop_IT(&htim3);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, RESET);
	  lock=0;
  }
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	//lock is used to prevent bouncing in situations where two falling edges occur in close time
	//so if two edges comes in close time after the first one lock will be one for 0.3 second->(3000*7200)/(72*10^6)


		if(lock==0)
		{
			HAL_TIM_Base_Start_IT(&htim3);
			lock=1;
			 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, SET);

		}


}




/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_SPI1_Init();

  MX_TIM3_Init();
  // HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
  // HAL_Delay(500);
  // HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
  // HAL_Delay(500);
  // HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
  // HAL_Delay(500);

  // opts.clientid = "subscriber";
  // opts.nodelimiter = 0;
  // opts.delimiter = "\n";
  // opts.qos = QOS0;
  // opts.username = "bentur";
  // opts.password = "1234";
  // opts.host = targetIP;
  // opts.port = targetPort;
  // opts.showtopics = 0;
  configureMQttClient();

  uint8_t mac[] = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef}; // Mac address
  uint8_t ip[] = {192, 168, 1, 144};                    // IP address
  uint8_t sn[] = {255, 255, 255, 0};                    // Subnet mask
  uint8_t gw[] = {192, 168, 1, 1};                      // Gateway address

  initializeW5500(mac, ip, sn, gw); // configures the MAC address, IP address, subnet mask and gateway of the device or node.

  int rc = 0;
  unsigned char buf[100];

  Network n;

  NewNetwork(&n, SOC_NUM);
  ConnectNetwork(&n, targetIP, targetPort);
  MQTTClientInit(&mqttClient, &n, 1000, buf, 100, tempBuffer, 2048);

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.willFlag = 0;
  data.MQTTVersion = 3;
  data.clientID.cstring = opts.clientid;
  data.username.cstring = opts.username;
  data.password.cstring = opts.password;
  data.keepAliveInterval = 60;
  data.cleansession = 1;

  while (MQTTConnect(&mqttClient, &data) != SUCCESSS)
    ;

  while (MQTTSubscribe(&mqttClient, "Subscribe/Topic", opts.qos, messageArrived) != SUCCESSS)
    ;

  /* Infinite loop */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, RESET);
  /* USER CODE BEGIN WHILE */

  while (1)
  {

    schedule(2000);

    MQTTYield(&mqttClient, data.keepAliveInterval);
  }
  /* USER CODE END 3 */
}
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7200-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 50000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OnePulse_Init(&htim3, TIM_OPMODE_SINGLE) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
	  GPIO_InitTypeDef GPIO_InitStruct = {0};

	  /* GPIO Ports Clock Enable */
	  __HAL_RCC_GPIOC_CLK_ENABLE();
	  __HAL_RCC_GPIOD_CLK_ENABLE();
	  __HAL_RCC_GPIOA_CLK_ENABLE();

	  /*Configure GPIO pin Output Level */
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);

	  /*Configure GPIO pin Output Level */
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_12, GPIO_PIN_RESET);

	  /*Configure GPIO pin : PC14 */
	  GPIO_InitStruct.Pin = GPIO_PIN_14;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	  /*Configure GPIO pin : PA1 */
	  GPIO_InitStruct.Pin = GPIO_PIN_1;
	  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	  GPIO_InitStruct.Pull = GPIO_PULLUP;
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  /*Configure GPIO pins : PA4 PA12 */
	  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_12;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	  /*Configure GPIO pin : PA9 */
	  GPIO_InitStruct.Pin = GPIO_PIN_9;
	  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	  /*Configure GPIO pin : PA10 */
	  GPIO_InitStruct.Pin = GPIO_PIN_10;
	  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	  /*Configure GPIO pin : PA13 */
	  GPIO_InitStruct.Pin = GPIO_PIN_13;
	  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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

#ifdef  USE_FULL_ASSERT
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
