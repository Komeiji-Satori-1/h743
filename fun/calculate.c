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
    printf("Calculate_Input_Impedance begin\n");
    printf("FFT_Process ADC_Ui begin\n");
    FFT_Process(ADC_Ui, &FFT_Ampl1); // 先算 Ui，结果存 FFT_Ampl1
    printf("FFT_Ampl1(Ui)=%.4f  peak_bin=%lu\n", FFT_Ampl1, (unsigned long)FFT_mag_max_index);
    printf("FFT_Process ADC_Us begin\n");
    FFT_Process(ADC_Us, &FFT_Ampl2); // 再算 Us，结果存 FFT_Ampl2
    printf("FFT_Ampl2(Us)=%.4f  peak_bin=%lu\n", FFT_Ampl2, (unsigned long)FFT_mag_max_index);
    Ri = (float)Rs * FFT_Ampl1 / (FFT_Ampl2 - FFT_Ampl1);
    printf("Ri=%.3f\n",Ri);
    //sprintf((char *)buf, "%.1f", Ri);
    //HMI_send_string("t0", (char *)buf);
    //printf("Calculate_Input_Impedance done\n");
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
    printf("Calculate_Output_Impedance begin\n");
    /* 步骤1: 继电器当前接通，此帧数据即为带载测量值 */
    printf("FFT_Process ADC_U0 begin\n");
    FFT_Process(ADC_U0, &FFT_Ampl1); // FFT_Ampl1 = U0 (带载)
    printf("FFT_Process ADC_U0 done\n");
    /* 步骤2: 断开负载，等待信号稳定 */

    Relay_Off();
    HAL_Delay(50);
    /* 步骤3~4: 重采样，获取空载电压 */
    if (Acquire_All_ADC_Samples_Blocking(200U) == 1U)
    {
        printf("FFT_Process ADC_U∞ begin\n");
        FFT_Process(ADC_U0, &FFT_Ampl2); // FFT_Ampl2 = U∞ (空载)
        printf("FFT_Process ADC_U∞ done\n");
        R0 = (float)RL * (FFT_Ampl2 - FFT_Ampl1) / FFT_Ampl1;
        printf("R0=%.3f\n",R0);
        //sprintf((char *)buf, "%.1f", R0);
        //HMI_send_string("t1", (char *)buf);
    }

    /* 步骤5: 恢复负载接通，保证后续测量环境一致 */
    Relay_On();
    HAL_Delay(50);
    printf("Calculate_Output_Impedance done\n");
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
    printf("Calculate_Gain begin\n");
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
        printf("Au=%.3f\n",Au);
        //sprintf((char *)buf, "%.1f", Au);
        //HMI_send_string("t2", (char *)buf);
    }

    Relay_On();
    HAL_Delay(50);
    printf("Calculate_Gain done\n");
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
            float val_lo = (Au - 35.0f) * 255.0f / 15.0f;
            if (val_lo < 0.0f)   val_lo = 0.0f;
            if (val_lo > 255.0f) val_lo = 255.0f;
            wave_data[j] = (uint8_t)val_lo;
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
            float val_hi = (Au - 35.0f) * 255.0f / 15.0f;
            if (val_hi < 0.0f)   val_hi = 0.0f;
            if (val_hi > 255.0f) val_hi = 255.0f;
            wave_data[j] = (uint8_t)val_hi;
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

    HMI_Wave_Fast(1, 0, j, wave_data);

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
    errortype fault = FAULT_UNKNOWN;

    /* ================================================================
     * Level 1: 1kHz 单频点测量
     * 先判断输出直流工作点（电阻故障影响直流偏置，电容故障不影响）
     * ================================================================ */
    f = 1000.0f;
    ad9833_set_freq_ch(f, ad9833_Sine, ad9833_CH0);
    Set_ADC_SampleRate(20000.0f);
    HAL_Delay(100);
    Acquire_All_ADC_Samples_Blocking(300U); /* 丢弃第一帧（信号建立瞬态） */
    Acquire_All_ADC_Samples_Blocking(300U); /* 使用稳态帧 */

    /* 输入阻抗（使用当前 relay-on 采样帧，不受 relay 状态影响） */
    printf("=== [ErrorDetect] Calculate_Input_Impedance start ===\n");
    Calculate_Input_Impedance(Rs);
    /* 输出直流（relay 断开 = 空载，即 DC_Uinf，单位：ADC 计数）
     * 12bit ADC / 3.3V VREF: 1 LSB ≈ 0.806 mV
     *   正常 / 电容故障 DC_Uinf ≈ 2.756V → ~3421 counts
     *   R1开路 / R2短路  DC_Uinf = 3.300V → 4095 counts（饱和）
     *   R3开路           DC_Uinf ≈ 0.071V → ~88 counts
     *   R4短路           DC_Uinf ≈ 0.018V → ~22 counts */
    Relay_Off();
    HAL_Delay(50);
    Acquire_All_ADC_Samples_Blocking(300U);
    float dc_u0 = Calculate_DC_Value(ADC_U0);

    /* 增益（函数内部自行 relay-off 重采样） */
    Calculate_Gain();
    float au_1k = Au;

    printf("L1: DC_U0=%.1f Ri=%.1f Au=%.3f\n", dc_u0, Ri, au_1k);

    /* ---- 分支1: 输出直流饱和到 VCC ---- */
    if (dc_u0 > 65500.0f)           /* > ~3.07V: R1开路或R2短路（正常≈3421，饱和=4095） */
    {
        if (Ri > 5000.0f)
            fault = FAULT_R1_OPEN;     /* 实测 Ri~14400Ω */
        else if (Ri < 500.0f)
            fault = FAULT_R2_SHORT;    /* 实测 Ri~72-110Ω */
        /* else: FAULT_UNKNOWN */
    }
    /* ---- 分支2: 输出直流接近 0 ---- */
    else if (dc_u0 < 2000.0f)       /* < ~0.16V: R3开路或R4短路 */
    {
        if (Ri > 140.0f)
            fault = FAULT_R3_OPEN;     /* 实测 Ri~1100Ω, DC_U0~0.054V */
        else
            fault = FAULT_R4_SHORT;    /* 实测 Ri~100Ω,  DC_U0~0.017V */
    }
    /* ---- 分支3: 直流工作点正常（电容故障或正常）---- */
    else
    {
        if (Ri > 350000.0f)
        {
            fault = FAULT_C1_OPEN;     /* 实测 Ri~387k-395k */
        }
        else if (Ri > 8000.0f && Ri < 13000.0f)
        {
            fault = FAULT_C2_OPEN;     /* 实测 Ri~10315-10329 */
        }
        else if (Ri > 2300.0f && Ri < 2600.0f && au_1k > 42.0f)
        {
            fault = FAULT_C2_DOUBLE;   /* 实测 Ri~2449-2454, Au~42.64dB */
        }
        else
        {
            /* ============================================================
             * Level 2: 带宽检测
             * 正常截止频率 ≈ 171kHz；C3翻倍 fc≈100kHz；C3断路 fc>300kHz
             * 使用等效采样：alias_freq=1kHz，target_fs=(f-1k)/k
             * ============================================================ */

            /* 2.1 测 100kHz 增益（k=5，target_fs=19800Hz） */
            f = 100000.0f;
            ad9833_set_freq_ch(f, ad9833_Sine, ad9833_CH0);
            Set_ADC_SampleRate((f - 1000.0f) / 5.0f);
            HAL_Delay(100);
            Calculate_Gain();
            float au_100k = Au;
            float drop_100k = au_1k - au_100k;
            printf("L2: Au@100k=%.3f drop=%.3f\n", au_100k, drop_100k);

            /* 2.2 测 200kHz 增益（k=10，target_fs=19900Hz） */
            f = 200000.0f;
            ad9833_set_freq_ch(f, ad9833_Sine, ad9833_CH0);
            Set_ADC_SampleRate((f - 1000.0f) / 10.0f);
            HAL_Delay(100);
            Calculate_Gain();
            float au_200k = Au;
            float drop_200k = au_1k - au_200k;
            printf("L2: Au@200k=%.3f drop=%.3f\n", au_200k, drop_200k);

            if (drop_100k > 2.5f)
            {
                fault = FAULT_C3_DOUBLE;   /* fc≈100kHz，100kHz处已衰减>2.5dB */
            }
            else if (drop_200k < 2.5f)
            {
                fault = FAULT_C3_OPEN;     /* fc>300kHz，200kHz处几乎不衰减 */
            }
            else
            {
                /* ========================================================
                 * Level 3: 10Hz 相位检测
                 * 正常 Phase@10Hz ≈ -141.3°；C1翻倍 ≈ -145°
                 * ======================================================== */
                ad9833_set_freq_ch(10.0f, ad9833_Sine, ad9833_CH0);
                Set_ADC_SampleRate(500.0f);
                HAL_Delay(300);
                Acquire_All_ADC_Samples_Blocking(3000U);
                float phase_lf = Calculate_Phase_Deg();
                printf("L3: Phase@10Hz=%.3f\n", phase_lf);

                if (phase_lf < -143.0f)
                    fault = FAULT_C1_DOUBLE;   /* 正常~-141.3°，故障~-145° */
                else
                    fault = FAULT_NORMAL;
            }
        }
    }

    /* 恢复到 1kHz / 20kHz 采样率 */
    f = 1000.0f;
    ad9833_set_freq_ch(f, ad9833_Sine, ad9833_CH0);
    Set_ADC_SampleRate(20000.0f);
    HAL_Delay(100);

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

    printf("ErrorDetect: %s\n", fault_names[fault]);
    HMI_send_string("error", fault_names[fault]);
}