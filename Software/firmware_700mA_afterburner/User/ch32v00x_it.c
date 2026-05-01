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
 * File Name          : ch32v00x_it.c
 * Author             : V0giK
 * Version            : V3.4.0
 * Date               : 2026/04/30
 * Description        : Interrupt Service Routines.
 */

#include "ch32v00x_it.h"

/*============================================================================*/
/*                            KONSTANTEN                                      */
/*============================================================================*/

/* RC-Signal Validierungsgrenzen (Mikrosekunden) */
#define RC_PULSE_MIN_VALID   800
#define RC_PULSE_MAX_VALID   2500

/*============================================================================*/
/*                            EXTERNE VARIABLEN                               */
/*============================================================================*/

/* Gemessene RC-Pulsbreite in Mikrosekunden */
extern volatile uint16_t rc_pulse_us;

/* Flag: Neues RC-Signal empfangen */
extern volatile uint8_t rc_new_data;

/* Startzeit des RC-Pulses (TIM1-Counterwert bei Rising Edge) */
extern volatile uint16_t rc_start_time;

/*============================================================================*/
/*                            INTERRUPT-PROTOTYPEN                            */
/*============================================================================*/

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

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
 * ungueltiger Speicherzugriff).
 * Fuehrt einen System-Reset durch.
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
 * - TIM1 laeuft mit 1MHz (1us pro Tick)
 * - Gueltige Pulse: RC_PULSE_MIN_VALID - RC_PULSE_MAX_VALID
 * - Bei gueltigem Puls wird rc_new_data = 1 gesetzt
 */
void EXTI7_0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
        /* Interrupt-Flag loeschen */
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
            /* Gueltig: RC_PULSE_MIN_VALID bis RC_PULSE_MAX_VALID us     */
            /* Bei gueltigem Puls: Daten uebernehmen und Flag setzen     */
            /*------------------------------------------------------------*/
            if (diff >= RC_PULSE_MIN_VALID && diff <= RC_PULSE_MAX_VALID)
            {
                rc_pulse_us = diff;
                rc_new_data = 1;
            }
        }
    }
}
