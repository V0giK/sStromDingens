# sStromDingens – RC Afterburner Controller

Firmware fuer den CH32V003J4M6 (RISC-V) zur Steuerung einer Hochleistungs-LED ueber den AL8862 Konstantstromtreiber.

---

## Hardware

| Komponente | Pin | Funktion |
|:---|:---|:---|
| **RC Input** | PC4 (Pin 7) | RC-Empfaengersignal (1000–2000 µs) |
| **PWM Out** | PC2 (Pin 6) | TIM2_CH2 Hardware-PWM fuer AL8862 |
| **SWD** | PD1 (Pin 8) | WCH-LinkE Programmierung |

> **Hinweis:** PC1 und PA1/PD6 haben in dieser Firmware keine Funktion mehr.

---

## Firmware-Version

| Eigenschaft | Wert |
|-------------|------|
| **Version** | 3.4.1 |
| **PWM-Methode** | TIM2 Hardware-PWM @ 2 kHz |
| **RC-Zeitbasis** | TIM1 @ 1 MHz (1 µs/Tick) |
| **Framework** | WCH HAL (ch32v00x) |

---

## Features – Die volle FX-Suite

| Feature | Beschreibung |
|---------|-------------|
| **Hysterese** | Einschalten ab **1650 µs**, Ausschalten erst unter **1550 µs** – kein nerviges Flattern am Schwellwert |
| **Variabler Spool-Up** | Jeder Zündvorgang dauert zufällig **80–130 ms**, um mechanische Wiederholung zu vermeiden |
| **Ramp-Down** | Nach dem Flash sanftes Absenken auf den Sollwert in **80 ms** |
| **Vollgas-Burst** | Sprung auf >**1950 µs** → kurzer 20-ms-Blitz auf 100 % als "Ueberdruck"-Effekt |
| **Flame-Out** | Abruptes Gas-weg (von >1800 µs auf <1200 µs) → **150 ms** heftiges Flackern (±40 %) als "Flamme erstickt"-Simulation |
| **Cool-Down** | Sanftes Abklingen auf 0 % in **250 ms** – wie ein gluehender Turbinenkern |
| **Power-On Selftest** | LED geht nach Einschalten fuer **200 ms** auf **50 %** als Lebenszeichen |
| **Flicker (normal)** | Zufaelliges Rauschen ±15 % im Betrieb |
| **Heat-Shimmer** | Ab **1900 µs**: verstaerktes Rauschen ±25 % fuer Turbulenzen im Vollgasbereich |
| **Fail-Safe** | Bei Signalverlust (Timeout > 50 ms) sofort **LED AUS** |

---

## State-Machine

```
    Einschalten
        │
        ▼
┌─────────────────┐
│  POWER-ON TEST  │──200 ms @ 50%──►
└─────────────────┘                │
                                   ▼
┌─────────────────┐   Gas ≥1650 µs    ┌─────────────┐   zufaellig    ┌─────────────┐
│     IDLE        │ ────────────────► │  SPOOL_UP   │ ───80–130 ms──►│ RAMP_DOWN   │
│    (LED AUS)    │                   │ (Flash 100%)│                │(Flash→Soll) │
└─────────────────┘                   └─────────────┘                └──────┬──────┘
       ▲                        ▲                                          │
       │                        │                                          │ 80 ms
       │                        │                                          ▼
       │               Gas <1550 µs                               ┌─────────────┐
       │               oder abruptes Gas-weg                      │   RUNNING   │
       │               (von >1800 auf <1200)                      │  (Flicker)  │
       │                                                          └──────┬──────┘
       │                                                                 │
       │         Gas ≥1650 µs      ┌─────────────┐   Gas <1550 µs        │
       └───────────────────────────│   BURST     │ ◄─────────────────────┘
                                   │  (20 ms @   │   oder Flame-Out
                                   │   100%)     │   (von >1800 auf <1200)
                                   └──────┬──────┘
                                          │
                                          ▼
                                   ┌─────────────┐
                                   │  FLAMEOUT   │
                                   │ (150 ms wild│
                                   │  flackern)  │
                                   └──────┬──────┘
                                          │
                                          ▼
                                   ┌─────────────┐    fertig    ┌─────────────┐
                                   │  COOL_DOWN  │ ───────────► │    IDLE     │
                                   │ (250 ms auf │              │   (LED AUS) │
                                   │     0%)     │              └─────────────┘
                                   └─────────────┘
```

---

## Parameter & Konfiguration

Alle Parameter sind als `#define` in `User/main.c` direkt im Code konfigurierbar.

| Parameter | Standard | Beschreibung |
|:---|:---|:---|
| `THRESHOLD_ON` | 1650 µs | Einschaltschwelle |
| `THRESHOLD_OFF` | 1550 µs | Ausschaltschwelle |
| `PULSE_MIN` | 1000 µs | RC-Minimum (Leerlauf) |
| `PULSE_MAX` | 2000 µs | RC-Maximum (Vollgas) |
| `PWM_MAX_DUTY` | 60 % | Maximale Helligkeit (Standard, ca. 500mA). Alternativ: 80% (ca. 666mA) oder 100% (ca. 830mA) |
| `PWM_FREQ_HZ` | 2000 Hz | Hardware-PWM-Frequenz |
| `SPOOL_UP_MIN_MS` | 80 ms | Min. Zünd-Flash-Dauer |
| `SPOOL_UP_MAX_MS` | 130 ms | Max. Zünd-Flash-Dauer |
| `RAMP_DOWN_MS` | 80 ms | Rampe Flash → Sollwert |
| `COOL_DOWN_MS` | 250 ms | Rampe Sollwert → 0 % |
| `FLAMEOUT_MS` | 150 ms | Dauer des Flame-Out-Effekts |
| `BURST_MS` | 20 ms | Dauer des Vollgas-Bursts |
| `SELFTEST_MS` | 200 ms | Dauer des Power-On-Tests |
| `SELFTEST_DUTY` | 50 % | Helligkeit beim Selftest |
| `FLICKER_NORMAL` | ±15 % | Normales Flackern |
| `FLICKER_HEAT` | ±25 % | Heat-Shimmer-Flackern |
| `HEAT_THRESHOLD` | 1900 µs | Schwellwert fuer Heat-Shimmer |
| `RC_TIMEOUT_TICKS` | 500 | 50 ms Fail-Safe-Timeout |

---

## Pinbelegung CH32V003J4M6 (SOP-8)

| Pin | Bezeichnung | Funktion |
|:---:|:---|:---|
| 1 | PA1 / PD6 | *nicht verwendet* |
| 2 | VSS | GND |
| 3 | PA2 | *nicht verwendet* |
| 4 | VDD | 3.3 V |
| 5 | PC1 | *nicht verwendet* |
| **6** | **PC2** | **PWM-Ausgang (TIM2_CH2)** |
| **7** | **PC4** | **RC-Eingang** |
| 8 | PD1 / SWIO | Programmierung (WCH-LinkE) |

---

## Build & Flash

### Voraussetzungen
- MounRiver Studio oder RISC-V Embedded GCC Toolchain
- WCH-LinkE Programmieradapter

### CLI Build

**Compiler:**
```bash
riscv-none-embed-gcc.exe -march=rv32ec -mabi=ilp32e -mcmodel=medlow \
  -ffunction-sections -fdata-sections -Os -Wall \
  -IDebug -ICore -IUser -IPeripheral/inc \
  -c User/main.c -o obj/User/main.o
```

**Linken:**
```bash
riscv-none-embed-gcc.exe -march=rv32ec -mabi=ilp32e -mcmodel=medlow \
  -ffunction-sections -fdata-sections -Os -nostartfiles \
  -Xlinker --gc-sections -T Ld/Link.ld -o firmware.elf \
  [alle .o Dateien] -lprintf
```

**Hex-Datei erzeugen:**
```bash
riscv-none-embed-objcopy.exe -O ihex firmware.elf firmware.hex
```

### Flashen
- **GUI:** WCH-LinkUtility
- **Datei:** `firmware.hex`

---

## Dateistruktur

```
firmware_700mA_afterburner/
├── User/
│   ├── main.c              # Hauptlogik & State-Machine
│   ├── ch32v00x_it.c       # Interrupt-Handler (EXTI)
│   ├── debug.c / debug.h   # Delay-Funktionen
│   └── system_ch32v00x.c   # System-Init
├── Peripheral/
│   ├── inc/                # HAL-Header (ch32v00x_*.h)
│   └── src/                # HAL-Quellen (ch32v00x_*.c)
├── Core/                   # RISC-V Core
├── Startup/                # Startup-Code
├── Ld/
│   └── Link.ld             # Linker-Skript
├── firmware.hex            # Fertiges Flash-Image
└── README.md               # Diese Datei
```

---

## Changelog

### v3.4.1 (FX Edition)
- **Hinweis:** `PWM_MAX_DUTY` Standard jetzt 60% (ca. 500mA), konfigurierbar auf 80% (666mA) oder 100% (830mA)

### v3.4.0 (FX Edition)
- **Neu:** Power-On Selftest
- **Neu:** Hysterese (EIN/AUS getrennte Schwellwerte)
- **Neu:** Variabler Spool-Up (zufällige Flash-Dauer)
- **Neu:** Vollgas-Burst (20 ms Blitz bei >1950 µs)
- **Neu:** Flame-Out (heftiges Flackern bei abruptem Gas-weg)
- **Behalten:** Ramp-Down, Cool-Down, Flicker, Heat-Shimmer

### v3.3.0
- **Neu:** Ramp-Down (sanftes Absenken nach Flash)
- **Neu:** Cool-Down (sanftes Abklingen beim Ausschalten)

### v3.2.0
- **Fix:** Hardware-PWM auf PC2 mit korrektem TIM2-Remap (`GPIO_PartialRemap1_TIM2`)
- **Änderung:** TIM2_CH2 @ 2 kHz statt Software-PWM

### v3.0.0
- **Neu:** Afterburner-State-Machine
- **Neu:** Spool-Up, Flicker, Heat-Shimmer

---

## Lizenz

GNU General Public License v3.0 (GPLv3)

Copyright (C) 2026 V0giK

---

## Autor

**V0giK** – sStromDingens Projekt
