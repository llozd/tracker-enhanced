## ADDED Requirements

### Requirement: New engines alongside the existing ones
The synth model list SHALL offer PRISM, MONO, GRIT, HARM, PHASE and POLY (the ports of PRISM, SH-101, MS800 and K5, and the new PHASE and PROPHET engines) in addition to every existing engine. No other maker's trademark appears on the device.

#### Scenario: Pick a new engine
- **WHEN** the user picks one of the new engines on a synth part
- **THEN** the part plays it with its init patch, and the other parts are unaffected

#### Scenario: Existing projects
- **WHEN** a project that uses only the existing engines is opened
- **THEN** it loads and sounds as before

### Requirement: Saved with the project
A part using a new engine SHALL save and reload its engine and parameters with the project and as a `.patch` file.

#### Scenario: Reload
- **WHEN** a project with a new engine on a part is saved and opened again
- **THEN** the part has the same engine and parameter values

### Requirement: Real-time safe
The new engines SHALL not allocate memory or touch the SD card while rendering, and a part at its maximum voices SHALL not drop audio with typical sample tracks playing.

#### Scenario: Full part
- **WHEN** a new engine plays its maximum number of voices alongside sample tracks
- **THEN** audio continues without gaps
