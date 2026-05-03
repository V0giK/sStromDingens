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
 * Version            : V3.0.1
 * Date               : 2026/05/01
 * Description        : RC-Signal to AL8862 Hardware-PWM converter (EXTI + TIM2)
 *
 * Kompilieroptionen (eine auswaehlen):
 *   #define LED_1W    // 330mA LED, max 100% Duty-Cycle (Original-Hardware)
 *   #define LED_3W    // 666mA LED, max 80% Duty-Cycle (modifizierte Hardware)
 *   #define LED_500   // 500mA LED, max 60% Duty-Cycle (modifizierte Hardware)
 *   #define LED_666   // 666mA LED, max 80% Duty-Cycle (modifizierte Hardware)
 *   #define LED_830   // 830mA LED, max 100% Duty-Cycle (modifizierte Hardware)
 */

/* LED-Typ auswaehlen:
 * LED_1W  = 1W LED (330mA), PWM_MAX_DUTY = 100%, Original-Hardware
 * LED_3W  = 3W LED (666mA), PWM_MAX_DUTY = 80%,  modifizierte Hardware (2x300 mOhm)
 * LED_500 = 500mA         , PWM_MAX_DUTY = 60%,  modifizierte Hardware (2x300 mOhm)
 * LED_666 = 666mA         , PWM_MAX_DUTY = 80%,  modifizierte Hardware (2x300 mOhm)
 * LED_830 = 830mA         , PWM_MAX_DUTY = 100%, modifizierte Hardware (2x300 mOhm)
 */
#define LED_500

#include "debug.h"
#include "ch32v00x_exti.h"
#include "ch32v00x_tim.h"

/*============================================================================*/
/*                            PIN-DEFINITIONEN                                */
/*============================================================================*/

#define RCC_GPIO_ENABLE  (RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO)

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

/* LED-Typ Validierung und PWM_MAX_DUTY automatisch setzen */
#if defined(LED_1W)
    #define PWM_MAX_DUTY    100     /* 1W LED: max 100% Duty-Cycle (330mA) */
#elif defined(LED_3W)
    #define PWM_MAX_DUTY    80      /* 3W LED: max 80% Duty-Cycle (666mA) */
#elif defined(LED_500)
    #define PWM_MAX_DUTY    60      /* max 60% Duty-Cycle (500mA) */
#elif defined(LED_666)
    #define PWM_MAX_DUTY    80      /* max 80% Duty-Cycle (666mA) */
#elif defined(LED_830)
    #define PWM_MAX_DUTY    100     /* max 100% Duty-Cycle (830mA) */
#else
    #error "Bitte LED_1W, LED_3W, LED_500, LED_666 oder LED_830 in main.c definieren!"
#endif

/* RC-Signal Grenzen in Mikrosekunden */
#define RC_MIN_US           1000    /* Minimum: 1ms Pulsbreite */
#define RC_MAX_US           2000    /* Maximum: 2ms Pulsbreite */

/* Hysterese fuer On/Off-Modus */
#define ONOFF_THRESHOLD_ON   1550    /* Einschalten ab hier */
#define ONOFF_THRESHOLD_OFF  1450    /* Ausschalten erst unter hier */

/* RC-Signal Validierungsgrenzen */
#define RC_PULSE_MIN_VALID   800    /* Minimum fuer gueltigen Puls (Toleranz) */
#define RC_PULSE_MAX_VALID   2500   /* Maximum fuer gueltigen Puls (Toleranz) */

/* Timeout: 500 * 100us = 50ms ohne Signal bis Fail-Safe aktiviert wird */
#define RC_TIMEOUT_TICKS    500

/* PWM-Parameter */
#define PWM_FREQ_HZ         2000    /* Hardware-PWM Frequenz in Hz */

/*============================================================================*/
/*                            GLOBALE VARIABLEN                               */
/*============================================================================*/

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
 * - PC2: PWM-Ausgang (AF_PP fuer TIM2_CH2)
 * - PC1: Mode-Jumper (Pull-Up, GND = Linear-Modus)
 * - PD6: Fail-Safe-Jumper (Pull-Up, GND = LED AN bei Signalverlust)
 */
void GPIO_Init_Custom(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* GPIO-Takt aktivieren (AFIO bereits in RCC_GPIO_ENABLE enthalten) */
    RCC_APB2PeriphClockCmd(RCC_GPIO_ENABLE, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /* RC-Eingang: IN_FLOATING da RC-Empfaenger bereits 3.3V liefert */
    GPIO_InitStructure.GPIO_Pin = RC_INPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(RC_INPUT_PORT, &GPIO_InitStructure);

    /* PWM-Ausgang: Alternate Function Push-Pull fuer TIM2_CH2 */
    GPIO_InitStructure.GPIO_Pin = PWM_OUTPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
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
/*                            TIM2-INITIALISIERUNG (Hardware-PWM)             */
/*============================================================================*/

/**
 * @brief Initialisiert TIM2 fuer Hardware-PWM auf PC2 (TIM2_CH2)
 *
 * Konfiguration:
 * - Remap: PartialRemap1 -> TIM2_CH2 auf PC2
 * - Prescaler: 0 -> Keine Teilung, 24MHz Takt
 * - Period: 11999 -> 24MHz / 12000 = 2kHz PWM-Frequenz
 * - Channel 2: PWM Mode 1, Output Enable
 * - Polarity: High (Active High fuer AL8862)
 */
void TIM2_PWM_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* TIM2 PartialRemap1: CH2 auf PC2 (laut CH32V003RM) */
    GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);

    /* Zeitbasis: 2kHz PWM-Frequenz */
    TIM_TimeBaseStructure.TIM_Period = (24000 / 2) - 1; /* 24MHz / 12000 = 2kHz */
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* PWM Mode 1 konfigurieren */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputState_Disable;
    TIM_OCInitStructure.TIM_Pulse = 0; /* Initial 0% Duty */
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);

    /* Enable preload */
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);

    /* TIM2-Interrupts explizit deaktivieren (falls Residualstaetig aus alter FW) */
    NVIC_DisableIRQ(TIM2_IRQn);

    TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief Setzt den PWM-Duty-Cycle fuer TIM2_CH2
 *
 * @param duty Duty-Cycle in Prozent (0-100)
 */
static void set_pwm_output(uint8_t duty)
{
    if (duty > 100)
    {
        duty = 100;
    }
    uint16_t compare = (uint16_t)(((uint32_t)duty * (TIM2->ATRLR + 1)) / 100);
    TIM_SetCompare2(TIM2, compare);
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
 *    - Neues RC-Signal verarbeiten -> pwm_output berechnen
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
    TIM2_PWM_Init();

    /* Jumper-Stati einlesen (Pull-Up: OFFEN=1, GND=0) */
    linear_mode = !GPIO_ReadInputDataBit(MODE_JUMPER_PORT, MODE_JUMPER_PIN);
    failsafe_high = !GPIO_ReadInputDataBit(FAILSAFE_JUMPER_PORT, FAILSAFE_JUMPER_PIN);

    /* PWM initial auf 0 (LED AUS) */
    set_pwm_output(0);

    /* Startup-Verzoegerung: 1 Sekunde warten bis System stabil */
    Delay_Ms(1000);

    uint8_t onoff_state = 0;         /* 0 = LED AUS, 1 = LED AN (On/Off-Modus) */

    uint8_t pwm_output = 0;

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
                /* 1000us -> pwm_output = 0            -> LED AUS             */
                /* 2000us -> pwm_output = PWM_MAX_DUTY   -> LED AN (max)      */
                /*------------------------------------------------------------*/
                if (rc_pulse_us <= RC_MIN_US)
                {
                    pwm_output = 0;
                }
                else if (rc_pulse_us >= RC_MAX_US)
                {
                    pwm_output = PWM_MAX_DUTY;
                }
                else
                {
                    /* Linear auf 0 ... PWM_MAX_DUTY skalieren */
                    pwm_output = (uint8_t)(((uint32_t)(rc_pulse_us - RC_MIN_US) * PWM_MAX_DUTY)
                                         / (RC_MAX_US - RC_MIN_US));
                }
            }
            else
            {
                /*------------------------------------------------------------*/
                /* ON/OFF-MODUS: Hysterese-Schaltung                         */
                /*                                                             */
                /* Einschalten: >= 1550us (ONOFF_THRESHOLD_ON)                */
                /* Ausschalten: < 1450us (ONOFF_THRESHOLD_OFF)                */
                /* Zwischen 1450-1550us: Zustand beibehalten (Hysterese)      */
                /*------------------------------------------------------------*/
                if (rc_pulse_us >= ONOFF_THRESHOLD_ON)
                {
                    onoff_state = 1;
                }
                else if (rc_pulse_us < ONOFF_THRESHOLD_OFF)
                {
                    onoff_state = 0;
                }
                /* Ansonsten: onoff_state unveraendert (Hysterese-Bereich) */

                pwm_output = onoff_state ? PWM_MAX_DUTY : 0;
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
            /* failsafe_high = 1 (PD6=GND) -> LED AN (max) */
            /* failsafe_high = 0 (PD6=OFFEN) -> LED AUS   */
            /*------------------------------------------------------------*/
            pwm_output = failsafe_high ? PWM_MAX_DUTY : 0;
        }

        set_pwm_output(pwm_output);

        /* Loop-Verzoegerung: 100us */
        Delay_Us(100);
    }
}
