# Changelog

## Unreleased source — 2026-10-16

- Firmware source 0.3.3-dev, build ID 30528454ec253ca4d5710162. No BIN exists; it is host-tested only, not built with the TI toolchain, flashed or physically tested. A rejected RF stop, or one with no frame end, now leaves RF marked started so the next `stop` retries. The status `fault` field keeps the first cause. `make clean` removes only generated intermediates and refuses a receipted release folder; builds check the pinned TI compiler revision (not yet run on a TI host). Release-tool checks no longer use `assert`, so `python -O` cannot hide a failure. New host tests cover `src/radio.c`, `src/acquisition.c` and the worst-case 1709-byte frame. Wire protocol, commands, ACKs and fault-code meanings are unchanged. `BUILD_IDENTITY.json` is now byte-identical on every OS.
- 0.3.2-rc1 (build 87589b2c32638ae119334c72) remains the accepted BIN; its source documentation is preserved in `HISTORY/v0.3.2-rc1/`. 0.3.1-rc1 (abb27b48c1bdb956c0de704b) remains for timed tests and rollback.
- Host training tools 0.1.0-preview.4 (unreleased source): atomic output writes that never replace an existing file; camera import skips labels that round to zero length and keeps the review file's declared label source. 34 training and 9 camera-import synthetic tests pass. The published preview.3 ZIP is unchanged.
- Add `schemas/original-radar-dataset.v1.schema.json` (JSON Schema draft 2020-12) for the real v1 export, with a checker, a sample generator and a README that maps the dataset repository's planned SCHEMA.md fields to export fields.
- The TI stream decoder (`radar_protocol.js`) resynchronizes at the next magic word when a corrupt length overruns the next frame; `tests/protocol.test.js` now sits beside it.
- Add `tools/check-shared-modules.js`, `tools/release_manifest.py`, `tools/check_links.py`, `tests/run-all.js`, a root `.gitignore` and CI (`.github/workflows/ci.yml`). Fix broken links in the shared docs. Shared bundle v0.6-bundle3 pins 32 files, including the schema, tools and three shared docs, matched byte-for-byte to Radar Studio v0.9.0-preview.6 source.
- No new hardware validation, recordings, BIN or release asset.

## Radar + camera enclosure R02 — 2026-10-13

- Add the R02 prototype enclosure for the IWR6843AOPEVM and Logitech C920e under `hardware/enclosure-r02/`: editable CadQuery source, eleven STEP/STL part types, an assembly STEP (two PCB pins omitted), BOM, assembly notes, validation reports and SHA-256 checksums.
- An enclosure-specific license permits personal and internal use and customer-directed paid fabrication; independent retail/resale is restricted. Users keep their own datasets, measurements, models and research outputs.
- Fit-check prototype only. Physical C920e fit, retention, weather resistance, RF, thermal and load performance are unvalidated. TI and community C920 device geometry is excluded.

## First combined physical run — 2026-10-12

- Add `docs/FIRST_COMBINED_RUN_2026-10-12.md`. Firmware 0.3.2-rc1 with Radar Studio v0.9.0-preview.5 recorded 283 consecutive 5 Hz frames over 56.4 seconds, acknowledged manual Stop and returned to fault-free idle. The C920e recording passed file-integrity and video-decoding checks.
- This was an owner-run test reviewed by the development agent, not independent validation. The camera-label segment stayed unknown, the importer rejected it for missing human-matched timing, and the radar baseline admitted 0 of 283 frames.
- Keepalive-loss/disconnect Stop, endurance, calibration, camera/radar alignment and activity accuracy remain untested. No recordings, BIN changes or dataset samples were published.

## Host training tools v0.1.0-preview.3 and camera import — 2026-10-12

- Add `original_training/camera_import.py`. It converts a verified local recording folder plus a camera review into an original-radar-dataset v1 file. It checks video and raw file receipts, accepted radar identity, data-port pairing proof and human-matched clock anchors.
- Unreviewed camera proposals and unknown or ambiguous activity stay excluded. The output goes through the same quality, split and held-out gates. 7 synthetic camera-import tests pass; the 33 training tests are unchanged.
- The tools do not decode video, run pose inference, train automatically or validate synchronization. Tagged original-training-v0.1.0-preview.3.

## Original sensing 0.3.2-rc1 — 2026-10-12

- Add explicit `session continuous`: 5 Hz frames until manual Stop, a fault or loss of the matching exact-session control keepalive (rolling 5000 ms lease). Bounded 1–5 second sessions remain; `session 0` and 60-second numeric configs stay rejected. Timestamps are extended across the raw-clock wrap.
- Build 87589b2c32638ae119334c72, built and packaged with the official TI tools; host tests and a mocked 72-minute synthetic continuous run pass. The candidate has its own `RELEASE_MANIFEST_0.3.2-rc1.json`; the 0.3.1-rc1 BIN and manifest are unchanged for rollback.
- Physical acceptance was pending at release. Matching viewer: Radar Studio v0.9.0-preview.4 and later.

## Correctness and workflow patch — 2026-10-11

- Host training tools v0.1.0-preview.2 check timestamps and sequences across whole segments and reject all windows touched by discontinuities, including window boundaries and five-second takes. 33 synthetic training tests cover the corrected admission.
- Historical original-radar-v1 collector fails its result when Stop cannot be confirmed, including a closed port, without hiding prior errors or interrupts. 13 synthetic collector tests pass. This collector is not a controller for current original-radar-v2 firmware.
- Shared TI offline extractor v0.6-bundle2 keeps unfinished/incomplete events as ambiguity coverage and matches the current Radar Studio v0.9.0-preview.3 source. All 23 shared file hashes are reconciled.
- Current collection instructions stop for initial no-RF receipt review before later separately authorized manual RF tests. Earlier archives, firmware BIN/build identity, schemas and licenses remain unchanged; no physical acceptance is claimed.

## Host training tools v0.1.0-preview.1 — 2026-10-11

- Add isolated original_training/ Python standard-library tools for original-radar dataset v1 validation, quality-gated features, deterministic baseline training and independent evaluation.
- Require human-reviewed labels and disjoint session/participant splits. Retain explicit rejects, unknowns and source-integrity limitations; no trained weights or accuracy claim.
- Include 28 synthetic tests, JavaScript-export integration contract, camera-sync groundwork, unchanged license and bounded resource guidance.
- Host software only: no firmware changes, no private recordings, no camera selection or contribution-upload activation. Existing TI modules and release assets are unchanged.


## Original sensing 0.3.1-rc1 — 2026-10-11

- Apply documented alternating AOP physical-RX feed polarity (+1,−1,+1,−1) in spatial configuration; keep raw phasors, generic estimator thresholds, profile and acceptance gates unchanged.
- Add self-test version 2 with analytic AOP-inversion coverage and exact build identity abb27b48c1bdb956c0de704b. This corrects a fixed hardware convention, not calibration or validated tracking accuracy.
- Publish audited source/BIN/tests/notices and sanitized aggregate evidence. Private recordings and phasors remain excluded; all 98 prior notice/config files preserved.
- Physical validation remains pending. Matching viewer: Radar Studio v0.8.0-preview.2; prior releases stay unchanged.


## Original sensing 0.3.0-rc1 — 2026-10-11

- Added strict original-radar-v2 dual-UART separation: Enhanced control 115200, Standard data 921600; repaired JSON LF framing and bounded per-lane write guards.
- Added fresh no-RF nonce/probe pairing and one-shot run permission; configuration changes, stops and faults revoke pairing. Firmware duration is limited to 1–5 seconds.
- Preserved the scalar spatial repair, boot self-test, current mounting guide and all 98 prior license/notice/config files; added an intentionally incomplete measured-geometry acceptance template.
- Published audited source/BIN, tests and sanitized aggregate validation. Initial physical no-RF dual-port acceptance remains pending; matching viewer: Radar Studio v0.8.0-preview.1. Three exploratory peaks are not validated people tracking.
- Existing releases, hosted links and private-data boundaries remain unchanged. No contribution-upload backend or finalized data terms are provided by this release.

## Original sensing 0.2.2-rc1 — 2026-10-11

- Add bounded UART transmit-ready checks before and after each write, so the next record cannot overwrite a pending final newline. A timeout or short write latches transport off with fault 401 until reboot; Start rejects the latch or any application fault.
- Build e085ac80bef63e23cb8185f4. Offline-verified only (host tests, framing harness, instruction emulation); physical acceptance was pending. Not tagged as a separate release; its documentation was added to `HISTORY/v0.2.2-rc1/` with 0.3.0-rc1, which keeps the transport guard.


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
