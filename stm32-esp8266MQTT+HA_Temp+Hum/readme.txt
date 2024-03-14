1.修改esp8266.c下的wifi账号及密码

2.修改ha.c下的PROID、ACCESS_KEY_key和DEVICE_NAME
PROID为MQTT服务端添加的用户名
ACCESS_KEY_key为密码
DEVICE_NAME为设备名
若MQTT服务端未开启客户端认证则不用修改

3.板子：STM32F103ZE核心板+esp8266WiFi模块 

主要参考视频：
B站：
世界不大我姓张：基于STM32的DHT11温湿度模块代码详解
https://www.bilibili.com/video/BV1We4y197pc/?spm_id_from=333.880.my_history.page.click&vd_source=b567447237dcfa732a3045ea99c6ec3d
画卿师兄：【挽救小白第一季】STM32+8266+小程序智能家居毕设实战
https://www.bilibili.com/video/BV1ae411W7yD/?p=9&spm_id_from=333.880.my_history.page.click&vd_source=b567447237dcfa732a3045ea99c6ec3d

王铭东：Micropython+ESP32+HomeAssistant 智能家居系 源码+教程
https://www.bilibili.com/video/BV1gM411v7u7/?p=12&spm_id_from=333.880.my_history.page.click&vd_source=b567447237dcfa732a3045ea99c6ec3d

壹知半解：小白物联网智能家居毕设参考STM32+ESP8266/MQTT+OneNet+UniApp
https://www.bilibili.com/video/BV1HH4y1o7EN/?p=13&spm_id_from=333.788.top_right_bar_window_history.content.click&vd_source=b567447237dcfa732a3045ea99c6ec3d

