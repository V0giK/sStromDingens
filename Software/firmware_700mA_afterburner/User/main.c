/*
 * sStromDingens – RC Afterburner Controller
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
 * Version            : V3.4.1
 * Date               : 2026/04/30
 * Description        : RC Afterburner Controller (FX Edition)
 */

#include "debug.h"
#include "ch32v00x_exti.h"
#include "ch32v00x_tim.h"

/*============================================================================*/
/*                            PIN-DEFINITIONEN                                */
/*============================================================================*/

#define RCC_GPIO_ENABLE  (RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD)

#define RC_INPUT_PIN    GPIO_Pin_4
#define RC_INPUT_PORT   GPIOC

#define PWM_OUTPUT_PIN  GPIO_Pin_2
#define PWM_OUTPUT_PORT GPIOC

/*============================================================================*/
/*                            PARAMETER-DEFINITIONEN                          */
/*============================================================================*/

/* RC-Signal Grenzen in Mikrosekunden */
#define PULSE_MIN           1000    /* Minimum: 1ms Pulsbreite (Leerlauf) */
#define PULSE_MAX           2000    /* Maximum: 2ms Pulsbreite (Vollgas) */

/* Hysterese-Schwellwerte */
#define THRESHOLD_ON        1650    /* Einschalten ab hier */
#define THRESHOLD_OFF       1550    /* Ausschalten erst unter hier */

/* Flame-Out Erkennung */
#define FLAMEOUT_HIGH       1800    /* Vorheriger Wert ueber hier */
#define FLAMEOUT_LOW        1200    /* Aktueller Wert unter hier */

/* Burst Erkennung */
#define BURST_THRESHOLD     1950    /* Ab hier Vollgas-Burst */

/* RC-Signal Validierungsgrenzen */
#define RC_PULSE_MIN_VALID   800    /* Minimum fuer gueltigen Puls (Toleranz) */
#define RC_PULSE_MAX_VALID   2500   /* Maximum fuer gueltigen Puls (Toleranz) */

/* Timeout: 500 * 100us = 50ms ohne Signal bis Fail-Safe aktiviert wird */
#define RC_TIMEOUT_TICKS    500

/* PWM-Parameter */
#define PWM_FREQ_HZ         2000    /* Hardware-PWM Frequenz in Hz */
#define PWM_MAX_DUTY        60      /* (ca. 500 mA) Maximale Ausgangsleistung in % (0-100) */
//#define PWM_MAX_DUTY        80      /* (ca. 666 mA) Maximale Ausgangsleistung in % (0-100) */
//#define PWM_MAX_DUTY        100     /* (ca. 830 mA) Maximale Ausgangsleistung in % (0-100) */

/* Afterburner-Effekte */
#define SPOOL_UP_MIN_MS     80      /* Minimale Flash-Dauer */
#define SPOOL_UP_MAX_MS     130     /* Maximale Flash-Dauer */
#define RAMP_DOWN_MS        80      /* Von Flash auf Sollwert in ms */
#define COOL_DOWN_MS        250     /* Von Sollwert auf 0 in ms */
#define FLAMEOUT_MS         150     /* Dauer des Flame-Out-Flackerns in ms */
#define BURST_MS            20      /* Dauer des Vollgas-Bursts in ms */
#define FLICKER_NORMAL      15      /* Basis-Flicker-Amplitude in % */
#define FLICKER_HEAT        25      /* Flicker-Amplitude im Heat-Shimmer-Bereich in % */
#define HEAT_THRESHOLD      1900    /* Ab dieser Pulsweite: verstaerkter Flicker */

/* Power-On Selftest */
#define SELFTEST_MS         200     /* Dauer des Selftests in ms */
#define SELFTEST_DUTY       50      /* Helligkeit waehrend Selftest in % */

/*============================================================================*/
/*                            STATE-MACHINE                                   */
/*============================================================================*/

#define STATE_SELFTEST      0
#define STATE_IDLE          1
#define STATE_SPOOL_UP      2
#define STATE_RAMP_DOWN     3
#define STATE_RUNNING       4
#define STATE_BURST         5
#define STATE_FLAMEOUT      6
#define STATE_COOL_DOWN     7

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
/*                            HILFSFUNKTIONEN                                 */
/*============================================================================*/

/**
 * @brief 16-Bit Xorshift Pseudo-Random Number Generator
 */
static uint8_t random_uint8(void)
{
    static uint16_t rng = 0xACE1u;
    rng ^= (rng << 7);
    rng ^= (rng >> 9);
    rng ^= (rng << 8);
    return (uint8_t)(rng);
}

/**
 * @brief Liefert eine Zufallszahl im Bereich [min, max]
 */
static uint8_t random_range(uint8_t min, uint8_t max)
{
    return min + (random_uint8() % (max - min + 1));
}

/**
 * @brief Berechnet die Soll-Helligkeit aus RC-Pulsbreite
 *
 * @return Helligkeit in Prozent (0-PWM_MAX_DUTY)
 */
static uint8_t calculate_target_brightness(void)
{
    uint16_t pulse_clamped = rc_pulse_us;
    if (pulse_clamped > PULSE_MAX)
    {
        pulse_clamped = PULSE_MAX;
    }
    if (pulse_clamped < THRESHOLD_ON)
    {
        pulse_clamped = THRESHOLD_ON;
    }

    uint16_t pulse_range = PULSE_MAX - THRESHOLD_ON;
    uint16_t pulse_offset = pulse_clamped - THRESHOLD_ON;

    /* Basishelligkeit 10% ... PWM_MAX_DUTY% mappen */
    uint8_t base_brightness;

    if (pulse_offset == 0)
    {
        base_brightness = 10;
    }
    else
    {
        base_brightness = (uint8_t)(10 + (uint32_t)(pulse_offset * (PWM_MAX_DUTY - 10)) / pulse_range);
    }

    /* Flicker berechnen */
    int8_t flicker;
    if (rc_pulse_us >= HEAT_THRESHOLD)
    {
        flicker = (int8_t)((random_uint8() % (FLICKER_HEAT * 2 + 1)) - FLICKER_HEAT);
    }
    else
    {
        flicker = (int8_t)((random_uint8() % (FLICKER_NORMAL * 2 + 1)) - FLICKER_NORMAL);
    }

    int16_t result = (int16_t)base_brightness + flicker;

    if (result < 0)
    {
        result = 0;
    }
    else if (result > PWM_MAX_DUTY)
    {
        result = PWM_MAX_DUTY;
    }

    return (uint8_t)result;
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
/*                            GPIO-INITIALISIERUNG                            */
/*============================================================================*/

/**
 * @brief Initialisiert alle GPIO-Pins
 *
 * Konfiguration:
 * - PC4: RC-Eingang (IN_FLOATING, da externer 3.3V-Pegel)
 * - PC2: PWM-Ausgang (AF_PP fuer TIM2_CH2)
 */
void GPIO_Init_Custom(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* GPIO-Takt und AFIO (fuer EXTI und TIM2-Remap) aktivieren */
    RCC_APB2PeriphClockCmd(RCC_GPIO_ENABLE | RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /* RC-Eingang: IN_FLOATING da RC-Empfaenger bereits 3.3V liefert */
    GPIO_InitStructure.GPIO_Pin = RC_INPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(RC_INPUT_PORT, &GPIO_InitStructure);

    /* PWM-Ausgang: Alternate Function Push-Pull fuer TIM2_CH2 */
    GPIO_InitStructure.GPIO_Pin = PWM_OUTPUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(PWM_OUTPUT_PORT, &GPIO_InitStructure);
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
 * Interrupt-Prioritaet: 0 (hoechste)
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
/*                            TIM1-INITIALISIERUNG (Zeitbasis)                */
/*============================================================================*/

/**
 * @brief Initialisiert TIM1 als Zeitbasis fuer Pulsbreitenmessung
 *
 * Konfiguration:
 * - Prescaler: 24-1 -> 24MHz / 24 = 1MHz = 1us pro Tick
 * - Period: 0xFFFF -> Timer laeuft bis 65535, dann Overflow
 * - Aufloesung: 1us
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

    TIM_Cmd(TIM2, ENABLE);
}

/*============================================================================*/
/*                            HAUPTPROGRAMM                                   */
/*============================================================================*/

/**
 * @brief Hauptprogramm
 *
 * Ablauf:
 * 1. System initialisieren (GPIO, EXTI, TIM1, TIM2-HW-PWM)
 * 2. Power-On Selftest
 * 3. Hauptschleife mit Afterburner-State-Machine
 *    - Neues RC-Signal verarbeiten -> pwm_output berechnen
 *    - Timeout ueberwachen -> Fail-Safe (LED AUS)
 */
int main(void)
{
    uint8_t signal_timeout = 0;     /* 1 = Kein Signal seit 50ms */
    uint16_t no_signal_counter = 0; /* Zaehlt Loop-Iterationen ohne Signal */

    uint8_t afterburner_state = STATE_SELFTEST;
    uint16_t state_counter = 0;     /* Generischer Zaehler fuer States */

    /* Ramp-Variablen */
    uint8_t pwm_output = 0;         /* Tatsaechlicher Ausgabewert */
    uint8_t pwm_target = 0;         /* Berechneter Zielwert */
    uint8_t ramp_start_value = 0;   /* Startwert fuer Rampe */
    uint16_t ramp_counter = 0;      /* Zaehler fuer Rampe */
    uint16_t ramp_ticks = 0;        /* Gesamtdauer der Rampe in 100us */

    /* FX-Variablen */
    uint16_t rc_pulse_prev = 0;     /* Vorheriger RC-Wert fuer Sprung-Erkennung */
    uint8_t spool_duration = 0;     /* Zufaellige Spool-Up-Dauer */

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

    /* RNG mit unvorhersehbarem Wert initialisieren */
    random_uint8(); /* Dummy-Aufruf um RNG zu starten */

    /* PWM initial auf 0 (LED AUS) */
    pwm_output = 0;
    set_pwm_output(0);

    /* Startup-Verzoegerung: 1 Sekunde warten bis System stabil */
    Delay_Ms(1000);

    /* Power-On Selftest initialisieren */
    state_counter = (uint16_t)(SELFTEST_MS * 10); /* 2000 Ticks = 200ms */
    pwm_output = SELFTEST_DUTY;

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

            switch (afterburner_state)
            {
                /*--------------------------------------------------------*/
                /* STATE_SELFTEST: Power-On LED-Test                     */
                /*--------------------------------------------------------*/
                case STATE_SELFTEST:
                    /* Timer-Handling in zweitem Block */
                    break;

                /*--------------------------------------------------------*/
                /* STATE_IDLE: Nachbrenner ist aus                       */
                /*--------------------------------------------------------*/
                case STATE_IDLE:
                    if (rc_pulse_us >= THRESHOLD_ON)
                    {
                        /* Spool-Up starten: Zufaelliger Flash */
                        afterburner_state = STATE_SPOOL_UP;
                        spool_duration = random_range(SPOOL_UP_MIN_MS, SPOOL_UP_MAX_MS);
                        state_counter = (uint16_t)(spool_duration * 10); /* 100us-Ticks */
                        pwm_output = PWM_MAX_DUTY;
                        pwm_target = PWM_MAX_DUTY;
                    }
                    else
                    {
                        pwm_output = 0;
                        pwm_target = 0;
                    }
                    break;

                /*--------------------------------------------------------*/
                /* STATE_SPOOL_UP: Zund-Flash simulieren                 */
                /*--------------------------------------------------------*/
                case STATE_SPOOL_UP:
                    if (rc_pulse_us < THRESHOLD_OFF)
                    {
                        /* Gas zurueck -> Cool-Down starten */
                        afterburner_state = STATE_COOL_DOWN;
                        ramp_start_value = pwm_output;
                        ramp_counter = 0;
                        ramp_ticks = (uint16_t)(COOL_DOWN_MS * 10);
                        pwm_target = 0;
                    }
                    /* Timer-Handling in zweitem Block */
                    break;

                /*--------------------------------------------------------*/
                /* STATE_RAMP_DOWN: Von Flash auf Sollwert rampen        */
                /*--------------------------------------------------------*/
                case STATE_RAMP_DOWN:
                    if (rc_pulse_us < THRESHOLD_OFF)
                    {
                        /* Gas zurueck -> Cool-Down starten */
                        afterburner_state = STATE_COOL_DOWN;
                        ramp_start_value = pwm_output;
                        ramp_counter = 0;
                        ramp_ticks = (uint16_t)(COOL_DOWN_MS * 10);
                        pwm_target = 0;
                    }
                    /* Ramp-Interpolation in zweitem Block */
                    break;

                /*--------------------------------------------------------*/
                /* STATE_RUNNING: Nachbrenner aktiv mit Flicker          */
                /*--------------------------------------------------------*/
                case STATE_RUNNING:
                    /* Flame-Out Erkennung: abruptes Gas-weg */
                    if ((rc_pulse_prev >= FLAMEOUT_HIGH) && (rc_pulse_us <= FLAMEOUT_LOW))
                    {
                        afterburner_state = STATE_FLAMEOUT;
                        state_counter = (uint16_t)(FLAMEOUT_MS * 10);
                    }
                    /* Vollgas-Burst Erkennung */
                    else if ((rc_pulse_prev <= BURST_THRESHOLD) && (rc_pulse_us > BURST_THRESHOLD))
                    {
                        afterburner_state = STATE_BURST;
                        state_counter = (uint16_t)(BURST_MS * 10);
                        pwm_output = PWM_MAX_DUTY;
                    }
                    /* Normales Ausschalten */
                    else if (rc_pulse_us < THRESHOLD_OFF)
                    {
                        afterburner_state = STATE_COOL_DOWN;
                        ramp_start_value = pwm_output;
                        ramp_counter = 0;
                        ramp_ticks = (uint16_t)(COOL_DOWN_MS * 10);
                        pwm_target = 0;
                    }
                    else
                    {
                        pwm_target = calculate_target_brightness();
                        pwm_output = pwm_target;
                    }
                    break;

                /*--------------------------------------------------------*/
                /* STATE_BURST: Kurzer Ueberdruck-Blitz                  */
                /*--------------------------------------------------------*/
                case STATE_BURST:
                    if (rc_pulse_us < THRESHOLD_OFF)
                    {
                        /* Gas zurueck -> Cool-Down starten */
                        afterburner_state = STATE_COOL_DOWN;
                        ramp_start_value = pwm_output;
                        ramp_counter = 0;
                        ramp_ticks = (uint16_t)(COOL_DOWN_MS * 10);
                        pwm_target = 0;
                    }
                    /* Timer-Handling in zweitem Block */
                    break;

                /*--------------------------------------------------------*/
                /* STATE_FLAMEOUT: Heftiges Flackern beim Absterben      */
                /*--------------------------------------------------------*/
                case STATE_FLAMEOUT:
                    if (rc_pulse_us >= THRESHOLD_ON)
                    {
                        /* Gas wieder da -> sofort neu zuenden */
                        afterburner_state = STATE_SPOOL_UP;
                        spool_duration = random_range(SPOOL_UP_MIN_MS, SPOOL_UP_MAX_MS);
                        state_counter = (uint16_t)(spool_duration * 10);
                        pwm_output = PWM_MAX_DUTY;
                        pwm_target = PWM_MAX_DUTY;
                    }
                    /* Timer-Handling in zweitem Block */
                    break;

                /*--------------------------------------------------------*/
                /* STATE_COOL_DOWN: Sanftes Abklingen                    */
                /*--------------------------------------------------------*/
                case STATE_COOL_DOWN:
                    if (rc_pulse_us >= THRESHOLD_ON)
                    {
                        /* Gas wieder da -> sofort neu zuenden */
                        afterburner_state = STATE_SPOOL_UP;
                        spool_duration = random_range(SPOOL_UP_MIN_MS, SPOOL_UP_MAX_MS);
                        state_counter = (uint16_t)(spool_duration * 10);
                        pwm_output = PWM_MAX_DUTY;
                        pwm_target = PWM_MAX_DUTY;
                    }
                    /* Ramp-Interpolation in zweitem Block */
                    break;

                default:
                    afterburner_state = STATE_IDLE;
                    pwm_output = 0;
                    pwm_target = 0;
                    break;
            }

            /* Vorherigen Wert fuer naechste Iteration speichern */
            rc_pulse_prev = rc_pulse_us;
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

        /* Fail-Safe-Behandlung: Immer LED AUS bei Signalverlust */
        if (signal_timeout)
        {
            afterburner_state = STATE_IDLE;
            pwm_output = 0;
            pwm_target = 0;
        }

        /* State-Counter dekrementieren wenn kein neues RC-Signal */
        if (afterburner_state == STATE_SELFTEST)
        {
            state_counter--;
            if (state_counter == 0)
            {
                afterburner_state = STATE_IDLE;
                pwm_output = 0;
            }
            else
            {
                pwm_output = SELFTEST_DUTY;
            }
        }
        else if (afterburner_state == STATE_SPOOL_UP)
        {
            state_counter--;
            if (state_counter == 0)
            {
                /* Flash vorbei -> Rampe auf Sollwert */
                afterburner_state = STATE_RAMP_DOWN;
                ramp_start_value = pwm_output;
                ramp_counter = 0;
                ramp_ticks = (uint16_t)(RAMP_DOWN_MS * 10);
                pwm_target = calculate_target_brightness();
            }
            else
            {
                pwm_output = PWM_MAX_DUTY;
            }
        }
        else if (afterburner_state == STATE_BURST)
        {
            state_counter--;
            if (state_counter == 0)
            {
                afterburner_state = STATE_RUNNING;
                pwm_output = calculate_target_brightness();
            }
            else
            {
                pwm_output = PWM_MAX_DUTY;
            }
        }
        else if (afterburner_state == STATE_FLAMEOUT)
        {
            state_counter--;
            if (state_counter == 0)
            {
                afterburner_state = STATE_COOL_DOWN;
                ramp_start_value = pwm_output;
                ramp_counter = 0;
                ramp_ticks = (uint16_t)(COOL_DOWN_MS * 10);
                pwm_target = 0;
            }
            else
            {
                int8_t flicker = (int8_t)((random_uint8() % 81) - 40);
                int16_t result = (int16_t)calculate_target_brightness() + flicker;
                if (result < 0) result = 0;
                if (result > PWM_MAX_DUTY) result = PWM_MAX_DUTY;
                pwm_output = (uint8_t)result;
            }
        }
        else if (afterburner_state == STATE_COOL_DOWN)
        {
            ramp_counter++;
            if (ramp_counter >= ramp_ticks)
            {
                afterburner_state = STATE_IDLE;
                pwm_output = 0;
            }
            else
            {
                pwm_output = ramp_start_value - (uint8_t)(((uint32_t)ramp_start_value * ramp_counter) / ramp_ticks);
            }
        }
        else if (afterburner_state == STATE_RAMP_DOWN)
        {
            ramp_counter++;
            if (ramp_counter >= ramp_ticks)
            {
                afterburner_state = STATE_RUNNING;
                pwm_output = pwm_target;
            }
            else
            {
                if (ramp_start_value > pwm_target)
                {
                    pwm_output = ramp_start_value - (uint8_t)(((uint32_t)(ramp_start_value - pwm_target) * ramp_counter) / ramp_ticks);
                }
                else
                {
                    pwm_output = ramp_start_value + (uint8_t)(((uint32_t)(pwm_target - ramp_start_value) * ramp_counter) / ramp_ticks);
                }
            }
        }

        /* PWM-Ausgang aktualisieren */
        set_pwm_output(pwm_output);

        /* Loop-Verzoegerung: 100us */
        Delay_Us(100);
    }
}
