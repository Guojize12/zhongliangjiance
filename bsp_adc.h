#ifndef __BSP_ADC_H__
#define __BSP_ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "bsp_gpio.h"

/*
 * 重量传感器检测（PD0和PD11），由软件定时器驱动。
 * - 控制引脚：PB9（PD0组），PB10（PD11组）
 * - ADC通道：PD0 -> FL_ADC_EXTERNAL_CH10，PD11 -> FL_ADC_EXTERNAL_CH8
 * - 阈值：原始ADC > 500 判定为“已插入”，否则为“未插入”
 * - 防抖：需要连续多次确认样本才能切换为“已插入”
 * - 未插入时：保持控制引脚高电平并持续周期检测
 * - 检测到已插入：控制引脚拉低并停止检测定时器
 */

#define WEIGHT_ADC_THRESHOLD_DEFAULT     500        // 默认插入检测阈值（原始ADC单位）
#define WEIGHT_INSERT_CONFIRM_COUNT      3          // 防抖连续确认次数
#define WEIGHT_SAMPLE_PERIOD_MS_DEFAULT  50         // 检测定时器采样周期（ms），见 bsp_timer.h

/* 外部接口 */
void BSP_ADC_Init(void);

/* 启动检测流程（上电并开始软定时采样） */
void BSP_ADC_Weight1_Start(void);  // PD0，控制PB9
void BSP_ADC_Weight2_Start(void);  // PD11，控制PB10

/* 可选：手动停止（断电并停止定时器，通常由插入检测自动停止） */
void BSP_ADC_Weight1_Stop(void);
void BSP_ADC_Weight2_Stop(void);

/* 状态查询：0 = 未插入，1 = 已插入 */
uint8_t  BSP_ADC_Weight1_Status(void);
uint8_t  BSP_ADC_Weight2_Status(void);

/* 每路的最近一次原始ADC值 */
uint16_t BSP_ADC_Weight1_LastADC(void);
uint16_t BSP_ADC_Weight2_LastADC(void);

/* 运行时参数设置 */
void BSP_ADC_Weight_SetThreshold(uint16_t threshold);
void BSP_ADC_Weight_SetSamplePeriod(uint16_t period_ms);
void BSP_ADC_Weight_SetConfirmCount(uint8_t count);

/* 便捷接口：同时启动两路检测 */
void BSP_ADC_Weight_StartAll(void);

/* 弱回调事件钩子（如需可在app层重写） */
void BSP_ADC_OnWeight1Inserted(void);
void BSP_ADC_OnWeight1NotInserted(void);
void BSP_ADC_OnWeight2Inserted(void);
void BSP_ADC_OnWeight2NotInserted(void);

#ifdef __cplusplus
}
#endif

#endif /*__BSP_ADC_H__ */