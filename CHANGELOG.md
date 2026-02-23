# Changelog - sStromDingens

Alle Änderungen dieses Projekts werden in diesem Dokument festgehalten.

Versionsschema: **X.Y.Z**
- **X**: Hardware-Version
- **Y**: Software-Hauptversion
- **Z**: Software-Bugfix

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
