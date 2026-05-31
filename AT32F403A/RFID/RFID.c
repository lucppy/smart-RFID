#include "RFID.h"
#include "delay.h"
#include "string.h"
#include "usart.h"

u8 RFID_TxPacket[7];				     //�������ݰ�����:FF 01 02 03 04 FE
u8 RFID_buf[512];				         //�������ݰ�����
u32 RFID_cnt = 0, RFID_cntPre = 0;       //�������ݰ����������,�ϴν������ݰ����������
u8 RFID_RxFlag;					         //�������ݰ������־λ
u8 RFIDCard[12] = {0};                   //�洢��ǰ�����ı�ǩEPC
int8_t RSSI = 0;                         //�洢��ǰ�����ı�ǩ�ź�ǿ��

TagNode tagCache[TAG_CACHE_SIZE];        // ��ǩ����

// ��̬��ѯ���������ֳ���������ɵ���
#define RFID_POLL_MIN_MS      30        // ���30msһ��
#define RFID_POLL_MAX_MS      300       // ����300msһ��
#define RFID_POLL_STEP_MS     20        // ÿ�ε���20ms
#define RFID_POLL_HIT_STREAK  2         // �������ж��ٴξͼ���
#define RFID_POLL_MISS_STREAK 3         // ����δ���ж��ٴξͼ���
#define RFID_POLL_INIT_MS     80        // ��ʼ��ѯ���80ms

// ��̬��ѯ״̬
static uint32_t rfid_poll_interval_ms = RFID_POLL_INIT_MS;  // ��ǰ��ѯ���
static uint32_t rfid_last_poll_tick = 0;                    // �ϴη�����ѯ�����ʱ��
static uint32_t rfid_last_hit_tick = 0;                     // �ϴγɹ�������ǩ��ʱ��
static uint8_t rfid_hit_streak = 0;                         // �������м���
static uint8_t rfid_miss_streak = 0;                        // ����δ���м���

//�жϱ�ǩ�Ƿ��ʱ���ظ���ȡ
uint8_t RFID_CheckDuplicate(uint8_t *epc, int8_t rssi)
{
    uint32_t now = GetTick();   // 1ms��ʱ��

    //�˲�
    int8_t filtered_rssi = rssi;

    // 1�����Ƿ��Ѿ�����
    for(int i = 0; i < TAG_CACHE_SIZE; i++)
    {
        //������
        if(tagCache[i].valid &&
                memcmp(tagCache[i].epc, epc, EPC_LEN) == 0)
        {
            filtered_rssi = RSSI_Filter(tagCache[i].rssi, rssi);// �����˲����RSSI
            // �źŹ���,����
            if(filtered_rssi < RSSI_THRESHOLD_DEFAULT)
            {
                return 1; // ��Ϊ��Ч
            }

            // �ж�ʱ�䴰��
            if(now - tagCache[i].last_time < TIME_WINDOW_MS)
            {
                tagCache[i].rssi = filtered_rssi;
                return 1;   // �ظ�
            }
            else
            {
                tagCache[i].last_time = now;
                tagCache[i].rssi = filtered_rssi;
                return 0;   // ��ʱ,��������
            }
        }
    }

    // 2������,�����±�ǩ
    for(int i = 0; i < TAG_CACHE_SIZE; i++)
    {
        if(!tagCache[i].valid)
        {
            memcpy(tagCache[i].epc, epc, EPC_LEN);
            tagCache[i].last_time = now;
            tagCache[i].rssi = filtered_rssi;
            tagCache[i].valid = 1;
            return 0;
        }
    }

    // 3�����������,������ɴ���(LRU)
    int oldest = 0;
    for(int i = 1; i < TAG_CACHE_SIZE; i++)
    {
        if(tagCache[i].last_time < tagCache[oldest].last_time)
        {
            oldest = i;
        }
    }

    memcpy(tagCache[oldest].epc, epc, EPC_LEN);
    tagCache[oldest].last_time = now;
    tagCache[oldest].rssi = filtered_rssi;
    tagCache[oldest].valid = 1;

    return 0;
}

//�����������ڱ�ǩ
void RFID_CleanExpired(void)
{
    uint32_t now = GetTick();

    for(int i = 0; i < TAG_CACHE_SIZE; i++)
    {
        if(tagCache[i].valid &&
                now - tagCache[i].last_time > TIME_WINDOW_MS)
        {
            tagCache[i].valid = 0;
        }
    }
}


int8_t RSSI_Filter(int8_t last_rssi, int8_t new_rssi)
{
    return (last_rssi * 7 + new_rssi) / 8; // ��һ�׵�ͨ
}

//У��ͼ���
u8 Get_Checksum(void)
{
    u8 i = 1;
	u32 sum=0;
    while(i < RFID_cnt - 1)
    {
		sum+=RFID_buf[i++];
    }
	if((sum&0x000000FF)==RFID_buf[RFID_cnt-1])
		return Check_True;
	else return Check_False;
}

/**
  * ��  ��:���ڷ���һ���ֽ�
  * ��  ��:Byte Ҫ���͵�һ���ֽ�
  * ����ֵ:��
  */
void Serial_SendByte(uint8_t Byte)
{
    usart_data_transmit(USART1, Byte);
    while (usart_flag_get(USART1, USART_TDBE_FLAG) == RESET);
    /*�´�д�����ݼĴ������Զ����������ɱ�־λ���ʴ�ѭ�������������־λ*/
}

/**
  * ��  ��:���ڷ���һ������
  * ��  ��:Array Ҫ���͵�������׵�ַ
  * ��  ��:Length Ҫ���͵�����ĳ���
  * ����ֵ:��
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i ++)		//��������
    {
        Serial_SendByte(Array[i]);		//���ε���Serial_SendByte����ÿ���ֽ�����
    }
}

/**
  * ��  ��:���ڷ���һ���ַ���
  * ��  ��:String Ҫ���͵��ַ������׵�ַ
  * ����ֵ:��
  */
void Serial_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i ++)//�����ַ�����(�ַ���),�����ַ���������־λ��ֹͣ
    {
        Serial_SendByte(String[i]);		//���ε���Serial_SendByte����ÿ���ֽ�����
    }
}

/**
  * ��  ��:�η�����(�ڲ�ʹ��)
  * ����ֵ:����ֵ����X��Y�η�
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;	//���ý����ʼֵΪ1
    while (Y --)			//ִ��Y��
    {
        Result *= X;		//��X�۳˵����
    }
    return Result;
}

/**
  * ��  ��:���ڷ�������
  * ��  ��:Number Ҫ���͵����֣���Χ��0~4294967295
  * ��  ��:Length Ҫ���͵����ֵĳ���
  * ����ֵ:��
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i ++)		//�������ֳ��ȱ������ֵ�ÿһλ
    {
        Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');	//���ε���Serial_SendByte����ÿ���ֽ�����
    }
}



/**
  * ��  ��:��RFID���͵�����ѯ����
  * ��  ��:��
  * ����ֵ:��
  */
void RFID_SearchOnce(void)
{
    uint32_t now = GetTick();//��ȡ��ǰʱ���

    // ʱ��բ�ţ���������Ͳ�������
    if ((uint32_t)(now - rfid_last_poll_tick) < rfid_poll_interval_ms)
    {
        return;
    }

    // ����һ������δ���У����ۼ� miss���ﵽ��ֵ�ͷ���
    if ((uint32_t)(now - rfid_last_hit_tick) > rfid_poll_interval_ms)
    {
        if (rfid_miss_streak < RFID_POLL_MISS_STREAK) {// �ۼ� miss
            rfid_miss_streak++;
        }
        rfid_hit_streak = 0;// �������м���

        if (rfid_miss_streak >= RFID_POLL_MISS_STREAK && rfid_poll_interval_ms < RFID_POLL_MAX_MS)// �ﵽmiss��ֵ����δ��������������
        {
            rfid_poll_interval_ms += RFID_POLL_STEP_MS;// ���Ӽ��
            if (rfid_poll_interval_ms > RFID_POLL_MAX_MS)// �����������
            {
                rfid_poll_interval_ms = RFID_POLL_MAX_MS;// ����Ϊ�����
            }
            rfid_miss_streak = 0;// ����δ���м���
        }
    }

    rfid_last_poll_tick = now;// �����ϴ���ѯʱ��

    Serial_SendByte(0xBB);
    u8 temp[5] = {0x00, 0x22, 0x00, 0x00, 0x22};
    Serial_SendArray(temp, 5);
    Serial_SendByte(0x7E);
}

/**
  * ��  ��:��ȡ���ڽ��հ���־λ
  * ��  ��:��
  * ����ֵ:���ڽ��հ���־λ����Χ��0~1�����յ����ݰ��󣬱�־λ��1����ȡ���־λ�Զ�����
  */
uint8_t RFID_GetRxFlag(void)
{
    if (RFID_RxFlag == 1)			//�����־λΪ1
    {
        RFID_RxFlag = 0;
        return 1;					//�򷵻�1,���Զ������־λ
    }
    return 0;						//�����־λΪ0,�򷵻�0
}

/**
  * ��  ��:��մ������ݽ��ջ���
  * ��  ��:��
  * ����ֵ:��
  */
void RFID_Clear(void)
{
    memset(RFID_buf, 0, sizeof(RFID_buf));
    RFID_cnt = 0;
}

/**
  * ��  ��:�������ݽ��,��ȡ����
  * ��  ��:��
  * ����ֵ:�ɹ���ȡ���ŷ���1
  */
u8 RFID_Unpacket(void)
{
    if(RFID_buf[2] == 0x22&&Get_Checksum())
    {
        for(u8 i = 0; i < 12; i++)
        {
            RFIDCard[i] = RFID_buf[i + 8];
        }
        RSSI = RFID_buf[5];

        // ����һ�Σ�����
        rfid_last_hit_tick = GetTick();// �����ϴ�����ʱ��
        if (rfid_hit_streak < RFID_POLL_HIT_STREAK)// �ۼ� hit
        {
            rfid_hit_streak++;
        }
        rfid_miss_streak = 0;// ����δ���м���
        if (rfid_hit_streak >= RFID_POLL_HIT_STREAK && rfid_poll_interval_ms > RFID_POLL_MIN_MS)// �ﵽhit��ֵ����δ����С����������
        {
            if (rfid_poll_interval_ms > RFID_POLL_MIN_MS + RFID_POLL_STEP_MS)// �жϼ�ȥһ����᲻�������С���
            {
                rfid_poll_interval_ms -= RFID_POLL_STEP_MS;// ���ټ��
            }
            else
            {
                rfid_poll_interval_ms = RFID_POLL_MIN_MS;// ����Ϊ��С���
            }
            rfid_hit_streak = 0;// �������м���
        }
        RFID_Clear();
        return 1;
    }
    else
    {
        RFID_Clear();
        return 0;
    }

}

/**
  * ��  ��:USART1�жϺ���
  * ��  ��:��
  * ����ֵ:��
  */
void USART1_IRQHandler(void)
{

    if(usart_interrupt_flag_get(USART1, USART_RDBF_FLAG) != RESET)
    {
        if(RFID_cnt >= sizeof(RFID_buf)) RFID_cnt = 0;
        RFID_buf[RFID_cnt] = usart_data_receive(USART1);
        if(RFID_buf[RFID_cnt] == 0x7E) RFID_RxFlag = 1;
        else RFID_cnt++;
        usart_flag_clear(USART1, USART_RDBF_FLAG);
    }

}
