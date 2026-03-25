# AGENTS.md - sStromDingens

Dieses Dokument enthält wichtige Informationen für die KI-gestützte Weiterentwicklung des Projekts.

---

## Projektübersicht

| Eigenschaft | Wert |
|-------------|------|
| **Name** | sStromDingens |
| **Beschreibung** | RC-Signal zu PWM Konverter für 1W LED-Dimmung |
| **Hardware Version** | 2.0 |
| **Firmware Version** | 2.0.0 |
| **Status** | Hardware fertig, Firmware funktionsfähig (v1.2.0) |
| **Lizenz** | GPLv3 |

### Versionsschema

Das Projekt verwendet das Versionsschema **X.Y.Z**:
- **X**: Hardware-Version
- **Y**: Software-Hauptversion  
- **Z**: Software-Bugfix

Beispiel: Version **1.2.3** = Hardware v1, Software v2.3

---

## Hardware

### Schaltung

```
Eingang (5V-60V LiPo 2S-3S)
    │
    ├─► AMS1117-3.3V ──► CH32V003J4M6 (MCU)
    │
    └─► AL8862 (Konstantstrom-LED-Treiber)
              │
              ▼
         1W LED (ohne Vorwiderstand)
```

### Verwendete Bauteile

| Referenz | Bauteil | Funktion |
|----------|---------|----------|
| U1 | AL8862SP-13 | LED-Treiber ( Konstantstrom ) |
| U2 | CH32V003J4M6 | RISC-V Mikrocontroller |
| U3 | AMS1117-3.3 | 3.3V Spannungsregler |
| L1 | 15uH SMD | Induktor für Step-Down |
| D1, D7 | SS24 | Gleichrichter |
| D3, D6 | SMF3.3CA | TVS-Diode (Schutz) |
| D8 | SMAJ26CA | TVS-Diode (Schutz) |
| C4 | 22uF 10V | Eingangs-Elko |
| C5 | 10uF 25V | Ausgangs-Elko |

### Wichtige Datasheets

- [AL8862 Datasheet](Hardware/Documents/AL8862_C526360.pdf)
- [CH32V003 Datasheet](Hardware/Documents/ch32v003.pdf)
- [CH32V003J4M6 Pinout](Hardware/Documents/CH32V003J4M6_PinOut.txt)

### KiCad Projekt

- **Pfad**: `Hardware/KiCad/sStromDingens/`
- **BOM**: `Hardware/KiCad/sStromDingens/production/bom.csv`

---

## Software / Firmware

### Microcontroller

| Eigenschaft | Wert |
|-------------|------|
| **MCU** | CH32V003J4M6 (SOP-8) |
| **Architektur** | RISC-V RV32EC |
| **Flash** | 16KB |
| **RAM** | 2KB |
| **Takt** | 24MHz (intern) |

### Toolchain

| Komponente | Tool |
|------------|------|
| **Compiler** | riscv-none-embed-gcc |
| **IDE** | MounRiver Studio |
| **Programmer** | WCH-LinkE |

### Pinbelegung CH32V003J4M6

| Pin | Funktion | Beschreibung |
|-----|----------|---------------|
| 5 (PC1) | Mode-Jumper | GND = Linear, OFFEN = On/Off |
| 6 (PC2) | PWM-Ausgang | Software-PWM (TIM2) → AL8862 CTRL |
| 7 (PC4) | RC-Eingang | EXTI-Interrupt für Pulsbreitenmessung |
| 1 (PD6) | Fail-Safe | GND = LED AN bei Signalverlust |

### RC-Signal Spezifikation

| Parameter | Wert |
|-----------|------|
| **Eingang** | Standard RC-Servo-Signal |
| **Periode** | 20ms (50Hz) |
| **Pulsbreite** | 1ms - 2ms |
| **Timeout** | 50ms ohne Signal |

### Betriebsmodi

| Modus | Aktivierung | Verhalten |
|-------|-------------|-----------|
| Linear | PC1 = GND | 1ms → LED AUS, 2ms → LED AN (proportionales Dimmen) |
| On/Off | PC1 = OFFEN | <1.5ms → LED AUS, ≥1.5ms → LED AN |
| Fail-Safe (AN) | PD4 = GND + Timeout | LED AN |
| Fail-Safe (AUS) | PD4 = OFFEN + Timeout | LED AUS |

### PWM-Ausgang für AL8862

| Parameter | Wert |
|-----------|------|
| **Frequenz** | ~100Hz (Software-PWM) |
| **Spannungsbereich** | 0V - 3.3V |
| **Duty-Cycle** | 0% - 100% |

### Timer-Nutzung

| Timer | Funktion | Beschreibung |
|-------|----------|-------------|
| TIM1 | Zeitbasis | 1µs Auflösung für Pulsbreitenmessung |
| TIM2 | Software-PWM | 10kHz Interrupt → 100Hz PWM (100 Stufen) |

---

## Build-Prozess

### MounRiver Studio

1. Projekt öffnen: `Software/firmware/firmware.project`
2. Build: `Project → Build Project` (Strg+B)
3. Flashen: `Tools → WCH-LinkUtility`

### CLI Build (Terminal)

**Compiler:**
```
riscv-none-embed-gcc -march=rv32ec -mabi=ilp32e -mcmodel=medlow -ffunction-sections -fdata-sections -Os -Wall
```

**Toolchain-Pfad:**
```
C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH\Toolchain\RISC-V Embedded GCC\bin\
```

**Beispiel komplett (im firmware-Ordner):**
```
"C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH\Toolchain\RISC-V Embedded GCC\bin\riscv-none-embed-gcc.exe" -march=rv32ec -mabi=ilp32e -mcmodel=medlow -ffunction-sections -fdata-sections -Os -Wall -IDebug -ICore -IUser -IPeripheral/inc -c User/main.c -o obj/main.o
...
```

**Linker:**
```
riscv-none-embed-gcc -march=rv32ec -mabi=ilp32e -mcmodel=medlow -ffunction-sections -fdata-sections -Os -nostartfiles -Xlinker --gc-sections -T Ld/Link.ld -o firmware.elf [object files] -lprintf -specs=nosys.specs -specs=nano.specs
```

### Flashen

**WCH-LinkUtility CLI:**
```
"C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH\Others\SWDTool\default\WCH-LinkUtility.exe"
```
(Öffnet GUI - Datei auswählen und flashen)

### Kompiler-Flags

```
-mcpu=rv32ec
-mabi=ilp32e
-march=rv32ec
-mcmodel=medlow
-ffunction-sections
-fdata-sections
```

---

## Projektstruktur

```
sStromDingens/
├── AGENTS.md           # Diese Datei
├── README.md           # Benutzer-Doku
├── CHANGELOG.md        # Aenderungshistorie
├── LICENSE             # GPLv3 Lizenz
├── Hardware/
│   ├── Documents/      # Datasheets
│   └── KiCad/
│       └── sStromDingens/
│           ├── production/    # Fertigungsdaten
│           └── *.kicad_*      # KiCad Projekt
└── Software/
    └── firmware/
        ├── User/       # Anwendungscode
        ├── Core/       # RISC-V Core
        ├── Peripheral/ # HAL Treiber
        ├── Debug/      # Debug-Funktionen
        ├── Startup/    # Startup-Code
        └── Ld/         # Linker-Skript
```

---

## Coding-Standards

- **Sprache**: C (GNU99)
- **Einrückung**: 4 Leerzeichen
- **Kommentare**: Deutsch
- **Benennung**: `snake_case` für Variablen, `UPPER_SNAKE` für Defines
- **Includes**: Relative Pfade mit `"..."` (nicht `<...>`)

