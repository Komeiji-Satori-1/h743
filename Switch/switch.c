#include "gpio.h"

/*继电器闭合*/
void Relay_On(void)
{
    HAL_GPIO_WritePin(RELAY_1_GPIO_Port, RELAY_1_Pin, GPIO_PIN_RESET);
    //printf("relay on\n");
}

/*继电器断开*/
void Relay_Off(void)
{
    HAL_GPIO_WritePin(RELAY_1_GPIO_Port, RELAY_1_Pin, GPIO_PIN_SET);
    //printf("relay off\n");
}
