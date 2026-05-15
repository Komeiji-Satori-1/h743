#include "usart_deal.h"

#define len 128

extern volatile uint8_t task_measure;
extern volatile uint8_t task_sweep;
extern volatile uint8_t task_fault;
extern volatile uint8_t task_none;

typedef enum
{
    CMD_NUMB,
    TEST01,
    TEST02,
    TEST03,
} USART_RX_CMD_TYPE;
USART_RX_CMD_TYPE usart_rx_cmd_state;

uint8_t usart_rx_buffer[len] = {0};
volatile uint8_t usart_rx_buffer_index = 0;
volatile uint8_t usart_rx_proc_flag = 0;
char usart_rx_cmd[len] = {0};
uint8_t uart_rx_byte = 0;


void Usart_Send_Computer(UART_HandleTypeDef huart, char *msg)
{
    HAL_UART_Transmit(&huart, (uint8_t *)msg, strlen(msg), 1000);
}

//void My_Usart_Init(void)
//{
//    printf("bkcmd=0\xff\xff\xff");
//    HAL_Delay(20);

//    memset(usart_rx_buffer, 0, sizeof(usart_rx_buffer));
//    usart_rx_buffer_index = 0;
//    usart_rx_proc_flag = 0;

//    HAL_UART_Receive_IT(&huart1, &usart_rx_buffer[usart_rx_buffer_index], 1);
//}

void My_Usart_Init(void)
{
    printf("bkcmd=0\xff\xff\xff");
    HAL_Delay(20);

    uart_rx_byte = 0;
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
}


//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    if (huart->Instance == USART1)
//    {
//        if (usart_rx_buffer[usart_rx_buffer_index] == '\0')
//        {
//            usart_rx_proc_flag = 1; // 设置处理标志，表示接收完成
//        }
//        else
//        {
//            usart_rx_buffer_index++;
//            HAL_UART_Receive_IT(&huart1, &usart_rx_buffer[usart_rx_buffer_index], 1);
//        }
//    }
//}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        switch (uart_rx_byte)
        {
        case 0x01:
            task_measure = 1;
            task_sweep = 0;
            task_fault = 0;
            task_none = 0;
            break;

        case 0x02:
            task_measure = 0;
            task_sweep = 1;
            task_fault = 0;
            task_none = 0;
            break;

        case 0x03:
            task_measure = 0;
            task_sweep = 0;
            task_fault = 1;
            task_none = 0;
            break;

        case 0x04:
            task_measure = 0;
            task_sweep = 0;
            task_fault = 0;
            task_none = 1;
            break;

        default:
            break;
        }

        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    }
}


//void Usart_Rx_Proc(void)
//{
//    if (usart_rx_proc_flag)
//    {

//        usart_rx_proc_flag = 0;// 重置处理标志
//        strcpy(usart_rx_cmd, (char *)usart_rx_buffer); // 将接收的命令复制到处理缓冲区
//        if (strcmp(usart_rx_cmd, "test01") == 0)
//        {
//            usart_rx_cmd_state = TEST01;
//        }
//        else if (strcmp(usart_rx_cmd, "test02") == 0)
//        {
//            usart_rx_cmd_state = TEST02;
//        }
//        else if (strcmp(usart_rx_cmd, "test03") == 0)
//        {
//            usart_rx_cmd_state = TEST03;
//        }
//        else
//        {
//            usart_rx_cmd_state = CMD_NUMB; // 未识别的命令
//        }

//        switch (usart_rx_cmd_state)
//        {
//        case TEST01:
//            task_measure = 1;
//            task_sweep = 0;
//            task_fault = 0;
//            task_none = 0;
//            break;

//        case TEST02:
//            task_measure = 0;
//            task_sweep = 1;
//            task_fault = 0;
//            task_none = 0;
//            break;

//        case TEST03:
//            task_measure = 0;
//            task_sweep = 0;
//            task_fault = 1;
//            task_none = 0;
//            break;

//        default:
//            task_none = 1;
//            task_measure = 0;
//            task_sweep = 0;
//            task_fault = 0;
//            break;
//        }
//        memset(usart_rx_buffer, 0, sizeof(usart_rx_buffer));
//        usart_rx_buffer_index = 0;
//        HAL_UART_Receive_IT(&huart1, &usart_rx_buffer[usart_rx_buffer_index], 1);
//    }
//}

