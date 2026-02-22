/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32v00x_it.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/25
 * Description        : Main Interrupt Service Routines.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 *******************************************************************************/
#include <ch32v00x_it.h>

#define RC_MIN_US   1000
#define RC_MAX_US   2000

extern volatile uint16_t rc_pulse_us;
extern volatile uint8_t rc_valid;

static uint16_t rc_capture_old = 0;

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_CC_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  none
 */
void HardFault_Handler(void)
{
  NVIC_SystemReset();
  while (1)
  {
  }
}

/*********************************************************************
 * @fn      TIM1_CC_IRQHandler
 *
 * @brief   TIM1 Capture Compare IRQ - RC Input Capture
 *
 * @return  none
 */
void TIM1_CC_IRQHandler(void)
{
    uint16_t capture;
    uint16_t diff;

    if (TIM_GetITStatus(TIM1, TIM_IT_CC4) != RESET)
    {
        capture = TIM_GetCapture4(TIM1);

        if (rc_capture_old != 0)
        {
            if (capture >= rc_capture_old)
            {
                diff = capture - rc_capture_old;
            }
            else
            {
                diff = (0xFFFF - rc_capture_old) + capture + 1;
            }

            if (diff >= RC_MIN_US && diff <= (RC_MAX_US + 500))
            {
                rc_pulse_us = diff;
                rc_valid = 1;
            }
        }

        rc_capture_old = capture;
        TIM_ClearITPendingBit(TIM1, TIM_IT_CC4);
    }
}


