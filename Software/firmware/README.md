# sStromDingens – Basis-Firmware

RC-gesteuerter LED-Treiber für 1W (330mA) bis 3W (830mA) LEDs, konfigurierbar über Compile-Time Defines.

---

## Hardware

| Komponente | Pin | Funktion |
|:---|:---|:---|
| **RC Input** | PC4 (Pin 7) | RC-Empfaengersignal (1000–2000 µs) |
| **PWM Out**  | PC2 (Pin 6) | TIM2_CH2 Hardware-PWM für AL8862 |
| **Mode-Jumper** | PC1 (Pin 5) | GND = Linear, OFFEN = On/Off |
| **Fail-Safe** | PD6 (Pin 1) | GND = LED AN bei Signalverlust |
| **SWD** | PD1 (Pin 8) | WCH-LinkE Programmierung |

---

## Firmware-Version

| Eigenschaft | Wert |
|-------------|------|
| **Version** | 3.0.1 |
| **PWM-Methode** | TIM2 Hardware-PWM @ 2 kHz |
| **RC-Zeitbasis** | TIM1 @ 1 MHz (1 µs/Tick) |
| **Framework** | WCH HAL (ch32v00x) |

---

## LED-Typ auswählen

In `User/main.c` einen der folgenden Defines aktivieren (nur eine gleichzeitig):

```c
#define LED_1W   /* 330mA LED, max 100% Duty-Cycle — Original-Hardware */
// oder
#define LED_3W   /* 666mA LED, max 80% Duty-Cycle  — modifizierte Hardware */
// oder
#define LED_500  /* 500mA LED, max 60% Duty-Cycle  — modifizierte Hardware */
// oder
#define LED_666  /* 666mA LED, max 80% Duty-Cycle  — modifizierte Hardware */
// oder
#define LED_830  /* 830mA LED, max 100% Duty-Cycle — modifizierte Hardware */
```

| LED-Typ | Max. Strom | PWM_MAX_DUTY | Hardware |
|---------|-----------|--------------|----------|
| LED_1W (330mA) | 330 mA | 100 % | Original-Hardware |
| LED_3W (666mA) | 666 mA | 80 % | Modifiziert (2x300 mOhm parallel) |
| LED_500 | 500 mA | 60 % | Modifiziert (2x300 mOhm parallel) |
| LED_666 | 666 mA | 80 % | Modifiziert (2x300 mOhm parallel) |
| LED_830 | 830 mA | 100 % | Modifiziert (2x300 mOhm parallel) |

> **Achtung:** Die modifizierte Hardware (zusätzlicher 300mOhm parallel zu R4) erhöht den LED-Strom signifikant. Allein über `PWM_MAX_DUTY` wird der Strom begrenzt — dennoch wird der AL8862 bei hohen Leistungen sehr heiss. Zwingend eine **dauerhafte zusätzliche Kühlung** erforderlich (z.B. Kühlkörper, Gehäuselüfter).

---

## Betriebsmodi

| Modus | Jumper PC1 | Verhalten |
|-------|-----------|-----------|
| **Linear** | GND | Proportionales Dimmen: 1000 µs → AUS, 2000 µs → max |
| **On/Off** | OFFEN | Hysterese: EIN ab 1550 µs, AUS erst unter 1450 µs |

### Hysterese im On/Off-Modus

```
1550 µs ──┬── EIN (onoff_state = 1)
          │
    Totzone │   Zustand bleibt erhalten
          │
1450 µs ──┴── AUS (onoff_state = 0)
```

Das verhindert Flattern am Schwellwert bei leicht schwankendem RC-Signal.

### Fail-Safe

Bei Signalverlust (> 50 ms Timeout):
- **PD6 = GND** → LED geht auf Maximum
- **PD6 = OFFEN** → LED geht AUS

---

## Pinbelegung CH32V003J4M6 (SOP-8)

| Pin | Bezeichnung | Funktion |
|:---:|:---|:---|
| **1** | **PD6** | **Fail-Safe-Jumper** |
| 2 | VSS | GND |
| 3 | PA2 | *nicht verwendet* |
| 4 | VDD | 3.3 V |
| **5** | **PC1** | **Mode-Jumper** |
| **6** | **PC2** | **PWM-Ausgang (TIM2_CH2)** |
| **7** | **PC4** | **RC-Eingang** |
| 8 | PD1 / SWIO | Programmierung (WCH-LinkE) |

---

## Build & Flash

### Voraussetzungen
- MounRiver Studio oder RISC-V Embedded GCC Toolchain
- WCH-LinkE Programmieradapter

### LED-Typ konfigurieren

In `User/main.c` oberste Zeile anpassen:

```c
#define LED_1W   // Original-Hardware, 330mA
// oder
#define LED_3W   // Modifizierte Hardware, 666mA
// oder
#define LED_500  // Modifizierte Hardware, 500mA
// oder
#define LED_666  // Modifizierte Hardware, 666mA
// oder
#define LED_830  // Modifizierte Hardware, 830mA
```

### CLI Build

**Compiler:**
```bash
riscv-none-embed-gcc.exe -march=rv32ec -mabi=ilp32e -mcmodel=medlow \
  -ffunction-sections -fdata-sections -Os -Wall \
  -IDebug -ICore -IUser -IPeripheral/inc \
  -c User/main.c -o User/main.o
```

**Linken:**
```bash
riscv-none-embed-gcc.exe -march=rv32ec -mabi=ilp32e -mcmodel=medlow \
  -ffunction-sections -fdata-sections -Os -nostartfiles \
  -Xlinker --gc-sections -T Ld/Link.ld -o firmware.elf [object files]
```

**Hex-Datei erzeugen:**
```bash
riscv-none-embed-objcopy.exe -O ihex firmware.elf firmware.hex
```

### Flashen
- **GUI:** WCH-LinkUtility
- **Datei:** `firmware.hex`

---

## Changelog

### v3.0.1
- **Neu:** Fuenf Compile-time LED-Varianten: `LED_1W`, `LED_3W`, `LED_500`, `LED_666`, `LED_830`
- **Hinweis:** `LED_1W` nutzt Original-Hardware; alle anderen erfordern modifizierte Hardware (2x300 mOhm parallel)

### v3.0.0
- **Neu:** Hardware-PWM auf PC2 via TIM2_CH2 (2 kHz)
- **Neu:** Compile-time LED-Auswahl (`LED_1W` / `LED_3W`)
- **Neu:** Hysterese im On/Off-Modus (1450/1550 µs)
- **Entfernt:** Software-PWM (100 Hz, TIM2-ISR)

### v2.0.0
- **Letzte Version mit Software-PWM**

---

## Lizenz

GNU General Public License v3.0 (GPLv3)

Copyright (C) 2026 V0giK
