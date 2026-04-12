# sStromDingens – RC-gesteuerter LED-Treiber (1W / 330mA)

[![License: GPLv3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Version: 2.0.0](https://img.shields.io/badge/Version-2.0.0-green.svg)](CHANGELOG.md)

**[English Version](README_EN.md)**

---

## Inhaltsangabe

- [Kurzbeschreibung](#kurzbeschreibung)
- [Betriebsmodi](#betriebsmodi)
- [Schnellstart](#schnellstart)
- [Hardware](#hardware)
- [Software](#software)
- [Projektstruktur](#projektstruktur)
- [Lizenz & Mitmachen](#lizenz--mitmachen)

---

## Kurzbeschreibung

RC-gesteuerter LED-Treiber für 1W LEDs (330mA) – klein, effizient, RISC-V-basiert.

```
RC-Signal ──► CH32V003 ──► AL8862 ──► 1W LED
```

**Vorteile:**

- Konstantstrom ohne Vorwiderstand
- Kompakte Platine für 1W LEDs (oder 3W mit Modifikation)
- Fail-Safe bei Signalverlust
- 5V–12.6V Eingangsspannung (LiPo 2S–3S); möglich bis 26V (6S, außerhalb AMS1117-Spezifikation)

**Ideal für:** RC-Fahrzeuge, Flugzeuge, Drohnen, Modellbau, Modellbahnen

---

## Betriebsmodi

### Dimmen / Ein-Aus

| Modus | Lötjumper DIM | Verhalten |
|-------|----------------|-----------|
| 🌗 **Dimmen** | Geschlossen | 1ms → LED AUS, 2ms → LED AN (linear dimmen) |
| 💡 **Ein/Aus** | Offen | <1.5ms → LED AUS, ≥1.5ms → LED AN |

### Fail-Safe

| Modus | Lötjumper ON | Verhalten |
|-------|--------------|-----------|
| **Fail-Safe AN** | Geschlossen | LED an bei Signalverlust (>50ms) |
| **Fail-Safe AUS** | Offen | LED aus bei Signalverlust (>50ms) |

**Standalone-Betrieb:** Ohne RC-Empfänger betreibbar. Lötjumper ON muss geschlossen sein – nach Timeout (50ms) geht die LED automatisch an.

---

## Schnellstart

### Was brauche ich?

| Komponente | Beschreibung | Hinweis |
|------------|--------------|---------|
| sStromDingens PCB | Hardware v2.0 | Gerber: `Hardware/KiCad/sStromDingens/production/` |
| 1W oder 3W LED | Beliebige Farbe | 3W benötigt zusätzlichen Widerstand |
| LiPo 2S-6S | 7.4V – 22.2V | 2S–3S empfohlen |
| RC-Empfänger | Beliebig | Optional bei ON-Jumper geschlossen |
| WCH-LinkE | Programmer | Für Firmware-Flash |

### In 3 Schritten zur LED-Beleuchtung

**1. Platine bestellen & bestücken**
   - Gerber-Datei: `sStromDingens_2.0.zip`
   - BOM: `bom.csv`
   - Nach Bestückungsdruck löten

**2. Firmware flashen**
   - WCH-LinkE anschließen (SWIO, GND, 3V3)
   - Firmware: `Software/firmware/firmware.hex`
   - Mit WCH-LinkUtility flashen

**3. Anschließen & Loslegen**

| Anschluss | Funktion |
|-----------|----------|
| VIN+ | LiPo Plus (5V–26V, max 6S) |
| VIN- | LiPo Minus (GND) |
| RC-IN | RC-Signal vom Empfänger |
| LED+ | LED Plus-Pol |
| LED- | LED Minus-Pol |

---

## Hardware

### Komponenten

| Bauteil | Beschreibung |
|---------|--------------|
| AL8862SP-13 | LED-Treiber (Konstantstrom, Step-Down) |
| CH32V003J4M6 | RISC-V Mikrocontroller (SOP-8, 24MHz) |
| AMS1117-3.3 | 3.3V Spannungsregulator |
| 15µH Induktor | Für Step-Down Wandler |
| SMF3.3CA (x2) | TVS-Diode (Überspannungsschutz) |
| SMAJ26CA | TVS-Diode (Eingangsschutz) |

### Pinbelegung CH32V003J4M6

| Pin | Funktion | Beschreibung |
|-----|----------|--------------|
| PC1 (Pin 5) | Lötjumper DIM (Mode) | Geschlossen = Linear, Offen = On/Off |
| PC2 (Pin 6) | PWM-Ausgang | → AL8862 CTRL |
| PC4 (Pin 7) | RC-Eingang | EXTI-Interrupt |
| PD6 (Pin 1) | Lötjumper ON (Fail-Safe) | Geschlossen = LED AN bei Signalverlust |

### Timer-Nutzung

| Timer | Funktion |
|-------|----------|
| TIM1 | Pulsbreitenmessung (1µs Auflösung) |
| TIM2 | Software-PWM (10kHz → 100Hz PWM) |

### 3W-LED Modifikation

Für 3W LEDs (ca. 650-700mA): Einen zusätzlichen **300mΩ Widerstand** parallel zum bestehenden R4 (300mΩ) löten (huckepack). Das halbiert den Gesamtwiderstand und verdoppelt den Strom.

### Bilder

| Top | PCB | Bottom |
|-----|-----|--------|
| ![Top](images/sStromDingens1_0_T.jpg) | ![PCB](images/sStromDingens1_0_PCB.jpg) | ![Bottom](images/sStromDingens1_0_B.jpg) |

---

## Software

### RC-Signal

| Parameter | Wert |
|-----------|------|
| Periode | 20ms (50Hz) |
| Pulsbreite | 1ms – 2ms |
| Timeout | 50ms ohne Signal |

### PWM-Ausgang

| Parameter | Wert |
|-----------|------|
| Frequenz | 100Hz |
| Spannung | 0V – 3.3V |
| Duty-Cycle | 0% – 100% (100 Stufen) |

### Firmware Build

**MounRiver Studio:**

```
Project → Build Project (Strg+B)
```

**CLI-Build:** Siehe [AGENTS.md](AGENTS.md) für Toolchain-Pfad, Compiler-Flags und Linker-Kommandos.

**Flashen:** WCH-LinkUtility – siehe [AGENTS.md](AGENTS.md) für Pfad.

---

## Projektstruktur

```
sStromDingens/
├── .github/          # GitHub-spezifische Konfiguration
├── Hardware/
│   ├── Documents/    # Datenblätter (AL8862, CH32V003)
│   └── KiCad/        # KiCad-Projekt, Gerber, BOM
├── Software/
│   └── firmware/     # Firmware (CH32V003)
│       ├── User/     # main.c, ch32v00x_it.c, debug.c
│       ├── Core/     # RISC-V Core
│       ├── Peripheral/ # HAL-Treiber
│       ├── Startup/  # Startup-Code
│       └── Ld/       # Linker-Skript
├── images/           # Bilder (Top, PCB, Bottom)
├── AGENTS.md         # KI-Dokumentation & Build-Referenz
├── CHANGELOG.md      # Änderungshistorie
├── LICENSE           # GNU GPLv3
├── README.md         # Deutsche Dokumentation
└── README_EN.md      # Englische Dokumentation
```

---

## Lizenz & Mitmachen

Pull Requests willkommen! Bei Fragen: [Issues](https://github.com/V0giK/sStromDingens/issues)

Lizenziert unter **[GNU GPLv3](LICENSE)**.

Änderungen: **[CHANGELOG](CHANGELOG.md)**

Entwickelt von [V0giK](https://github.com/V0giK).

---

> „Small chip, bright light – powered by RISC-V"
