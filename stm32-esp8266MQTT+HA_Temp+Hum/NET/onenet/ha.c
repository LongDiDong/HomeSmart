/**
	************************************************************
	************************************************************
	************************************************************
	*	�ļ����� 	ha.c
	*
	*	���ߣ� 		�ż���
	*
	*	���ڣ� 		2017-05-08
	*
	*	�汾�� 		V1.1
	*
	*	˵���� 		��haƽ̨�����ݽ����ӿڲ�
	*
	*	�޸ļ�¼��	V1.0��Э���װ�������ж϶���ͬһ���ļ������Ҳ�ͬЭ��ӿڲ�ͬ��
	*				V1.1���ṩͳһ�ӿڹ�Ӧ�ò�ʹ�ã����ݲ�ͬЭ���ļ�����װЭ����ص����ݡ�
	************************************************************
	************************************************************
	************************************************************
**/

//��Ƭ��ͷ�ļ�
#include "stm32f10x.h"

//�����豸
#include "esp8266.h"

//Э���ļ�
#include "ha.h"
#include "mqttkit.h"

//�㷨
#include "base64.h"
#include "hmac_sha1.h"

//Ӳ������
#include "usart.h"
#include "delay.h"


//C��
#include <string.h>
#include <stdio.h>


/*
*    Usernames and passwords are set on the MQTT server. 
*    If the server allows anonymous login without a password, 
*    there is no need to modify them.
*/

#define PROID			"User"         //MQTT user  name

#define ACCESS_KEY		"password"    //MQTT user  psw

#define DEVICE_NAME		"diver" //MQTT client  ID

#include "cJSON.h"


char devid[16];

char key[48];




extern unsigned char esp8266_buf[512];




/*
************************************************************
*	�������ƣ�	OTA_UrlEncode
*
*	�������ܣ�	sign��Ҫ����URL����
*
*	��ڲ�����	sign�����ܽ��
*
*	���ز�����	0-�ɹ�	����-ʧ��
*
*	˵����		+			%2B
*				�ո�		%20
*				/			%2F
*				?			%3F
*				%			%25
*				#			%23
*				&			%26
*				=			%3D
************************************************************
*/
static unsigned char OTA_UrlEncode(char *sign)
{

	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);
	
	if(sign == (void *)0 || sign_len < 28)
		return 1;
	
	for(; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;
	
	for(i = 0, j = 0; i < sign_len; i++)
	{
		switch(sign_t[i])
		{
			case '+':
				strcat(sign + j, "%2B");j += 3;
			break;
			
			case ' ':
				strcat(sign + j, "%20");j += 3;
			break;
			
			case '/':
				strcat(sign + j, "%2F");j += 3;
			break;
			
			case '?':
				strcat(sign + j, "%3F");j += 3;
			break;
			
			case '%':
				strcat(sign + j, "%25");j += 3;
			break;
			
			case '#':
				strcat(sign + j, "%23");j += 3;
			break;
			
			case '&':
				strcat(sign + j, "%26");j += 3;
			break;
			
			case '=':
				strcat(sign + j, "%3D");j += 3;
			break;
			
			default:
				sign[j] = sign_t[i];j++;
			break;
		}
	}
	
	sign[j] = 0;
	
	return 0;

}

/*
************************************************************
*	�������ƣ�	OTA_Authorization
*
*	�������ܣ�	����Authorization
*
*	��ڲ�����	ver��������汾�ţ����ڸ�ʽ��Ŀǰ��֧�ָ�ʽ"2018-10-31"
*				res����Ʒid
*				et������ʱ�䣬UTC��ֵ
*				access_key��������Կ
*				dev_name���豸��
*				authorization_buf������token��ָ��
*				authorization_buf_len������������(�ֽ�)
*
*	���ز�����	0-�ɹ�	����-ʧ��
*
*	˵����		��ǰ��֧��sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char HA_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//����ǩ����Base64������ �� URL������
	char hmac_sha1_buf[64];							//����ǩ��
	char access_key_base64[64];						//����access_key��Base64������
	char string_for_signature[72];					//����string_for_signature������Ǽ��ܵ�key

//----------------------------------------------------�����Ϸ���--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------��access_key����Base64����----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
	//UsartPrintf(USART_DEBUG, "access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------����string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
	//UsartPrintf(USART_DEBUG, "string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------����-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
	//UsartPrintf(USART_DEBUG, "hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------�����ܽ������Base64����------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------��Base64����������URL����---------------------------------------------------
	OTA_UrlEncode(sign_buf);
	//UsartPrintf(USART_DEBUG, "sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------����Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
	//UsartPrintf(USART_DEBUG, "Token: %s\r\n", authorization_buf);
	
	return 0;

}

//==========================================================
//	�������ƣ�	HA_RegisterDevice
//
//	�������ܣ�	�ڲ�Ʒ��ע��һ���豸
//
//	��ڲ�����	access_key��������Կ
//				pro_id����ƷID
//				serial��Ψһ�豸��
//				devid�����淵�ص�devid
//				key�����淵�ص�key
//
//	���ز�����	0-�ɹ�		1-ʧ��
//
//	˵����		
//==========================================================
_Bool HA_RegisterDevice(void)
{

	_Bool result = 1;
	unsigned short send_len = 11 + strlen(DEVICE_NAME);
	char *send_ptr = NULL, *data_ptr = NULL;
	
	char authorization_buf[144];													//���ܵ�key
	
	send_ptr = malloc(send_len + 240);
	if(send_ptr == NULL)
		return result;
	
	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80\r\n", "CONNECT"))
		DelayXms(500);
	
	HA_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, NULL,
							authorization_buf, sizeof(authorization_buf), 1);
	
	snprintf(send_ptr, 240 + send_len, "POST /mqtt/v1/devices/reg HTTP/1.1\r\n"
					"Authorization:%s\r\n"
					"Host:ota.heclouds.com\r\n"
					"Content-Type:application/json\r\n"
					"Content-Length:%d\r\n\r\n"
					"{\"name\":\"%s\"}",
	
					authorization_buf, 11 + strlen(DEVICE_NAME), DEVICE_NAME);
	
	ESP8266_SendData((unsigned char *)send_ptr, strlen(send_ptr));
	
	/*
	{
	  "request_id" : "f55a5a37-36e4-43a6-905c-cc8f958437b0",
	  "code" : "ha_common_success",
	  "code_no" : "000000",
	  "message" : null,
	  "data" : {
		"device_id" : "589804481",
		"name" : "mcu_id_43057127",
		
	"pid" : 282932,
		"key" : "indu/peTFlsgQGL060Gp7GhJOn9DnuRecadrybv9/XY="
	  }
	}
	*/
	
	data_ptr = (char *)ESP8266_GetIPD(250);							//�ȴ�ƽ̨��Ӧ
	
	if(data_ptr)
	{
		data_ptr = strstr(data_ptr, "device_id");
	}
	
	if(data_ptr)
	{
		char name[16];
		int pid = 0;
		
		if(sscanf(data_ptr, "device_id\" : \"%[^\"]\",\r\n\"name\" : \"%[^\"]\",\r\n\r\n\"pid\" : %d,\r\n\"key\" : \"%[^\"]\"", devid, name, &pid, key) == 4)
		{
			UsartPrintf(USART_DEBUG, "create device: %s, %s, %d, %s\r\n", devid, name, pid, key);
			result = 0;
		}
	}
	
	free(send_ptr);
	ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK");
	
	return result;

}

//==========================================================
//	�������ƣ�	HA_DevLink
//
//	�������ܣ�	��ha��������
//
//	��ڲ�����	��
//
//	���ز�����	1-�ɹ�	0-ʧ��
//
//	˵����		��haƽ̨��������
//==========================================================

_Bool HA_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//Э���

	unsigned char *dataPtr;
	
	_Bool status = 1;
	
	UsartPrintf(USART_DEBUG, "HA_DevLink\r\n"
							"PROID: %s,	AUIF: %s,	DEVID:%s\r\n"
                        , PROID, ACCESS_KEY, DEVICE_NAME);
	
	if(MQTT_PacketConnect(PROID, ACCESS_KEY, DEVICE_NAME, 256, 0, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//�ϴ�ƽ̨
		
		dataPtr = ESP8266_GetIPD(250);									//�ȴ�ƽ̨��Ӧ
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:UsartPrintf(USART_DEBUG, "Tips:	���ӳɹ�\r\n");status = 0;break;
					
					case 1:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ�Э�����\r\n");break;
					case 2:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ��Ƿ���clientid\r\n");break;
					case 3:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ�������ʧ��\r\n");break;
					case 4:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ��û������������\r\n");break;
					case 5:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ��Ƿ�����(����token�Ƿ�)\r\n");break;
					
					default:UsartPrintf(USART_DEBUG, "ERR:	����ʧ�ܣ�δ֪����\r\n");break;
				}
			}
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//ɾ��
	}
	else
		UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");
	
	return status;
	
}

unsigned char HA_FillBuf(char *buf)
{
	
	char text[128];
	
	memset(text, 0, sizeof(text));
	strcpy(buf, "{\"unique_id\":\"HA-ESP32-05-4\",");
    strcat(buf, text);
    
    memset(text, 0, sizeof(text));
    strcpy(text, "\"name\":\"envir\",");
    strcat(buf, text);
   
    memset(text, 0, sizeof(text));
    strcpy(text, "\"state_topic\":\"HA-ESP32-05/4/state\",");
    strcat(buf, text);
    
    memset(text, 0, sizeof(text));
    strcpy(text, "\"json_attributes_topic\":\"HA-ESP32-05/4/attributes\", \"device\":{");
    strcat(buf, text);
    
    memset(text, 0, sizeof(text));
    strcpy(text, "\"identifiers\":\"ESP32-05\",");
    strcat(buf, text);
    
    memset(text, 0, sizeof(text));
    strcpy(text, "\"manufacturer\":\"ren\",");
    strcat(buf, text);
    
    memset(text, 0, sizeof(text));
    strcpy(text, "\"model\":\"HA\",");
    strcat(buf, text);
    
    memset(text, 0, sizeof(text));
    strcpy(text, "\"name\":\"ESP32-05\",");
    strcat(buf, text);
 
	strcat(buf, "}}");
	
	return strlen(buf);

}

//==========================================================
//	�������ƣ�	HA_SendData
//
//	�������ܣ�	�ϴ����ݵ�ƽ̨
//
//	��ڲ�����	type���������ݵĸ�ʽ
//
//	���ز�����	��
//
//	˵����		
//==========================================================
//void HA_SendData(void)
//{
//	
//	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//Э���
//	
//	char buf[256];
//	
//	short body_len = 0, i = 0;
//	
//	UsartPrintf(USART_DEBUG, "Tips:	HA_SendData-MQTT\r\n");
//	
//	memset(buf, 0, sizeof(buf));
//	
//	body_len = HA_FillBuf(buf);																	//��ȡ��ǰ��Ҫ���͵����������ܳ���
//	
//	if(body_len)
//	{
//		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//���
//		{
//			for(; i < body_len; i++)
//				mqttPacket._data[mqttPacket._len++] = buf[i];
//			
//			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//�ϴ����ݵ�ƽ̨
//			UsartPrintf(USART_DEBUG, "Send %d Bytes\r\n", mqttPacket._len);
//			
//			MQTT_DeleteBuffer(&mqttPacket);															//ɾ��
//		}
//		else
//			UsartPrintf(USART_DEBUG, "WARN:	EDP_NewBuffer Failed\r\n");
//	}
//	
//}

//==========================================================
//	�������ƣ�	HA_Subscribe
//
//	�������ܣ�	����
//
//	��ڲ�����	topics�����ĵ�topic
//				topic_cnt��topic����
//
//	���ز�����	SEND_TYPE_OK-�ɹ�	SEND_TYPE_SUBSCRIBE-��Ҫ�ط�
//
//	˵����		
//==========================================================
void HA_Subscribe(const char *topics[], unsigned char topic_cnt)
{
	
	unsigned char i = 0;
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};							//Э���
	
	for(; i < topic_cnt; i++)
		UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topics[i]);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL2, topics, topic_cnt, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);					//��ƽ̨���Ͷ�������
		
		MQTT_DeleteBuffer(&mqttPacket);											//ɾ��
	}

}


//==========================================================
//	�������ƣ�	HA_Publish
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
void HA_Publish(const char *topic, const char *msg)
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
//	�������ƣ�	HA_Subscribe
//
//	�������ܣ�	����
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================
//void HA_Subscribe(void)
//{
//	
//	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//Э���
//	
//	char topic_buf[56];
//	const char *topic = topic_buf;
//	
//	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/cmd/#", PROID, DEVICE_NAME);
//	
//	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);
//	
//	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
//	{
//		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//��ƽ̨���Ͷ�������
//		
//		MQTT_DeleteBuffer(&mqtt_packet);										//ɾ��
//	}

//}

//==========================================================
//	�������ƣ�	HA_RevPro
//
//	�������ܣ�	ƽ̨�������ݼ��
//
//	��ڲ�����	dataPtr��ƽ̨���ص�����
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void HA_RevPro(unsigned char *cmd)
{
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short topic_len = 0;
	unsigned short req_len = 0;
	
	unsigned char qos = 0;
	static unsigned short pkt_id = 0;
	
	unsigned char type = 0;
	
	short result = 0;

	char *dataPtr = NULL;
	char numBuf[10];
	int num = 0;
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_PUBLISH:																//���յ�Publish��Ϣ
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				char *data_ptr = NULL;
				
				UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
																	cmdid_topic, topic_len, req_payload, req_len);
				
				data_ptr = strstr(cmdid_topic, "request/");									//����cmdid
				if(data_ptr)
				{
					char topic_buf[80], cmdid[40];
					
					data_ptr = strchr(data_ptr, '/');
					data_ptr++;
					
					memcpy(cmdid, data_ptr, 36);											//����cmdid
					cmdid[36] = 0;
					
					snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/cmd/response/%s",
															PROID, DEVICE_NAME, cmdid);
					HA_Publish(topic_buf, "ojbk");										//�ظ�����
				}
			}
			
		case MQTT_PKT_PUBACK:														//����Publish��Ϣ��ƽ̨�ظ���Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");
			
		break;
		
		case MQTT_PKT_SUBACK:																//����Subscribe��Ϣ��Ack
		
			if(MQTT_UnPacketSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe Err\r\n");
		
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//��ջ���
	
	if(result == -1)
		return;
	
	dataPtr = strchr(req_payload, ':');					//����':'

	if(dataPtr != NULL && result != -1)					//����ҵ���
	{
		dataPtr++;
		
		while(*dataPtr >= '0' && *dataPtr <= '9')		//�ж��Ƿ����·��������������
		{
			numBuf[num++] = *dataPtr++;
		}
		numBuf[num] = 0;
		
		num = atoi((const char *)numBuf);				//תΪ��ֵ��ʽ
	}

	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}

//==========================================================
//	�������ƣ�	HA_Device_registration
//
//	�������ܣ�	��HAע���豸��ʵ��
//
//	��ڲ�����	Device_name: ע���豸��  Entity_name��ע��ʵ����  Type_name��ʵ������  Entity_ID��ʵ����
//
//	���ز�����	homeassistant_state_topic�������ϴ��Ļ���
//
//	˵����		
//==========================================================

char *HA_Device_registration(struct Dev_info_Tpye dev_info)
{
    cJSON *root = cJSON_CreateObject();
    char unique_id[48];
    char Entity_name[48];
    char state_topic[48];
    char json_attributes_topic[48];
    char identifiers[48];
    char Device_name[48];
    char homeassistnat_config_topic[128];
    char *pubtopic = malloc(sizeof(char) * 48);;

    
    //����ע����Ϣ
    sprintf(unique_id, "HA-%s-%s", dev_info.Device_name, dev_info.Entity_ID);
    sprintf(Entity_name, "%s", dev_info.Entity_name);
    sprintf(state_topic, "HA-%s/%s/state", dev_info.Device_name, dev_info.Entity_ID);
    sprintf(json_attributes_topic, "HA-%s/%s/attributes", dev_info.Device_name, dev_info.Entity_ID);
    sprintf(identifiers, "%s", dev_info.Device_name);
    sprintf(Device_name, "%s", dev_info.Device_name);
    sprintf(homeassistnat_config_topic, "homeassistant/%s/HA/HA-%s-%s/config", dev_info.Type_name, dev_info.Device_name, dev_info.Entity_ID);
    
    //�����ϴ����ݻ�����
    strcpy(pubtopic, state_topic);
  
    // ��Ӽ�ֵ�Ե�JSON����ʵ����Ϣ
    cJSON_AddStringToObject(root, "unique_id", unique_id);  //ʵ��ΨһID
    cJSON_AddStringToObject(root, "name", Entity_name);     //ʵ����
    cJSON_AddStringToObject(root, "state_topic", state_topic);      //�ϴ����ݻ�����
    cJSON_AddStringToObject(root, "json_attributes_topic", json_attributes_topic);
    
    //��������ʵ��ע��׷����
    if(dev_info.Type_name == "switch")
    {
        char homeassistant_control_topic[48];   
        
        sprintf(homeassistant_control_topic, "HA-%s/%s/set", dev_info.Device_name, dev_info.Entity_ID);
        
        //������Ϣ����
        cJSON_AddStringToObject(root, "command_topic", homeassistant_control_topic);    
        
        //���Ľ��ջ���������ת��
        const char* tmpsub[] = {homeassistant_control_topic};
    
        //���Ļ���
        HA_Subscribe(tmpsub, 1);
    }

    // ����һ��Ƕ�׵�JSON�����豸��Ϣ
    cJSON *device_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(device_obj, "identifiers", identifiers);
    cJSON_AddStringToObject(device_obj, "manufacturer", "Rensiming");
    cJSON_AddStringToObject(device_obj, "model", "HomeAssistant");
    cJSON_AddStringToObject(device_obj, "name", Device_name);
    cJSON_AddItemToObject(root, "device", device_obj);

    // ��JSON����ת��Ϊ�ַ���
    char *json_str = cJSON_Print(root);
    
    //����ע������
    HA_Publish(homeassistnat_config_topic, json_str);
    
    //�����ϴ����ݻ�����
    return pubtopic;
    
}

