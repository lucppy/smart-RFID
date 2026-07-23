//单片机头文件
#include "at32f403a_407.h"
//网络
#include "Broker.h"
#include "esp8266.h"

//硬件驱动
#include "delay.h"
#include "usart.h"
#include "oled.h"
#include "Timer.h"
#include "Flash.h"
#include "RFID.h"

//C库
#include <string.h>
#include <stdio.h>

#define Broker_Address		"AT+CIPSTART=\"TCP\",\"broker.emqx.io\",1883\r\n"//MQTT代理服务器地址
#include "Object.h"
void Hardware_Init(void);
void Read_Card(void);
void Check_Card(void);

char oled_str[50];                      //OLED显示缓冲区
volatile uint8_t clear_from = 0;        //需要清除的起始行
volatile uint8_t clear_to   = 0;        //需要清除的结束行
volatile uint8_t clear_req  = 0;        //清除请求标志，TIM2中断服务程序设置为1，主循环检测后清除为0
u8 tempcard = 10;                       //临时变量，记录当前卡号在FLASH中的位置，0-8为对应位置，9为满了，10为未找到
extern u8 RFIDCard[12];                 //当前读到的卡号
u8 ID_temp[10][12];                     //临时变量，记录FLASH中存储的卡号，10行对应10个位置，12列对应64位卡号的12个字节
Object new_objetct[9];                  //当前的9个对象，包含卡号、RSSI等信息
int flash_w_flag = -1;                  //FLASH写入标志，-1表示不需要写入，0-8表示需要重写对应位置的卡号
extern u8 RFID_buf[512];                //RFID接收缓冲区
extern int8_t RSSI;                     //当前读到的卡的RSSI值



Send_Setting send_settting;
volatile int disconnect = 0;
volatile u8 search = 0;                 //读卡标志，0为暂停，1为读卡中

int main(void)
{
    unsigned char *dataPtr = NULL;

    memset(ID_temp, 0xFF, sizeof(ID_temp));
    Hardware_Init();				//初始化外围硬件

    ESP8266_Init();					//初始化ESP8266

    OLED_Clear();
    OLED_ShowString(1, 1, "Connect MQTTs");
    OLED_ShowString(2, 1, "Server...");
    ESP8266_CMD Connect = {NULL, NULL, 0};
    Connect.cmd =  Broker_Address;
    Connect.res = "CONNECT";
    while(ESP8266_SendCmd(&Connect))
        DelayXms(500);

    OLED_ShowString(3, 1, "Connect MQTT ");
    OLED_ShowString(4, 1, "Server Success");
    DelayXms(500);

    OLED_Clear();
    OLED_ShowString(1, 1, "Device login ...");

    Connect.cmd =  Broker_Address;
    Connect.res = "CONNECT";
    Connect.debug = 1;
    while(Broker_Link())			//接入broker
    {
        ESP8266_SendCmd(&Connect);
        DelayXms(500);
    }
    OLED_ShowString(3, 1, "login finish");
    Broker_Subscribe();
    OLED_Clear();


    while(1)
    {
        if(search)
        {
            /***********读卡中*************/
            OLED_ShowCNString(1, 1, (uint32_t[])
            {
                0, 1, 2
            }, 3);
            OLED_ShowString(1, 7, "..."); //读卡中...
            RFID_SearchOnce();
            RFID_CleanExpired();
            Read_Card();
            if(RFID_GetRxFlag() == 1)
            {
//			UsartPrintf(USART_DEBUG, "%02X\r\n", RFID_buf[2]);
                if(RFID_Unpacket() == 1)
                {
                    if(!RFID_CheckDuplicate(RFIDCard, RSSI))
                    {
                    sprintf(oled_str, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X ",
                            RFIDCard[0], RFIDCard[1], RFIDCard[2], RFIDCard[3],
                            RFIDCard[4], RFIDCard[5], RFIDCard[6], RFIDCard[7],
                            RFIDCard[8], RFIDCard[9], RFIDCard[10], RFIDCard[11]);

                    OLED_ShowCNString(2, 1, (uint32_t[])
                    {
                        1, 3
                    }, 2);
                    OLED_ShowString(2, 5, ":"); //卡号:
                    OLED_ShowString(3, 1, oled_str);
//                    UsartPrintf(USART_DEBUG, "%s\r\n", oled_str);
//                    UsartPrintf(USART_DEBUG, "%d\r\n", RSSI);
                    u32 address = 0;
                    Check_Card();
//                UsartPrintf(USART_DEBUG, "tempcard=%d\r\n", tempcard);
                    if(tempcard > 8)
                    {
                        OLED_ShowCNString(4, 1, (uint32_t[])
                        {
                            1, 3, 8, 61
                        }, 4); //卡号已满;
                        clear_from = 3;
                        clear_to   = 4;
                        tmr_counter_enable(TMR2, TRUE);
                    }
                    else
                    {
                        switch(tempcard)
                        {
                            case 0:
                                address = FLASH_ADDR0;
                                break;
                            case 1:
                                address = FLASH_ADDR1;
                                break;
                            case 2:
                                address = FLASH_ADDR2;
                                break;
                            case 3:
                                address = FLASH_ADDR3;
                                break;
                            case 4:
                                address = FLASH_ADDR4;
                                break;
                            case 5:
                                address = FLASH_ADDR5;
                                break;
                            case 6:
                                address = FLASH_ADDR6;
                                break;
                            case 7:
                                address = FLASH_ADDR7;
                                break;
                            case 8:
                                address = FLASH_ADDR8;
                                break;
                            default:
                                break;
                        }
                        send_settting.send_reagy = 1;
                        send_settting.sendwhich |= (0x0001 << tempcard);
                        send_settting.send_number++;
                        FLASH_W4(address, RFIDCard);
                        Read_Card();

                        clear_from = 3;
                        clear_to   = 4;
                        tmr_counter_enable(TMR2, TRUE);

                        new_objetct[tempcard].RSSI = RSSI;
                        new_objetct[tempcard].serial_number = tempcard;
                        strcpy(new_objetct[tempcard].card, oled_str);
                    }
                    } // CheckDuplicate 为非重复
                }
            }
        }
        else
        {
            OLED_ShowCNString(1, 1, (uint32_t[])
            {
                0, 1, 84, 85
            }, 4);//读卡暂停
        }

        if (clear_req)
        {
            OLED_ClearLines(clear_from, clear_to);
            clear_from = clear_to = 0;
            clear_req = 0;
        }

        if(flash_w_flag != -1 ) //0<=flash_w_flag<=9,flash_w_flag:objectX-1
        {

            UsartPrintf(USART_DEBUG, "Rewrite FLASH%d\r\n", flash_w_flag);
            u32 tempaddress = FLASH_ADDR0 + flash_w_flag * 0x00000800;
            FLASH_Clear(tempaddress);
            Read_Card();

            flash_w_flag = -1;
        }

        if(GetTick() - send_settting.time >= 60000 && !send_settting.send_reagy)
        {
            Broker_Ping();
            send_settting.time = GetTick();
            disconnect++;
        }
        if(send_settting.send_reagy)
        {
            Broker_SendData();							//发送数据
            send_settting.send_reagy = 0;
            disconnect++;
            ESP8266_Clear();
        }

        if(disconnect >= 10) //认为断连
        {
            UsartPrintf(USART_DEBUG, "Disconnect\r\n");
            ESP8266_Init();
            Connect.cmd =  Broker_Address;
            Connect.res = "CONNECT";
            Connect.debug = 0;
            while(Broker_Link())			//接入OneNET
            {
                ESP8266_SendCmd(&Connect);
                DelayXms(500);
            }
            UsartPrintf(USART_DEBUG, "Connect again\r\n");
            disconnect = 0;
            OLED_Clear();
        }

        dataPtr = ESP8266_GetIPD(0);
        if(dataPtr != NULL)
            Broker_RevPro(dataPtr);
        else if(ESP8266_CheckClosed())
        {
            disconnect = 10;
        }

    }

}
/*
************************************************************
*	函数名称：	system_clock_config
*
*	函数功能：	配置系统时钟 72MHz
*               HEXT(8MHz) -> PLL x9 -> 72MHz
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		替代 STM32 SetSysClockTo72()
************************************************************
*/
void system_clock_config(void)
{
    /* 复位 CRM */
    crm_reset();

    /* 开启外部高速晶振 HEXT(8MHz) */
    crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, TRUE);

    /* 等待晶振稳定 */
    while(crm_hext_stable_wait() == ERROR)
    {
    }

    /* PLL: HEXT x9 = 72MHz */
    crm_pll_config(CRM_PLL_SOURCE_HEXT, CRM_PLL_MULT_9, CRM_PLL_OUTPUT_RANGE_LE72MHZ);

    /* 使能 PLL */
    crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, TRUE);

    /* 等待 PLL 锁定 */
    while(crm_flag_get(CRM_PLL_STABLE_FLAG) != SET)
    {
    }

    /* AHB 不分频: HCLK = 72MHz */
    crm_ahb_div_set(CRM_AHB_DIV_1);

    /* APB2 不分频: PCLK2 = 72MHz */
    crm_apb2_div_set(CRM_APB2_DIV_1);

    /* APB1 不分频: PCLK1 = 72MHz */
    crm_apb1_div_set(CRM_APB1_DIV_1);

    /* 自动调速模式 */
    crm_auto_step_mode_enable(TRUE);

    /* 切换系统时钟到 PLL */
    crm_sysclk_switch(CRM_SCLK_PLL);

    /* 等待切换完成 */
    while(crm_sysclk_switch_status_get() != CRM_SCLK_PLL)
    {
    }

    /* 关闭自动调速 */
    crm_auto_step_mode_enable(FALSE);

    /* 更新 system_core_clock 为 72MHz */
    system_core_clock_update();
}

/*
************************************************************
*	函数名称：	Hardware_Init
*
*	函数功能：	硬件初始化
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		初始化单片机功能以及外接设备
************************************************************
*/
void Hardware_Init(void)
{
    /* 使能 FPU（Cortex-M4F 需要） */
    SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));

    /* 配置系统时钟: HEXT(8MHz) -> PLL x9 -> 72MHz */
    system_clock_config();

    nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);

    Delay_Init();									//systick初始化

    Usart1_Init(115200);

    Usart2_Init(115200);

    Usart3_Init(115200);

    Timer_Init();

    OLED_Init();			//初始化OLED

    Read_Card();
}
/*
************************************************************
*	函数名称：	Check_Card
*
*	函数功能：	检查EPC是否已经存在
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		检查EPC是否已经存在
************************************************************
*/
void Check_Card(void)
{
    Read_Card();
    u8 i = 0;
    tempcard = 0xFF;
    /* 1. 先判断是否已存在 */
    for(i = 0; i <= 8; i++)
    {
        if(RFIDCard[0] == ID_temp[i][0] &&
                RFIDCard[1] == ID_temp[i][1] &&
                RFIDCard[2] == ID_temp[i][2] &&
                RFIDCard[3] == ID_temp[i][3] &&
                RFIDCard[4] == ID_temp[i][4] &&
                RFIDCard[5] == ID_temp[i][5] &&
                RFIDCard[6] == ID_temp[i][6] &&
                RFIDCard[7] == ID_temp[i][7] &&
                RFIDCard[8] == ID_temp[i][8] &&
                RFIDCard[9] == ID_temp[i][9] &&
                RFIDCard[10] == ID_temp[i][10] &&
                RFIDCard[11] == ID_temp[i][11] )
        {
            tempcard = i;
            return;      // 已存在，直接返回
        }
    }

    /* 2. 再找第一个空位 */
    for(i = 0; i <= 8; i++)
    {
        if(ID_temp[i][0] == 0xFF &&
                ID_temp[i][1] == 0xFF &&
                ID_temp[i][2] == 0xFF &&
                ID_temp[i][3] == 0xFF &&
                ID_temp[i][4] == 0xFF &&
                ID_temp[i][5] == 0xFF &&
                ID_temp[i][6] == 0xFF &&
                ID_temp[i][7] == 0xFF &&
                ID_temp[i][8] == 0xFF &&
                ID_temp[i][9] == 0xFF &&
                ID_temp[i][10] == 0xFF &&
                ID_temp[i][11] == 0xFF
          )
        {
            tempcard = i;
            return;
        }
    }

    /* 3. 满了 */
    tempcard = 9;
}
/*
************************************************************
*	函数名称：	Read_Card
*
*	函数功能：	将EPC存储在FLASH
*
*	入口参数：	无
*
*	返回参数：	无
************************************************************
*/
void Read_Card(void)
{
    u8 i = 0;
    u32 address;
    while(i <= 8)
    {
        switch(i)
        {
            case 0:
                address = FLASH_ADDR0;
                break;
            case 1:
                address = FLASH_ADDR1;
                break;
            case 2:
                address = FLASH_ADDR2;
                break;
            case 3:
                address = FLASH_ADDR3;
                break;
            case 4:
                address = FLASH_ADDR4;
                break;
            case 5:
                address = FLASH_ADDR5;
                break;
            case 6:
                address = FLASH_ADDR6;
                break;
            case 7:
                address = FLASH_ADDR7;
                break;
            case 8:
                address = FLASH_ADDR8;
                break;
            default:
                break;
        }
        ID_temp[i][0] = FLASH_R(address);
        ID_temp[i][1] = FLASH_R(address + 2);
        ID_temp[i][2] = FLASH_R(address + 4);
        ID_temp[i][3] = FLASH_R(address + 6);
        ID_temp[i][4] = FLASH_R(address + 8);
        ID_temp[i][5] = FLASH_R(address + 10);
        ID_temp[i][6] = FLASH_R(address + 12);
        ID_temp[i][7] = FLASH_R(address + 14);
        ID_temp[i][8] = FLASH_R(address + 16);
        ID_temp[i][9] = FLASH_R(address + 18);
        ID_temp[i][10] = FLASH_R(address + 20);
        ID_temp[i][11] = FLASH_R(address + 22); //读取64位卡号
        i++;
    }

}

void TMR2_GLOBAL_IRQHandler(void)
{
    if (tmr_interrupt_flag_get(TMR2, TMR_OVF_INT) == SET)
    {
        clear_req = 1;
        tmr_flag_clear(TMR2, TMR_OVF_INT);
        tmr_counter_enable(TMR2, FALSE);
    }
}
