/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : V0giK
 * Version            : V1.1.0
 * Date               : 2026/03/10
 * Description        : RC-Signal to AL8862 PWM converter (Software-PWM)
 *********************************************************************************
 * Copyright (c) 2026 V0giK
 *******************************************************************************/

#include "debug.h"

#define RCC_GPIO_ENABLE  (RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD)

#define RC_INPUT_PIN    GPIO_Pin_4
#define RC_INPUT_PORT   GPIOC
#define RC_INPUT_SOURCE GPIO_PinSource4

#define PWM_OUTPUT_PIN  GPIO_Pin_2
#define PWM_OUTPUT_PORT GPIOC
#define PWM_OUTPUT_SOURCE GPIO_PinSource2

#define MODE_JUMPER_PIN     GPIO_Pin_1
#define MODE_JUMPER_PORT   GPIOC

#define FAILSAFE_JUMPER_PIN    GPIO_Pin_4
#define FAILSAFE_JUMPER_PORT   GPIOD

#define RC_MIN_US           1000
#define RC_MAX_US           2000
#define RC_THRESHOLD_US     1500
#define RC_TIMEOUT_MS       50
#define PWM_FREQUENCY_HZ    1000
#define LOOP_TICK_MS        10
#define PWM_TICK_US         100

#define VERSION_MAJOR       1
#define VERSION_MINOR       1
#define VERSION_PATCH      0
#define VERSION_STRING      "v1.1.0"

volatile uint16_t rc_pulse_us = 0;
volatile uint8_t rc_valid = 0;
volatile uint8_t pwm_duty_set = 0;
volatile uint16_t pwm_counter = 0;
volatile uint16_t pwm_period = 10000 / PWM_TICK_US;

void GPIO_Init_Custom(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_GPIO_ENABLE, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = RC_INPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
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

void TIM1_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = 2400 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 20 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_BothEdge;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x03;
    TIM_ICInit(TIM1, &TIM_ICInitStructure);

    TIM_ITConfig(TIM1, TIM_IT_CC4, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM1, ENABLE);
}

void TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = (48000000 / 8 / (1000000 / PWM_TICK_US)) - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 8 - 1;
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

void Set_PWM_Duty(uint8_t duty_percent)
{
    pwm_duty_set = duty_percent;
}

int main(void)
{
    uint8_t linear_mode = 0;
    uint8_t failsafe_high = 0;
    uint8_t pwm_duty = 0;
    uint8_t signal_timeout = 0;
    uint8_t no_signal_counter = 0;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    GPIO_Init_Custom();
    TIM1_Init();
    TIM2_Init();

    linear_mode = !GPIO_ReadInputDataBit(MODE_JUMPER_PORT, MODE_JUMPER_PIN);
    failsafe_high = !GPIO_ReadInputDataBit(FAILSAFE_JUMPER_PORT, FAILSAFE_JUMPER_PIN);

    Set_PWM_Duty(0);

    while (1)
    {
        Delay_Ms(LOOP_TICK_MS);

        if (rc_valid)
        {
            no_signal_counter = 0;
            signal_timeout = 0;

            uint16_t rc_pulse = rc_pulse_us;
            rc_valid = 0;

            if (linear_mode)
            {
                if (rc_pulse <= RC_MIN_US)
                {
                    pwm_duty = 0;
                }
                else if (rc_pulse >= RC_MAX_US)
                {
                    pwm_duty = 100;
                }
                else
                {
                    pwm_duty = (uint8_t)(((rc_pulse - RC_MIN_US) * 100) / (RC_MAX_US - RC_MIN_US));
                }
            }
            else
            {
                pwm_duty = (rc_pulse >= RC_THRESHOLD_US) ? 100 : 0;
            }
        }
        else
        {
            no_signal_counter++;
            if (no_signal_counter >= (RC_TIMEOUT_MS / LOOP_TICK_MS))
            {
                signal_timeout = 1;
            }

            if (signal_timeout)
            {
                if (failsafe_high)
                {
                    pwm_duty = 100;
                }
                else
                {
                    pwm_duty = 0;
                }
            }
        }

        Set_PWM_Duty(pwm_duty);
    }
}
