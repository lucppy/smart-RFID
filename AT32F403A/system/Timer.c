#include "at32f403a_407.h"
#include "OLED.h"


/**
  * 函    数：定时中断初始化
  * 参    数：无
  * 返 回 值：无
  */
void Timer_Init(void)
{
    /*开启TIM2的时钟*/
    crm_periph_clock_enable(CRM_TMR2_PERIPH_CLOCK, TRUE);

    /*配置时钟源*/
    tmr_internal_clock_set(TMR2);

    /*时基单元初始化：PSC=7200-1, ARR=50000-1, 溢出频率10kHz, 溢出时间5秒 (基于72MHz主频)*/
    tmr_base_init(TMR2, 50000 - 1, 7200 - 1);
    tmr_cnt_dir_set(TMR2, TMR_COUNT_UP);
    tmr_clock_source_div_set(TMR2, TMR_CLOCK_DIV1);

    /*中断输出配置*/
    tmr_flag_clear(TMR2, TMR_OVF_FLAG);
    tmr_interrupt_enable(TMR2, TMR_OVF_INT, TRUE);

    /*NVIC配置*/
    nvic_irq_enable(TMR2_GLOBAL_IRQn, 8, 0);

    tmr_counter_enable(TMR2, TRUE);


    /*开启TIM3的时钟*/
    crm_periph_clock_enable(CRM_TMR3_PERIPH_CLOCK, TRUE);

    /*配置时钟源*/
    tmr_internal_clock_set(TMR3);

    /*时基单元初始化：PSC=7200-1, ARR=10000-1, 溢出时间1秒 (基于72MHz主频)*/
    tmr_base_init(TMR3, 10000 - 1, 7200 - 1);
    tmr_cnt_dir_set(TMR3, TMR_COUNT_UP);
    tmr_clock_source_div_set(TMR3, TMR_CLOCK_DIV1);

    /*中断输出配置*/
    tmr_flag_clear(TMR3, TMR_OVF_FLAG);
    tmr_interrupt_enable(TMR3, TMR_OVF_INT, TRUE);

    /*NVIC配置*/
    nvic_irq_enable(TMR3_GLOBAL_IRQn, 3, 0);
}


void TMR3_GLOBAL_IRQHandler(void)
{
    if (tmr_interrupt_flag_get(TMR3, TMR_OVF_INT) == SET)
    {
		tmr_counter_enable(TMR3, FALSE);
        tmr_flag_clear(TMR3, TMR_OVF_INT);
    }
}
