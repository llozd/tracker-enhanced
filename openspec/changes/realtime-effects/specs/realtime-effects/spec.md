## ADDED Requirements

### Requirement: Inserts on every audio channel and the master
Each sample track, each synth part and the master SHALL have 3 insert slots, and every effect SHALL be usable in any slot.

#### Scenario: Insert on a track
- **WHEN** the user puts an effect in a slot of a track
- **THEN** only that track's audio is processed, including what it sends to the delay and reverb

#### Scenario: Insert on the master
- **WHEN** the user puts a glue compressor on the master
- **THEN** the whole mix is processed before the limiter

#### Scenario: Slot order
- **WHEN** a channel has an EQ in slot 1 and a compressor in slot 2
- **THEN** the EQ's output feeds the compressor

### Requirement: Effects available
The inserts SHALL offer a PQ1549-style parametric EQ, a 3-band EQ, compressor, limiter, glue and FET compressors, drives, tape, tube, transformer, console, exciter, chorus, flanger, phaser and reverb.

#### Scenario: Chorus on a synth part
- **WHEN** the user puts a chorus on a synth part
- **THEN** the part is widened and thickened, and its sends hear the chorus too

### Requirement: Bounded CPU and memory
The firmware SHALL not drop audio when effects are enabled.

#### Scenario: Limit reached
- **WHEN** enabling another effect (or another reverb past the reverb limit) would exceed the budget
- **THEN** it is refused, the user is told, and audio continues cleanly

### Requirement: Saved and exported
Insert settings SHALL be saved with the project, and exports SHALL include the inserts.

#### Scenario: Reload
- **WHEN** a project with inserts is saved and opened again
- **THEN** every slot has the same effect and parameters

#### Scenario: Old projects
- **WHEN** a project saved before inserts existed is opened
- **THEN** every slot is empty and it sounds as before
