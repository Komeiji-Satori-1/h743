#ifndef AD9833_H
#define AD9833_H

#include "main.h"

/* =========================================================================
 * SPI GPIO 宏（原 My_SPI/My_SPI.h）
 * ========================================================================= */
#define AD9833_SPI_SCK_H    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_14, GPIO_PIN_SET)
#define AD9833_SPI_SCK_L    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_14, GPIO_PIN_RESET)
#define AD9833_SPI_SDA_H    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_12, GPIO_PIN_SET)
#define AD9833_SPI_SDA_L    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_12, GPIO_PIN_RESET)
#define AD9833_SPI_CS_H     HAL_GPIO_WritePin(GPIOH, GPIO_PIN_11, GPIO_PIN_SET)
#define AD9833_SPI_CS_L     HAL_GPIO_WritePin(GPIOH, GPIO_PIN_11, GPIO_PIN_RESET)
#define MCP41010_CS_H       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
#define MCP41010_CS_L       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)

/* =========================================================================
 * AD9833 寄存器地址
 * ========================================================================= */
#define ad9833_Reg_control  (0 << 14)
#define ad9833_Reg_freq0    (1 << 14)
#define ad9833_Reg_freq1    (2 << 14)
#define ad9833_Reg_phase0   (6 << 13)
#define ad9833_Reg_phase1   (7 << 13)

/* =========================================================================
 * AD9833 控制位
 * ========================================================================= */
#define ad9833_Reg_control_B28      (1 << 13)
#define ad9833_Reg_control_HLB      (1 << 12)
#define ad9833_Reg_control_FSELECT  (1 << 11)
#define ad9833_Reg_control_PSELECT  (1 << 10)
#define ad9833_Reg_control_Reset    (1 << 8)
#define ad9833_Reg_control_SLEEP1   (1 << 7)
#define ad9833_Reg_control_SLEEP12  (1 << 6)
#define ad9833_Reg_control_OPBITEN  (1 << 5)
#define ad9833_Reg_control_DIV2     (1 << 3)
#define ad9833_Reg_control_MODE     (1 << 1)

/* =========================================================================
 * 通道选择
 * ========================================================================= */
#define ad9833_CH0  0
#define ad9833_CH1  1

/* =========================================================================
 * 频率字计算常量
 * ========================================================================= */
#define AD9833_MCLK_HZ          25000000UL
#define AD9833_FREQ_BITS        28
#define AD9833_FREQ_DATA_BITS   14
#define AD9833_FREQ_WORD_SCALE  (1ULL << AD9833_FREQ_BITS)
#define AD9833_FREQ_WORD_MAX    ((1UL << AD9833_FREQ_BITS) - 1UL)
#define AD9833_FREQ_DATA_MASK   ((1U << AD9833_FREQ_DATA_BITS) - 1U)
#define ad9833_Freq             1000

/* =========================================================================
 * 波形类型
 * ========================================================================= */
#define ad9833_Sine         ((0 << 5) | (0 << 1) | (0 << 3))
#define ad9833_Triangle     ((0 << 5) | (1 << 1) | (0 << 3))
#define ad9833_Square       ((1 << 5) | (0 << 1) | (1 << 3))
#define ad9833_Square_Div2  ((1 << 5) | (0 << 1) | (0 << 3))

/* =========================================================================
 * 扫频状态结构体
 * ========================================================================= */
typedef struct
{
    uint32_t start_hz;
    uint32_t stop_hz;
    uint32_t step_hz;
    uint32_t current_hz;
    uint32_t dwell_ms;
    uint32_t last_tick;
    uint16_t type;
    uint8_t  ch;
    uint8_t  enable;
} ad9833_sweep_t;

/* =========================================================================
 * 波形配置结构体（原 My_BLL/dds.h）
 * ========================================================================= */
typedef struct
{
    uint16_t type;
    uint16_t ch;
    uint32_t freq;
    uint16_t phase;
} WaveFormConfig_t;

extern WaveFormConfig_t WaveFormConfig;

/* =========================================================================
 * 函数声明
 * ========================================================================= */

/* SPI底层 */
void AD9833_SPI_16bits_Write(uint16_t data);

/* AD9833驱动层 */
void     ad9833_init(void);
void     ad9833_write_reg(uint16_t value);
void     ad9833_set_waveform(uint16_t type);
void     ad9833_set_freq(uint32_t freq, uint16_t type);
void     ad9833_set_freq_ch(uint32_t freq, uint16_t type, uint8_t ch);
void     ad9833_set_freq_word(uint32_t freq_word, uint16_t type, uint8_t ch);
uint32_t ad9833_freq_to_word(uint32_t freq_hz);
void     ad9833_set_amplitude(uint8_t amp);

/* 扫频接口 */
void    ad9833_sweep_start(ad9833_sweep_t *sweep, uint32_t start_hz, uint32_t stop_hz, uint32_t step_hz, uint32_t dwell_ms, uint16_t type);
void    ad9833_sweep_start_ch(ad9833_sweep_t *sweep, uint32_t start_hz, uint32_t stop_hz, uint32_t step_hz, uint32_t dwell_ms, uint16_t type, uint8_t ch);
uint8_t ad9833_sweep_process(ad9833_sweep_t *sweep);
void    ad9833_sweep_stop(ad9833_sweep_t *sweep);

/* BLL封装层（原 My_BLL/dds.h） */
void waveset(uint32_t Freq, uint16_t type, uint16_t ch);

/* APL应用层（原 My_APL/dds_apl.h） */
void dds_process(void);

#endif /* AD9833_H */
