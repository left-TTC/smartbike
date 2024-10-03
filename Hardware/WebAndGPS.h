#ifndef __WEBANDGPS_H
#define __WEBANDGPS_H


void Serial_Init2(void);
void AT_Init(void);
void GPS_open(void);
extern void USART2_IRQHandler(void);
void Lock_Do(void);
void GPS_Send(void);
void AT_Reset(void);
void Check_Netstate(void);
void CSQ_Check(void);
void MQTT_Check(void);
void Send_LongTimeNoMove(void);
void Send_NoBluetooth(void);
void Send_IllegalMove(void);
void Send_BatteryunlockNotify(void);
void DTR_Enable(void);
void DTR_Disable(void);


#endif
