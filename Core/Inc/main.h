/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RELAY_1_Pin GPIO_PIN_0
#define RELAY_1_GPIO_Port GPIOC
#define ADC_Us_Pin GPIO_PIN_0
#define ADC_Us_GPIO_Port GPIOA
#define ADC_Ui_Pin GPIO_PIN_1
#define ADC_Ui_GPIO_Port GPIOA
#define ADC_U0_Pin GPIO_PIN_2
#define ADC_U0_GPIO_Port GPIOA
#define AD9833_Cs_Pin GPIO_PIN_4
#define AD9833_Cs_GPIO_Port GPIOA
#define ADC_AD8307_Pin GPIO_PIN_1
#define ADC_AD8307_GPIO_Port GPIOB
#define AD9833_FSY_Pin GPIO_PIN_11
#define AD9833_FSY_GPIO_Port GPIOH
#define AD9833_SDA_Pin GPIO_PIN_12
#define AD9833_SDA_GPIO_Port GPIOH
#define AD9833_SCK_Pin GPIO_PIN_14
#define AD9833_SCK_GPIO_Port GPIOH
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
