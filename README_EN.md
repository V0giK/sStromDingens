# sStromDingens – RC-Controlled LED Driver (1W / 3W)

[![License: GPLv3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Version: 3.0.0](https://img.shields.io/badge/Version-3.0.0-green.svg)](CHANGELOG.md)

**[Deutsche Version](README.md)**

---

## Table of Contents

- [Overview](#overview)
- [Operating Modes](#operating-modes)
- [Quick Start](#quick-start)
- [Hardware](#hardware)
- [Software](#software)
- [Project Structure](#project-structure)
- [License & Contributing](#license--contributing)

---

## Overview

RC-controlled LED driver for 1W (330mA) and 3W (700mA) LEDs – small, efficient, RISC-V-based.

```
RC-Signal ──► CH32V003 ──► AL8862 ──► LED (1W or 3W)
```

**Benefits:**

- Constant current without resistor
- Compact board for 1W and 3W LEDs
- LED type selectable at compile-time (1W = 100%, 3W = 80% max duty-cycle)
- Fail-Safe on signal loss
- 5V–12.6V input voltage (LiPo 2S–3S); possible up to 16.8V (4S, outside AMS1117 spec)

**Ideal for:** RC vehicles, airplanes, drones, model building, model railroads

---

## Operating Modes

### Dimming / On-Off

| Mode | Solder Jumper DIM (PC1) | Behavior |
|------|-------------------------|----------|
| 🌗 **Dimming** | GND (closed) | 1ms → LED OFF, 2ms → LED ON (linear dim) |
| 💡 **On/Off** | Open | Hysteresis: ON at 1550µs, OFF below 1450µs |

### Fail-Safe

| Mode | Solder Jumper ON (PD6) | Behavior |
|------|--------------------------|----------|
| **Fail-Safe ON** | GND (closed) | LED on when signal lost (>50ms) |
| **Fail-Safe OFF** | Open | LED off when signal lost (>50ms) |

**Standalone operation:** Usable without RC receiver. Solder jumper ON must be closed – after timeout (50ms) the LED turns on automatically.

---

## Quick Start

### What do I need?

| Component | Description | Note |
|-----------|-------------|------|
| sStromDingens PCB | Hardware v2.0 | Gerber: `Hardware/KiCad/sStromDingens/production/` |
| 1W or 3W LED | Any color | 3W requires additional resistor |
| LiPo 2S-4S | 7.4V – 16.8V | 2S–3S recommended |
| RC Receiver | Any | Optional with ON jumper closed |
| WCH-LinkE | Programmer | For firmware flash |

### 3 Steps to LED Lighting

**1. Order & assemble PCB**
   - Gerber file: `sStromDingens_2.0.zip`
   - BOM: `bom.csv`
   - Solder according to silkscreen

**2. Flash firmware**
   - Connect WCH-LinkE (SWIO, GND, 3V3)
   - Firmware: `Software/firmware/firmware.hex`
   - Flash with WCH-LinkUtility

**3. Connect & Go**

| Connector | Function |
|-----------|----------|
| VIN+ | LiPo Plus (5V–16.8V, max 4S) |
| VIN- | LiPo Minus (GND) |
| RC-IN | RC signal from receiver |
| LED+ | LED positive |
| LED- | LED negative |

---

## Hardware

### Components

| Part | Description |
|------|-------------|
| AL8862SP-13 | LED driver (constant current, step-down) |
| CH32V003J4M6 | RISC-V microcontroller (SOP-8, 24MHz) |
| AMS1117-3.3 | 3.3V voltage regulator |
| 15µH inductor | For step-down converter |
| SMF3.3CA (x2) | TVS diode (overvoltage protection) |
| SMAJ26CA | TVS diode (input protection) |

### CH32V003J4M6 Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| PC1 (Pin 5) | Solder Jumper DIM (Mode) | Closed = Linear, Open = On/Off |
| PC2 (Pin 6) | PWM output | → AL8862 CTRL |
| PC4 (Pin 7) | RC input | EXTI interrupt |
| PD6 (Pin 1) | Solder Jumper ON (Fail-Safe) | Closed = LED ON on signal loss |

### Timer Usage

| Timer | Function |
|-------|----------|
| TIM1 | Pulse width measurement (1µs resolution) |
| TIM2 | Hardware PWM (2kHz on PC2 via TIM2_CH2) |

### 3W LED Modification

For 3W LEDs (approx. 650-700mA): Solder an additional **300mΩ resistor** in parallel to the existing R4 (300mΩ) (piggyback). This halves the total resistance and doubles the current.

> ⚠️ **Important:** With 3W LEDs the **AL8862 gets very hot**! Mandatory  
> **permanent additional cooling** required (e.g. heatsink, case fan,  
> thermally conductive connection to aluminum chassis).

### Images

| Top | PCB | Bottom |
|-----|-----|--------|
| ![Top](images/sStromDingens1_0_T.jpg) | ![PCB](images/sStromDingens1_0_PCB.jpg) | ![Bottom](images/sStromDingens1_0_B.jpg) |

---

## Software

### RC Signal

| Parameter | Value |
|-----------|-------|
| Period | 20ms (50Hz) |
| Pulse width | 1ms – 2ms |
| Timeout | 50ms without signal |

### PWM Output

| Parameter | Value |
|-----------|-------|
| Frequency | 2 kHz (Hardware PWM) |
| Voltage | 0 V – 3.3 V |
| Duty cycle | 0 % – 100 % (1W) or 0 % – 80 % (3W) |

> **LED type configuration:** In `Software/firmware/User/main.c`, enable one of the following defines:
> ```c
> #define LED_1W   // 330mA LED, max 100% duty-cycle
> // or
> #define LED_3W   // 700mA LED, max 80% duty-cycle
> ```

### Firmware Build

**MounRiver Studio:**

```
Project → Build Project (Ctrl+B)
```

**CLI Build:** See [AGENTS.md](AGENTS.md) for toolchain path, compiler flags, and linker commands.

**Flashing:** WCH-LinkUtility – see [AGENTS.md](AGENTS.md) for path.

---

## Project Structure

```
sStromDingens/
├── .github/          # GitHub-specific configuration
├── Hardware/
│   ├── Documents/    # Datasheets (AL8862, CH32V003)
│   └── KiCad/        # KiCad project, Gerber, BOM
├── Software/
│   ├── firmware/                   # Base firmware v3.0.0 (1W/3W, Hardware-PWM)
│   │   ├── User/     # main.c (LED selection), ch32v00x_it.c, debug.c
│   │   ├── Core/     # RISC-V Core
│   │   ├── Peripheral/ # HAL drivers
│   │   ├── Startup/  # Startup code
│   │   ├── Ld/       # Linker script
│   │   └── README.md # Base firmware documentation
│   └── firmware_700mA_afterburner/ # Afterburner v3.4.0 (3W LED, FX-Suite)
│       ├── User/     # main.c (Afterburner FX Engine)
│       ├── Core/
│       ├── Peripheral/
│       ├── Startup/
│       ├── Ld/
│       └── README.md # Afterburner documentation
├── images/           # Images (Top, PCB, Bottom)
├── AGENTS.md         # AI documentation & build reference
├── CHANGELOG.md      # Change history
├── LICENSE           # GNU GPLv3
├── README.md         # German documentation
└── README_EN.md      # English documentation
```

---

## License & Contributing

Pull requests welcome! Questions: [Issues](https://github.com/V0giK/sStromDingens/issues)

Licensed under **[GNU GPLv3](LICENSE)**.

Changes: **[CHANGELOG](CHANGELOG.md)**

Developed by [V0giK](https://github.com/V0giK).

---

> „Small chip, bright light – powered by RISC-V"
