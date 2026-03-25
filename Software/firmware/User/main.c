/*
 * sStromDingens – RC-Signal to PWM Converter
 * Copyright (C) 2026 V0giK
 *
 * This file is part of sStromDingens.
 *
 * sStromDingens is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * sStromDingens is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sStromDingens.  If not, see <https://www.gnu.org/licenses/>.
 *
 * File Name          : main.c
 * Author             : V0giK
 * Version            : V2.0.0
 * Date               : 2026/03/17
 * Description        : RC-Signal to AL8862 PWM converter (EXTI + TIM2)
 */

#include "debug.h"
#include "ch32v00x_exti.h"

/*============================================================================*/
/*                            PIN-DEFINITIONEN                                */
/*============================================================================*/

#define RCC_GPIO_ENABLE  (RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD)

#define RC_INPUT_PIN    GPIO_Pin_4
#define RC_INPUT_PORT   GPIOC

#define PWM_OUTPUT_PIN  GPIO_Pin_2
#define PWM_OUTPUT_PORT GPIOC

#define MODE_JUMPER_PIN     GPIO_Pin_1
#define MODE_JUMPER_PORT   GPIOC

#define FAILSAFE_JUMPER_PIN    GPIO_Pin_6
#define FAILSAFE_JUMPER_PORT   GPIOD

/*============================================================================*/
/*                            PARAMETER-DEFINITIONEN                          */
/*============================================================================*/

/* RC-Signal Grenzen in Mikrosekunden */
#define RC_MIN_US           1000    /* Minimum: 1ms Pulsbreite */
#define RC_MAX_US           2000    /* Maximum: 2ms Pulsbreite */
#define RC_THRESHOLD_US     1500    /* Schwellwert fuer On/Off-Modus */

/* RC-Signal Validierungsgrenzen */
#define RC_PULSE_MIN_VALID   800    /* Minimum fuer gueltigen Puls (Toleranz) */
#define RC_PULSE_MAX_VALID   2500   /* Maximum fuer gueltigen Puls (Toleranz) */

/* Timeout: 500 * 100us = 50ms ohne Signal bis Fail-Safe aktiviert wird */
#define RC_TIMEOUT_TICKS    500

/* PWM-Parameter */
#define PWM_STEPS           100    /* Anzahl PWM-Stufen (0-100 = 0%-100%) */

/*============================================================================*/
/*                            GLOBALE VARIABLEN                               */
/*============================================================================*/

/* PWM-Duty-Cycle (0-100), wird von TIM2-ISR fuer PWM-Ausgabe verwendet */
volatile uint8_t pwm_duty = 0;

/* Gemessene RC-Pulsbreite in Mikrosekunden */
volatile uint16_t rc_pulse_us = 0;

/* Flag: Neues RC-Signal empfangen */
volatile uint8_t rc_new_data = 0;

/* Startzeit des RC-Pulses (TIM1-Counterwert bei Rising Edge) */
volatile uint16_t rc_start_time = 0;

/*============================================================================*/
/*                            GPIO-INITIALISIERUNG                            */
/*============================================================================*/

/**
 * @brief Initialisiert alle GPIO-Pins
 * 
 * Konfiguration:
 * - PC4: RC-Eingang (IN_FLOATING, da externer 3.3V-Pegel)
 * - PC2: PWM-Ausgang (Push-Pull fuer AL8862 CTRL)
 * - PC1: Mode-Jumper (Pull-Up, GND = Linear-Modus)
 * - PD6: Fail-Safe-Jumper (Pull-Up, GND = LED AN bei Signalverlust)
 */
void GPIO_Init_Custom(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* GPIO-Takt und AFIO (fuer EXTI) aktivieren */
    RCC_APB2PeriphClockCmd(RCC_GPIO_ENABLE | RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /* RC-Eingang: IN_FLOATING da RC-Empfaenger bereits 3.3V liefert */
    GPIO_InitStructure.GPIO_Pin = RC_INPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(RC_INPUT_PORT, &GPIO_InitStructure);

    /* PWM-Ausgang: Push-Pull fuer AL8862 CTRL-Pin */
    GPIO_InitStructure.GPIO_Pin = PWM_OUTPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PWM_OUTPUT_PORT, &GPIO_InitStructure);

    /* Mode-Jumper: Pull-Up, Jumper gegen GND zieht auf Low */
    GPIO_InitStructure.GPIO_Pin = MODE_JUMPER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(MODE_JUMPER_PORT, &GPIO_InitStructure);

    /* Fail-Safe-Jumper: Pull-Up, Jumper gegen GND zieht auf Low */
    GPIO_InitStructure.GPIO_Pin = FAILSAFE_JUMPER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(FAILSAFE_JUMPER_PORT, &GPIO_InitStructure);
}

/*============================================================================*/
/*                            EXTI-INITIALISIERUNG                            */
/*============================================================================*/

/**
 * @brief Konfiguriert EXTI fuer RC-Signal-Erkennung
 * 
 * EXTI-Line4 wird auf PC4 geroutet. Bei jeder Flanke (Rising und Falling)
 * wird der EXTI7_0_IRQHandler aufgerufen.
 * 
 * Interrupt-Prioritaet: 0 (hoechste, damit RC-Signal nicht verpasst wird)
 */
void EXTI_Init_Custom(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* PC4 mit EXTI-Line4 verbinden */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);

    /* EXTI-Line4 konfigurieren: Rising + Falling Edge */
    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* NVIC fuer EXTI-Line4..0 konfigurieren */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI7_0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*============================================================================*/
/*                            TIM1-INITIALISIERUNG                            */
/*============================================================================*/

/**
 * @brief Initialisiert TIM1 als Zeitbasis fuer Pulsbreitenmessung
 * 
 * Konfiguration:
 * - Prescaler: 24-1 -> 24MHz / 24 = 1MHz = 1us pro Tick
 * - Period: 0xFFFF -> Timer laeuft bis 65535, dann Overflow
 * - Aufloesung: 1us (passend fuer RC-Signal 1000-2000us)
 * 
 * TIM1 dient nur als Zeitbasis, kein Input-Capture.
 */
void TIM1_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 24 - 1;   /* 1us Aufloesung bei 24MHz */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    TIM_Cmd(TIM1, ENABLE);
}

/*============================================================================*/
/*                            TIM2-INITIALISIERUNG                            */
/*============================================================================*/

/**
 * @brief Initialisiert TIM2 fuer Software-PWM
 * 
 * TIM2 erzeugt einen 10kHz Interrupt (100us Periodendauer).
 * Der Interrupt-Handler zaehlt von 0 bis (PWM_STEPS-1) und erzeugt so eine 100Hz PWM
 * mit PWM_STEPS Stufen Aufloesung (1% pro Stufe).
 * 
 * Berechnung:
 * - SystemClock: 24MHz
 * - Prescaler: 0 (keine Teilung)
 * - Period: 2399 -> 24MHz / 2400 = 10kHz Interrupt
 * - PWM-Frequenz: 10kHz / PWM_STEPS = 100Hz
 * 
 * Interrupt-Prioritaet: 1 (niedriger als EXTI, damit RC-Signal Vorrang hat)
 */
void TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = 2400 - 1;    /* 10kHz Interrupt */
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* Update-Interrupt aktivieren */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    /* NVIC konfigurieren */
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  /* Niedriger als EXTI */
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

/*============================================================================*/
/*                            HAUPTPROGRAMM                                   */
/*============================================================================*/

/**
 * @brief Hauptprogramm
 * 
 * Ablauf:
 * 1. System initialisieren (GPIO, EXTI, TIM1, TIM2)
 * 2. Jumper-Stati einlesen (Linear/On/Off, Fail-Safe)
 * 3. Hauptschleife:
 *    - Neues RC-Signal verarbeiten -> pwm_duty berechnen
 *    - Timeout ueberwachen -> Fail-Safe aktivieren
 */
int main(void)
{
    uint8_t linear_mode = 0;        /* 1 = Linear-Modus, 0 = On/Off-Modus */
    uint8_t failsafe_high = 0;      /* 1 = LED AN bei Signalverlust */
    uint8_t signal_timeout = 0;     /* 1 = Kein Signal seit 50ms */
    uint16_t no_signal_counter = 0; /* Zaehlt Loop-Iterationen ohne Signal */

    /* NVIC-Prioritaetsgruppe konfigurieren */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    /* System initialisieren */
    SystemCoreClockUpdate();
    Delay_Init();

    /* Hardware initialisieren */
    GPIO_Init_Custom();
    EXTI_Init_Custom();
    TIM1_Init();
    TIM2_Init();

    /* Jumper-Stati einlesen (Pull-Up: OFFEN=1, GND=0) */
    linear_mode = !GPIO_ReadInputDataBit(MODE_JUMPER_PORT, MODE_JUMPER_PIN);
    failsafe_high = !GPIO_ReadInputDataBit(FAILSAFE_JUMPER_PORT, FAILSAFE_JUMPER_PIN);

    /* PWM initial auf 0 (LED AUS) */
    pwm_duty = 0;

    /* Startup-Verzoegerung: 1 Sekunde warten bis System stabil */
    Delay_Ms(1000);

    /*========================================================================*/
    /*                            HAUPTSCHLEIFE                               */
    /*========================================================================*/

    while (1)
    {
        if (rc_new_data)
        {
            /* Neues RC-Signal empfangen */
            rc_new_data = 0;
            signal_timeout = 0;
            no_signal_counter = 0;

            if (linear_mode)
            {
                /*------------------------------------------------------------*/
                /* LINEAR-MODUS: Proportionales Dimmen                       */
                /* 1000us -> pwm_duty = 0         -> LED AUS                  */
                /* 2000us -> pwm_duty = PWM_STEPS -> LED AN                   */
                /*------------------------------------------------------------*/
                if (rc_pulse_us <= RC_MIN_US)
                {
                    pwm_duty = 0;
                }
                else if (rc_pulse_us >= RC_MAX_US)
                {
                    pwm_duty = PWM_STEPS;
                }
                else
                {
                    /* Linear interpolieren: (rc_pulse_us - 1000) / 10 */
                    pwm_duty = (uint8_t)((rc_pulse_us - RC_MIN_US) / 10);
                }
            }
            else
            {
                /*------------------------------------------------------------*/
                /* ON/OFF-MODUS: Schwellwert-basiertes Schalten              */
                /* < 1500us -> pwm_duty = 0         -> LED AUS                */
                /* >= 1500us -> pwm_duty = PWM_STEPS -> LED AN                */
                /*------------------------------------------------------------*/
                if (rc_pulse_us >= RC_THRESHOLD_US)
                {
                    pwm_duty = PWM_STEPS;
                }
                else
                {
                    pwm_duty = 0;
                }
            }
        }
        else
        {
            /* Kein neues Signal: Timeout-Zaehler inkrementieren */
            no_signal_counter++;
            if (no_signal_counter >= RC_TIMEOUT_TICKS)
            {
                signal_timeout = 1;
            }
        }

        if (signal_timeout)
        {
            /*------------------------------------------------------------*/
            /* FAIL-SAFE: Bei Signalverlust LED auf definierten Zustand  */
            /* failsafe_high = 1 (PD6=GND) -> pwm_duty = PWM_STEPS -> LED AN */
            /* failsafe_high = 0 (PD6=OFFEN) -> pwm_duty = 0 -> LED AUS   */
            /*------------------------------------------------------------*/
            pwm_duty = failsafe_high ? PWM_STEPS : 0;
        }

        /* Loop-Verzoegerung: 100us */
        Delay_Us(100);
    }
}
