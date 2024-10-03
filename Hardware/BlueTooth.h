#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

void Blue_Init(void);
void Blue_Forbid(void);
void Blue_check(void);
extern void USART3_IRQHandler(void);
void Battery_openNotify(void);
void Battery_offNotify(void);

#endif
