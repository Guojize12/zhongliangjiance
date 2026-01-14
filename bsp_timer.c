#include "bsp_timer.h"
#include "bsp_tim.h"
#include "bsp_config.h"

//timer handle list head.
static struct Timer* head_handle = NULL;

//Timer ticks
static uint64_t _timer_ticks = 0;

#define USE_SIMIC_DEF //by simic

/**
  * @brief  Initializes the timer struct handle.
  * @param  handle: the timer handle strcut.
  * @param  timeout_cb: timeout callback.
  * @param  repeat: repeat interval time.
  * @retval None
  */
void BSP_TIMER_Init(struct Timer* handle, void (*timeout_cb)(), uint64_t timeout, uint64_t repeat)
{
    // memset(handle, sizeof(struct Timer), 0);
    handle->timeout_cb = timeout_cb;

#ifdef USE_SIMIC_DEF
    handle->timeout_last    = timeout;
#else
    handle->timeout    = _timer_ticks + timeout;
#endif

    handle->repeat     = repeat;
}

/**
  * @brief  Start the timer work, add the handle into work list.
  * @param  btn: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int BSP_TIMER_Start(struct Timer* handle)
{
    struct Timer* target = head_handle;
    while(target)
    {
        if(target == handle)
            return -1;  //already exist.
        target = target->next;
    }
    handle->next = head_handle;

#ifdef USE_SIMIC_DEF
    handle->timeout = _timer_ticks + handle->timeout_last;
#endif

    head_handle  = handle;

    return 0;
}

/**
  * @brief  Stop the timer work, remove the handle off work list.
  * @param  handle: target handle strcut.
  * @retval None
  */
void BSP_TIMER_Stop(struct Timer* handle)
{
    struct Timer** curr;
    for(curr = &head_handle; *curr;)
    {
        struct Timer* entry = *curr;
        if(entry == handle)
        {
            *curr = entry->next;
        }
        else
            curr = &entry->next;
    }
}

/**
  * @brief  main loop.
  * @param  None.
  * @retval None
  */
void BSP_TIMER_Handle(void)
{
    struct Timer* target;
    for(target = head_handle; target; target = target->next)
    {
        if(_timer_ticks >= target->timeout)
        {
            if(target->repeat == 0)
            {
                BSP_TIMER_Stop(target);
            }
            else
            {
                target->timeout = _timer_ticks + target->repeat;
            }
            target->timeout_cb();
        }
    }
}

/**
  * @brief  修改超时间隔并执行
  * @param  handle: target handle strcut.
  * @retval None.
  */

void BSP_TIMER_Modify_Start(struct Timer* handle,uint64_t timeout, uint64_t repeat)
{
    BSP_TIMER_Stop(handle);
    handle->timeout_last    = timeout; 
	  handle->repeat     = repeat;
    BSP_TIMER_Start(handle);
}
/**
  * @brief  修改超时间隔
  * @param  handle: target handle strcut.
  * @retval None.
  */
void BSP_TIMER_Modify(struct Timer* handle,uint64_t timeout)
{
    BSP_TIMER_Stop(handle);
    handle->timeout_last    = timeout;
}

/**
  * @brief  background ticks, timer repeat invoking interval 0.1ms.
  * @param  None.
  * @retval None.
  */
void BSP_TIMER_Ticks_100US(uint64_t time)
{
    _timer_ticks = time/10;
}
/**
  * @brief  background ticks, timer repeat invoking interval 1ms.
  * @param  None.
  * @retval None.
  */
void BSP_TIMER_Ticks_1MS(void)
{
    _timer_ticks++;
}

/**
  * @brief  starts the TIM Base generation in interrupt mode.
  * @param  None.
  * @retval _timer_ticks.
  */
uint64_t BSP_TIMER_Ticks_Get(void)
{
    return _timer_ticks;
}

/*****END OF FILE****/

