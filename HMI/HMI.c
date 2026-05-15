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
// wf_id: 控件的数字 id（属性面板中的 id 字段），不是控件名
void HMI_Wave(int wf_id, int ch, int val)
{
    // 格式: add id,ch,val + 0xFF 0xFF 0xFF
    printf("add %d,%d,%d\xff\xff\xff", wf_id, ch, val);
}

// 快速波形显示（连续数据）
void HMI_Wave_Fast(char* wf_name, int ch, int count, float* show_data)
{
    printf("addt %s,%d,%d\xff\xff\xff", wf_name, ch, count);
    HAL_Delay(20);

    HAL_UART_Transmit(&huart1, (uint8_t*)show_data, count, 1000);

    HAL_Delay(20);
}

// 清空波形
// wf_id: 控件的数字 id，255 表示清空所有通道
void HMI_Wave_Clear(int wf_id, int ch)
{
    // 格式: cle id,ch + 0xFF 0xFF 0xFF
    printf("cle %d,%d\xff\xff\xff", wf_id, ch);
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



