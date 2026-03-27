# sStromDingens – Simple 1W LED Lighting via RC Signal

**[Deutsche Version](README.md)**

---

## 🎯 Purpose

**One board – two options:**

| Mode | Solder Jumper DIM | Application |
|------|-------------------|-------------|
| 🌗 **Dimming** | Closed | Headlights, dimmable lamps |
| 💡 **On/Off** | Open | Tail lights, position lights |

**Benefits:**
- Constant current without resistor
- Compact board for 1W LEDs (or 3W with modification)
- Fail-Safe on signal loss
- 5V–12.6V input voltage (LiPo 2S–3S); possible up to 26V (6S, outside AMS1117 spec)

**Ideal for:** RC vehicles, airplanes, drones, model building, model railroads

```
RC-Signal ──► CH32V003 ──► AL8862 ──► 1W LED
               (Dim or On/Off via Jumper)
```

---

## ⚡ Quick Start

### What do I need?

| Component | Description | Note |
|-----------|-------------|-------|
| sStromDingens PCB | Hardware v2.0 | Gerber: `Hardware/KiCad/sStromDingens/production/` |
| 1W or 3W LED | Any color | 3W requires additional resistor |
| LiPo 2S-6S | 7.4V – 22.2V | 2S–3S recommended |
| RC Receiver | Any | Standard RC setup |
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
| VIN+ | LiPo Plus (5V–26V, max 6S) |
| VIN- | LiPo Minus (GND) |
| RC-IN | RC signal from receiver |
| LED+ | LED positive |
| LED- | LED negative |

### 3W-LED Modification

For 3W LEDs (approx. 650-700mA): Solder an additional **300mΩ resistor** in parallel to the existing R4 (300mΩ) (piggyback). This halves the total resistance and doubles the current.

---

## 🔧 Hardware

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
| TIM2 | Software PWM (10kHz → 100Hz PWM) |

### Images

| Top | PCB | Bottom |
|-----|-----|--------|
| ![Top](images/sStromDingens1_0_T.jpg) | ![PCB](images/sStromDingens1_0_PCB.jpg) | ![Bottom](images/sStromDingens1_0_B.jpg) |

---

## 💻 Software

### RC Signal

| Parameter | Value |
|-----------|-------|
| Period | 20ms (50Hz) |
| Pulse width | 1ms – 2ms |
| Timeout | 50ms without signal |

### PWM Output

| Parameter | Value |
|-----------|-------|
| Frequency | 100Hz |
| Voltage | 0V – 3.3V |
| Duty cycle | 0% – 100% (100 steps) |

### Operating Modes

| Mode | Activation | Behavior |
|------|------------|----------|
| **Linear** | DIM closed | 1ms → OFF, 2ms → ON (dim) |
| **On/Off** | DIM open | <1.5ms → OFF, ≥1.5ms → ON |
| **Fail-Safe ON** | ON closed + Timeout | LED on |
| **Fail-Safe OFF** | ON open + Timeout | LED off |

### Firmware Build

```
MounRiver Studio → Project → Build Project (Ctrl+B)
```

---

## 📂 Project Structure

```
sStromDingens/
├── Hardware/     # KiCad, Gerber, BOM
├── Software/     # Firmware (CH32V003)
├── images/       # Images
├── AGENTS.md     # AI documentation
└── CHANGELOG.md  # Change history
```

---

## 🤝 Contributing & License

Pull requests welcome! Questions: [Issues](https://github.com/V0giK/sStromDingens/issues)

Licensed under **[GNU GPLv3](LICENSE)**.

Developed by [V0giK](https://github.com/V0giK).

---

> „Small chip, bright light – powered by RISC-V"
