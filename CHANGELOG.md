# Changelog

## Original sensing 0.2.0-public1 — 2026-10-10
- Added custom experimental BIN, original source/collector, historical matching CFG and incomplete current-mount template under firmware/original-radar-sensing.
- Preserved BIN/application bytes; portable build paths and public docs updated. Complete TI/Sun/BIOS/XDC notices accompany distribution; third-party rights remain separate.
- Current reported mount is 1.2192 m with estimated positive chip offset 0.003175–0.00635 m; tilt unconfirmed. Prior 2 m/15-degree plan is not the current mount.
- No vitals estimates; stationary returns retained. No completed tracker/classifier or hardware validation. Existing v0.6 shared modules unchanged.

## v0.6-bundle1 — 2026-10-10
- Added local offline feature/model/review modules, CLI, tests and API guide; 23 exact shared paths match Radar Studio v0.6.
- Preserved the 2.00 m mount-top reference, 15-degree tilt and unknown measured antenna offset.
- Tests cover offline logic, CLI processes and mocked UI; no physical hardware or real-world accuracy validation. No data/model weights published.

## 2026-10-10 — Mounting standard documentation

- Selected the 2.00 m floor-to-flat-mount-TOP reference, excluding screws, with a final 15-degree downward tilt for the experimental activity setup.
- Antenna-center height remains 2.00 m plus the measured signed vertical offset after tilt; the positive offset in the photographed arrangement has no verified numeric value yet.
- Physical adjustment/calibration is planned, not completed. No optimal-accuracy claim, code change or guessed firmware configuration.
- Documentation-only update. Existing release tags, ZIPs, code and configurations remain unchanged; historical release manifests describe their frozen release snapshots, not these later documentation edits.

## v0.5-bundle1 — 2026-10-10
- Published 14 hash-pinned original motion/recorder modules, test/support files, API and capture guide identical to the Radar Studio v0.5 bundle.
- 29 motion and 24 capture tests pass on synthetic inputs; hardware/browser/real-data validation pending.
- No actual recordings or pretrained model included. Software license unchanged; dataset terms remain separate.

## 2026-10-10 — Public scaffold
- Linked visualizer, recorder and dataset projects.
