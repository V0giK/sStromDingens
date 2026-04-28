#ifndef __DEBUG_H
#define __DEBUG_H

#include <ch32v00x.h>

#define SDI_PR_OPEN   1
#define SDI_PR_CLOSE  0

#define SDI_PRINT     SDI_PR_CLOSE

void Delay_Init(void);
void Delay_Us(uint32_t n);
void Delay_Ms(uint32_t n);
void USART_Printf_Init(uint32_t baudrate);
void SDI_Printf_Enable(void);

#endif
