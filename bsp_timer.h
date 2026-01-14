#ifndef _BSP_TIMER_H_
#define _BSP_TIMER_H_

#ifdef __cplusplus  
extern "C" {  
#endif  
	
#include "stdint.h"
#include "stddef.h"

#define TIMEOUT_1MS     1
#define TIMEOUT_5MS     5
#define TIMEOUT_10MS    10
#define TIMEOUT_15MS    15
#define TIMEOUT_20MS    20
#define TIMEOUT_25MS    25
#define TIMEOUT_40MS    40
#define TIMEOUT_50MS    50

#define TIMEOUT_80MS   80
#define TIMEOUT_100MS   100
#define TIMEOUT_150MS   150
#define TIMEOUT_105MS   105
#define TIMEOUT_200MS   200
#define TIMEOUT_250MS   250
#define TIMEOUT_400MS   400
#define TIMEOUT_500MS  	500
#define TIMEOUT_800MS  	800
#define TIMEOUT_1S      1000
#define TIMEOUT_1300MS      1300
#define TIMEOUT_1500MS      1500
#define TIMEOUT_2S      2000
#define TIMEOUT_2_5S      2500
#define TIMEOUT_3S      3000
#define TIMEOUT_4S      4000
#define TIMEOUT_5S      5000
#define TIMEOUT_7S      7000
#define TIMEOUT_8S      8000
#define TIMEOUT_9S     9000
#define TIMEOUT_9_5S     9500
#define TIMEOUT_10S     10000
#define TIMEOUT_13S     13000
#define TIMEOUT_20S     20000
#define TIMEOUT_22S     22000
#define TIMEOUT_60S     60000
#define TIMEOUT_120S    120000
#define TIMEOUT_150S    150000

#define TIMEOUT_15M     (TIMEOUT_60S*15)

typedef struct Timer {
    uint64_t timeout;
		uint64_t timeout_last; //by simic   优化BSP_TIMER_Start,不立即进 timeout
    uint64_t repeat;
    void (*timeout_cb)(void);
    struct Timer* next;
}Timer;

void BSP_TIMER_Init(struct Timer* handle, void(*timeout_cb)(), uint64_t timeout, uint64_t repeat);
int  BSP_TIMER_Start(struct Timer* handle);
void BSP_TIMER_Stop(struct Timer* handle);
void BSP_TIMER_Ticks(uint64_t time);
void BSP_TIMER_Modify_Start(struct Timer* handle,uint64_t timeout, uint64_t repeat);
void BSP_TIMER_Modify(struct Timer* handle,uint64_t timeout);
void BSP_TIMER_Ticks_100US(uint64_t time);
void BSP_TIMER_Ticks_1MS(void);

uint64_t BSP_TIMER_Ticks_Get(void);
void BSP_TIMER_Handle(void);
#ifdef __cplusplus
} 
#endif

#endif

/*****END OF FILE****/
