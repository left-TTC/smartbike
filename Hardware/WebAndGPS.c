#include "stm32f10x.h"                  // Device header
#include <string.h>
#include "DELAY.h"
#include "OLED.h"
#include <stdlib.h> 
#include <stdio.h>

#define BUFFER_SIZE 128         //4Gbuffer
#define BUFFER_SIZE2 128        //GPSbuffer
#define BUFFER_SIZE1 32        //Init judge buffer

volatile uint8_t netstate[5] = {0,0,0,0,0};
volatile uint8_t OK_Flag = 0;
volatile uint8_t SUCCESS_Flag =0;
volatile uint8_t Register_Flag = 0;
char Signal_String[15] = "       ";
char fault_string[10] = "       ";

//--------------------------------------------------Init--------------------------
void Serial_Init2(void)         //USART2
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;         //TX
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;    
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;         //RX
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;              //used to control the hibernation mode
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;              //used to control the hibernation mode
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_6);                       //ENpin high-level enable
	
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &USART_InitStructure);
	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure); 
	
	USART_Cmd(USART2, ENABLE);
	
}

//----------------------------------------------Send function-------------
void Send_Command(const char* command)   
{
	 while (*command)
	 {
		 USART_SendData(USART2,*command);
		 while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
		 command ++;	 
	 }
	 
	 USART_SendData(USART2, '\r');
     while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    
     USART_SendData(USART2, '\n');
     while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	 	 
}

void CreateAndSendCommand(char *input)          
{
	char command[100];
	strcpy(command, "AT+MPUB=\"HH\",0,0,\"");    //"AT+MPUB=\"HH\",0,0,\"Connection successful\"";
	strcat(command, input);
	strcat(command, "\"");
	
	Send_Command(command);
}
//------------------------------------main send function-----------------------------
void Send_LongTimeNoMove(void)             //if the user disconnects Bluetooth for a long time
{
	CreateAndSendCommand("you haven't moved for a long time,we have turned off the lock for you");
}

void Send_NoBluetooth(void)
{
	CreateAndSendCommand("BlueTooth is not connected or has been disconnected.cut off after 20s");
}

void Send_IllegalMove(void)
{
	CreateAndSendCommand("Warning! illegal movement of vehicles");
}

void Send_BatteryunlockNotify(void)
{
	CreateAndSendCommand("your Battery warehouse is not closed,please pay attention");
}

//-------------------------------------serve init------------------------------------
void AT_Reset(void)          //restart
{
	const char* at_Reset = "AT+RESET"; 
	Send_Command(at_Reset);
	int retryCount = 0;
	Delay_s(5);
	OLED_Clear();
	
	while(OK_Flag == 0 || Register_Flag == 0)      //Try once at most
	{
		Send_Command(at_Reset);         //[15:41:29.528] [15:41:34.084]
		Delay_s(5);
		retryCount++;
		if (retryCount > 1)
		{
			OLED_ShowString(1,1,"SIM UNFOUND");
		}
	}
	Register_Flag = 0;
	OK_Flag = 0;
}

void AT_DTRStart(void)          //Enable slow clock,premise of turning on the hibernation mode
{
	const char* at_SleepENABLE = "AT+CSCLK=1"; 
	Send_Command(at_SleepENABLE);
	int retryCount = 0;
	Delay_ms(10);
	OLED_Clear();
	
	while(OK_Flag == 0 )     
	{
		Send_Command(at_SleepENABLE);         
		Delay_ms(10);
		retryCount++;
		if (retryCount > 20)
		{
			OLED_ShowString(1,1,"SLEPP BAD  ");
		}                      //SLEPP DISA 
	}   
	OK_Flag = 0;
}

void AT_Distrubute(void)     //APN
{
	const char* at_Distrubute = "AT+QICSGP=1,1,\"\",\"\",\"\""; 
	Send_Command(at_Distrubute);
	
	int retryCount = 0;
	Delay_ms(10);            //[15:03:38.533]send  [15:03:38.540]receive
	while(OK_Flag == 0)      
	{
		Send_Command(at_Distrubute);
		Delay_ms(10);
		retryCount ++;
		if (retryCount > 19)
		{
			OLED_ShowString(1,1,"APN404");
		}
	}
	
	OK_Flag = 0;
}

void AT_NETOPEN(void)       //open net
{
	const char* at_OpenNet = "AT+NETOPEN";
	Send_Command(at_OpenNet);
	int retryCount = 0;
	OLED_Clear();
	Delay_s(4);                  //[15:03:48.564]  [15:03:52.460]
	
	while(OK_Flag == 0 || SUCCESS_Flag == 0 || Register_Flag == 0)      
	{
		Send_Command(at_OpenNet);
		Delay_s(4);
		retryCount ++;
		if (retryCount > 2)
		{
			OLED_ShowString(1,1,"NET404");
		}
	}
	
	OK_Flag = 0;
	SUCCESS_Flag = 0;
	Register_Flag = 0;

}

void AT_ClienID(void)       
{
	const char* at_ClientID = "AT+MCONFIG=\"STM32_TEST\"";
	Send_Command(at_ClientID);
	int retryCount = 0;
	OLED_Clear();
	Delay_ms(10);             //[15:08:16.950]  [15:08:16.956]
	
	while(OK_Flag == 0 )      
	{
		Send_Command(at_ClientID);
		Delay_ms(10);
		retryCount ++;
		if (retryCount > 20)
		{
			OLED_ShowString(1,1,"ID404");
		}
	}
	
	OK_Flag = 0;
}

void AT_MQTTMesseage(void)      
{
	const char* at_MQTTS = "AT+MIPSTART=\"broker.emqx.io\",1883";
	Send_Command(at_MQTTS);
	int retryCount = 0;
	OLED_Clear();
	Delay_ms(600);          //[15:08:16.956]     [15:08:16.505]
		
	while(OK_Flag == 0 || SUCCESS_Flag == 0)    
	{
		Send_Command(at_MQTTS);
		Delay_ms(600);
		retryCount ++;
		if (retryCount > 3)
		{
			OLED_ShowString(1,1,"MQTTID404");
		}
	}
	
	OK_Flag = 0;
	SUCCESS_Flag = 0;
}

void AT_MQTTConnect(void)       
{
	const char* at_MQTTC = "AT+MCONNECT=1,60";
	Send_Command(at_MQTTC);
	int retryCount = 0;
	OLED_Clear();
	Delay_ms(1000);  //[15:10:46.185]     [15:10:47.130]
	
	while(OK_Flag == 0 || SUCCESS_Flag ==0)   
	{
		Send_Command(at_MQTTC);
		Delay_ms(1000);
		retryCount ++;
		if (retryCount > 3)
		{
			OLED_ShowString(1,1,"MQTTID404");
		}
	}
	
	OK_Flag = 0;
	SUCCESS_Flag = 0;
}

void AT_TopicOut(void)       
{
	const char* at_TopicOutC = "AT+MSUB=\"phone\",0";
	Send_Command(at_TopicOutC);
	int retryCount = 0;
	OLED_Clear();
	Delay_ms(450);     //[15:12:51.043]      [15:12:51.490]
	
	while(OK_Flag == 0 || SUCCESS_Flag == 0)                       
	{
		Send_Command(at_TopicOutC);
		Delay_ms(450);
		retryCount ++;
		if (retryCount > 3)
		{
			OLED_ShowString(1,1,"MQTTSUB404");
		}
	}
	
	OK_Flag = 0;
	SUCCESS_Flag = 0;
	OLED_Clear();
	
	char Ensure[15];
	
	strcpy(Ensure,"LASTLY SUCCESSFUL");
	OLED_Clear();
	OLED_ShowString(1,1,"ALRIGHT");
	CreateAndSendCommand(Ensure);

	Delay_s(2);                                
	OLED_Clear();                             
}

void AT_Init(void)           
{
	AT_Distrubute();  
	AT_NETOPEN();
	AT_ClienID();
	AT_MQTTMesseage();
	AT_MQTTConnect();
	AT_TopicOut();
	AT_DTRStart();
	
	OLED_ShowString(1, 1, "ADValue:");
	OLED_ShowString(2, 1, "Volatge:0.00V");    
}

//------------------------------------------main use---------------------------------

char rxBuffer[BUFFER_SIZE];
char Lock_data[BUFFER_SIZE];
char web_Size[BUFFER_SIZE1];
volatile uint16_t bufferIndex = 0;
char gps_data[BUFFER_SIZE2];
int OK_Number = 0; 
int BikeLock_number = 0;
int BatteryLock_number = 2;          //standby status


void Lock_Do(void)
{
	char Lock1[10];
	char Lock2[10];
	strcpy(Lock1,"Lock Open");
	strcpy(Lock2,"Lock Off");
                                                 //0x20000018 rxBuffer[] "+MSUB: "phone", 2 bytes,"on""	
	if (strstr(Lock_data, "\"battery on\"") != NULL)     
	{
		memset(Lock_data, 0, BUFFER_SIZE);
		BatteryLock_number = 1;        
	}
	if (strstr(Lock_data, "\"battery off\"") != NULL)     
	{
		memset(Lock_data, 0, BUFFER_SIZE);
		BatteryLock_number = 0;
	}
	else if (strstr(Lock_data, "\"on\"") != NULL)     
	{
		CreateAndSendCommand(Lock1);
		memset(Lock_data, 0, BUFFER_SIZE);
		BikeLock_number = 1;
	}
	else if (strstr(Lock_data, "\"off\"") != NULL)    
	{
		CreateAndSendCommand(Lock2);
		memset(Lock_data, 0, BUFFER_SIZE);
		BikeLock_number = 0;
	}
}

void DTR_Enable(void)                 //used to turn on the hibernation mode
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_7);
	GPIO_SetBits(GPIOA,GPIO_Pin_7);   
}

void DTR_Disable(void)                //used to turn off the hibernation mode
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_7);
}

//----------------------------------------receive Latitude and longtitude---------------------
char Test_Latitude[15] = "Latitude";
char Test_Longtitude[15] = "Longtitude";
volatile int Site_move = 0;           //if the position has not been moved for a long time ,the paramter is 1

void GPS_open(void)
{
	const char* GPS_OpenCommand = "AT+MGPSC=1";
	Send_Command(GPS_OpenCommand);
//	Delay_s(30);                                 //30+60=1.5 min
}


void GPS_min(char A[2][14],char Num[2][20])  //A is the unconverted longtitude and latitude
{
	double n1 = atof(A[0]) / 60.0;     //39.011160
	double n2 = atof(A[1]) / 60.0;	
	char buffer[2][20];
	sprintf(buffer[0], "%.6f", n1);
	sprintf(buffer[1], "%.6f", n2);      //modify Num to the real longtitude and latitude
	strcpy(Num[0],buffer[0]+2);
	strcpy(Num[1],buffer[1]+2);
}

void GPS_FEN(char L[2][11],char K[2][20])                           //Unit conversion
{
	char a1[14];
	char a2[3];
	char b1[14];
	char b2[3];
	
	strncpy(a1, L[0], 9);       //latitude
    a1[9] = '\0'; 
	strncpy(a2, L[0]+9, 2);     //N
    a2[2] = '\0';
	strncpy(b1, L[1], 9);   
    b1[9] = '\0'; 
	strncpy(b2, L[1]+9, 2);     //E
    b2[2] = '\0'; 
	
	char NUM[2][20];
	char A[2][14];
	strcpy(A[0],a1);            //latitude
	strcpy(A[1],b1);
	
	GPS_min(A,NUM);             
	
	strcat(NUM[0],a2);
	strcat(NUM[1],b2);
	strcpy(K[0],NUM[0]);
	strcpy(K[1],NUM[1]);
}

void GPS_GetReal(char dai[2][20],char REAL[2][20])            
{
	char Lo1[3];
	char Lo2[12];
	char La1[4];
	char La2[12];
	
	strncpy(Lo1, dai[1], 2);    //the frist two
    Lo1[2] = '\0';               
    
    strncpy(Lo2, dai[1] + 2, 12);  
    Lo2[11] = '\0';                                      
    
    strncpy(La1, dai[0], 3);    //123
	La1[3] = '\0';
    
    strncpy(La2, dai[0] + 3, 12);       //24.869769,E
    La2[11] = '\0';
	
	char L[2][11];
	strcpy(L[0],Lo2);    
	strcpy(L[1],La2);    
	char K[2][20];       
	
	GPS_FEN(L,K);      
	
	char dot[2] =".";
	
	strcat(Lo1,dot);
	strcat(Lo1,K[0]);    
	strcat(La1,dot);
	strcat(La1,K[1]);    
	
	strcpy(REAL[0],Lo1);  
	REAL[0][12]='\0';
	strcpy(REAL[1],La1);	
	REAL[1][13]='\0';
}                                                       //$GNRMC,,V,,,,,,,,,E,N*08
                                                        //$GNRMC,020539.000,A,4138.976244,N,12324.854130,E,0.464,0.00,220924,,E,A*08
//                                                        $GNRMC,080633.000,A,4139.011160,N,12324.869769,E,0.099,351.29,190924,,E,A*01
void GPS_Get (char gps_FristLine[],char result[2][20])  //$GNRMC,080646.000,A,4139.030265,N,12324.860172,E,2.263,355.12,190924,,E,A*0A
{                                                       //0123456789012345678901234567890123456789012345678901234567890123456789012345
	int k;                                   //           0         1         2         3         4         5         6         7   
	int p;  
	char gps_Longitude[20];   
	char gps_Latitude[20];    

	
	for(k=0,p=20;k<13;k++)
	{
		gps_Latitude[k]= gps_FristLine[p+k];
	}
	
	for(k=0,p=34;k<14;k++)
	{
		gps_Longitude[k]= gps_FristLine[p+k];
	}
	gps_Longitude[14] = '\0';
	gps_Latitude[15]= '\0';
	
	char dai[2][20];
	char TRUE[2][20];
	
	strcpy(dai[0],gps_Longitude);    
	strcpy(dai[1],gps_Latitude);     
	
	GPS_GetReal(dai,TRUE);          
	
	strcpy(result[0],TRUE[0]);
	strcpy(result[1],TRUE[1]);     
}

void GPS_Send(void)    
{
	char gps_FristLine[BUFFER_SIZE2];
	
	memset(gps_FristLine, 0, sizeof(gps_FristLine));
	strcpy(gps_FristLine,gps_data);
	char result[2][20];
	
	GPS_Get(gps_FristLine,result);

	//strcpy(result[0],"41.650199,N");
	//strcpy(result[1],"123.414355,E");
	                                               
	char site[50];
	char space[5] = "   ";
	strcpy(site,result[0]);     //Lattitude
	strcat(site,space);         //Longtitude
	strcat(site,result[1]);
	site[26] = '\0';
	
	if(strncmp(Test_Latitude,result[0],7) == 0 && strncmp(Test_Longtitude,result[1],8) == 0)   //shows that position has not changed
	{
		Site_move = 1;
	}
	else  //moved,update the lacation
	{
		Site_move = 0;
		strcpy(Test_Latitude,result[0]);
		strcpy(Test_Longtitude,result[1]);
	}

	CreateAndSendCommand(site);
		
}
//----------------------------------check web--------------------------------------
void MQTT_Check(void)         
{
	const char* Check_Cimmand = "AT+MQTTSTATU";
	Send_Command(Check_Cimmand);
}

void CSQ_Check(void)
{
	const char* Check_Cimmand = "AT+CSQ";
	Send_Command(Check_Cimmand);
}

void CQS_Do(char input[BUFFER_SIZE1])              
{
	char Signal_Number[3];
	Signal_Number[0] = input[6];
	Signal_Number[1] = input[7];
	if (Signal_Number[1] == ',')
	{
		Signal_Number[1] = '\0';
	}
	else
	{
		Signal_Number[2] = '\0';
	}
	
	int Signal_Intensity = atof(Signal_Number);
	
	if(Signal_Intensity == 99 || Signal_Intensity <= 4)
	{
		netstate[2] = 1;           
	}
	else if (Signal_Intensity >= 15 && Signal_Intensity < 99)
	{
		netstate[3] = 1; 
	}
	else if (Signal_Intensity < 15 && Signal_Intensity > 4)
	{
		netstate[4] = 1; 
	}
}

void Check_Netstate(void)
{
	if(netstate[0] == 1 && netstate[2] ==0)                 
	{
		OK_Flag = 0;
		SUCCESS_Flag = 0;
		Register_Flag = 0;
		
		AT_Reset();
		AT_Init();	
		
		strcpy(fault_string,"         ");
		
		netstate[0] = 0;	
	}
	if(netstate[1] == 1)           
	{
		OK_Flag = 0;
		SUCCESS_Flag = 0;
		Register_Flag = 0;
		
		AT_Reset();
		AT_Init();	
		
		strcpy(fault_string,"         ");
		
		netstate[1] = 0;
	}
	if(netstate[2] ==1)
	{
		strcpy(Signal_String,"SIGNAL LOST");  
		netstate[2] = 0;
	}
	if(netstate[3] ==1)
	{
		strcpy(Signal_String,"SIGNAL OK  ");
		netstate[3] = 0;
	}
	if(netstate[4] ==1)
	{
		strcpy(Signal_String,"SIGNAL BAD ");
		netstate[4] = 0;
	}
}
//-------------------------------------------IQ------------------------------------------
void USART2_IRQHandler(void)
{		
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) 
    {
        char receivedData = USART_ReceiveData(USART2);
      
        if (receivedData == '\n' || receivedData == '\r')
        {
            if (bufferIndex > 0)       
            {
                rxBuffer[bufferIndex] = '\0';                      
				
				if(strstr(rxBuffer, "phone") != NULL)                  //receive the commands issued by users            
                {
					memset(Lock_data, 0, BUFFER_SIZE);
					strncpy(Lock_data, rxBuffer, BUFFER_SIZE - 1);
					Lock_data[bufferIndex] = '\0';
				}
				else if (strstr(rxBuffer,"$GNRMC") != NULL)
				{
					memset(gps_data, 0, BUFFER_SIZE2);             
					strncpy(gps_data, rxBuffer, BUFFER_SIZE2 - 1);
					gps_data[bufferIndex] = '\n';
					gps_data[bufferIndex+1] = '\0';                
				}
				//-----------------Init------------------
				else if (strstr(rxBuffer,"OK") != NULL )  
				{
					OK_Number ++;
					OK_Flag = 1;                        //recievedata == "ok"
				}
				else if (strstr(rxBuffer,"SUCCESS") != NULL)           
				{
					SUCCESS_Flag = 1;                     
				}
				else if (strstr(rxBuffer,"+CEREG: 1") != NULL)           
				{
					Register_Flag = 1;                     //reset and  NETopen
				}
				//---------------------web--------------------------
				else if (strstr(rxBuffer,"+MQTT:DISCONNECTED") != NULL || strstr(rxBuffer,"+NETOPEN:FAIL") != NULL)  
				{
					netstate[0] = 1;            //SIM
					strcpy(fault_string,"SIM404   ");
				}
				else if (strstr(rxBuffer,"+MQTTSTATU: 0") != NULL)   //need AT+MQTTSTATU
				{
					netstate[1] = 1;            //mqtt disconnected  
					strcpy(fault_string,"MQTT404  ");
				}
				else if (strstr(rxBuffer,"+CSQ:") != NULL)             //+CSQ: 25,99  need AT+CSQ
				{                                                      //01234567
					memset(web_Size, 0, BUFFER_SIZE1);                 //OK
					strncpy(web_Size, rxBuffer, BUFFER_SIZE1 - 1);     //
					web_Size[bufferIndex] = '\n';
					web_Size[bufferIndex+1] = '\0';
					
					CQS_Do(web_Size);
				}
				memset(rxBuffer, 0, BUFFER_SIZE);
            }
            bufferIndex = 0;           
        }
        else
        {
            if (bufferIndex < BUFFER_SIZE - 1)
            {
                rxBuffer[bufferIndex++] = receivedData;
            }
            else
            {
                bufferIndex = 0; 
            }
        }
    }
}




