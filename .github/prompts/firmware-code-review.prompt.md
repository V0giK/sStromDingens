---
description: "Review CH32V003 C firmware code for quality, standards, and embedded best practices"
name: "Firmware Code Review"
argument-hint: "Paste code or refer to a file"
---

# Firmware Code Review

Review the provided C code for the **sStromDingens** CH32V003J4M6 firmware with focus on:

## Code Quality Checks

- **Memory efficiency**: RAM (2KB) and flash (16KB) constraints
- **Performance**: Minimal overhead on 48MHz RISC-V processor
- **Embedded best practices**: Proper ISR handling, timer/peripheral usage, register access
- **Safety**: Buffer overflows, uninitialized variables, division by zero

## Standards Compliance

Verify against project coding standards:
- **Language**: C (GNU99)
- **Indentation**: 4 spaces (not tabs)
- **Naming**: `snake_case` for variables, `UPPER_SNAKE` for defines
- **Includes**: Relative paths with `"..."` (not angle brackets)
- **Comments**: German language preferred

## Hardware Context Checks

Review based on pinout and peripheral assignment:
- **RC Input**: PC4 (TIM1_CH4) for input-capture
- **PWM Output**: PC2 (TIM2) for software PWM to AL8862 LED driver
- **Mode Jumper**: PC1 (selects Linear vs On/Off mode)
- **Fail-Safe**: PD4 (determines behavior on RC signal loss)

## Specific Review Points

- [ ] Timer configuration matches RC/PWM specs (20ms period, 1-2ms pulse, ~100Hz PWM)
- [ ] RC signal timeout handling (50ms without signal → fail-safe)
- [ ] Interrupt handlers are minimal and efficient
- [ ] No infinite loops (use appropriate wait states)
- [ ] Peripheral registers accessed safely
- [ ] Linker script and startup code integration correct

## Output Format

Provide:
1. **Summary**: Pass/Fail with key findings
2. **Issues Found**: Listed by severity (Critical/Warning/Info)
3. **Suggestions**: Optimizations or best practices
4. **Compliance**: Checklist against standards above

---

**Project Reference**: See [AGENTS.md](../../../AGENTS.md) for hardware pinout, timer usage, and complete peripheral documentation.
