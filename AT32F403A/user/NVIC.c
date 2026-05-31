#include "at32f403a_407.h"

void NVIC_Configuration(void){
	crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);

	/* NVIC中断分组 */
	nvic_priority_group_config(NVIC_PRIORITY_GROUP_2);

	/* NVIC配置 */
	nvic_irq_enable(USART3_IRQn, 0, 0);
	nvic_irq_enable(USART1_IRQn, 1, 1);
	nvic_irq_enable(EXINT1_IRQn, 2, 2);
}
