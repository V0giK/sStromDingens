# Changelog - sStromDingens

Alle Änderungen dieses Projekts werden in diesem Dokument festgehalten.

---

## [1.0.0] - 2026-02-22

### Hardware
- Erste Version der Platine (v1.0)
- AL8862 Konstantstrom-LED-Treiber
- CH32V003J4M6 Mikrocontroller (SOP-8)
- Eingangsspannung: 5V-60V (LiPo 2S-3S)
- 1W LED ohne Vorwiderstand

### Software
- Erste Version der Firmware (v1.0)
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

---

## [Unreleased]

### Geplant
- [ ] Mehrere LED-Kanäle
- [ ] Soft-Start / Fade-Effekte
- [ ] Temperaturschutz
- [ ] Status-LED
- [ ] Firmware-Update via Bootloader
