//��Ƭ��ͷ�ļ�
#include "at32f403a_407.h"

//�����豸����
#include "esp8266.h"

//Ӳ������
#include "usart.h"
#include "oled.h"
#include "timer.h"
#include "delay.h"
#include "Flash.h"

//C��
#include <string.h>
#include <stdio.h>

/* CHANGE BEGIN */
//#define ESP8266_WIFI_INFO		"AT+CWJAP=\"HiWiFi\",\"5555720720\",\"d4:ee:07:03:e7:54\"\r\n"
/* CHANGE END */

unsigned char esp8266_buf[512];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;
extern u8 time;
//==========================================================
//	�������ƣ�	ESP8266_Clear
//
//	�������ܣ�	��ջ���
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����
//==========================================================
void ESP8266_Clear(void)
{

    memset(esp8266_buf, 0, sizeof(esp8266_buf));
    esp8266_cnt = 0;
}

/* ADD BEGIN - ��̬�����ȵ㣨��FLASH��ȡSSID/���룩 */

/**
  * @brief   ������ FLASH �б���� WiFi �ȵ�
  * @param   ��
  * @retval  ��
  * @note    �᳢������10�Σ�ÿ�μ��2�룻ʧ�ܺ� OLED ��ʾ���󲢽�����ѭ�����ɸ�Ϊ������������
  */
void ESP8266_ConnectToAP(void)
{
    char ssid[33] = { 0 };
    char pwd[33] = { 0 };
    char cmd_buf[128];
    uint8_t retry = 0;
    ESP8266_CMD connectCmd;

    /* �� FLASH ��ȡ WiFi ���� */
    if (!WIFI_LoadConfig(ssid, pwd, 33)) {
        OLED_Clear();
        OLED_ShowString(1, 1, "No WiFi config!");
        OLED_ShowString(2, 1, "Please set via");
        OLED_ShowString(3, 1, "USB or UART");
        while (1);  /* ִֹͣ�У��ȴ��ⲿ���� */
    }

    /* ���� AT+CWJAP ���� */
    sprintf(cmd_buf, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);
    connectCmd.cmd = cmd_buf;
    connectCmd.res = "GOT IP";
    connectCmd.debug = 1;   /* ������������ڣ����ڹ۲� */

    OLED_ShowString(4, 1, "Connecting WiFi ");
    while (ESP8266_SendCmd(&connectCmd) && retry++ < 10) {
        DelayXms(2000);
        OLED_ShowString(4, 1, "Retry...       ");
    }

    if (retry >= 10) {
        OLED_ShowString(4, 1, "WiFi Failed!   ");
        while (1);  /* ����ʧ�ܣ�ֹͣ */
    }
    else {
        OLED_ShowString(4, 1, "WiFi Connected ");
        DelayXms(500);
    }
}

/**
  * @brief   ��� ESP8266 �Ƿ������� AP
  * @param   ��
  * @retval  1 - ������, 0 - δ����
  * @note    ͨ������ AT+CWJAP? ��������Ӧ
  */
uint8_t ESP8266_IsConnected(void)
{
    ESP8266_CMD checkCmd;
    checkCmd.cmd = "AT+CWJAP?\r\n";
    checkCmd.res = "+CWJAP:\"";
    checkCmd.debug = 0;

    ESP8266_Clear();
    if (ESP8266_SendCmd(&checkCmd) == 0) {
        /* �ҵ� +CWJAP:" ˵�������� */
        return 1;
    }
    return 0;
}
/* ADD END */


//==========================================================
//	�������ƣ�	ESP8266_WaitRecive
//
//	�������ܣ�	�ȴ��������
//
//	��ڲ�����	��
//
//	���ز�����	REV_OK-�������		REV_WAIT-���ճ�ʱδ���
//
//	˵����		ѭ�����ü���Ƿ�������
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

    if(esp8266_cnt == 0) 							//������ռ���Ϊ0 ��˵��û�д��ڽ��������У�����ֱ����������������
        return REV_WAIT;

    if(esp8266_cnt == esp8266_cntPre)				//�����һ�ε�ֵ�������ͬ����˵���������
    {
        esp8266_cnt = 0;							//��0���ռ���
//        UsartPrintf(USART_DEBUG,"buf: %s\r\n", esp8266_buf);
        return REV_OK;								//���ؽ�����ɱ�־
    }

    esp8266_cntPre = esp8266_cnt;					//��Ϊ��ͬ

    return REV_WAIT;								//���ؽ���δ��ɱ�־

}

//==========================================================
//	�������ƣ�	ESP8266_SendCmd
//
//	�������ܣ�	��������
//
//	��ڲ�����	cmd������
//				res����Ҫ���ķ���ָ��
//
//	���ز�����	0-�ɹ�	1-ʧ��
//
//	˵����
//==========================================================
_Bool ESP8266_SendCmd(ESP8266_CMD *cmd)
{

    unsigned char timeOut = 200;

    Usart_SendString(USART3, (unsigned char *)cmd->cmd, strlen((const char *)cmd->cmd));

    while(timeOut--)
    {
        if(ESP8266_WaitRecive() == REV_OK)							//����յ�����
        {
            if(cmd->debug==1)
            {
                UsartPrintf(USART_DEBUG, "%s\r\n", esp8266_buf);
            }
            if(strstr((const char *)esp8266_buf, cmd->res) != NULL)		//����������ؼ���
            {
                ESP8266_Clear();									//��ջ���

                return 0;
            }
        }

        DelayXms(10);
    }

    return 1;

}

//==========================================================
//	�������ƣ�	ESP8266_SendData
//
//	�������ܣ�	��������
//
//	��ڲ�����	data������
//				len������
//
//	���ز�����	��
//
//	˵����
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

    char cmdBuf[32];

    ESP8266_Clear();								//��ս��ջ���
    sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//��������
//    UsartPrintf(USART_DEBUG, cmdBuf);
	ESP8266_CMD ESP8266_SendData = {NULL, NULL, 0};
    ESP8266_SendData.cmd =  cmdBuf;
    ESP8266_SendData.res = ">";
    ESP8266_SendData.debug = 0;
    if(!ESP8266_SendCmd(&ESP8266_SendData))//�յ���>��ʱ���Է�������
    {
//        UsartPrintf(USART_DEBUG, "SEND: MQTT CONNECT Sending\r\n");
        Usart_SendString(USART3, data, len);   //�����豸������������
    }
}

//==========================================================
//	�������ƣ�	ESP8266_GetIPD
//
//	�������ܣ�	��ȡƽ̨���ص�����
//
//	��ڲ�����	�ȴ���ʱ��(����10ms)
//
//	���ز�����	ƽ̨���ص�ԭʼ����
//
//	˵����		��ͬ�����豸���صĸ�ʽ��ͬ����Ҫȥ����
//				��ESP8266�ķ��ظ�ʽΪ	"+IPD,x:yyy"	x�������ݳ��ȣ�yyy����������
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

    char *ptrIPD = NULL;

    do
    {
        if(ESP8266_WaitRecive() == REV_OK)								//����������
        {
            ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//������IPD��ͷ
            if(ptrIPD == NULL)											//���û�ҵ���������IPDͷ���ӳ٣�������Ҫ�ȴ�һ�ᣬ�����ᳬ���趨��ʱ��
            {
                UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
            }
            else
            {
                ptrIPD = strchr(ptrIPD, ':');							//�ҵ�':'
                if(ptrIPD != NULL)
                {
                    ptrIPD++;
                    return (unsigned char *)(ptrIPD);
                }
                else
                    return NULL;

            }
        }

        DelayXms(50);													//��ʱ�ȴ�
    }
    while(timeOut--);

    return NULL;														//��ʱ��δ�ҵ������ؿ�ָ��

}

//==========================================================
//	�������ƣ�	ESP8266_Init
//
//	�������ܣ�	��ʼ��ESP8266
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����
//==========================================================
void ESP8266_Init(void)
{
    ESP8266_CMD ESP8266_Connect = {NULL, NULL, 0};
    ESP8266_Clear();
    UsartPrintf(USART_DEBUG, "1. AT\r\n");
    OLED_Clear();
    OLED_ShowString(1, 1, "1.AT...");
    ESP8266_Connect.cmd = "AT\r\n";
    ESP8266_Connect.res = "OK";
    while(ESP8266_SendCmd(&ESP8266_Connect))
    {
        DelayXms(500);
    }

//    UsartPrintf(USART_DEBUG, "2. CWMODE\r\n");
    OLED_ShowString(2, 1, "2.CWMODE...");
    ESP8266_Connect.cmd = "AT+CWMODE=1\r\n";
    ESP8266_Connect.res = "OK";
    while(ESP8266_SendCmd(&ESP8266_Connect))
    {
        DelayXms(500);
    }

//    UsartPrintf(USART_DEBUG, "3. AT+CWDHCP\r\n");
    OLED_ShowString(3, 1, "3.AT+CWDHCP...");
    ESP8266_Connect.cmd = "AT+CWDHCP=1,1\r\n";
    ESP8266_Connect.res = "OK";
    while(ESP8266_SendCmd(&ESP8266_Connect))
    {
        DelayXms(500);
    }


    /* ADD BEGIN - �滻Ϊ��̬�ȵ����� */
//    UsartPrintf(USART_DEBUG, "4. CWJAP\r\n");
/*  OLED_ShowString(4, 1, "4.CWJAP...");
    ESP8266_Connect.cmd = ESP8266_WIFI_INFO;
    ESP8266_Connect.res = "GOT IP";
    while(ESP8266_SendCmd(&ESP8266_Connect))
    {
        DelayXms(5000);
    }*/

    
    OLED_ShowString(4, 1, "4.CWJAP...");
    ESP8266_ConnectToAP();   /* ʹ�ô�FLASH��ȡ���õ����Ӻ��� */
    /* ADD END */

    UsartPrintf(USART_DEBUG, "5. ESP8266 Init OK\r\n");
    OLED_Clear();
    OLED_ShowString(1, 1, "ESP8266 Init OK");
    DelayXms(500);
}

//==========================================================
//	�������ƣ�	USART3_IRQHandler
//
//	�������ܣ�	����3�շ��ж�
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����
//==========================================================
void USART3_IRQHandler(void)
{

    if(usart_interrupt_flag_get(USART3, USART_RDBF_FLAG) != RESET) //�����ж�
    {
        if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0; //��ֹ���ڱ�ˢ��
        esp8266_buf[esp8266_cnt++] = usart_data_receive(USART3);
        usart_flag_clear(USART3, USART_RDBF_FLAG);
    }

}

