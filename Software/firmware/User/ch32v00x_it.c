/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32v00x_it.c
 * Author             : V0giK
 * Version            : V1.2.0
 * Date               : 2026/03/17
 * Description        : Main Interrupt Service Routines.
 *********************************************************************************
 * Copyright (c) 2026 V0giK
 *******************************************************************************/
#include <ch32v00x_it.h>

extern volatile uint8_t pwm_duty;
extern volatile uint16_t rc_pulse_us;
extern volatile uint8_t rc_new_data;
extern volatile uint16_t rc_start_time;

static uint8_t pwm_counter = 0;

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void NMI_Handler(void)
{
    while (1)
    {
    }
}

void HardFault_Handler(void)
{
    NVIC_SystemReset();
    while (1)
    {
    }
}

void EXTI7_0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line4);

        if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4))
        {
            rc_start_time = TIM1->CNT;
        }
        else
        {
            uint16_t rc_end_time = TIM1->CNT;
            uint16_t diff;

            if (rc_end_time >= rc_start_time)
            {
                diff = rc_end_time - rc_start_time;
            }
            else
            {
                diff = (0xFFFF - rc_start_time) + rc_end_time + 1;
            }

            if (diff >= 800 && diff <= 2500)
            {
                rc_pulse_us = diff;
                rc_new_data = 1;
            }
        }
    }
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        if (pwm_counter < pwm_duty)
        {
            GPIO_SetBits(GPIOC, GPIO_Pin_2);
        }
        else
        {
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);
        }

        pwm_counter++;
        if (pwm_counter >= 100)
        {
            pwm_counter = 0;
        }
    }
}
