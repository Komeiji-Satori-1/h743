#include "FFT.h"
#include "tim.h"
#include "AD9833.h"

void Calculate_Input_Impedance(int Rs);
void Calculate_Output_Impedance(int RL);
void Calculate_Gain(void);
void sweep_freq(float begin_freq, float end_freq, float step_freq);
float Calculate_UpperCutoff_Freq(float *freq_buf, float *gain_buf, uint16_t n, float ref_gain_db);
float Calculate_Phase_Deg(void);
void ErrorDetect(void);