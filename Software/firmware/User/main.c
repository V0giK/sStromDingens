/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/25
 * Description        : RC-Signal to AL8862 PWM converter
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
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
#define PWM_FREQUENCY_HZ    200
#define LOOP_TICK_MS        10

#define VERSION_MAJOR       1
#define VERSION_MINOR       0
#define VERSION_PATCH      0
#define VERSION_STRING      "v1.0.0"

volatile uint16_t rc_pulse_us = 0;
volatile uint8_t rc_valid = 0;

void GPIO_Init_Custom(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_GPIO_ENABLE, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = RC_INPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(RC_INPUT_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PWM_OUTPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
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
    TIM_OCInitTypeDef TIM_OCInitStructure;
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

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);

    GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, ENABLE);

    TIM_ITConfig(TIM1, TIM_IT_CC4, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM1, ENABLE);
}

void Set_PWM_Duty(uint8_t duty_percent)
{
    uint32_t period = TIM1->ATRLR + 1;
    uint32_t pulse = (period * duty_percent) / 100;
    TIM_SetCompare2(TIM1, pulse);
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

    while (1)
    {
        Delay_Ms(LOOP_TICK_MS);

        if (rc_valid)
        {
            no_signal_counter = 0;
            signal_timeout = 0;

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
                    pwm_duty = (uint8_t)(((rc_pulse_us - RC_MIN_US) * 100) / (RC_MAX_US - RC_MIN_US));
                }
            }
            else
            {
                pwm_duty = (rc_pulse_us >= RC_THRESHOLD_US) ? 100 : 0;
            }
            rc_valid = 0;
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
