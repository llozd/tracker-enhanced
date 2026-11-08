## MODIFIED Requirements

### Requirement: Roll retriggers are continuous
Roll hits SHALL follow each other without an audible gap.

#### Scenario: Flat roll on a sustained sample
- **WHEN** a Roll runs on a long sample
- **THEN** each hit starts at the end of the previous one with no silent gap and no click

#### Scenario: Volume ramp rolls
- **WHEN** a Roll with rising or falling volume runs
- **THEN** volume steps are preserved
