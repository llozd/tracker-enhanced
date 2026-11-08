## ADDED Requirements

### Requirement: Original Tracker build
The repository SHALL build an Original Tracker (MK66) firmware alongside Tracker+ and Tracker Mini.

#### Scenario: Full build
- **WHEN** `make all` runs
- **THEN** Plus, Mini and Original `.ptf` files are produced

#### Scenario: CI
- **WHEN** CI builds a commit
- **THEN** all three firmwares are built and uploaded

### Requirement: Platform feature exclusion
Features with significant DSP, CPU or memory overhead SHALL be excluded from the Original build at compile time.

#### Scenario: Realtime effects on the Original
- **WHEN** the Original firmware is built
- **THEN** realtime effect code is not compiled in and no effect menus or slots are shown

#### Scenario: Sequencer improvements on the Original
- **WHEN** the Original firmware is built
- **THEN** lines per beat (up to 16), legato rolls and other sequencer improvements are included

### Requirement: Cross-device projects
Projects SHALL open on all supported devices.

#### Scenario: Plus project with effects opened on the Original
- **WHEN** a project using realtime effects is opened, edited and saved on the Original
- **THEN** it plays without the effects there, and the effect settings are intact when reopened on a Plus
