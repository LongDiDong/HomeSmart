#ifndef _HA_H_
#define _HA_H_


struct Dev_info_Tpye
{
    char Device_name[48];
    char Entity_name[48];
    char Type_name[48]; 
    char Entity_ID[24];
    
};



_Bool HA_RegisterDevice(void);

_Bool HA_DevLink(void);

//void HA_SendData(void);

//void HA_Subscribe(void);

void HA_Subscribe(const char *topics[], unsigned char topic_cnt);

void HA_Publish(const char *topic, const char *msg);

void HA_RevPro(unsigned char *cmd);

unsigned char HA_FillBuf(char *buf);

char *HA_Device_registration(struct Dev_info_Tpye dev_info);
//const char *Device_name, const char *Entity_name, const char *Type_name ,int Device_ID, int Entity_ID, 
#endif
