#ifndef	__usart_deal_H__
#define __usart_deal_H__

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include "string.h"
#include "usart.h"
#include "main.h"
#include "HMI.h"

void Usart_Send_Computer(UART_HandleTypeDef huart , char *msg);
void My_Usart_Init(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
//void Usart_Rx_Proc(void);

#endif
