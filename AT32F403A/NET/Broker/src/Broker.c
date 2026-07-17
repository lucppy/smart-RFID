//��Ƭ��ͷ�ļ�
#include "at32f403a_407.h"

//�����豸
#include "esp8266.h"

//Э���ļ�
#include "Broker.h"
#include "mqttkit.h"
#include "Object.h"

//Ӳ������
#include "usart.h"
#include "delay.h"

//C��
#include <string.h>
#include <stdio.h>
#include "cJSON.h"


#define PROID			"9"

#define ACCESS_KEY		"M"

#define DEVICE_NAME		"t"

extern Object new_objetct[9];
extern u8 tempcard;
extern unsigned char esp8266_buf[512];
u8 respond = 0;
extern Send_Setting send_settting;
extern int flash_w_flag;
extern int8_t RSSI;
extern volatile int disconnect;
extern volatile u8 search;
extern volatile uint8_t clear_from ;
extern volatile uint8_t clear_to   ;
extern volatile uint8_t clear_req  ;

//==========================================================
//	�������ƣ�	Broker_Link
//
//	�������ܣ�	��broker��������
//
//	��ڲ�����	��
//
//	���ز�����	1-�ɹ�	0-ʧ��
//
//	˵����		��brokerƽ̨��������
//==========================================================
_Bool Broker_Link(void)
{

    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//Э���

    unsigned char *dataPtr;

    char authorization_buf[160];

    _Bool status = 1;


    if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 60, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
    {
        ESP8266_SendData(mqttPacket._data, mqttPacket._len);//�ϴ�ƽ̨
        dataPtr = ESP8266_GetIPD(250);									//�ȴ�ƽ̨��Ӧ
        if(dataPtr != NULL)
        {
            if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
            {
                switch(MQTT_UnPacketConnectAck(dataPtr))
                {
                    case 0:
                        UsartPrintf(USART_DEBUG, "Tips:	���ӳɹ�\r\n");
                        status = 0;
                        break;

                    case 1:
                        UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ�Э�����\r\n");
                        break;
                    case 2:
                        UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ��Ƿ���clientid\r\n");
                        break;
                    case 3:
                        UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ�������ʧ��\r\n");
                        break;
                    case 4:
                        UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ��û������������\r\n");
                        break;
                    case 5:
                        UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ��Ƿ�����(����token�Ƿ�)\r\n");
                        break;

                    default:
                        UsartPrintf(USART_DEBUG, "ERR:	����ʧ�ܣ�δ֪����\r\n");
                        break;
                }
            }
        }

        MQTT_DeleteBuffer(&mqttPacket);								//ɾ��
    }
    else
        UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");

    return status;

}

uint32_t Broker_FillBuf(char *buf)
{

    char text[48];

    memset(text, 0, sizeof(text));
    sprintf(text,  "{\"send_number\":%d,\"params\":{", send_settting.send_number);
    strcat(buf, text);

    if(send_settting.send_number > 0 && send_settting.sendwhich != 0)
    {

        for(u8 i = 0; i < 9; i++)
        {
            if((send_settting.sendwhich & (0x0001 << i)) != 0 && send_settting.send_number > 1)
            {
                memset(text, 0, sizeof(text));
                sprintf(text, "\"object%d\":{\"value\":{", i + 1);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"id\":%d,", i + 1);
                strcat(buf, text);
                if(new_objetct[i].RSSI != 0)
                {
                    memset(text, 0, sizeof(text));
                    sprintf(text, "\"RSSI\":%d,", new_objetct[i].RSSI);
                    strcat(buf, text);
                }
                memset(text, 0, sizeof(text));
                sprintf(text, "\"name\":\"%s\",", new_objetct[i].name);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"note\":\"%s\",", new_objetct[i].note);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"card\":\"%s\",}}", new_objetct[i].card);
                strcat(buf, text);

                send_settting.sendwhich &= ~(0x0001 << i);
                send_settting.send_number--;
            }
            else if((send_settting.sendwhich & (0x0001 << i)) != 0 && send_settting.send_number == 1)
            {
                memset(text, 0, sizeof(text));
                sprintf(text, "\"object%d\":{\"value\":{", i + 1);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"id\":%d,", i + 1);
                strcat(buf, text);
                if(new_objetct[i].RSSI != 0)
                {
                    memset(text, 0, sizeof(text));
                    sprintf(text, "\"RSSI\":%d,", new_objetct[i].RSSI);
                    strcat(buf, text);
                }
                memset(text, 0, sizeof(text));
                sprintf(text, "\"name\":\"%s\",", new_objetct[i].name);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"note\":\"%s\",", new_objetct[i].note);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"card\":\"%s\"}}", new_objetct[i].card);
                strcat(buf, text);

                send_settting.sendwhich &= ~(0x0001 << i);
                send_settting.send_number--;
            }
        }
    }

    strcat(buf, "}}");
    UsartPrintf(USART_DEBUG, "buf:%s\r\n", buf);
    return strlen(buf);
}

//==========================================================
//	�������ƣ�	Broker_SendData
//
//	�������ܣ�	�ϴ����ݵ���Ϣ����������
//
//	��ڲ�����	type���������ݵĸ�ʽ
//
//	���ز�����	��
//
//	˵����
//==========================================================
void Broker_SendData(void)
{

    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//Э���

    char buf[512];

    u32 body_len = 0, i = 0;

    //    UsartPrintf(USART_DEBUG, "Tips:	OneNet_SendData-MQTT\r\n");

    //    UsartPrintf(USART_DEBUG, "SendData: begin\r\n");
    memset(buf, 0, sizeof(buf));
    body_len = Broker_FillBuf(buf);		//��ȡ��ǰ��Ҫ���͵����������ܳ���
    //    UsartPrintf(USART_DEBUG, "FillBuf Done, len=%d\r\n", body_len);
    if(body_len)
    {
        //		UsartPrintf(USART_DEBUG, "Packing data...\r\n");
        if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//���
        {
            for(; i < body_len; i++)
                mqttPacket._data[mqttPacket._len++] = buf[i];

            ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//�ϴ�����
            MQTT_DeleteBuffer(&mqttPacket);															//ɾ��
        }
        else
            UsartPrintf(USART_DEBUG, "WARN:	EDP_NewBuffer Failed\r\n");
    }

}

//==========================================================
//	�������ƣ�	Broker_Publish
//
//	�������ܣ�	������Ϣ
//
//	��ڲ�����	topic������������
//				msg����Ϣ����
//
//	���ز�����	��
//
//	˵����
//==========================================================
void Broker_Publish(const char *topic, const char *msg)
{

    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//Э���

    UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);

    if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
    {
        ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//��ƽ̨���Ͷ�������

        MQTT_DeleteBuffer(&mqtt_packet);										//ɾ��
    }

}

//==========================================================
//	�������ƣ�	Broker_Subscribe
//
//	�������ܣ�	����
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����
//==========================================================
void Broker_Subscribe(void)
{

    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//Э���

    char topic_buf[56];
    const char *topic = topic_buf;

    snprintf(topic_buf, sizeof(topic_buf), "asset/command");

    UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);

    if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
    {
        ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//��ƽ̨���Ͷ�������

        MQTT_DeleteBuffer(&mqtt_packet);										//ɾ��
    }

}

//==========================================================
//	�������ƣ�	Broker_Ping
//
//	�������ܣ�	��������
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����
//==========================================================
void Broker_Ping(void)
{

    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//Э���

    if(MQTT_PacketPing(&mqtt_packet) == 0)
    {
        ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//��ƽ̨���Ͷ�������

        MQTT_DeleteBuffer(&mqtt_packet);										//ɾ��
    }

}

//==========================================================
//	�������ƣ�	Broker_RevPro
//
//	�������ܣ�	���ĵ���Ϣ����
//
//	��ڲ�����	dataPtr�����ĵ���Ϣ
//
//	���ز�����	��
//
//	˵����
//==========================================================
void Broker_RevPro(unsigned char *cmd)
{

    char *req_payload = NULL;
    char *cmdid_topic = NULL;

    unsigned short topic_len = 0;
    unsigned short req_len = 0;

    unsigned char qos = 0;
    static unsigned short pkt_id = 0;

    unsigned char type = 0;

    short result = 0;

    cJSON *raw_json, *params_json,*id_json,*delete_json;
    cJSON *search_json;

    type = MQTT_UnPacketRecv(cmd);
    switch(type)
    {
        case MQTT_PKT_PUBLISH:																//���յ�Publish��Ϣ

            result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
            if(result == 0)
            {
//                UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
//                            cmdid_topic, topic_len, req_payload, req_len);

                raw_json = cJSON_Parse(req_payload);
                params_json = cJSON_GetObjectItem(raw_json, "params");
                delete_json = cJSON_GetObjectItem(params_json, "delete");
                if(delete_json != NULL)
                {
					id_json = cJSON_GetObjectItem(delete_json, "id");
                    if(id_json->type == cJSON_Number)
                    {
						flash_w_flag = id_json->valueint-1;
                        UsartPrintf(USART_DEBUG, "delete asset%d\r\n", id_json->valueint);
                    }
                }
    
                send_settting.time = GetTick();
                ESP8266_Clear();
                search_json = cJSON_GetObjectItem(params_json, "search");
                if(search_json != NULL)
                {
                    if(search_json->type == cJSON_Number)
                    {
                        if(search_json->valueint != search)
                        {
                            clear_req = 1;
                            clear_from = 1;
                            clear_to = 4;
                        }
                        search = search_json->valueint;
                        UsartPrintf(USART_DEBUG, "search:%d\r\n", search);
                    }
                }

                cJSON_Delete(raw_json);
                break;
            }

        case MQTT_PKT_PUBACK:														//����Publish��Ϣ���������ظ���Ack

            if(MQTT_UnPacketPublishAck(cmd) == 1)
            {
                disconnect = 0;
                UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");
            }

            break;

        case MQTT_PKT_SUBACK:																//����Subscribe��Ϣ��Ack

            if(MQTT_UnPacketSubscribe(cmd) == 0)
                UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe OK\r\n");
            else
                UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe Err\r\n");
            break;

        /*ADD BRGIN*/
        case MQTT_PKT_PINGRESP:																//����ping��Ϣ��Ack
            disconnect--;
            UsartPrintf(USART_DEBUG, "Tips:	MQTT ping OK\r\n");
            break;

        default:
            result = -1;
            break;
    }

    ESP8266_Clear();									//��ջ���

    if(result == -1)
        return;

}

