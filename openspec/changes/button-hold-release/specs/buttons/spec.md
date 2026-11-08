## ADDED Requirements

### Requirement: Held buttons stay held
A physically held button SHALL stay reported as pressed until it is released.

#### Scenario: Bus error while holding
- **WHEN** an I2C error occurs while FX1 is held and the knob is turned
- **THEN** the knob keeps scrolling effect types
