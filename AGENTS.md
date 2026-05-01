# AGENTS.md - sStromDingens

Dieses Dokument enthaelt wichtige Informationen fuer die KI-gestuetzte Weiterentwicklung des Projekts.
Letzte Aktualisierung: 2026-05-01 (v3.4.0 Afterburner Edition)

---

## Projektuebersicht

| Eigenschaft | Wert |
|-------------|------|
| **Name** | sStromDingens |
| **Beschreibung** | RC-gesteuerter LED-Treiber (1W / 3W), Hardware-PWM, Afterburner-FX |
| **Hardware Version** | 2.0 |
| **Firmware** | v3.0.0 (Basis), v3.4.0 (Afterburner FX) |
| **MCU** | CH32V003J4M6 (RISC-V RV32EC) |
| **Framework** | WCH HAL (ch32v00x) |
| **Status** | Hardware fertig, Firmware v3.4.0 fertig (Afterburner Edition) |
| **Lizenz** | GPLv3 |

### Versionsschema

Das Projekt verwendet das Versionsschema **X.Y.Z**:
- **X**: Hardware-Version
- **Y**: Software-Hauptversion
- **Z**: Software-Bugfix (und FX-Unterversion)

Beispiel: Version **1.2.3** = Hardware v1, Software v2.3

---

## Firmware-Varianten

| Ordner | Beschreibung | LED-Typ | Status |
|--------|-------------|---------|--------|
| `Software/firmware/` | Basis-Firmware (Hardware-PWM), compile-time LED-Auswahl | 1W (330mA) oder 3W (700mA) | v3.0.0 stabil |
| `Software/firmware_700mA_afterburner/` | 3W-LED Afterburner (700mA), Hardware-PWM @ 2kHz, FX-Suite | 3W (700mA) | v3.4.0 stabil |

> **Hinweis:** Die Basis-Firmware (`firmware/`) unterstuetzt beide LED-Typen ueber `#define LED_1W` oder `#define LED_3W` in `User/main.c`. `PWM_MAX_DUTY` wird automatisch angepasst (100% fuer 1W, 80% fuer 3W). Die Afterburner-Variante bleibt unabhaengig.

---

## Build-Kommandos

### MounRiver Studio

1. Projekt oeffnen: `Software/firmware/firmware.project` (Original)
2. Build: `Project -> Build Project` (Strg+B)
3. Flashen: `Tools -> WCH-LinkUtility`

### CLI Build (Terminal) - Afterburner v3.4.0

**Toolchain-Pfad:**
```
C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH\Toolchain\RISC-V Embedded GCC\bin\
```

**Kompiler-Flags fuer alle Module:**
```
-march=rv32ec -mabi=ilp32e -mcmodel=medlow -ffunction-sections -fdata-sections -Os -Wall
```

**Include-Pfade:**
```
-IDebug -ICore -IUser -IPeripheral/inc
```

**Schritt 1: Alle C-Dateien kompilieren**

```powershell
$GCC = "C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH\Toolchain\RISC-V Embedded GCC\bin\riscv-none-embed-gcc.exe";
$FLAGS = @("-march=rv32ec","-mabi=ilp32e","-mcmodel=medlow","-ffunction-sections","-fdata-sections","-Os","-Wall","-IDebug","-ICore","-IUser","-IPeripheral/inc");

# User-Code
& $GCC $FLAGS -c User/main.c -o User/main.o;
& $GCC $FLAGS -c User/ch32v00x_it.c -o User/ch32v00x_it.o;
& $GCC $FLAGS -c User/debug.c -o User/debug.o;
& $GCC $FLAGS -c User/system_ch32v00x.c -o User/system_ch32v00x.o;

# HAL
& $GCC $FLAGS -c Core/core_riscv.c -o Core/core_riscv.o;
& $GCC $FLAGS -c Peripheral/src/ch32v00x_tim.c -o Peripheral/src/ch32v00x_tim.o;
& $GCC $FLAGS -c Peripheral/src/ch32v00x_gpio.c -o Peripheral/src/ch32v00x_gpio.o;
& $GCC $FLAGS -c Peripheral/src/ch32v00x_rcc.c -o Peripheral/src/ch32v00x_rcc.o;
& $GCC $FLAGS -c Peripheral/src/ch32v00x_exti.c -o Peripheral/src/ch32v00x_exti.o;
& $GCC $FLAGS -c Peripheral/src/ch32v00x_misc.c -o Peripheral/src/ch32v00x_misc.o;
& $GCC $FLAGS -c Peripheral/src/ch32v00x_usart.c -o Peripheral/src/ch32v00x_usart.o;

# Startup (Assembly)
& $GCC $FLAGS -c Startup/startup_ch32v00x.S -o Startup/startup_ch32v00x.o;
```

**Schritt 2: Linken**

> **WICHTIG:** `-specs=nosys.specs` und `-specs=nano.specs` funktionieren NICHT mit dieser Toolchain-Version.
> Die Option `-lprintf` kann ebenfalls Probleme verursachen und sollte weggelassen werden, falls Linker-Fehler auftreten.

```powershell
$GCC = "C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH\Toolchain\RISC-V Embedded GCC\bin\riscv-none-embed-gcc.exe";
& $GCC -march=rv32ec -mabi=ilp32e -mcmodel=medlow `
  -ffunction-sections -fdata-sections -Os -nostartfiles `
  -Xlinker --gc-sections -T Ld/Link.ld -o firmware.elf `
  Startup/startup_ch32v00x.o Core/core_riscv.o `
  User/system_ch32v00x.o User/ch32v00x_it.o User/main.o User/debug.o `
  Peripheral/src/ch32v00x_tim.o Peripheral/src/ch32v00x_gpio.o `
  Peripheral/src/ch32v00x_rcc.o Peripheral/src/ch32v00x_exti.o `
  Peripheral/src/ch32v00x_misc.o Peripheral/src/ch32v00x_usart.o;
```

**Schritt 3: Hex/Bin erzeugen**

```powershell
$OBJCOPY = "C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH\Toolchain\RISC-V Embedded GCC\bin\riscv-none-embed-objcopy.exe";
& $OBJCOPY -O ihex firmware.elf firmware.hex;
& $OBJCOPY -O binary firmware.elf firmware.bin;
```

### Inkrementeller Build (nur geaenderte Dateien)

Nach Aenderungen nur das betroffene `.o` neu kompilieren und Schritt 2+3 wiederholen.

### Flashen

**GUI:** WCH-LinkUtility
```
C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH\Others\SWDTool\default\WCH-LinkUtility.exe
```
**Datei:** `firmware.hex` (von Schritt 3 oder MounRiver Build)

---

## Projektstruktur

```
sStromDingens/
├── .github/                        # GitHub-spezifische Konfiguration
├── Hardware/
│   ├── Documents/                  # Datenblaetter (AL8862, CH32V003RM)
│   └── KiCad/                      # KiCad-Projekt, Gerber, BOM
├── Software/
│   ├── firmware/                   # Basis-Firmware v3.0.0 (HW-PWM, compile-time LED-Auswahl)
│   │   ├── User/                   # main.c, ch32v00x_it.c, debug.c
│   │   ├── Core/                   # RISC-V Core
│   │   ├── Peripheral/             # HAL-Treiber
│   │   ├── Startup/                # Startup-Code
│   │   ├── Ld/                     # Linker-Skript
│   │   └── README.md               # Basis-Firmware Dokumentation
│   └── firmware_700mA_afterburner/ # Afterburner v3.4.0 (3W LED, FX)
│       ├── User/                     # main.c (Afterburner FX Engine)
│       ├── Core/
│       ├── Peripheral/
│       ├── Startup/
│       ├── Ld/
│       └── README.md               # Afterburner-spezifische Dokumentation
├── images/                         # Bilder (Top, PCB, Bottom)
├── AGENTS.md                       # KI-Dokumentation & Build-Referenz
├── CHANGELOG.md                    # Aenderungshistorie
├── LICENSE                         # GNU GPLv3
├── README.md                       # Deutsche Dokumentation (Original)
└── README_EN.md                    # Englische Dokumentation (Original)
```

---

## Coding-Standards

### Sprache & Format

| Regel | Wert |
|-------|------|
| **Sprache** | C (GNU99, GNU-Extensions erlaubt) |
| **Einrueckung** | 4 Leerzeichen (keine Tabs) |
| **Kommentare** | Deutsch |
| **max. Zeilenlaenge** | 100 Zeichen |

### Includes (Reihenfolge)

```c
// 1. Standard-Bibliotheken
#include &lt;stdint.h&gt;

// 2. CMSIS / Vendor-Headers
#include &lt;ch32v00x.h&gt;

// 3. Peripherie-Treiber
#include "ch32v00x_exti.h"
#include "ch32v00x_tim.h"
#include "ch32v00x_gpio.h"

// 4. Lokale Projekt-Headers
#include "debug.h"
```

**Regel:** Projekt-Headers mit `"..."` inkludieren, nicht `&lt;...&gt;`.

### Typen

| Typ | Verwendung |
|-----|------------|
| `uint8_t`, `uint16_t`, `uint32_t` | Unsigned Integer |
| `int16_t` | Signed Integer |
| `volatile uint8_t` | ISR-geteilte Variablen |
| `volatile uint16_t` | ISR-geteilte Variablen (Pulsbreite) |

**Regel:** NIEMALS primitive Typen wie `int`, `long` verwenden (plattformabhaengig).

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

**Oeffentliche Funktionen:**
```c
/**
 * @brief Kurze Beschreibung
 *
 * @param param_name Beschreibung des Parameters
 * @return Rueckgabewert Beschreibung
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

**Prioritaeten (Basis-Firmware):**
| Interrupt | Prioritaet | Grund |
|-----------|------------|-------|
| EXTI (RC-Signal) | 0 (hoechste) | RC-Signal nicht verpassen |
| Andere | Standard | - |

> **Hinweis:** TIM2 laeuft in Hardware-PWM-Modus ohne Interrupt. Kein TIM2-IRQ mehr noetig.

**Prioritaeten (Afterburner v3.4.0):**
| Interrupt | Prioritaet | Grund |
|-----------|------------|-------|
| EXTI (RC-Signal) | 0 (hoechste) | RC-Signal nicht verpassen |
| Andere | Standard | - |

> **Hinweis:** TIM2 laeuft in Hardware-PWM-Modus ohne Interrupt. Kein TIM2-IRQ mehr noetig.

### Variablen

**Globale Variablen (in main.c definiert, in ch32v00x_it.c als extern verwendet):**
```c
volatile uint16_t rc_pulse_us = 0;   /* Gemessene Pulsbreite */
volatile uint8_t rc_new_data = 0;    /* Flag: neuer Puls verfuegbar */
volatile uint16_t rc_start_time = 0; /* TIM1-Counter bei Rising Edge */
```

> **Hinweis:** `ch32v00x_it.c` darf nur in `main.c` definierte Variablen als `extern` verwenden.
> Die alten Variablen `pwm_duty` und `mode_jumper` existieren nicht mehr in v3.0.0.
> `pwm_output` ist eine lokale Variable in `main()` (keine ISR-Teilung noetig bei Hardware-PWM).

### Kommentare

**Funktions-Header (Doxygen-Stil):**
```c
/**
 * @brief Initialisiert GPIO-Pins
 *
 * Konfiguriert:
 * - PC4: RC-Eingang (IN_FLOATING)
 * - PC2: PWM-Ausgang (AF_PP fuer TIM2_CH2)
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

## Hardware-Details (Afterburner v3.4.0)

### MCU Pinbelegung CH32V003J4M6 (SOP-8)

**Basis-Firmware (firmware/):**

| Pin | Bezeichnung | Funktion | Bemerkung |
|:---:|:---|:---|:---|
| **1** | **PD6** | **Fail-Safe-Jumper** | Pull-Up, GND = LED AN bei Timeout |
| 2 | VSS | GND | |
| 3 | PA2 | *nicht verwendet* | |
| 4 | VDD | 3.3 V | |
| **5** | **PC1** | **Mode-Jumper** | Pull-Up, GND = Linear, OFFEN = On/Off |
| **6** | **PC2** | **PWM-Ausgang (TIM2_CH2)** | AF_PP → AL8862 CTRL |
| **7** | **PC4** | **RC-Eingang (EXTI4)** | IN_FLOATING |
| 8 | PD1 / SWIO | Programmierung (WCH-LinkE) | |

**Afterburner v3.4.0 (firmware_700mA_afterburner/):**

| Pin | Bezeichnung | Funktion | Bemerkung |
|:---:|:---|:---|:---|
| 1 | PA1 / PD6 | *nicht verwendet* | Fail-Safe-Jumper entfernt |
| 2 | VSS | GND | |
| 3 | PA2 | *nicht verwendet* | |
| 4 | VDD | 3.3 V | |
| 5 | PC1 | *nicht verwendet* | Mode-Jumper entfernt |
| **6** | **PC2** | **PWM-Ausgang (TIM2_CH2)** | AF_PP → AL8862 CTRL |
| **7** | **PC4** | **RC-Eingang (EXTI4)** | IN_FLOATING |
| 8 | PD1 / SWIO | Programmierung (WCH-LinkE) | |

### Timer-Nutzung

| Timer | Funktion | Konfiguration |
|-------|----------|---------------|
| TIM1 | Zeitbasis | 1us Aufloesung (Prescaler 24) |
| TIM2 | Hardware-PWM | 2kHz, TIM2_CH2 auf PC2 via PartialRemap1 |

### RC-Signal Spezifikation

| Parameter | Wert |
|-----------|------|
| Periode | 20ms (50Hz) |
| Pulsbreite | 1000us - 2000us |
| Timeout | 50ms (500 x 100us) |
| Toleranz | 800us - 2500us |

### LED-Driver (AL8862)

| Parameter | Wert |
|-----------|------|
| Typ | Buck-Boost Konstantstromtreiber |
| Max. Strom | 330mA (1W LED) bis 700mA (3W LED, nur mit Kühlung) |
| Steuerung | PWM-Dimming via CTRL-Pin |
| PWM-Frequenz | >500 Hz (hier: 2kHz Hardware-PWM) |
| Max. Duty | 100% bei 1W, 80% bei 3W (configurierbar via PWM_MAX_DUTY) |

### Nachbrenner-Modi (Afterburner FX v3.4.0)

| Modus | Verhalten |
|-------|-----------|
| Standard (Linear) | 1000us -> AUS, 2000us -> AN, Zwischenwerte proportional |
| Hysterese | EIN ab 1650us, AUS erst unter 1550us |
| Spool-Up | Zufaelliger Flash 80-130ms @ 100% beim Zuenden |
| Ramp-Down | 80ms sanftes Absenken Flash -> Sollwert |
| Cool-Down | 250ms sanftes Abklingen auf AUS |
| Vollgas-Burst | 20ms Blitz auf 100% bei Sprung ueber 1950us |
| Flame-Out | 150ms wildes Flackern bei abruptem Gas-weg |
| Flicker | +/-15% normales Rauschen, +/-25% im Heat-Bereich (>1900us) |
| Fail-Safe | LED AUS bei Signalverlust (>50ms) |

---

## Firmware-Architektur

### Basis-Firmware (firmware/)

Die Basis-Firmware hat keine State-Machine im Afterburner-Stil. Sie bietet zwei Modi über PC1-Jumper:

| Modus | Verhalten |
|-------|-----------|
| **Linear** | Proportionales Dimmen: 1000us → AUS, 2000us → PWM_MAX_DUTY% |
| **On/Off** | Hysterese: EIN ab 1550us, AUS erst unter 1450us |

`PWM_MAX_DUTY` wird zur Compile-Zeit via `#define LED_1W` (100%) oder `#define LED_3W` (80%) gesetzt.

### State-Machine (Afterburner v3.4.0)

```
POWER-ON -> SELFTEST (200ms @ 50%) -> IDLE

IDLE --(Gas>=1650us)--> SPOOL_UP (Flash @ 100%, 80-130ms)
                       --(Gas&lt;1550us)--> COOL_DOWN (250ms)
                       --(Timeout)--> IDLE

SPOOL_UP --(Zeit abgelaufen)--> RAMP_DOWN (80ms Flash->Soll)

RAMP_DOWN --(Fertig)--> RUNNING

RUNNING --(Gas&lt;1550us)--> COOL_DOWN
        --(Sprung&gt;1950us)--> BURST (20ms @ 100%)
        --(Abrupt &gt;1800->&lt;1200)--> FLAMEOUT (150ms wild)

BURST --(Fertig)--> RUNNING

FLAMEOUT --(Gas wieder&gt;=1650)--> SPOOL_UP
         --(Zeit abgelaufen)--> COOL_DOWN

COOL_DOWN --(Fertig)--> IDLE
           --(Gas wieder&gt;=1650)--> SPOOL_UP
```

### Wichtige Defines (main.c – Afterburner v3.4.0)

```c
#define THRESHOLD_ON        1650    /* Einschalten ab hier */
#define THRESHOLD_OFF       1550    /* Ausschalten erst unter hier */
#define FLAMEOUT_HIGH       1800    /* Vorheriger Wert ueber hier */
#define FLAMEOUT_LOW        1200    /* Aktueller Wert unter hier */
#define BURST_THRESHOLD     1950    /* Vollgas-Burst ab hier */
#define PWM_MAX_DUTY        80      /* Maximale Ausgangsleistung in % */
#define PWM_FREQ_HZ         2000    /* Hardware-PWM Frequenz */
#define SELFTEST_MS         200     /* Dauer Power-On Test */
#define SELFTEST_DUTY       50      /* Helligkeit Selftest */
#define SPOOL_UP_MIN_MS     80      /* Min. Spool-Up Flash */
#define SPOOL_UP_MAX_MS     130     /* Max. Spool-Up Flash */
#define RAMP_DOWN_MS        80      /* Rampe Flash->Soll */
#define COOL_DOWN_MS        250     /* Rampe Soll->AUS */
#define FLAMEOUT_MS         150     /* Flame-Out Dauer */
#define BURST_MS            20      /* Burst Dauer */
#define FLICKER_NORMAL      15      /* +/- % normal */
#define FLICKER_HEAT        25      /* +/- % Heat-Shimmer */
#define HEAT_THRESHOLD      1900    /* Heat-Shimmer ab hier */
#define RC_TIMEOUT_TICKS    500     /* 50ms Fail-Safe Timeout */
```

### Basis-Firmware Defines (firmware/User/main.c v3.0.0)

```c
#define LED_1W              /* Oder LED_3W – setzt PWM_MAX_DUTY automatisch */
#define RC_MIN_US           1000
#define RC_MAX_US           2000
#define ONOFF_THRESHOLD_ON   1550
#define ONOFF_THRESHOLD_OFF  1450
#define PWM_MAX_DUTY        Auto (100 bei LED_1W, 80 bei LED_3W)
#define PWM_FREQ_HZ         2000
#define RC_TIMEOUT_TICKS    500
```

### Gemeinsame Variablen (main.c / ch32v00x_it.c)

```c
/* In main.c definiert, in ch32v00x_it.c als extern verwendet */
volatile uint16_t rc_pulse_us = 0;   /* Gemessene Pulsbreite */
volatile uint8_t rc_new_data = 0;    /* Flag: neuer Puls verfuegbar */
volatile uint16_t rc_start_time = 0; /* TIM1-Counter bei Rising Edge */
```

> **Hinweis:** `ch32v00x_it.c` darf nur in `main.c` definierte Variablen als `extern` verwenden.
> Die alten Variablen `rc_valid` und `mode_jumper` existieren nicht mehr in v3.0.0.

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

Ungueltige Pulsbreiten (&lt; 800us oder &gt; 2500us) werden verworfen und loesen keinen Mode-Wechsel aus.

### Build-Fehler beheben

| Fehler | Ursache | Loesung |
|--------|---------|---------|
| `undefined reference to 'rc_valid'` | Alte Variable in ch32v00x_it.c | Entfernen oder durch `rc_new_data` ersetzen |
| `undefined reference to 'EXTI_Init'` | ch32v00x_exti.o fehlt | `ch32v00x_exti.c` kompilieren und linken |
| `.specs: No such file` | `-specs=` nicht unterstuetzt | Flag entfernen |
| `undefined reference to 'printf'` | `-lprintf` nicht verfuegbar | Flag entfernen |

---

## Lizenz

Dieses Projekt steht unter **GNU GPLv3**.
