# AGENTS.md - sStromDingens

Dieses Dokument enthält wichtige Informationen für die KI-gestützte Weiterentwicklung des Projekts.

---

## Projektübersicht

| Eigenschaft | Wert |
|-------------|------|
| **Name** | sStromDingens |
| **Beschreibung** | RC-gesteuerter LED-Treiber für 1W LEDs (330mA) |
| **Hardware Version** | 2.0 |
| **Firmware Version** | 2.0.0 |
| **MCU** | CH32V003J4M6 (RISC-V RV32EC) |
| **Framework** | WCH HAL (ch32v00x) |
| **Status** | Hardware fertig, Firmware fertig (v2.0.0) |
| **Lizenz** | GPLv3 |

### Versionsschema

Das Projekt verwendet das Versionsschema **X.Y.Z**:
- **X**: Hardware-Version
- **Y**: Software-Hauptversion
- **Z**: Software-Bugfix

Beispiel: Version **1.2.3** = Hardware v1, Software v2.3

---

## Build-Kommandos

### MounRiver Studio

1. Projekt öffnen: `Software/firmware/firmware.project`
2. Build: `Project → Build Project` (Strg+B)
3. Flashen: `Tools → WCH-LinkUtility`

### CLI Build (Terminal)

**Toolchain-Pfad:**
```
C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH\Toolchain\RISC-V Embedded GCC\bin\
```

**Kompiler-Flags:**
```
-march=rv32ec -mabi=ilp32e -mcmodel=medlow -ffunction-sections -fdata-sections -Os -Wall
```

**Vollständiger Build (im firmware-Ordner):**
```bash
# Kompilieren
riscv-none-embed-gcc.exe -march=rv32ec -mabi=ilp32e -mcmodel=medlow \
  -ffunction-sections -fdata-sections -Os -Wall \
  -IDebug -ICore -IUser -IPeripheral/inc \
  -c User/main.c -o obj/main.o

# Linken
riscv-none-embed-gcc.exe -march=rv32ec -mabi=ilp32e -mcmodel=medlow \
  -ffunction-sections -fdata-sections -Os -nostartfiles \
  -Xlinker --gc-sections -T Ld/Link.ld -o firmware.elf [object files] \
  -lprintf -specs=nosys.specs -specs=nano.specs
```

### Flashen

**GUI:** WCH-LinkUtility
```
C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH\Others\SWDTool\default\WCH-LinkUtility.exe
```

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

## Coding-Standards

### Sprache & Format

| Regel | Wert |
|-------|------|
| **Sprache** | C (GNU99) |
| **Einrückung** | 4 Leerzeichen (keine Tabs) |
| **Kommentare** | Deutsch |
| **max. Zeilenlänge** | 100 Zeichen |

### Includes (Reihenfolge)

```c
// 1. Standard-Bibliotheken
#include <stdint.h>

// 2. CMSIS / Vendor-Headers
#include <ch32v00x.h>

// 3. Peripherie-Treiber
#include "ch32v00x_exti.h"

// 4. Lokale Projekt-Headers
#include "debug.h"
#include "ch32v00x_it.h"
```

**Regel:** Projekt-Headers mit `"..."` inkludieren, nicht `<...>`.

### Typen

| Typ | Verwendung |
|-----|------------|
| `uint8_t`, `uint16_t`, `uint32_t` | Unsigned Integer |
| `int16_t` | Signed Integer |
| `volatile uint8_t` | ISR-geteilte Variablen |
| `volatile uint16_t` | ISR-geteilte Variablen (Pulsbreite) |

**Regel:** NIEMALS primitive Typen wie `int`, `long` verwenden (plattformabhängig).

### Benennung

| Typ | Konvention | Beispiel |
|-----|------------|----------|
| Defines | UPPER_SNAKE | `RC_MIN_US`, `PWM_STEPS` |
| Variablen | snake_case | `pwm_duty`, `rc_pulse_us` |
| Funktionen | snake_case | `GPIO_Init_Custom()` |
| Enum-Werte | UPPER_SNAKE | `STATE_IDLE` |
| GPIO-Pins | UPPER_SNAKE | `RC_INPUT_PIN`, `PWM_OUTPUT_PORT` |

### Header-Guards

```c
#ifndef __MODULE_NAME_H
#define __MODULE_NAME_H

// ... Content ...

#endif
```

### Funktionen

**Öffentliche Funktionen:**
```c
/**
 * @brief Kurze Beschreibung
 *
 * @param param_name Beschreibung des Parameters
 * @return Rückgabewert Beschreibung
 */
void function_name(uint32_t param_name);
```

**Lokale (statische) Funktionen:**
```c
/**
 * @brief Interne Hilfsfunktion
 */
static void helper_function(void);
```

### Interrupt-Handler

**Syntax:**
```c
void Handler_Name(void) __attribute__((interrupt("WCH-Interrupt-fast")));
```

**Prioritäten:**
| Interrupt | Priorität | Grund |
|-----------|-----------|-------|
| EXTI (RC-Signal) | 0 (höchste) | RC-Signal nicht verpassen |
| TIM2 (PWM) | 1 | RC-Signal hat Vorrang |
| Andere | Standard | - |

### Variablen

**ISR-geteilte Variablen (volatile):**
```c
volatile uint8_t pwm_duty = 0;       // Wird von main und TIM2-ISR
volatile uint16_t rc_pulse_us = 0;   // Wird von EXTI und main
volatile uint8_t rc_new_data = 0;    // Flag für neuen Puls
```

### Kommentare

**Funktions-Header (Doxygen-Stil):**
```c
/**
 * @brief Initialisiert GPIO-Pins
 *
 * Konfiguriert:
 * - PC4: RC-Eingang (IN_FLOATING)
 * - PC2: PWM-Ausgang (Push-Pull)
 */
void GPIO_Init_Custom(void);
```

**Abschnitts-Trenner im Code:**
```c
/*============================================================================*/
/*                            GPIO-INITIALISIERUNG                            */
/*============================================================================*/
```

---

## Hardware-Details

### MCU Pinbelegung

| Pin | Funktion | Beschreibung |
|-----|----------|---------------|
| PC1 | Mode-Jumper | GND = Linear, OFFEN = On/Off |
| PC2 | PWM-Ausgang | Software-PWM (TIM2) → AL8862 CTRL |
| PC4 | RC-Eingang | EXTI-Interrupt für Pulsbreitenmessung |
| PD6 | Fail-Safe | GND = LED AN bei Signalverlust |

### Timer-Nutzung

| Timer | Funktion | Konfiguration |
|-------|----------|---------------|
| TIM1 | Zeitbasis | 1µs Auflösung (Prescaler 24) |
| TIM2 | Software-PWM | 10kHz Interrupt → 100Hz PWM |

### RC-Signal Spezifikation

| Parameter | Wert |
|-----------|------|
| Periode | 20ms (50Hz) |
| Pulsbreite | 1ms - 2ms |
| Timeout | 50ms (500 × 100µs) |
| Toleranz | 800µs - 2500µs |

### Betriebsmodi

| Modus | Aktivierung | Verhalten |
|-------|-------------|-----------|
| Linear | PC1 = GND | 1ms → LED AUS, 2ms → LED AN |
| On/Off | PC1 = OFFEN | <1.5ms → AUS, ≥1.5ms → AN |
| Fail-Safe AN | PD6 = GND + Timeout | LED AN |
| Fail-Safe AUS | PD6 = OFFEN + Timeout | LED AUS |

---

## Fehlerbehandlung

### HardFault Handler

```c
void HardFault_Handler(void)
{
    NVIC_SystemReset();  // System-Reset bei kritischen Fehlern
    while (1) { }
}
```

### RC-Signal Validierung

Ungültige Pulsbreiten (< 800µs oder > 2500µs) werden verworfen und lösen keinen Mode-Wechsel aus.

---

## Lizenz

Dieses Projekt steht unter **GNU GPLv3**.
