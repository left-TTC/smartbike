#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
//#include "WEBANDGPS.h"
#include "string.h"
#include "AD.h"
#include "BLUETOOTH.h"
#include "CONTROLLER.h"
#include "BATTERY.h"

uint16_t ADValue;
float Voltage;
volatile int check_tooth = 0;
uint8_t Noblue_dirve = 0;
extern int BikeLock_number;
extern int BatteryLock_number;
extern char Signal_String[10];
extern char fault_string[10];
extern int Tooth_Flag;
extern int Site_move;

int main(void)
{
	OLED_Init();
	Serial_Init();
	AD_Init();
	//Serial_Init2();
	OLED_Init();
	//AT_Reset();
	//AT_Init();
	Blue_Init();
	Battery_Init();
	Controller_Init();
	//GPS_open();                                //delay
	
	uint32_t whilecount = 0;
	uint32_t Batterylockcount = 0;
	int Bikelockcount = 0;
	
	while (1)
	{
//--------------------------while ++-----------------------
		if(whilecount%10==0)
		{
			ADValue = AD_GetValue();
			Voltage = (float)ADValue / 4095 * 3.3;
		
			OLED_ShowNum(1, 9, ADValue, 4);
			OLED_ShowNum(2, 9, Voltage, 1);
			OLED_ShowNum(2, 11, (uint16_t)(Voltage * 100) % 100, 2);
		}
		
		whilecount++;                          //100 =1s
		Delay_ms(10);
		
		if (whilecount%100==0)       //a whilecount == 0.01s   1s
		{
		    //Lock_Do();              //judge whether to issue an unlock command
			Blue_check();           //Determine whether Bluetooth is connected
		}
		
//----------------------BikeLock on And Bluetooth connected----------------------
		if(BikeLock_number == 1 && Tooth_Flag == 0)          
		{
			if(Bikelockcount%10==0)        //logically redundant ,set just at once 
			{
				//DTR_Disable();
				Controller_on();
				Bikelockcount ++;           //when Bikelockcount = 1£¬don't enter the loop
			}
 			if(whilecount%100==0)         
			{
				unLockBikeCommand1();
			}			
			if(whilecount%100==20)
			{
				unLockBikeCommand2();
			}	
			if(whilecount%100==40)
			{
				unLockBikeCommand3();
			}
			if(whilecount%300== 0)                 //3s change the information displayed on the screen
			{
				OLED_ShowString(3,1,fault_string);
				OLED_ShowString(4,1,Signal_String);
			}
			if ( whilecount% 600==0)               //1min = 60s = 60*100  6s
			{
				//GPS_Send();                        
			}
			if (whilecount % 1000 == 0)            //10s check the network connection status
			{
				//MQTT_Check();
				//CSQ_Check();
				Noblue_dirve = 0;                  //reset,prevent the next Bluetooth disconnection and lock car directly without notification
			}
			if (whilecount % 1000 == 20)           //respond to the network situation
			{
				//Check_Netstate();
			}
			if (whilecount % 3000 == 0)       //30s
			{
				//if(Tooth_Flag == 1 && Site_move == 1)     //BuleTooth disconnected and haven't moved for a 2 min
				//{
					//check_tooth ++;
					//if(check_tooth > 5)
					//{
					//	Send_LongTimeNoMove();     //notify
					//	BikeLock_number = 0;       //Bikelock
					//}
				//}
				//else if(Tooth_Flag == 0 || Site_move == 0)  
				//{
				//	check_tooth = 0;
				//}
			}
			if(whilecount % 2000 == 0)
			{
				Get_BatteryLockStateon();
			}
		}
		
//--------------------Bikelock on But Bluetooth is disconnected or not connected-------------

		if(BikeLock_number == 1 && Tooth_Flag == 1)        //have opened the lock but BlueTooth is not connected
		{
			if(Bikelockcount%10==0)        //logically redundant ,set just at once 
			{
				//DTR_Disable();
				Controller_on();
				Bikelockcount ++;          //disconnect at the befinning:open;disconnect when driving:pin has been setten
			}
			if(whilecount%100==0)
			{
				unLockBikeCommand1();
			}			
			if(whilecount%100==20)
			{
				unLockBikeCommand2();
			}	
			if(whilecount%100==40)
			{
				unLockBikeCommand3();
			}
			if(whilecount%300== 0)
			{
				//OLED_ShowString(3,1,fault_string);
				//OLED_ShowString(4,1,Signal_String);
			}
			if ( whilecount% 600==0)               //1min = 60s = 60*100  6s
			{
				//GPS_Send();                        
			}
			if (whilecount % 1000 == 0)
			{
				//MQTT_Check();
				//CSQ_Check();
			}
			if (whilecount % 1000 == 20)
			{
				//Check_Netstate();
			}
			if(whilecount % 500 == 0)
			{
				Noblue_dirve ++;
				if(Noblue_dirve == 1)
				{
					//Send_NoBluetooth();              //issue a warning in the fifth second
				}
				else if (Noblue_dirve > 5)
				{
					BikeLock_number = 0;
					Noblue_dirve = 0;
				}
			}
			if(whilecount % 2000 == 0)
			{
				Get_BatteryLockStateon();
			}
		}
		
//---------------------------Bikelock off-------------------------	
		if(BikeLock_number == 0)                  //lock
		{
			if(Bikelockcount%10==1)        //logically redundant ,set just at once 
			{
				Controller_off();
				Bikelockcount = 0;
			}
			if(whilecount% 1000==0)            //10s
			{
				if(Voltage > 1)
				{
					//Send_IllegalMove();
				}					
			}
			if(whilecount% 1500==0)            //10s
			{
				//Get_BatteryLockStateoff();					
			}
			if(whilecount% 30000==0)            //1min = 60s = 60*100  1000ms =1s  5min
			{
				//DTR_Disable();                      	
			}
			if(whilecount% 30000==1000)            //1min = 60s = 60*100  1000ms =1s  5min
			{
				//GPS_Send();                       	
			}
			if(whilecount% 30000==1100)            //1min = 60s = 60*100  1000ms =1s  5min
			{
				//XDTR_Enable();                       	
			}
			
		}
//-------------------------Battery command----------------------------
		if(BatteryLock_number == 1)             //let Batterylock on
		{
			Batterylockcount ++;
			if(Batterylockcount%101==1)         //when Batterylockcount =1,102
			{
				BatteryLock_on();
			}
			if(Batterylockcount%101==0)         //delay ~1s
			{
				BatteryLock_Reset();
				BatteryLock_number = 2;
				Batterylockcount = 0;            //Batterylockcount(max) == 101
				Battery_openNotify();            //send notifications to Bluetooth
			}
		}
		
		if(BatteryLock_number == 0)             //let Batterylock off
		{
			Batterylockcount ++;
			if(Batterylockcount%101==1) 
			{
				BatteryLock_off();
			}
			if(Batterylockcount%101==0)           //delay ~1s
			{
				BatteryLock_Reset();
				BatteryLock_number = 2;
				Batterylockcount = 0;
				Battery_offNotify();
			}
		}
		
	}
}


