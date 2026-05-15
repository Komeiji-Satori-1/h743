#include "arm_math.h"
#include "arm_const_structs.h"
#include "math.h"
#include <stdio.h>
#include "main.h"
#include "usart.h"
#include "HMI.h"
#include "Switch.h"
#include "AD9833.h"

void FFT_Process(uint16_t *ADC_Buffer, float *FFT_Ampl);
void IFFT_Process(void);
void window(void);
void ADC_FFT_Get_Wave_Mes(uint32_t Row,float Fs,float *VPP,float *Freq,int correctNum);
void Find_BaseIndex(void);
void Process_FFT_mag(float *FFT_mag, float *FFT_mag_max, uint32_t *FFT_mag_max_index,float *FFT_Ampl);
void showdata(float *buffer, uint16_t n);
void FFT_SetSampling(float sampling_freq);
float Calculate_DC_Value(uint16_t *ADC_Buffer);