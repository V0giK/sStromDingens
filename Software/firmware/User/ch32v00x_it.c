/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32v00x_it.c
 * Author             : V0giK
 * Version            : V1.2.0
 * Date               : 2026/03/17
 * Description        : Main Interrupt Service Routines.
 *********************************************************************************
 * Copyright (c) 2026 V0giK
 *******************************************************************************/

#include "ch32v00x_it.h"

/*============================================================================*/
/*                            KONSTANTEN                                      */
/*============================================================================*/

/* RC-Signal Validierungsgrenzen (Mikrosekunden) */
#define RC_PULSE_MIN_VALID   800
#define RC_PULSE_MAX_VALID   2500

/* PWM-Parameter */
#define PWM_STEPS           100

/*============================================================================*/
/*                            EXTERNE VARIABLEN                               */
/*============================================================================*/

/* PWM-Duty-Cycle (0-100), definiert in main.c */
extern volatile uint8_t pwm_duty;

/* Gemessene RC-Pulsbreite in Mikrosekunden */
extern volatile uint16_t rc_pulse_us;

/* Flag: Neues RC-Signal empfangen */
extern volatile uint8_t rc_new_data;

/* Startzeit des RC-Pulses (TIM1-Counterwert bei Rising Edge) */
extern volatile uint16_t rc_start_time;

/*============================================================================*/
/*                            LOKALE VARIABLEN                                */
/*============================================================================*/

/* Zähler für Software-PWM: 0-99, dann Reset */
static uint8_t pwm_counter = 0;

/*============================================================================*/
/*                            INTERRUPT-PROTOTYPEN                            */
/*============================================================================*/

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*============================================================================*/
/*                            NMI-HANDLER                                     */
/*============================================================================*/

/**
 * @brief Non-Maskable Interrupt Handler
 * 
 * Wird bei schwerwiegenden Hardware-Fehlern aufgerufen.
 * Endlosschleife als sicherer Zustand.
 */
void NMI_Handler(void)
{
    while (1)
    {
    }
}

/*============================================================================*/
/*                            HARDFAULT-HANDLER                               */
/*============================================================================*/

/**
 * @brief Hard Fault Handler
 * 
 * Wird bei kritischen Fehlern aufgerufen (z.B. Stack-Overflow,
 * ungültiger Speicherzugriff).
 * Führt einen System-Reset durch.
 */
void HardFault_Handler(void)
{
    NVIC_SystemReset();
    while (1)
    {
    }
}

/*============================================================================*/
/*                            EXTI-HANDLER (RC-SIGNAL)                        */
/*============================================================================*/

/**
 * @brief EXTI-Line0..7 Interrupt Handler
 * 
 * Wird bei jeder Flanke am RC-Eingang (PC4) aufgerufen.
 * 
 * Funktionsweise:
 * - Rising Edge:  Speichert den TIM1-Counter als Startzeit
 * - Falling Edge: Berechnet die Pulsbreite und speichert sie in rc_pulse_us
 * 
 * Pulsbreitenmessung:
 * - TIM1 läuft mit 1MHz (1µs pro Tick)
 * - Gültige Pulse: RC_PULSE_MIN_VALID - RC_PULSE_MAX_VALID
 * - Bei gültigem Puls wird rc_new_data = 1 gesetzt
 */
void EXTI7_0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
        /* Interrupt-Flag löschen */
        EXTI_ClearITPendingBit(EXTI_Line4);

        if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4))
        {
            /*------------------------------------------------------------*/
            /* RISING EDGE: Puls beginnt                                  */
            /* Startzeit aus TIM1-Counter speichern                       */
            /*------------------------------------------------------------*/
            rc_start_time = TIM1->CNT;
        }
        else
        {
            /*------------------------------------------------------------*/
            /* FALLING EDGE: Puls endet                                   */
            /* Pulsbreite berechnen                                       */
            /*------------------------------------------------------------*/
            uint16_t rc_end_time = TIM1->CNT;
            uint16_t diff;

            /* Differenz berechnen (mit Overflow-Behandlung) */
            if (rc_end_time >= rc_start_time)
            {
                diff = rc_end_time - rc_start_time;
            }
            else
            {
                /* TIM1-Overflow: 0xFFFF - start + end + 1 */
                diff = (0xFFFF - rc_start_time) + rc_end_time + 1;
            }

            /*------------------------------------------------------------*/
            /* Puls validieren                                            */
            /* Gültig: RC_PULSE_MIN_VALID bis RC_PULSE_MAX_VALID µs      */
            /* Bei gültigem Puls: Daten übernehmen und Flag setzen       */
            /*------------------------------------------------------------*/
            if (diff >= RC_PULSE_MIN_VALID && diff <= RC_PULSE_MAX_VALID)
            {
                rc_pulse_us = diff;
                rc_new_data = 1;
            }
        }
    }
}

/*============================================================================*/
/*                            TIM2-HANDLER (SOFTWARE-PWM)                     */
/*============================================================================*/

/**
 * @brief TIM2 Update Interrupt Handler
 * 
 * Erzeugt eine 100Hz Software-PWM mit PWM_STEPS Stufen (1% Auflösung).
 * 
 * Funktionsweise:
 * - TIM2 Interrupt alle 100µs (10kHz)
 * - pwm_counter zählt von 0 bis (PWM_STEPS-1)
 * - Wenn counter < duty: PWM-Pin HIGH
 * - Wenn counter >= duty: PWM-Pin LOW
 * 
 * Beispiel bei pwm_duty = 30:
 * - Counter 0-29:  Pin HIGH (30 Ticks = 30%)
 * - Counter 30-99: Pin LOW  (70 Ticks = 70%)
 * - Ergebnis: 30% Duty-Cycle bei 100Hz
 */
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        /* Interrupt-Flag löschen */
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        /*------------------------------------------------------------*/
        /* PWM-AUSGABE                                                */
        /* AL8862: High = LED AN, Low = LED AUS                       */
        /*------------------------------------------------------------*/
        if (pwm_counter < pwm_duty)
        {
            /* PWM-Pin HIGH (LED AN) */
            GPIO_SetBits(GPIOC, GPIO_Pin_2);
        }
        else
        {
            /* PWM-Pin LOW (LED AUS) */
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);
        }

        /*------------------------------------------------------------*/
        /* COUNTER AKTUALISIEREN                                      */
        /* Nach PWM_STEPS Schritten Reset (100 * 100µs = 10ms = 100Hz) */
        /*------------------------------------------------------------*/
        pwm_counter++;
        if (pwm_counter >= PWM_STEPS)
        {
            pwm_counter = 0;
        }
    }
}
