# sStromDingens

RC-Signal zu PWM Konverter für 1W LED-Dimmung mit Konstantstromversorgung.

**Firmware Version:** v1.1.0

## Überblick

Dieses Projekt ermöglicht das Dimmen einer **1W LED** mittels RC-Signal (z.B. aus einem RC-Empfänger oder Fernsteuerung). Die LED wird **ohne Vorwiderstand** betrieben - die Helligkeit ist **unabhängig von der Akkuspannung**.

## Funktion

```
RC-Signal (1-2ms) ──► CH32V003J4M6 ──► PWM ──► AL8862 ──► 1W LED
     (50Hz)              (RISC-V)         (Dimmen)  ( Konstantstrom )
```

### Konstantstrom-LED-Treiber (AL8862)

Der AL8862 ist ein Step-Down DC-DC Konverter, der LEDs mit konstantem Strom versorgt:
- **Eingangsspannung**: 5V - 60V
- **Ausgangsstrom**: Bis zu 1A (einstellbar)
- **Effizienz**: Bis zu 97%
- **PWM-Dimming**: 0% - 100% über Steuersignal

Durch den Konstantstrombetrieb ist kein Vorwiderstand nötig und die Helligkeit bleibt konstant, egal ob der Akku voll oder fast leer ist.

## Hardware

### Stückliste (BOM)

Siehe: `Hardware/KiCad/sStromDingens/production/bom.csv`

| Bauteil | Funktion |
|---------|----------|
| AL8862 | LED-Treiber (Konstantstrom) |
| CH32V003J4M6 | RISC-V Mikrocontroller |
| AMS1117-3.3 | 3.3V Spannungsregler |
| 15uH Induktor | Für Step-Down Wandler |

### Pinbelegung

| Pin | Funktion |
|-----|----------|
| PC4 | RC-Eingang (TIM1_CH4 Input-Capture) |
| PC2 | PWM-Ausgang (Software-PWM via TIM2) → AL8862 CTRL |
| PC1 | Mode-Jumper (GND = Linear, OFFEN = On/Off) |
| PD4 | Fail-Safe-Jumper (GND = 100% bei Signalverlust) |

## Software

### RC-Signal Spezifikation

| Parameter | Wert |
|-----------|------|
| Periode | 20ms (50Hz) |
| Pulsbreite | 1ms - 2ms |
| Timeout | 50ms ohne Signal |

### PWM-Ausgang

| Parameter | Wert |
|-----------|------|
| Frequenz | ~100Hz (Software-PWM) |
| Spannungsbereich | 0V - 3.3V |
| Duty-Cycle | 0% - 100% |

### Betriebsmodi

| Modus | Aktivierung | Verhalten |
|-------|-------------|-----------|
| **Linear** | PC1 = GND | 1ms → 0%, 2ms → 100% |
| **On/Off** | PC1 = OFFEN | <1.5ms = Aus, ≥1.5ms = An |
| **Fail-Safe (100%)** | PD4 = GND + kein Signal | Ausgang = 100% |
| **Fail-Safe (0%)** | PD4 = OFFEN + kein Signal | Ausgang = 0% |

### Timer-Nutzung

| Timer | Funktion |
|-------|----------|
| TIM1 | RC-Input (Input-Capture an CH4) |
| TIM2 | Software-PWM (~10kHz Interrupt) |

### Build

Mit MounRiver Studio:
```
Project → Build Project (Strg+B)
```

## Projektstruktur

```
sStromDingens/
├── Hardware/           # KiCad Schaltung & Layout
│   └── KiCad/
└── Software/
    └── firmware/       # CH32V003 Firmware
        ├── User/       # main.c, ch32v00x_it.c
        ├── Core/       # RISC-V Core
        ├── Peripheral/ # HAL Treiber
        ├── Debug/      # Debug-Funktionen
        ├── Startup/    # Startup-Code
        └── Ld/         # Linker-Skript
```

## Lizenz

MIT License - see LICENSE file.
