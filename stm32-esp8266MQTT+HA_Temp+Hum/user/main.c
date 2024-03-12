/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	main.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.0
	*
	*	说明： 		接入ha，上传数据和命令控制
	*
	*	修改记录：	
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
#include "stm32f10x.h"

//网络协议层
#include "ha.h"

//网络设备
#include "esp8266.h"

//硬件驱动
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "dht11.h"

//C库
#include <string.h>
#include <stdio.h>
#include "cJSON.h"



#define ESP8266_ONENET_INFO		"AT+CIPSTART=\"TCP\",\"pytgwf.com\",1883\r\n"

struct Dev_info_Tpye ESP8266_EVM_Entity_01, ESP8266_EVM_Entity_02;

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
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//中断控制器分组设置

	Delay_Init();									//systick初始化
	
	Usart1_Init(115200);							//串口1，打印信息用
	
	Usart2_Init(115200);							//串口2，驱动ESP8266用
	
	Led_Init();									//蜂鸣器初始化
	
	UsartPrintf(USART_DEBUG, " Hardware init OK\r\n");
	
}

/*
************************************************************
*	函数名称：	main
*
*	函数功能：	
*
*	入口参数：	无
*
*	返回参数：	0
*
*	说明：		
************************************************************
*/
int main(void)
{
    
	unsigned short timeCount = 0;	//发送间隔变量
    
    char SendData_Buffer[20];       //数据发送缓存
    
    uint8_t DHT_Buffer[5];          //温湿度数据缓存
	
	Hardware_Init();				//初始化外围硬件
	
	ESP8266_Init();					//初始化ESP8266
    
    
    //温度检测实体
    strcpy(ESP8266_EVM_Entity_01.Device_name, "ESP8266_EVM");
    strcpy(ESP8266_EVM_Entity_01.Entity_name, "Temp");
    strcpy(ESP8266_EVM_Entity_01.Type_name, "sensor");
    strcpy(ESP8266_EVM_Entity_01.Entity_ID, "1");
    
    //湿度检测实体
    strcpy(ESP8266_EVM_Entity_02.Device_name, "ESP8266_EVM");
    strcpy(ESP8266_EVM_Entity_02.Entity_name, "Hum");
    strcpy(ESP8266_EVM_Entity_02.Type_name, "sensor");
    strcpy(ESP8266_EVM_Entity_02.Entity_ID, "2");
    
    //与服务器建立tcp连接
	UsartPrintf(USART_DEBUG, "Connect MQTTs Server...\r\n");
	while(ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT"))
        DelayXms(500);
    //接入Mqtt服务器，带账户密码
	while(HA_DevLink())
		DelayXms(500);
    UsartPrintf(USART_DEBUG, "Connect MQTTs Success\r\n");
    
    //向HA服务器注册温度检测实体
	char *dev_01_pubtopic = HA_Device_registration(ESP8266_EVM_Entity_01);
     
    //向HA服务器注册湿度检测实体
    char *dev_02_pubtopic = HA_Device_registration(ESP8266_EVM_Entity_02);
    
    //led闪烁提示接入成功
	Led_Set(LED_ON);				
	DelayMs(3000);
	Led_Set(LED_OFF);
    
	while(1)
	{		
		//发送间隔5s
        if(++timeCount >= 500)									
		{
			UsartPrintf(USART_DEBUG, "HA_SendData\r\n");
            
            //取出并发送数据
            if(DHT_Get_Temp_Humi_Data(DHT_Buffer))
            {   
                //取出温度数据
                sprintf(SendData_Buffer,"%d.%d",DHT_Buffer[2],DHT_Buffer[3]);
                //发送温度数据
                HA_Publish(dev_01_pubtopic, SendData_Buffer);
                
                //取出湿度数据
                sprintf(SendData_Buffer,"%d.%d",DHT_Buffer[0],DHT_Buffer[1]);
                //发送湿度数据
                HA_Publish(dev_02_pubtopic, SendData_Buffer);
            }
            
            else    UsartPrintf(USART_DEBUG, "NO Data\r\n");
            
			timeCount = 0;
            
			ESP8266_Clear();
		}
        
		DelayXms(10);
	
	}

}
