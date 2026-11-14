# Tasks

- [x] Choose the control (Shift + F7 / F8 on the parameter screen) and the undo behaviour (one previous patch)
- [x] Shift label swap and the two actions in `SynthEditor.cpp`
- [ ] Check on desktop and Mini hardware
- [x] Previous patch copy per part, valid only while the part keeps its engine and patch name
- [ ] Check the factory engines' parameter names on a device against the rules (their metadata is in the closed library)
- [x] Kept and limited parameters by display name, for every engine (`synthrandom::ruleFor`, host-checked on the open engines' names)
- [x] Generic randomiser over `Patch::Metadata`: mode parameters first, edit-curve mapped values, enums by step (`core/synth/SynthRandomiser.cpp`)
- [x] Popups: Patch randomised, Previous patch restored, No previous patch
- [x] Works on the new engines through the same metadata (no per-engine code)
- [ ] Hardware check: many randomisations per engine, none silent or stuck
