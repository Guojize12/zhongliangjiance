#ifndef __BSP_ADC_H__
#define __BSP_ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

#define ADC_VREF  (*((uint16_t *)(0x1FFFFB08)))   // 30℃ vref1.22采样值

#define USE_130V  130 //测量13V
#define USE_139V  139 //测量13.9V
#define USE_188V  188 //测量18.8V
#define USE_090V  90 //测量19V
#define USE_050V  50 //测量5V
#define USE_042V  42 //测量4.2V
#define USE_033V  33  //测量3.3V

#define  FILTER_TIMES     2      //滤波,最大最小各 FILTER_TIMES 次
#define  READ_ADC_TIMES   10     //读取ADC次数
#define  READ_ADC_NUM     6     //ADC个数

typedef struct
{
	  uint32_t adc_chl[READ_ADC_NUM+1];
	  uint32_t adc_chl_vol[READ_ADC_NUM+1];
	 
    uint32_t adc_value[READ_ADC_NUM+1]; //adc值
    uint32_t vol[READ_ADC_NUM+1]; //电压 mV
    int32_t  vol_lev;  //电量 0.1%
    uint32_t vol_sum;
    uint32_t vol_sub[READ_ADC_NUM+1][READ_ADC_TIMES+2];
    uint32_t vol_num;
	  uint32_t delay;
} adc_def;
extern adc_def g_adc_cfg;

uint8_t BSP_ADC_Voltage_Poll(void);

uint8_t  BSP_ADC_Level(uint16_t vol);
void BSP_ADC_Init(void);
#ifdef __cplusplus
}
#endif

#endif /*__BSP_ADC_H__ */
