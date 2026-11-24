## ADDED Requirements

### Requirement: Channels can be sent to a bus
Each sample track and synth part SHALL have an output of Master or Bus 1 to 4, saved with the project.

#### Scenario: Group tracks on a bus
- **WHEN** tracks 1 to 4 have Output set to Bus 1, and Bus 1 has an EQ and a compressor
- **THEN** tracks 1 to 4 are heard only through Bus 1's EQ and compressor, at their own volume and pan

#### Scenario: Changing the output while playing
- **WHEN** a playing track's output changes
- **THEN** the sound moves to the new output without a click

### Requirement: Buses have inserts and a volume
Each bus SHALL have 3 insert slots and a volume, and SHALL feed the master mix before the master inserts.

#### Scenario: Bus volume
- **WHEN** a bus's volume is changed
- **THEN** every channel on that bus gets louder or quieter together, after the bus effects

#### Scenario: Unused bus
- **WHEN** no channel is sent to a bus
- **THEN** the bus adds no sound and no noticeable CPU load

### Requirement: Sends stay on the channel
Delay and reverb sends SHALL be taken from the channel as before, whatever its output.

#### Scenario: Reverb send on a bused track
- **WHEN** a track on a bus has a reverb send
- **THEN** the reverb hears the track without the bus effects

### Requirement: Older projects
Projects saved before buses SHALL open with every channel sent to the master and every bus empty at 0 dB.

#### Scenario: Open a version 19 project
- **WHEN** a project saved before buses existed is opened
- **THEN** it sounds as it did before
