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
#include "sensor.h"
#include "mqtt_client.h"
#include "wifi_esp8266.h"
#include "data_report.h"
#include "edge_alarm.h"
#include "st7735.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define REPORT_INTERVAL_MS    5000U    /* 数据上报周期：5 秒  */
#define HEARTBEAT_INTERVAL_MS 30000U   /* 心跳周期：30 秒     */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void report_alarm(const SensorData *d, AlarmLevel level);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* 网络连接状态：1=已连，0=未连（未连时跳过上报，但仍实时采集显示） */
static int net_ok = 0;

/* 边缘告警发布：判定为预警/告警时，推送到 shm/{device_id}/alarm */
static void report_alarm(const SensorData *d, AlarmLevel level)
{
    char json[128];
    char topic[64];
    snprintf(json, sizeof(json),
             "{\"device_id\":\"%s\",\"type\":\"%s\",\"level\":%d,\"value\":%.3f,\"unit\":\"%s\"}",
             DEVICE_ID, SENSOR_TYPE_STR[d->type], (int)level, d->value, d->unit);
    snprintf(topic, sizeof(topic), TOPIC_ALARM, DEVICE_ID);
    MQTT_Publish(topic, json, (uint16_t)strlen(json), 0);
}

/* LCD 传感器缩写与 ASCII 单位（℃/με 等非 ASCII 用近似替代） */
static const char *LCD_NAME[SENSOR_TYPE_COUNT] = {
    "VIB", "STR", "TEM", "HUM", "WAT", "SEE", "DIS", "CRA", "LOA"
};
static const char *LCD_UNIT[SENSOR_TYPE_COUNT] = {
    "g", "uE", "C", "%", "m", "mL/m", "mm", "mm", "t"
};

/* 数值列起始 x（缩写 3 字符 + 空格 = 4 字符 = 32px，8x8 字体） */
#define LCD_VAL_X 32
/* 行高（8 像素字符 + 4 像素间距，改大更松散，最大 12） */
#define LCD_ROW_H 12

/* 画布局：标题 + 传感器名 + 单位（只画一次，数字随后局部刷新） */
static void lcd_draw_layout(void)
{
    ST7735_Clear(LCD_BLACK);
    ST7735_DrawString(0, 0, DEVICE_ID, LCD_YELLOW, LCD_BLACK);
    for (int i = 0; i < SENSOR_TYPE_COUNT; i++) {
        /* 传感器名（左） */
        ST7735_DrawString(0, (i + 1) * LCD_ROW_H, LCD_NAME[i], LCD_WHITE, LCD_BLACK);
        /* 单位（右，固定列） */
        ST7735_DrawString(LCD_VAL_X + 7 * 8 + 8, (i + 1) * LCD_ROW_H,
                          LCD_UNIT[i], LCD_GREEN, LCD_BLACK);
    }
}

/* 更新某行数值：固定 7 字符宽度覆盖，无残留 */
static void lcd_update_value(int row, const SensorData *d)
{
    char val[16];
    snprintf(val, sizeof(val), "%7.3f", d->value);
    ST7735_DrawString(LCD_VAL_X, (row + 1) * LCD_ROW_H, val, LCD_WHITE, LCD_BLACK);
}
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
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* 传感器初始化 */
  Sensor_InitAll();

  /* LCD 初始化 */
  ST7735_Init();

  /* 画布局（标题 + 名称 + 单位，只画一次） */
  lcd_draw_layout();

  /* WiFi 与 MQTT 连接：失败不阻塞，继续采集显示，只是不上报 */
  net_ok = 1;
  if (WiFi_Init() != 0) {
    net_ok = 0;
  }
  if (net_ok && WiFi_ConnectServer() != 0) {
    net_ok = 0;
  }
  if (net_ok) {
    /* 建立 MQTT 连接（broker 当前匿名，用户名/密码留空即可；
     * 若服务器开启认证，在此填入 cli.username / cli.password 再连接） */
    MQTT_Client cli;
    memset(&cli, 0, sizeof(cli));
    strncpy(cli.client_id, DEVICE_ID, sizeof(cli.client_id) - 1);
    cli.keepalive = 60;
    if (MQTT_Connect(&cli) != 0) {
      net_ok = 0;
    }
  }

  /* 周期计时起点 */
  uint32_t last_report = 0;
  uint32_t last_heartbeat = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();

    /* 周期采集 → 边缘告警 → 数据上报 */
    if (now - last_report >= REPORT_INTERVAL_MS) {
      last_report = now;

      SensorData data[SENSOR_TYPE_COUNT];
      int n = Sensor_ReadAll(data, SENSOR_TYPE_COUNT);

      for (int i = 0; i < n; i++) {
        AlarmLevel level = EdgeAlarm_CheckOne(&data[i]);
        if (level >= ALARM_LEVEL_WARNING && net_ok) {
          report_alarm(&data[i], level);
        }
      }

      if (net_ok) {
        DataReport_Publish(data, n);
      }

      /* LCD 只局部刷新数值（按传感器类型对齐到对应行，避免跳过失败项后错位） */
      for (int i = 0; i < n; i++) {
        lcd_update_value(data[i].type, &data[i]);
      }
    }

    /* 周期心跳 */
    if (now - last_heartbeat >= HEARTBEAT_INTERVAL_MS) {
      last_heartbeat = now;
      if (net_ok) {
        DataReport_Heartbeat();
      }
    }

    HAL_Delay(50);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);
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

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
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
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
