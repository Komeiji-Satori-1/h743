#include "calculate.h"

/* -----------------------------------------------------------------------
 * 外部引用
 * 这四个数组由 main.c 中的 Split_ADC_Buffers() 填充，
 * 每次 ADC DMA 完成后更新，长度均为 ADC_LEN 点。
 * ----------------------------------------------------------------------- */
extern uint16_t ADC_Us[1024];
extern uint16_t ADC_U0[1024];
extern uint16_t ADC_Ui[1024];
extern uint16_t ADC_8307[1024];
extern float FFT_Input[];
extern uint32_t FFT_mag_max_index;
extern int Rs;
extern int RL;
/* 外部函数：阻塞等待一次完整的 ADC DMA 帧采集完成
 * 用于需要在函数内部主动触发重采样的场合（如 Zo 测量时切换继电器后）
 * 返回 1=成功, 0=超时 */
extern uint8_t Acquire_All_ADC_Samples_Blocking(uint32_t timeout_ms);
extern void Set_ADC_SampleRate(float sample_rate_hz);
extern TIM_HandleTypeDef htim3;
// 变量
float FFT_Ampl1 = 0; // FFT_Process 输出幅值暂存 (第1路)
float FFT_Ampl2 = 0; // FFT_Process 输出幅值暂存 (第2
// int index_ui=0;
// int index_u0=0;
unsigned char buf[64]; // 浮点强制转换为文本 用于串口屏显示
float Ri, R0, Au;
float f = 1000.0f; // 初始频率

typedef enum
{
    FAULT_NORMAL = 0,
    FAULT_R1_OPEN,
    FAULT_R1_SHORT,
    FAULT_R2_OPEN,
    FAULT_R2_SHORT,
    FAULT_R3_OPEN,
    FAULT_R3_SHORT,
    FAULT_R4_OPEN,
    FAULT_R4_SHORT,
    FAULT_C1_OPEN,
    FAULT_C1_DOUBLE,
    FAULT_C2_OPEN,
    FAULT_C2_DOUBLE,
    FAULT_C3_OPEN,
    FAULT_C3_DOUBLE,
    FAULT_UNKNOWN
} errortype;

/**
 * @brief 计算被测放大器输入阻抗 Zi
 *
 * 原理：在信号源和放大器输入端之间串联已知电阻 Rs，
 *       通过 FFT 分别测量 Rs 前（Us）和 Rs 后（Ui）的幅值。
 *       由分压关系：Zi = Rs * Ui / (Us - Ui)
 *
 * 注意：Us 和 Ui 的幅值单位相同（ADC LSB），比值运算中单位自动消除，
 *       因此无需换算为实际电压即可直接计算阻抗（前提是两路前级增益相同）。
 *
 * @param Rs  串联已知电阻阻值 (Ω)
 */
void Calculate_Input_Impedance(int Rs)
{
    FFT_Process(ADC_Ui, &FFT_Ampl1); // 先算 Ui，结果存 FFT_Ampl1
    FFT_Process(ADC_Us, &FFT_Ampl2); // 再算 Us，结果存 FFT_Ampl2
    Ri = (float)Rs * FFT_Ampl1 / (FFT_Ampl2 - FFT_Ampl1);
    // printf("%.3f\n",Ri);
    sprintf((char *)buf, "%.1f", Ri);
    HMI_send_string("t0", (char *)buf);
}

/**
 * @brief 计算被测放大器输出阻抗 Zo
 *
 * 原理：等效输出模型为 Thevenin 源（Voc + Zo 串联）。
 *   - 继电器接通（接负载 RL）: 测 U0（带载输出电压）
 *   - 继电器断开（空载）:      测 U∞（开路输出电压）
 *   - 公式: Zo = RL * (U∞ - U0) / U0
 *
 * 时序：
 *   1. 先在继电器接通状态下（主循环已采好数据）取 U0 → FFT_Ampl1
 *   2. 断开继电器，等待 10ms 信号稳定
 *   3. 主动触发一次新采样（Acquire_All_ADC_Samples_Blocking）
 *   4. 取 U∞ → FFT_Ampl2
 *   5. 计算并发送结果，然后恢复继电器接通状态
 *
 * @param RL  外接负载电阻阻值 (Ω)
 */
void Calculate_Output_Impedance(int RL)
{
    /* 步骤1: 继电器当前接通，此帧数据即为带载测量值 */
    FFT_Process(ADC_U0, &FFT_Ampl1); // FFT_Ampl1 = U0 (带载)

    /* 步骤2: 断开负载，等待信号稳定 */

    Relay_Off();
    HAL_Delay(50);
    /* 步骤3~4: 重采样，获取空载电压 */
    if (Acquire_All_ADC_Samples_Blocking(200U) == 1U)
    {
        FFT_Process(ADC_U0, &FFT_Ampl2); // FFT_Ampl2 = U∞ (空载)
        R0 = (float)RL * (FFT_Ampl2 - FFT_Ampl1) / FFT_Ampl1;
        // printf("%.3f\n",R0);
        sprintf((char *)buf, "%.1f", R0);
        HMI_send_string("t1", (char *)buf);
    }

    /* 步骤5: 恢复负载接通，保证后续测量环境一致 */
    Relay_On();
    HAL_Delay(50);
}

/**
 * @brief 计算被测放大器电压增益 Av (dB)
 *
 * 公式：Av = 20 * lg(U0 / Ui)
 * 正值表示放大，负值表示衰减。
 *
 * 注意：此处使用 FFT 幅值比（峰值之比），由于是比值运算，
 *       不需要换算为实际电压，单位一致即可。
 */
void Calculate_Gain(void)
{
    Relay_Off();
    HAL_Delay(50);

    if (Acquire_All_ADC_Samples_Blocking(200U) == 1U)
    {
        FFT_Process(ADC_U0, &FFT_Ampl1); // FFT_Ampl1 = |U0| 幅值
        FFT_Process(ADC_Ui, &FFT_Ampl2); // FFT_Ampl2 = |Ui| 幅值

        /* Av(dB) = 20*log10(U0/Ui)，保护除零 */
        if (FFT_Ampl2 > 1e-6f)
            if (f <= 100000)
            {
                Au = 20.0f * log10f(FFT_Ampl1 * 2.6f / FFT_Ampl2 * 54.0f);
            }
            else if (f > 100000 & f <= 200000)
            {
                Au = 20.0f * log10f(FFT_Ampl1 * 2.6f / FFT_Ampl2 * (-0.0958f * f / 1000.0f + 63.207f));
            }
            else
                Au = 0.0f;
        // printf("%.3f\n",Au);
        sprintf((char *)buf, "%.1f", Au);
        HMI_send_string("t2", (char *)buf);
    }

    Relay_On();
    HAL_Delay(50);
}

float Calculate_UpperCutoff_Freq(float *freq_buf, float *gain_buf, uint16_t n, float ref_gain_db)
{
    float threshold = ref_gain_db - 3.0f;
    if (n < 2)
    {
        return 0.0f;
    }

    // 对数插值
    for (uint16_t i = 1; i < n; i++)
    {
        if (gain_buf[i - 1] > threshold && gain_buf[i] <= threshold)
        {
            float f1 = freq_buf[i - 1];
            float f2 = freq_buf[i];
            float g1 = gain_buf[i - 1];
            float g2 = gain_buf[i];

            if (f1 <= 0.0f || f2 <= 0.0f || fabsf(g2 - g1) < 1e-6f)
                return f1;

            float log_f1 = log10f(f1);
            float log_f2 = log10f(f2);

            float log_fc = log_f1 + (threshold - g1) * (log_f2 - log_f1) / (g2 - g1);

            return powf(10.0f, log_fc);
        }
    }

    return 0.0f;
}

void sweep_freq(float begin_freq, float end_freq, float step_freq)
{
    int point_count = (end_freq - begin_freq) / step_freq + 1;
    static float sweep_gain[200];
    static float sweep_freq_buf[200];
    static uint8_t wave_data[200];
    uint8_t j = 0;

    for (f = begin_freq; f <= end_freq; f += step_freq)
    {
        ad9833_set_freq_ch(f, ad9833_Sine, ad9833_CH0);
        ad9833_set_amplitude(128);
        HAL_Delay(100); // 等待信号稳定

        if (f <= 8000.0f)
        {
            if (Acquire_All_ADC_Samples_Blocking(200U) != 1U)
            {
                continue;
            }
            Calculate_Gain();
            sweep_gain[j] = Au;
            sweep_freq_buf[j] = f;
            wave_data[j] = (uint8_t)((Au - 35.0f) * 255.0f / 15.0f);
            j++;
        }
        else
        {
            float alias_freq = 1000.0f;
            uint32_t k = roundf(f / 20000.0f);
            if (k < 1)
                k = 1;

            float target_fs = (f - alias_freq) / k;
            Set_ADC_SampleRate(target_fs);
            HAL_Delay(100);

            if (Acquire_All_ADC_Samples_Blocking(200U) != 1U)
            {
                continue;
            }
            Calculate_Gain();
            sweep_gain[j] = Au;
            sweep_freq_buf[j] = f;
            wave_data[j] = (uint8_t)((Au - 35.0f) * 255.0f / 15.0f);
            j++;
        }
    }

    float ref_gain_db = 0.0f;
    uint16_t count = j < 5 ? j : 5;
    float sum = 0.0f;
    for (uint16_t i = 0; i < count; i++)
    {
        sum += sweep_gain[i];
    }
    ref_gain_db = sum / count;
    // showdata(sweep_gain, point_count);
    float upper_cutoff_freq = Calculate_UpperCutoff_Freq(sweep_freq_buf, sweep_gain, j, ref_gain_db);
    sprintf((char *)buf, "%.2f", upper_cutoff_freq);
    HMI_send_string("freq", (char *)buf);

    HMI_Wave_Fast("s0", 1, j, wave_data);

    memset(sweep_gain, 0, sizeof(sweep_gain));
    memset(sweep_freq_buf, 0, sizeof(sweep_freq_buf));
    memset(wave_data, 0, sizeof(wave_data));
}

float Calculate_Phase_Deg(void)
{
    float phase_in, phase_out, phase_deg;
    uint32_t base_index;

    // Ui相位
    FFT_Process(ADC_Ui, &FFT_Ampl1);
    base_index = FFT_mag_max_index;
    phase_in = atan2f(FFT_Input[2U * base_index + 1U], FFT_Input[2U * base_index]);

    // U0相位
    FFT_Process(ADC_U0, &FFT_Ampl2);
    phase_out = atan2f(FFT_Input[2U * base_index + 1U], FFT_Input[2U * base_index]);
    phase_deg = (phase_out - phase_in) * 180.0f / PI;

    while (phase_deg > 180.0f)
    {
        phase_deg -= 360.0f;
    }
    while (phase_deg < -180.0f)
    {
        phase_deg += 360.0f;
    }

    return phase_deg;
}

void ErrorDetect(void)
{
    ad9833_set_freq_ch(1000.0f, ad9833_Sine, ad9833_CH0);
    ad9833_set_amplitude(128);
    HAL_Delay(100);
    Acquire_All_ADC_Samples_Blocking(200U);
    float vdc_1k = Calculate_DC_Value(ADC_U0);
    Calculate_Gain();
    float gain_1k = Au;
    Calculate_Input_Impedance(Rs);
    float ri_1k = Ri;
    Calculate_Output_Impedance(RL);
    float r0_1k = R0;
    float phase_1k = Calculate_Phase_Deg();

    /* 1MHz */
    ad9833_set_freq_ch(1000000.0f, ad9833_Sine, ad9833_CH0);
    HAL_Delay(100);
    Acquire_All_ADC_Samples_Blocking(200U);
    Calculate_Gain();
    float gain_1m = Au;
    Calculate_Input_Impedance(Rs);
    float ri_1m = Ri;
    Calculate_Output_Impedance(RL);
    float r0_1m = R0;
    float phase_1m = Calculate_Phase_Deg();

    errortype fault = FAULT_UNKNOWN;

    /* 优先级1: 极端异常 - R3开路 */
    if (vdc_1k < 150.0f && r0_1k > 25000.0f)
    {
        fault = FAULT_R3_OPEN;
    }
    /* 优先级2: 极端异常 - R4短路 */
    else if (vdc_1k < 15.0f && gain_1k < -33.0f)
    {
        fault = FAULT_R4_SHORT;
    }
    /* 优先级3: 饱和状态细分 */
    else if (vdc_1k > 2900.0f)
    {
        if (r0_1k > -20.0f && r0_1k < 20.0f)
        {
            fault = FAULT_R3_SHORT;
        }
        else if (ri_1k > 500000.0f)
        {
            fault = FAULT_C1_OPEN;
        }
        else if (ri_1k > 130.0f && ri_1k < 220.0f &&
                 ri_1m > 30.0f && ri_1m < 70.0f)
        {
            fault = FAULT_C1_DOUBLE;
        }
        else if (ri_1k > 13000.0f && ri_1k < 17000.0f)
        {
            fault = FAULT_R1_OPEN;
        }
        else if (ri_1k > 9500.0f && ri_1k < 12500.0f &&
                 ri_1m > 2800.0f && ri_1m < 3600.0f)
        {
            fault = FAULT_R4_OPEN;
        }
        else if (ri_1k > 130.0f && ri_1k < 180.0f &&
                 ri_1m > 40.0f && ri_1m < 65.0f)
        {
            fault = FAULT_R2_OPEN;
        }
        else if (phase_1k > 30.0f && phase_1k < 90.0f &&
                 ri_1m > 450.0f && ri_1m < 800.0f)
        {
            fault = FAULT_R2_SHORT;
        }
    }
    /* 优先级4: R1短路 */
    else if (vdc_1k > 2800.0f && vdc_1k < 2850.0f &&
             gain_1k < -43.0f)
    {
        fault = FAULT_R1_SHORT;
    }
    /* 优先级5: C2开路 */
    else if (vdc_1k > 1820.0f && vdc_1k < 1850.0f &&
             gain_1k < -28.0f && phase_1k > 165.0f)
    {
        fault = FAULT_C2_OPEN;
    }
    /* 优先级6: 直流正常范围内的细微故障 */
    else if (vdc_1k > 1820.0f && vdc_1k < 1850.0f)
    {
        if (r0_1m > 1400.0f)
        {
            fault = FAULT_C3_OPEN;
        }
        else if (gain_1m < -14.0f && ri_1m > 900.0f)
        {
            fault = FAULT_C3_DOUBLE;
        }
        else if (gain_1k > 5.4f && phase_1k < -172.5f)
        {
            fault = FAULT_C2_DOUBLE;
        }
        else if (gain_1k > 4.6f && gain_1k < 4.9f &&
                 phase_1k > -169.0f && phase_1k < -165.0f &&
                 ri_1k > 2600.0f && ri_1k < 2900.0f)
        {
            fault = FAULT_NORMAL;
        }
    }
    const char *fault_names[] = {
        "Normal",
        "R1 Open",
        "R1 Short",
        "R2 Open",
        "R2 Short",
        "R3 Open",
        "R3 Short",
        "R4 Open",
        "R4 Short",
        "C1 Open",
        "C1 Double",
        "C2 Open",
        "C2 Double",
        "C3 Open",
        "C3 Double",
        "Unknown"};

    HMI_send_string("error", fault_names[fault]);
}