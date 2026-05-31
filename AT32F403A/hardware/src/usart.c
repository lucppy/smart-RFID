//硬件驱动
#include "usart.h"
#include "delay.h"

//C库
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

static unsigned char UsartPrintfBuf[1024];

/*
************************************************************
*  函数名称：  Usart1_Init
*
*  函数功能：  串口1初始化
*
*  入口参数：  baud：设定的波特率
*
*  返回参数：  无
*
*  说明：      TX-PA9     RX-PA10
************************************************************
*/
void Usart1_Init(unsigned int baud)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_USART1_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);

    //PA9   TXD
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_pins = GPIO_PINS_9;
    gpio_init(GPIOA, &gpio_init_struct);

    //PA10  RXD
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_pins = GPIO_PINS_10;
    gpio_init(GPIOA, &gpio_init_struct);

    usart_init(USART1, baud, USART_DATA_8BITS, USART_STOP_1_BIT);
    usart_transmitter_enable(USART1, TRUE);
    usart_receiver_enable(USART1, TRUE);
    usart_enable(USART1, TRUE);

    usart_interrupt_enable(USART1, USART_RDBF_INT, TRUE);

    nvic_irq_enable(USART1_IRQn, 0, 2);
}

/*
************************************************************
*  函数名称：  Usart2_Init
*
*  函数功能：  串口2初始化
*
*  入口参数：  baud：设定的波特率
*
*  返回参数：  无
*
*  说明：      TX-PA2     RX-PA3
************************************************************
*/
void Usart2_Init(unsigned int baud)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_USART2_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);

    //PA2   TXD
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_pins = GPIO_PINS_2;
    gpio_init(GPIOA, &gpio_init_struct);

    //PA3   RXD
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_pins = GPIO_PINS_3;
    gpio_init(GPIOA, &gpio_init_struct);

    usart_init(USART2, baud, USART_DATA_8BITS, USART_STOP_1_BIT);
    usart_transmitter_enable(USART2, TRUE);
    usart_receiver_enable(USART2, TRUE);
    usart_enable(USART2, TRUE);

    //USART2 接收中断未启用（调试串口，只发不收）
}

/*
************************************************************
*  函数名称：  Usart3_Init
*
*  函数功能：  串口3初始化
*
*  入口参数：  baud：设定的波特率
*
*  返回参数：  无
*
*  说明：      TX-PB10     RX-PB11
************************************************************
*/
void Usart3_Init(unsigned int baud)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_USART3_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);

    /*GPIO初始化*/
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_pins = GPIO_PINS_10;
    gpio_init(GPIOB, &gpio_init_struct);

    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_UP;
    gpio_init_struct.gpio_pins = GPIO_PINS_11;
    gpio_init(GPIOB, &gpio_init_struct);

    /*USART初始化*/
    usart_init(USART3, baud, USART_DATA_8BITS, USART_STOP_1_BIT);
    usart_transmitter_enable(USART3, TRUE);
    usart_receiver_enable(USART3, TRUE);
    usart_enable(USART3, TRUE);

    /*中断配置*/
    usart_interrupt_enable(USART3, USART_RDBF_INT, TRUE);

    nvic_irq_enable(USART3_IRQn, 0, 0);
}

/*
************************************************************
*  函数名称：  Usart_SendString
*
*  函数功能：  串口数据发送
*
*  入口参数：  USARTx：串口号
*             str：要发送的字符串
*             len：数据长度
*
*  返回参数：  无
*
*  说明：
************************************************************
*/
void Usart_SendString(usart_type *USARTx, unsigned char *str, unsigned short len)
{
    unsigned short count = 0;

    for(; count < len; count++)
    {
        usart_data_transmit(USARTx, *str++);
        while(usart_flag_get(USARTx, USART_TDC_FLAG) == RESET);
    }
}

/*
************************************************************
*  函数名称：  UsartPrintf
*
*  函数功能：  格式化打印
*
*  入口参数：  USARTx：串口号
*             fmt：格式字符串
*
*  返回参数：  无
*
*  说明：
************************************************************
*/
void UsartPrintf(usart_type *USARTx, char *fmt,...)
{
    va_list ap;
    unsigned char *pStr = UsartPrintfBuf;

    va_start(ap, fmt);
    vsnprintf((char *)UsartPrintfBuf, sizeof(UsartPrintfBuf), fmt, ap);
    va_end(ap);

    while(*pStr != 0)
    {
        usart_data_transmit(USARTx, *pStr++);
        while(usart_flag_get(USARTx, USART_TDC_FLAG) == RESET);
    }
}
