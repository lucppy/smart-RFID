#ifndef __RFID_H__
#define __RFID_H__

#include "at32f403a_407.h"

#define TAG_CACHE_SIZE   10         // ��ǩ�����С
#define EPC_LEN          12         // EPC����  
#define TIME_WINDOW_MS   100        // ʱ�䴰�ڣ���λ���룬ͬһ��ǩ�ڴ�ʱ�����ٴγ�����Ϊ�ظ�
#define RSSI_THRESHOLD_DEFAULT -70  // dBm RSSI��ֵ,�趨����-70dBM���ź�����

#define Check_True   1              //У��ɹ�
#define Check_False   0             //У��ʧ��
typedef struct
{
    uint8_t  epc[EPC_LEN];          // ��ǩEPC
    uint32_t last_time;             // �ϴ�ʶ��ʱ��
    uint8_t  valid;                 // �Ƿ���Ч
    int8_t   rssi;                  // �ź�ǿ��
} TagNode;


void RFID_SearchOnce(void);                                 //��RFID���͵�����ѯ����

uint8_t RFID_GetRxFlag(void);                               //��ȡ���ڽ��հ���־λ

void Serial_SendByte(uint8_t Byte);                         //���ڷ���һ���ֽڸ�RFID

void Serial_SendArray(uint8_t *Array, uint16_t Length);     //���ڷ���һ�������RFID

void Serial_SendString(char *String);                       //���ڷ���һ���ַ�����RFID

void Serial_SendNumber(uint32_t Number, uint8_t Length);    //���ڷ������ָ�RFID

void RFID_Clear(void);                                      //��մ������ݽ��ջ���

u8 RFID_Unpacket(void);                                     //�������ݽ��,��ȡ����

uint8_t RFID_CheckDuplicate(uint8_t *epc,int8_t rssi);      //�жϱ�ǩ�Ƿ��ʱ���ظ���ȡ

void RFID_CleanExpired(void);                               //�����������ڱ�ǩ

int8_t RSSI_Filter(int8_t last_rssi, int8_t new_rssi);      //��һ�׵�ͨ�˲�

u8 Get_Checksum(void);                                      //У��ͼ��

#endif

