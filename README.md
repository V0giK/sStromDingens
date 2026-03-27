# sStromDingens – Einfache 1W LED-Beleuchtung per RC-Signal

---

## 🎯 Sinn & Zweck

**Eine Platine – zwei Möglichkeiten:**

| Modus | Jumper PC1 | Anwendung |
|-------|------------|-----------|
| 🌗 **Dimmen** | Geschlossen | Scheinwerfer, dimmbare Lampen |
| 💡 **Ein/Aus** | Offen | Rücklichter, Positionslichter |

**Vorteile:**
- Konstantstrom ohne Vorwiderstand
- Kompakte Platine für 1W LEDs (oder 3W mit Modifikation)
- Fail-Safe bei Signalverlust
- 5V–12.6V Eingangsspannung (LiPo 2S–3S)

**Ideal für:** RC-Fahrzeuge, Flugzeuge, Drohnen, Modellbau, Modellbahnen

```
RC-Signal ──► CH32V003 ──► AL8862 ──► 1W LED
               (Dimmen oder Ein/Aus per Jumper)
```

---

## ⚡ Schnellstart

### Was brauche ich?

| Komponente | Beschreibung | Hinweis |
|------------|--------------|---------|
| sStromDingens PCB | Hardware v2.0 | Gerber: `Hardware/KiCad/sStromDingens/production/` |
| 1W oder 3W LED | Beliebige Farbe | 3W benötigt zusätzlichen Widerstand |
| LiPo 2S-3S | 7.4V – 12.6V | Standard RC-Akku |
| RC-Empfänger | Beliebig | Standard RC-Setup |
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
| VIN+ | LiPo Plus (5V–12.6V) |
| VIN- | LiPo Minus (GND) |
| RC-IN | RC-Signal vom Empfänger |
| LED+ | LED Plus-Pol |
| LED- | LED Minus-Pol |

### 3W-LED Modifikation

Für 3W LEDs (ca. 650-700mA): Einen zusätzlichen **300mΩ Widerstand** parallel zum bestehenden R4 (300mΩ) löten (huckepack). Das halbiert den Gesamtwiderstand und verdoppelt den Strom.

---

## 🔧 Hardware

### Komponenten

| Bauteil | Beschreibung |
|---------|--------------|
| AL8862SP-13 | LED-Treiber (Konstantstrom, Step-Down) |
| CH32V003J4M6 | RISC-V Mikrocontroller (SOP-8, 24MHz) |
| AMS1117-3.3 | 3.3V Spannungsregler |
| 15µH Induktor | Für Step-Down Wandler |
| SMF3.3CA (x2) | TVS-Diode (Überspannungsschutz) |
| SMAJ26CA | TVS-Diode (Eingangsschutz) |

### Pinbelegung CH32V003J4M6

| Pin | Funktion | Beschreibung |
|-----|----------|--------------|
| PC1 (Pin 5) | Mode-Jumper | GND = Linear, OFFEN = On/Off |
| PC2 (Pin 6) | PWM-Ausgang | → AL8862 CTRL |
| PC4 (Pin 7) | RC-Eingang | EXTI-Interrupt |
| PD6 (Pin 1) | Fail-Safe | GND = LED AN bei Signalverlust |

### Timer-Nutzung

| Timer | Funktion |
|-------|----------|
| TIM1 | Pulsbreitenmessung (1µs Auflösung) |
| TIM2 | Software-PWM (10kHz → 100Hz PWM) |

### Bilder

| Top | PCB | Bottom |
|-----|-----|--------|
| ![Top](images/sStromDingens1_0_T.jpg) | ![PCB](images/sStromDingens1_0_PCB.jpg) | ![Bottom](images/sStromDingens1_0_B.jpg) |

---

## 💻 Software

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

### Betriebsmodi

| Modus | Aktivierung | Verhalten |
|-------|-------------|-----------|
| **Linear** | PC1 = GND | 1ms → AUS, 2ms → AN (dimmen) |
| **On/Off** | PC1 = OFFEN | <1.5ms → AUS, ≥1.5ms → AN |
| **Fail-Safe AN** | PD6 = GND + Timeout | LED an |
| **Fail-Safe AUS** | PD6 = OFFEN + Timeout | LED aus |

### Firmware Build

```
MounRiver Studio → Project → Build Project (Strg+B)
```

---

## 📂 Projektstruktur

```
sStromDingens/
├── Hardware/     # KiCad, Gerber, BOM
├── Software/     # Firmware (CH32V003)
├── images/       # Bilder
├── AGENTS.md     # KI-Dokumentation
└── CHANGELOG.md  # Änderungshistorie
```

---

## 🤝 Mitmachen & Lizenz

Pull Requests willkommen! Bei Fragen: [Issues](https://github.com/V0giK/sStromDingens/issues)

Lizenziert unter **[GNU GPLv3](LICENSE)**.

Entwickelt von [V0kiK](https://github.com/V0giK).

---

> „Small chip, bright light – powered by RISC-V"
