#include "HMI.h"
#include "usart.h"

// 发送字符串（用于文本显示）
void HMI_send_string(char* obj_name, char* showdata)
{
    // 格式: obj_name.txt="content" + 0xFF 0xFF 0xFF
    printf("%s.txt=\"%s\"\xff\xff\xff", obj_name, showdata);
}

// 发送整数
void HMI_send_number(char* obj_name, int num)
{
    // 格式: obj_name.val=num + 0xFF 0xFF 0xFF
    printf("%s.val=%d\xff\xff\xff", obj_name, num);
}

// 发送浮点数（转换为整数或特定精度）
void HMI_send_float(char* obj_name, float num)
{
    // 格式: obj_name.val=num (乘以1000保留3位小数) + 0xFF 0xFF 0xFF
    printf("%s.val=%d\xff\xff\xff", obj_name, (int)(num * 1000));
}

// 添加波形数据点（单个点）
void HMI_Wave(int wf_id, int ch, int val)
{
    uint8_t buf[24];
    int len = snprintf((char *)buf, sizeof(buf) - 3, "add %d,%d,%d", wf_id, ch, val);
    buf[len]     = 0xFF;
    buf[len + 1] = 0xFF;
    buf[len + 2] = 0xFF;
    HAL_UART_Transmit(&huart1, buf, (uint16_t)(len + 3), 1000);
}

// 快速波形显示（连续数据）
void HMI_Wave_Fast(int wf_id, int ch, int count, uint8_t *data)
{
    uint8_t cmd[24];
    int len = snprintf((char *)cmd, sizeof(cmd) - 3, "addt %d,%d,%d", wf_id, ch, count);
    cmd[len]     = 0xFF;
    cmd[len + 1] = 0xFF;
    cmd[len + 2] = 0xFF;
    HAL_UART_Transmit(&huart1, cmd, (uint16_t)(len + 3), 1000);
    HAL_Delay(20);
    HAL_UART_Transmit(&huart1, data, (uint16_t)count, 1000);
    HAL_Delay(20);
}

// 清空波形
void HMI_Wave_Clear(int wf_id, int ch)
{
    uint8_t buf[20];
    int len = snprintf((char *)buf, sizeof(buf) - 3, "cle %d,%d", wf_id, ch);
    buf[len]     = 0xFF;
    buf[len + 1] = 0xFF;
    buf[len + 2] = 0xFF;
    HAL_UART_Transmit(&huart1, buf, (uint16_t)(len + 3), 1000);
}

// 额外功能：设置组件属性
void HMI_set_property(char* obj_name, char* property, int value)
{
    // 格式: obj_name.property=value + 0xFF 0xFF 0xFF
    printf("%s.%s=%d\xff\xff\xff", obj_name, property, value);
}

// 设置页面
void HMI_Set_Page(char *page_id)
{
    // 格式: page page_id + 0xFF 0xFF 0xFF
    printf("page %s\xff\xff\xff", page_id);
}



