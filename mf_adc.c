#include "mf_adc.h"
#include "fm33lc0xx_fl.h"

void MF_ADC_Common_Init(void)
{
	FL_ADC_CommonInitTypeDef ADC_CommonInitStruct;
	ADC_CommonInitStruct.clockSource = FL_RCC_ADC_CLK_SOURCE_RCHF;         // 设置ADC时钟来源为RCHF（RC高速时钟，通常为8MHz）
	ADC_CommonInitStruct.clockPrescaler = FL_RCC_ADC_PSC_DIV8;             // 设置时钟分频（RCHF / 8），降低ADC工作频率，匹配硬件要求
	// 去掉了无效参数，因为FM33LC0xx无reference、continuousConvMode、dataAlignment成员
	FL_ADC_CommonInit(&ADC_CommonInitStruct);                              // 调用底层驱动函数，实际完成寄存器硬件配置
}

void MF_ADC_Sampling_Init(void)
{
	FL_ADC_InitTypeDef Sampling_InitStruct;
	Sampling_InitStruct.waitMode = FL_ENABLE;
	Sampling_InitStruct.overrunMode = FL_ENABLE;
	Sampling_InitStruct.scanDirection = FL_ADC_SEQ_SCAN_DIR_FORWARD;
	Sampling_InitStruct.externalTrigConv = FL_ADC_TRIGGER_EDGE_NONE;
	Sampling_InitStruct.triggerSource = FL_ADC_TRGI_PA8;
	Sampling_InitStruct.fastChannelTime = FL_ADC_FAST_CH_SAMPLING_TIME_4_ADCCLK;
	Sampling_InitStruct.lowChannelTime = FL_ADC_SLOW_CH_SAMPLING_TIME_192_ADCCLK;
	Sampling_InitStruct.oversamplingMode = FL_ENABLE;
	Sampling_InitStruct.overSampingMultiplier = FL_ADC_OVERSAMPLING_MUL_16X;
	Sampling_InitStruct.oversamplingShift = FL_ADC_OVERSAMPLING_SHIFT_4B;
	FL_ADC_Init(ADC, &Sampling_InitStruct);
	FL_ADC_EnableSequencerChannel(ADC, FL_ADC_EXTERNAL_CH3);   // PA13
	FL_ADC_EnableSequencerChannel(ADC, FL_ADC_EXTERNAL_CH4);   // PA14
	FL_ADC_EnableSequencerChannel(ADC, FL_ADC_EXTERNAL_CH2);   // PA15
	FL_ADC_EnableSequencerChannel(ADC, FL_ADC_EXTERNAL_CH11);  // PD1
	FL_ADC_EnableSequencerChannel(ADC, FL_ADC_EXTERNAL_CH10);  // PD0
	FL_ADC_EnableSequencerChannel(ADC, FL_ADC_EXTERNAL_CH8);  // PD11
}

void MF_ADC_GPIO_Init(void)
{
	FL_GPIO_InitTypeDef GPIO_InitStruct;
	// PA13, PA14, PA15
	GPIO_InitStruct.pin = FL_GPIO_PIN_13 | FL_GPIO_PIN_14 | FL_GPIO_PIN_15;
	GPIO_InitStruct.mode = FL_GPIO_MODE_ANALOG;
	GPIO_InitStruct.outputType = FL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.pull = FL_DISABLE;
	GPIO_InitStruct.remapPin = FL_DISABLE;
	FL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// PD1, PD0, PD11
	GPIO_InitStruct.pin = FL_GPIO_PIN_1 | FL_GPIO_PIN_0 | FL_GPIO_PIN_11;
	GPIO_InitStruct.mode = FL_GPIO_MODE_ANALOG;
	GPIO_InitStruct.outputType = FL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.pull = FL_DISABLE;
	GPIO_InitStruct.remapPin = FL_DISABLE;
	FL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}