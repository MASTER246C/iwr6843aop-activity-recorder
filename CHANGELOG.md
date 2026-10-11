# Changelog

## Original sensing 0.3.1-rc1 — 2026-10-11

- Apply documented alternating AOP physical-RX feed polarity (+1,−1,+1,−1) in spatial configuration; keep raw phasors, generic estimator thresholds, profile and acceptance gates unchanged.
- Add self-test version 2 with analytic AOP-inversion coverage and exact build identity abb27b48c1bdb956c0de704b. This corrects a fixed hardware convention, not calibration or validated tracking accuracy.
- Publish audited source/BIN/tests/notices and sanitized aggregate evidence. Private recordings and phasors remain excluded; all 98 prior notice/config files preserved.
- Physical validation remains pending. Matching viewer v0.8.0-preview.2 forthcoming; prior releases stay unchanged.


## Original sensing 0.3.0-rc1 — 2026-10-11

- Added strict original-radar-v2 dual-UART separation: Enhanced control 115200, Standard data 921600; repaired JSON LF framing and bounded per-lane write guards.
- Added fresh no-RF nonce/probe pairing and one-shot run permission; configuration changes, stops and faults revoke pairing. Firmware duration is limited to 1–5 seconds.
- Preserved the scalar spatial repair, boot self-test, current mounting guide and all 98 prior license/notice/config files; added an intentionally incomplete measured-geometry acceptance template.
- Published audited source/BIN, tests and sanitized aggregate validation. Initial physical no-RF dual-port acceptance remains pending; matching viewer forthcoming. Three exploratory peaks are not validated people tracking.
- Existing releases, hosted links and private-data boundaries remain unchanged. No contribution-upload backend or finalized data terms are provided by this release.


## 2026-10-11 — Hosted Radar Studio launch

- Linked the public hosted viewer for desktop/ChromeOS Chrome with Web Serial; local versioned downloads remain available.
- Clarified matching firmware/configuration, browser-local recording/export and unchanged source-available/no-resale terms.
- Hosting does not broaden hardware/demo compatibility, firmware acceptance or dataset-publication permission. Existing release tags, artifacts, licenses and notices are unchanged; release manifests continue to describe their frozen packages.


## Original sensing 0.2.1-rc1 — 2026-10-10
- Corrected the reproducible spatial-estimator target instruction defect with explicit scalar stores; added a synthetic boot self-test before acquisition/radio initialization.
- Added exact build identity and status self-test fields; failure blocks acquisition and RF start while preserving UART status/stop service. No automatic configuration or RF start.
- Reproduced the old public BIN byte-for-byte; corrected TI-generated code passes offline emulation and host/boot-gate tests. Physical board/RF acceptance remains pending; first board step is status-only after a separately authorized flash.
- Published original source, candidate BIN, configuration, tests, notices, portable build instructions and sanitized validation summary. Private build/debug diagnostics and all private captures remain excluded.
- Existing shared JavaScript modules, mounting guidance, software license, third-party notices and previous release assets remain unchanged.

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
