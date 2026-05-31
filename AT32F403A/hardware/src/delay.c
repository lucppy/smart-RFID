//��Ƭ��ͷ�ļ�
#include "at32f403a_407.h"

//delayͷ�ļ�
#include "delay.h"


//��ʱϵ��
unsigned char UsCount = 0;
unsigned short MsCount = 0;

volatile uint32_t g_msTicks = 0;


uint32_t GetTick(void)
{
    return g_msTicks;
}
/*
************************************************************
*	�������ƣ�	Delay_Init
*
*	�������ܣ�	systick��ʼ��
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void Delay_Init(void)
{
    SysTick_Config(SystemCoreClock / 1000);
}

/*
************************************************************
*	�������ƣ�	DelayUs
*
*	�������ܣ�	΢�뼶��ʱ
*
*	��ڲ�����	us����ʱ��ʱ��
*
*	���ز�����	��
*
*	˵����		��ʱ��(21MHz)�����ʱ798915us
************************************************************
*/
//void DelayUs(unsigned short us)
//{

//	unsigned int ctrlResult = 0;
//	
//	us &= 0x00FFFFFF;											//ȡ��24λ
//	
//	SysTick->LOAD = us * UsCount;								//װ������
//	SysTick->VAL = 0;
//	SysTick->CTRL = 1;											//ʹ�ܵ�������
//	
//	do
//	{
//		ctrlResult = SysTick->CTRL;
//	}
//	while((ctrlResult & 0x01) && !(ctrlResult & (1 << 16)));	//��֤�����С�����Ƿ񵹼�����0
//	
//	SysTick->CTRL = 0;											//�رյ�������
//	SysTick->VAL = 0;

//}

/*
************************************************************
*	�������ƣ�	DelayXms
*
*	�������ܣ�	���뼶��ʱ
*
*	��ڲ�����	ms����ʱ��ʱ��
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void DelayXms(unsigned short ms)
{

    uint32_t start = g_msTicks;
    while(g_msTicks - start < ms);

}

/*
************************************************************
*	�������ƣ�	DelayMs
*
*	�������ܣ�	΢�뼶����ʱ
*
*	��ڲ�����	ms����ʱ��ʱ��
*
*	���ز�����	��
*
*	˵����		��ε���DelayXms����������ʱ
************************************************************
*/
void DelayMs(unsigned short ms)
{

	unsigned char repeat = 0;
	unsigned short remain = 0;
	
	repeat = ms / 500;
	remain = ms % 500;
	
	while(repeat)
	{
		DelayXms(500);
		repeat--;
	}
	
	if(remain)
		DelayXms(remain);

}
