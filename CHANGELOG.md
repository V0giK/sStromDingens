# Changelog - sStromDingens

Alle Änderungen dieses Projekts werden in diesem Dokument festgehalten.

Versionsschema: **X.Y.Z**
- **X**: Hardware-Version
- **Y**: Software-Hauptversion
- **Z**: Software-Bugfix

---

## [2.2.0] - 2026/03/25

### Hardware v2.0
- Layout ueberarbeitet und optimiert
- TVS-Dioden hinzugefuegt: SMF3.3CA (x2) fuer Eingangsschutz, SMAJ26CA fuer LED-Ausgang
- Ueberspannungsschutz SMAJ15A entfernt (D4, D5)
- Failsafe-Jumper von Pin 8 (PD4) auf Pin 1 (PD6) geaendert

### Software v2.0.0
- Failsafe-Pin von PD4 (Pin 8) auf PD6 (Pin 1) geaendert
- Hardware-Kompatibilitaet: Nur fuer Hardware v2.0

---

## [1.2.0] - 2026/03/17

### Hardware v1.0 (unverändert)

### Software v1.2.0
- Software-PWM via TIM2 Interrupt implementiert (100Hz, 100 Stufen)
- Linear-Modus implementiert (1ms→LED AUS, 2ms→LED AN)
- On/Off-Modus implementiert (<1.5ms→LED AUS, ≥1.5ms→LED AN)
- **Refactoring:** RC-Eingangsmessung auf EXTI-Interrupt umgestellt (statt Polling)
- **Bugfix:** Timer-Konfiguration für 24MHz SystemClock korrigiert
- **Bugfix:** signal_timeout Logik korrigiert
- AL8862 Dimming: pwm_duty=0→LED AUS, pwm_duty=100→LED AN

---

## [1.1.0] - 2026-03-10

### Hardware v1.0 (unverändert)

### Software v1.1.0
- Software-PWM via TIM2 implementiert (Hardware-PWM funktionierte nicht)
- Race-Condition behoben: Kopie von rc_pulse_us vor Löschen von rc_valid
- Author geändert zu V0giK, Copyright aktualisiert
- AGENTS.md und README.md aktualisiert

---

## [1.0.1] - 2026-02-23

### Hardware v1.0 (unverändert)

### Software v1.0.1
- Debug-Dateien (debug.h, debug.c) hinzugefügt
- printf-Unterstützung entfernt (16KB Flash-Limit)
- .clangd Konfiguration für Language Server
- PWM-Frequenz auf ~1000Hz korrigiert

---

## [1.0.0] - 2026-02-22

### Hardware v1.0
- Erste Version der Platine (v1.0)
- AL8862 Konstantstrom-LED-Treiber
- CH32V003J4M6 Mikrocontroller (SOP-8)
- Eingangsspannung: 5V-60V (LiPo 2S-3S)
- 1W LED ohne Vorwiderstand

### Software v1.0.0
- Erste Version der Firmware
- RC-Signal zu PWM Konverter
- Linearer Modus: 1ms → 0%, 2ms → 100%
- On/Off Modus: Schwelle bei 1.5ms
- Fail-Safe Konfiguration (per Jumper wählbar)
- RC-Timeout: 50ms

### Pinbelegung
| Pin | Funktion |
|-----|----------|
| Pin 1 | SWIO (Programmierung) |
| Pin 2 | VSS (GND) |
| Pin 4 | VDD (3.3V) |
| PC1 (Pin 5) | Mode-Jumper |
| PC2 (Pin 6) | PWM-Ausgang → AL8862 CTRL |
| PC4 (Pin 7) | RC-Eingang |
| PD4 (Pin 8) | Fail-Safe-Jumper |
