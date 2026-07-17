#ifndef __RFID_H__
#define __RFID_H__

#include "at32f403a_407.h"

#define TAG_CACHE_SIZE   64         // 标签缓存大小（手持场景扩容）
#define EPC_LEN          12         // EPC长度
#define TIME_WINDOW_MS   200        // 时间窗口ms，手持挥动匹配
#define RSSI_THRESHOLD_DEFAULT -70  // dBm RSSI阈值

#define Check_True   1
#define Check_False  0

/* 动态轮询参数 */
#define RFID_POLL_MIN_MS      30
#define RFID_POLL_MAX_MS      300
#define RFID_POLL_STEP_MS     20
#define RFID_POLL_HIT_STREAK  2
#define RFID_POLL_MISS_STREAK 3
#define RFID_POLL_INIT_MS     80

/* RSSI趋势窗口 */
#define RSSI_TREND_WINDOW     8

typedef struct
{
    uint8_t  epc[EPC_LEN];
    uint32_t last_time;
    uint8_t  valid;
    int8_t   rssi;
    uint8_t  hit_count;              // 命中计数，LRU淘汰时保留高频标签
} TagNode;


void RFID_SearchOnce(void);

uint8_t RFID_GetRxFlag(void);

void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);

void RFID_Clear(void);
u8 RFID_Unpacket(void);
uint8_t RFID_CheckDuplicate(uint8_t *epc, int8_t rssi);
void RFID_CleanExpired(void);
int8_t RSSI_Filter(int8_t last_rssi, int8_t new_rssi);
int8_t RSSI_GetTrend(void);
u8 Get_Checksum(void);

extern int8_t rssi_trend;

#endif
