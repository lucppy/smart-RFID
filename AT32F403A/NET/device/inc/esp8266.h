#ifndef _ESP8266_H_
#define _ESP8266_H_
#include "stdint.h"

#define REV_OK		0	//������ɱ�־
#define REV_WAIT	1	//����δ��ɱ�־

typedef struct buffer
{

    char *cmd;//����
    char *res;//ϣ�������Ļظ�
	u8 debug;//1��ʾ���ڵ������
}  ESP8266_CMD;

void ESP8266_Init(void);

void ESP8266_Clear(void);

//_Bool ESP8266_SendCmd(char *cmd, char *res);
_Bool ESP8266_SendCmd(ESP8266_CMD* cmd);
void ESP8266_SendData(unsigned char *data, unsigned short len);

unsigned char *ESP8266_GetIPD(unsigned short timeOut);

void WIFI_W(uint32_t add1, uint32_t add2, char* SSID, char* PWD);

void WIFI_R(uint32_t add1, uint32_t add2);

/* ADD BEGIN - �ֳֻ����죺WiFi ������״̬��� */
void ESP8266_ConnectToAP(void);          /* ����Ԥ���ȵ㣨��FLASH��ȡ���ã� */
uint8_t ESP8266_IsConnected(void);       /* ��� WiFi �Ƿ������� */
/* ADD END */

#endif
