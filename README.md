# sStromDingens

RC-Signal zu PWM Konverter für 1W LED-Dimmung mit Konstantstromversorgung.

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
| PC4 | RC-Eingang (TIM1_CH4) |
| PC2 | PWM-Ausgang (TIM1_CH2) → AL8862 CTRL |
| PC1 | Mode-Jumper (GND = Linear) |
| PD4 | Fail-Safe-Jumper (GND = 100%) |

## Software

### RC-Signal Spezifikation

| Parameter | Wert |
|-----------|------|
| Periode | 20ms (50Hz) |
| Pulsbreite | 1ms - 2ms |

### Betriebsmodi

| Modus | Aktivierung | Verhalten |
|-------|-------------|-----------|
| **Linear** | PC1 = GND | 1ms → 0%, 2ms → 100% |
| **On/Off** | PC1 = OFFEN | <1.5ms = Aus, ≥1.5ms = An |
| **Fail-Safe (100%)** | PD4 = GND + kein Signal | Ausgang = 100% |
| **Fail-Safe (0%)** | PD4 = OFFEN + kein Signal | Ausgang = 0% |

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
```

## Lizenz

MIT License - see LICENSE file.
