/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "gpio.h"
#include "tim.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FFT.h"
#include "HMI.h"
#include "calculate.h"
#include "math.h"
#include "usart_deal.h"

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
#define ADC_SIZE 1024
#define rank 2
#define TIM3_COUNTER_CLK_HZ 20000000
/* 通道配置：ADC_Us=IN16, ADC_Ui=IN17, ADC_U0=IN5, ADC_8307=IN14
   rank1是buffer的偶数项，rank2是奇数项
   ADC1: rank1=Us, rank2=Ui
   ADC2: rank1=U0, rank2=AD8307 detector output */
uint16_t ADC_Buffer1[ADC_SIZE * rank] = {0};
uint16_t ADC_Buffer2[ADC_SIZE * rank] = {0};
uint16_t ADC_Us[ADC_SIZE] = {0};
uint16_t ADC_U0[ADC_SIZE] = {0};
uint16_t ADC_Ui[ADC_SIZE] = {0};
uint16_t ADC_8307[ADC_SIZE] = {0};

/* Set only after both ADC1 and ADC2 finish one complete DMA frame. */
volatile uint8_t ADC_Flag = 0;
static volatile uint8_t adc1_done = 0;
static volatile uint8_t adc2_done = 0;
/* Input/output resistance values used by the basic measurement functions. */
int RL = 2000;
int Rs = 10000;
int r = 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Set_ADC_SampleRate(float sample_rate_hz)
{
    uint32_t arr;

    if (sample_rate_hz < 500.0f)
        sample_rate_hz = 500.0f;

    arr = (uint32_t)((TIM3_COUNTER_CLK_HZ / sample_rate_hz) + 0.5f);

    if (arr > 0U)
        arr -= 1U;

    HAL_TIM_Base_Stop(&htim3);

    __HAL_TIM_SET_AUTORELOAD(&htim3, arr);
    __HAL_TIM_SET_COUNTER(&htim3, 0U);

    HAL_TIM_Base_Start(&htim3);

    FFT_SetSampling(TIM3_COUNTER_CLK_HZ / (float)(arr + 1U));
}

void Split_ADC_Buffers(void)
{
    for (uint16_t i = 0; i < ADC_SIZE; i++)
    {
        ADC_Us[i] = ADC_Buffer1[2U * i];
        ADC_Ui[i] = ADC_Buffer1[2U * i + 1U];
        ADC_U0[i] = ADC_Buffer2[2U * i];
        ADC_8307[i] = ADC_Buffer2[2U * i + 1U];
    }
}

/* Start one new synchronous acquisition frame on ADC1 and ADC2. */
void Start_ADC_Capture(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_ADC_Stop_DMA(&hadc2);

    adc1_done = 0;
    adc2_done = 0;
    ADC_Flag = 0;

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC_Buffer1, ADC_SIZE * rank);
    HAL_ADC_Start_DMA(&hadc2, (uint32_t *)ADC_Buffer2, ADC_SIZE * rank);
}

/* Blocking helper for FFT/sweep functions when a fresh full frame is needed. */
uint8_t Acquire_All_ADC_Samples_Blocking(uint32_t timeout_ms)
{
    uint32_t tickstart = HAL_GetTick();

    Start_ADC_Capture();

    while (ADC_Flag == 0U)
    {
        if ((HAL_GetTick() - tickstart) > timeout_ms)
        {
            return 0U;
        }
    }

    ADC_Flag = 0U;
    Split_ADC_Buffers();
    return 1U;
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

    /* Configure the peripherals common clocks */
    PeriphCommonClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_ADC2_Init();
    MX_USART1_UART_Init();
    MX_TIM3_Init();
    /* USER CODE BEGIN 2 */
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    HAL_TIM_Base_Start(&htim3);
    ad9833_init();
    ad9833_set_amplitude(128);
    HAL_Delay(1000); // Nextion 在这 1 秒内完成启动
    My_Usart_Init(); // ← 现在 Nextion 能正确接收并执行 bkcmd=0
    Start_ADC_Capture();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        //         if(ADC_Flag==1){
        //              Split_ADC_Buffers();

        //              Calculate_Input_Impedance(Rs);
        //              Calculate_Output_Impedance(RL);
        //              Calculate_Gain();
        //              //sweep_freq(1000, 200000, 1000);
        //              Start_ADC_Capture();
        //           }

        main_state();

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

    /** Supply configuration update enable
     */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    /** Configure the main internal regulator output voltage
     */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
    {
    }
    /** Macro to configure the PLL clock source
     */
    __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief Peripherals Common Clock Configuration
 * @retval None
 */
void PeriphCommonClock_Config(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /** Initializes the peripherals clock
     */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInitStruct.PLL2.PLL2M = 2;
    PeriphClkInitStruct.PLL2.PLL2N = 12;
    PeriphClkInitStruct.PLL2.PLL2P = 2;
    PeriphClkInitStruct.PLL2.PLL2Q = 2;
    PeriphClkInitStruct.PLL2.PLL2R = 2;
    PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
    PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOMEDIUM;
    PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
    PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1)
    {
        HAL_ADC_Stop_DMA(hadc);
        adc1_done = 1U;
    }
    else if (hadc == &hadc2)
    {
        HAL_ADC_Stop_DMA(hadc);
        adc2_done = 1U;
    }

    if (adc1_done == 1U && adc2_done == 1U)
    {
        ADC_Flag = 1U;
    }
}
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/