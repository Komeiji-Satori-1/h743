#ifndef	__HMI_H__
#define __HMI_H__
 
#include "stm32h7xx_hal.h"
#include <stdint.h>  // 添加这个，用于 uint8_t 类型
 
void HMI_send_string(char* name, char* showdata);
void HMI_send_number(char* name, int num);
void HMI_send_float(char* name, float num);
void HMI_Wave(int wf_id, int ch, int val);
void HMI_Wave_Fast(int wf_id, int ch, int count, uint8_t* data);
void HMI_Wave_Clear(int wf_id, int ch);
void HMI_set_property(char* obj_name, char* property, int value);  // 新增
void HMI_Set_Page(char *page_id);

#endif