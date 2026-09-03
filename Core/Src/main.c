/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led.h"
#include "bsp_rs485.h"
#include "bsp_board_cfg.h"
#include "bsp_tick.h"
#include "modbus_slave.h"
#include "modbus_register.h"
#include "modbus.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* v0.4 demo: real Modbus slave engine on the SLAVE node.
 * MASTER node keeps a raw probe (0x03 request every 500ms) to verify
 * the two-board link until the full master stack lands in v0.7. */
static volatile uint32_t s_master_rx_cnt = 0;
static volatile uint32_t s_master_rx_ok  = 0;
static uint32_t s_probe_tick = 0;
static const uint8_t s_probe_frame[] = {
    /* slave 0x01, FC 0x03, start 0x0000, qty 0x0002, CRC computed below */
    0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#if (MODBUS_NODE_ROLE == NODE_ROLE_SLAVE)
/* feed every RX byte into the Modbus slave engine */
static void RS485_RxToSlave(uint8_t byte)
{
  MB_Slave_OnRxByte(byte, BSP_Tick_GetUs());
}
#else
/* MASTER probe mode: count raw bytes/response received */
static void RS485_RxProbeCb(uint8_t byte)
{
  (void)byte;
  s_master_rx_cnt++;
}
#endif
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
  /* USER CODE BEGIN 2 */
  LED_Init();
  BSP_Tick_Init();
  MB_REG_Init();
  RS485_Init(RS485_DEFAULT_BAUDRATE);

#if (MODBUS_NODE_ROLE == NODE_ROLE_SLAVE)
  MB_Slave_Init(RS485_DEFAULT_SLAVE_ID, RS485_DEFAULT_BAUDRATE);
  MB_Slave_SetTxFunc(RS485_SendFrame);
  RS485_SetRxCallback(RS485_RxToSlave);
#else
  RS485_SetRxCallback(RS485_RxProbeCb);
  s_probe_tick = HAL_GetTick();
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

#if (MODBUS_NODE_ROLE == NODE_ROLE_SLAVE)
    /* SLAVE: protocol engine poll (frame timeout detection + dispatch) */
    if (MB_Slave_Poll(BSP_Tick_GetUs()) != 0U)
    {
      LED2_Toggle;   /* a response was sent */
    }
    HAL_Delay(1);
#else
    /* MASTER probe: send read request every 500ms, LED1 = link alive */
    if ((HAL_GetTick() - s_probe_tick) >= 500U)
    {
      uint32_t before = s_master_rx_cnt;
      s_probe_tick = HAL_GetTick();
      RS485_SendFrame(s_probe_frame, sizeof(s_probe_frame));
      /* simple wait 10ms then check if any bytes came back */
      HAL_Delay(10);
      if (s_master_rx_cnt > before)
      {
        s_master_rx_ok++;
        LED1_ON;      /* response received */
        HAL_Delay(80);
        LED1_OFF;
      }
    }
    HAL_Delay(5);
#endif

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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 75;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
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
