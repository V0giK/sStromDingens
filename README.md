# sStromDingens – RC-Signal zu PWM Konverter

---

## ✨ Projektübersicht

**sStromDingens** ist ein kompakter RC-Signal zu PWM Konverter für 1W LED-Dimmung mit Konstantstromversorgung. Das Projekt ermöglicht das Dimmen einer 1W LED mittels RC-Signal – ohne Vorwiderstand und mit konstanter Helligkeit unabhängig von der Akkuspannung.

```
RC-Signal (1-2ms) ──► CH32V003J4M6 ──► PWM ──► AL8862 ──► 1W LED
     (50Hz)              (RISC-V)        (Dimmen)  (Konstantstrom)
```

---

## 🚀 Features

- **CH32V003J4M6**: RISC-V Mikrocontroller (24MHz, 16KB Flash, 2KB RAM)
- **AL8862 LED-Treiber**: Konstantstrom-Versorgung ohne Vorwiderstand
- **Zwei Betriebsmodi**: Linear (proportionales Dimmen) und On/Off (Schwellwert-basiert)
- **Fail-Safe**: Konfigurierbares Verhalten bei Signalverlust
- **100Hz PWM**: Software-PWM mit 100 Stufen Aufloesung
- **Eingangsspannung**: 5V - 26V moeglich, AMS1117-3.3 begrenzt auf LiPo 2S-3S (max. 12.6V)
- **Kompaktes Design**: SOP-8 Package, minimaler Platzbedarf

---

## 📂 Projektstruktur

```
sStromDingens/
├── Hardware/               # KiCad Schaltung & Layout
│   ├── Documents/          # Datasheets
│   └── KiCad/
│       └── sStromDingens/
│           └── production/ # Fertigungsdaten (BOM, Gerber)
├── Software/
│   └── firmware/           # CH32V003 Firmware
│       ├── User/           # main.c, ch32v00x_it.c
│       ├── Core/           # RISC-V Core
│       ├── Peripheral/     # HAL Treiber
│       ├── Debug/          # Debug-Funktionen
│       ├── Startup/       # Startup-Code
│       └── Ld/            # Linker-Skript
├── .github/
│   └── prompts/            # AI-Prompts fuer Code-Review
├── AGENTS.md               # KI-Entwicklungsdokumentation
├── CHANGELOG.md            # Aenderungshistorie
├── LICENSE                 # GPLv3 Lizenz
└── README.md               # Dieses Dokument
```

---

## 🧱 Komponenten

| Komponente | Beschreibung |
|------------|--------------|
| AL8862SP-13 | LED-Treiber (Konstantstrom, Step-Down DC-DC) |
| CH32V003J4M6 | RISC-V Mikrocontroller (SOP-8, 24MHz) |
| AMS1117-3.3 | 3.3V Spannungsregler |
| 15uH Induktor | Fuer Step-Down Wandler |
| SMF3.3CA (x2) | TVS-Diode (Schutz) |
| SMAJ26CA | TVS-Diode (Schutz) |

---

## 🔌 Hardwareaufbau

### Pinbelegung

| Pin | Funktion |
|-----|----------|
| PC1 (Pin 5) | Mode-Jumper (GND = Linear, OFFEN = On/Off) |
| PC2 (Pin 6) | PWM-Ausgang (Software-PWM via TIM2) -> AL8862 CTRL |
| PC4 (Pin 7) | RC-Eingang (EXTI-Interrupt) |
| PD6 (Pin 1) | Fail-Safe-Jumper (GND = LED AN bei Signalverlust) |

### Timer-Nutzung

| Timer | Funktion |
|-------|----------|
| TIM1 | Zeitbasis fuer Pulsbreitenmessung (1us Aufloesung) |
| TIM2 | Software-PWM (10kHz Interrupt -> 100Hz PWM) |

---

## 💻 Software

### RC-Signal Spezifikation

| Parameter | Wert |
|-----------|------|
| Periode | 20ms (50Hz) |
| Pulsbreite | 1ms - 2ms |
| Timeout | 50ms ohne Signal |

### PWM-Ausgang

| Parameter | Wert |
|-----------|------|
| Frequenz | 100Hz (Software-PWM) |
| Spannungsbereich | 0V - 3.3V |
| Duty-Cycle | 0% - 100% |

### Betriebsmodi

| Modus | Aktivierung | Verhalten |
|-------|-------------|-----------|
| **Linear** | PC1 = GND | 1ms -> LED AUS, 2ms -> LED AN (proportionales Dimmen) |
| **On/Off** | PC1 = OFFEN | <1.5ms -> LED AUS, >=1.5ms -> LED AN |
| **Fail-Safe (AN)** | PD4 = GND + kein Signal | LED AN |
| **Fail-Safe (AUS)** | PD4 = OFFEN + kein Signal | LED AUS |

### Build

Mit MounRiver Studio:
```
Project -> Build Project (Strg+B)
```

---

## 📜 Lizenz

Dieses Projekt steht unter der [GNU General Public License v3.0](LICENSE). Jede Quellcodedatei enthaelt einen entsprechenden Lizenz-Header.

---

## 🤝 Mitmachen & Entwicklung

Pull Requests, Verbesserungsvorschlaege oder Erweiterungen sind willkommen. Bei Fragen oder Problemen bitte [Issues](https://github.com/V0giK/sStromDingens/issues) verwenden.

---

## 🌍 Autor / Projekt

Entwickelt von [V0giK](https://github.com/V0giK) fuer RC-LED-Steuerung.

---

> „Small chip, bright light – powered by RISC-V"
