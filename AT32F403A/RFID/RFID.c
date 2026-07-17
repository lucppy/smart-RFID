#include "RFID.h"
#include "delay.h"
#include "string.h"
#include "usart.h"

u8 RFID_TxPacket[7];
u8 RFID_buf[512];
u32 RFID_cnt = 0, RFID_cntPre = 0;
u8 RFID_RxFlag;
u8 RFIDCard[12] = {0};
int8_t RSSI = 0;

TagNode tagCache[TAG_CACHE_SIZE];

/* 动态轮询状态 */
static uint32_t rfid_poll_interval_ms = RFID_POLL_INIT_MS;
static uint32_t rfid_last_poll_tick  = 0;
static uint32_t rfid_last_hit_tick   = 0;
static uint8_t  rfid_hit_streak      = 0;
static uint8_t  rfid_miss_streak     = 0;

/* RSSI趋势状态 */
static int8_t rssi_history[RSSI_TREND_WINDOW];
static uint8_t rssi_history_idx = 0;
static uint8_t rssi_history_full = 0;
int8_t rssi_trend = 0;   // -1=远离 0=稳定 +1=靠近

/*
 * RFID_CheckDuplicate - LRU去重+命中计数
 * 返回 1=重复(跳过), 0=新标签(继续处理)
 */
uint8_t RFID_CheckDuplicate(uint8_t *epc, int8_t rssi)
{
    uint32_t now = GetTick();
    int8_t filtered_rssi = rssi;

    /* 1. 查找是否已存在 */
    for (int i = 0; i < TAG_CACHE_SIZE; i++)
    {
        if (tagCache[i].valid &&
            memcmp(tagCache[i].epc, epc, EPC_LEN) == 0)
        {
            filtered_rssi = RSSI_Filter(tagCache[i].rssi, rssi);
            if (tagCache[i].hit_count < 255)
                tagCache[i].hit_count++;

            if (filtered_rssi < RSSI_THRESHOLD_DEFAULT)
            {
                tagCache[i].last_time = now;
                return 1;
            }

            if (now - tagCache[i].last_time < TIME_WINDOW_MS)
            {
                tagCache[i].rssi = filtered_rssi;
                return 1;
            }
            else
            {
                tagCache[i].last_time = now;
                tagCache[i].rssi = filtered_rssi;
                return 0;
            }
        }
    }

    /* 2. 找空位插入 */
    for (int i = 0; i < TAG_CACHE_SIZE; i++)
    {
        if (!tagCache[i].valid)
        {
            memcpy(tagCache[i].epc, epc, EPC_LEN);
            tagCache[i].last_time = now;
            tagCache[i].rssi = filtered_rssi;
            tagCache[i].valid = 1;
            tagCache[i].hit_count = 1;
            return 0;
        }
    }

    /* 3. 缓存满 - LRU淘汰 */
    int victim = 0;
    for (int i = 1; i < TAG_CACHE_SIZE; i++)
    {
        if (tagCache[i].hit_count < tagCache[victim].hit_count)
            victim = i;
        else if (tagCache[i].hit_count == tagCache[victim].hit_count &&
                 tagCache[i].last_time < tagCache[victim].last_time)
            victim = i;
    }

    memcpy(tagCache[victim].epc, epc, EPC_LEN);
    tagCache[victim].last_time = now;
    tagCache[victim].rssi = filtered_rssi;
    tagCache[victim].valid = 1;
    tagCache[victim].hit_count = 1;

    return 0;
}

void RFID_CleanExpired(void)
{
    uint32_t now = GetTick();
    for (int i = 0; i < TAG_CACHE_SIZE; i++)
    {
        if (tagCache[i].valid &&
            now - tagCache[i].last_time > TIME_WINDOW_MS)
        {
            tagCache[i].valid = 0;
        }
    }
}

/*
 * RSSI_Filter - 自适应EMA低通
 * 波动<3dB: alpha=0.1强平滑
 * 波动>=3dB: alpha=0.4快速跟踪
 */
int8_t RSSI_Filter(int8_t last_rssi, int8_t new_rssi)
{
    int8_t diff = new_rssi - last_rssi;
    if (diff < 0) diff = -diff;

    if (diff < 3)
        return (last_rssi * 9 + new_rssi) / 10;
    else
        return (last_rssi * 6 + new_rssi * 4) / 10;
}

/*
 * RSSI_GetTrend - 线性趋势检测
 * 返回: -1=远离 0=稳定 +1=靠近
 */
int8_t RSSI_GetTrend(void)
{
    int16_t sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    uint8_t n = rssi_history_full ? RSSI_TREND_WINDOW : rssi_history_idx;
    if (n < 3) return 0;

    uint8_t oldest = rssi_history_full ? rssi_history_idx : 0;
    for (uint8_t j = 0; j < n; j++)
    {
        uint8_t pos = (oldest + j) % RSSI_TREND_WINDOW;
        sum_x  += j;
        sum_y  += rssi_history[pos];
        sum_xy += j * rssi_history[pos];
        sum_x2 += j * j;
    }

    int16_t num = (int16_t)n * sum_xy - sum_x * sum_y;
    int16_t den = (int16_t)n * sum_x2 - sum_x * sum_x;
    if (den == 0) return 0;

    int16_t slope = num / den;

    if (slope > 1)       return 1;
    else if (slope < -1) return -1;
    else                 return 0;
}

static void RSSI_PushTrend(int8_t rssi)
{
    rssi_history[rssi_history_idx] = rssi;
    rssi_history_idx++;
    if (rssi_history_idx >= RSSI_TREND_WINDOW)
    {
        rssi_history_idx = 0;
        rssi_history_full = 1;
    }
}

u8 Get_Checksum(void)
{
    u8 i = 1;
    u32 sum = 0;
    while (i < RFID_cnt - 1)
    {
        sum += RFID_buf[i++];
    }
    if ((sum & 0x000000FF) == RFID_buf[RFID_cnt - 1])
        return Check_True;
    else return Check_False;
}

void Serial_SendByte(uint8_t Byte)
{
    usart_data_transmit(USART1, Byte);
    while (usart_flag_get(USART1, USART_TDBE_FLAG) == RESET);
}

void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    for (uint16_t i = 0; i < Length; i++)
        Serial_SendByte(Array[i]);
}

void Serial_SendString(char *String)
{
    for (uint8_t i = 0; String[i] != '\0'; i++)
        Serial_SendByte(String[i]);
}

uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) Result *= X;
    return Result;
}

void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
        Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
}

/*
 * RFID_SearchOnce - 动态轮询
 * 命中加速(min 30ms), 空扫减速(max 300ms)
 */
void RFID_SearchOnce(void)
{
    uint32_t now = GetTick();

    if ((uint32_t)(now - rfid_last_poll_tick) < rfid_poll_interval_ms)
        return;

    /* 空扫检测(跳过冷启动) */
    if (rfid_last_hit_tick != 0 &&
        (uint32_t)(now - rfid_last_hit_tick) > rfid_poll_interval_ms)
    {
        if (rfid_miss_streak < RFID_POLL_MISS_STREAK)
            rfid_miss_streak++;
        rfid_hit_streak = 0;

        if (rfid_miss_streak >= RFID_POLL_MISS_STREAK &&
            rfid_poll_interval_ms < RFID_POLL_MAX_MS)
        {
            rfid_poll_interval_ms += RFID_POLL_STEP_MS;
            if (rfid_poll_interval_ms > RFID_POLL_MAX_MS)
                rfid_poll_interval_ms = RFID_POLL_MAX_MS;
        }
        /* 已达最大间隔或已调整 → 重置计数 */
        if (rfid_miss_streak >= RFID_POLL_MISS_STREAK)
            rfid_miss_streak = 0;
    }

    rfid_last_poll_tick = now;

    Serial_SendByte(0xBB);
    u8 temp[5] = {0x00, 0x22, 0x00, 0x00, 0x22};
    Serial_SendArray(temp, 5);
    Serial_SendByte(0x7E);
}

uint8_t RFID_GetRxFlag(void)
{
    if (RFID_RxFlag == 1)
    {
        RFID_RxFlag = 0;
        return 1;
    }
    return 0;
}

void RFID_Clear(void)
{
    memset(RFID_buf, 0, sizeof(RFID_buf));
    RFID_cnt = 0;
}

u8 RFID_Unpacket(void)
{
    if (RFID_buf[2] == 0x22 && Get_Checksum())
    {
        for (u8 i = 0; i < 12; i++)
            RFIDCard[i] = RFID_buf[i + 8];
        RSSI = RFID_buf[5];

        /* 命中: 加速轮询 + RSSI趋势 */
        rfid_last_hit_tick = GetTick();
        if (rfid_hit_streak < RFID_POLL_HIT_STREAK)
            rfid_hit_streak++;
        rfid_miss_streak = 0;

        if (rfid_hit_streak >= RFID_POLL_HIT_STREAK &&
            rfid_poll_interval_ms > RFID_POLL_MIN_MS + RFID_POLL_STEP_MS)
        {
            rfid_poll_interval_ms -= RFID_POLL_STEP_MS;
        }
        else if (rfid_poll_interval_ms <= RFID_POLL_MIN_MS + RFID_POLL_STEP_MS &&
                 rfid_poll_interval_ms > RFID_POLL_MIN_MS)
        {
            rfid_poll_interval_ms = RFID_POLL_MIN_MS;
        }
        /* 已达最小间隔或已调整 → 重置计数 */
        if (rfid_hit_streak >= RFID_POLL_HIT_STREAK)
            rfid_hit_streak = 0;

        RSSI_PushTrend(RSSI);
        rssi_trend = RSSI_GetTrend();

        RFID_Clear();
        return 1;
    }
    else
    {
        RFID_Clear();
        return 0;
    }
}

void USART1_IRQHandler(void)
{
    if (usart_interrupt_flag_get(USART1, USART_RDBF_FLAG) != RESET)
    {
        if (RFID_cnt >= sizeof(RFID_buf)) RFID_cnt = 0;
        RFID_buf[RFID_cnt] = usart_data_receive(USART1);
        if (RFID_buf[RFID_cnt] == 0x7E) RFID_RxFlag = 1;
        else RFID_cnt++;
        usart_flag_clear(USART1, USART_RDBF_FLAG);
    }
}
