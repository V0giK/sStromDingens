/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : V0giK
 * Version            : V1.2.0
 * Date               : 2026/03/17
 * Description        : RC-Signal to AL8862 PWM converter (EXTI + TIM2)
 *********************************************************************************
 * Copyright (c) 2026 V0giK
 *******************************************************************************/

#include "debug.h"
#include "ch32v00x_exti.h"

#define RCC_GPIO_ENABLE  (RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD)

#define RC_INPUT_PIN    GPIO_Pin_4
#define RC_INPUT_PORT   GPIOC

#define PWM_OUTPUT_PIN  GPIO_Pin_2
#define PWM_OUTPUT_PORT GPIOC

#define MODE_JUMPER_PIN     GPIO_Pin_1
#define MODE_JUMPER_PORT   GPIOC

#define FAILSAFE_JUMPER_PIN    GPIO_Pin_4
#define FAILSAFE_JUMPER_PORT   GPIOD

#define RC_MIN_US           1000
#define RC_MAX_US           2000
#define RC_THRESHOLD_US     1500
#define RC_TIMEOUT_TICKS    500

volatile uint8_t pwm_duty = 0;
volatile uint16_t rc_pulse_us = 0;
volatile uint8_t rc_new_data = 0;
volatile uint16_t rc_start_time = 0;

void GPIO_Init_Custom(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_GPIO_ENABLE | RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = RC_INPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(RC_INPUT_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PWM_OUTPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PWM_OUTPUT_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = MODE_JUMPER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(MODE_JUMPER_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = FAILSAFE_JUMPER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(FAILSAFE_JUMPER_PORT, &GPIO_InitStructure);
}

void EXTI_Init_Custom(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);

    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI7_0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void TIM1_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 24 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    TIM_Cmd(TIM1, ENABLE);
}

void TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = 2400 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

int main(void)
{
    uint8_t linear_mode = 0;
    uint8_t failsafe_high = 0;
    uint8_t signal_timeout = 0;
    uint16_t no_signal_counter = 0;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    GPIO_Init_Custom();
    EXTI_Init_Custom();
    TIM1_Init();
    TIM2_Init();

    linear_mode = !GPIO_ReadInputDataBit(MODE_JUMPER_PORT, MODE_JUMPER_PIN);
    failsafe_high = !GPIO_ReadInputDataBit(FAILSAFE_JUMPER_PORT, FAILSAFE_JUMPER_PIN);

    pwm_duty = 0;

    Delay_Ms(1000);

    while (1)
    {
        if (rc_new_data)
        {
            rc_new_data = 0;
            signal_timeout = 0;
            no_signal_counter = 0;

            if (linear_mode)
            {
                if (rc_pulse_us <= RC_MIN_US)
                {
                    pwm_duty = 0;
                }
                else if (rc_pulse_us >= RC_MAX_US)
                {
                    pwm_duty = 100;
                }
                else
                {
                    pwm_duty = (uint8_t)((rc_pulse_us - RC_MIN_US) / 10);
                }
            }
            else
            {
                if (rc_pulse_us >= RC_THRESHOLD_US)
                {
                    pwm_duty = 100;
                }
                else
                {
                    pwm_duty = 0;
                }
            }
        }
        else
        {
            no_signal_counter++;
            if (no_signal_counter >= RC_TIMEOUT_TICKS)
            {
                signal_timeout = 1;
            }
        }

        if (signal_timeout)
        {
            pwm_duty = failsafe_high ? 100 : 0;
        }

        Delay_Us(100);
    }
}
