#ifndef _USART_H_
#define _USART_H_


#include "at32f403a_407.h"


#define USART_DEBUG		USART2		//调试打印使用的串口


void Usart1_Init(unsigned int baud);

void Usart2_Init(unsigned int baud);

void Usart_SendString(usart_type *USARTx, unsigned char *str, unsigned short len);

void UsartPrintf(usart_type *USARTx, char *fmt,...);

void Usart3_Init(unsigned int baud);

#endif
