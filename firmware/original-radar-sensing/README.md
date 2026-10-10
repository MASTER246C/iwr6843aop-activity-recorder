# Original Radar Sensing 0.2.1-rc1

**Release candidate for supervised hardware acceptance. Not yet board-tested.**

This release corrects a reproducible malformed target instruction sequence in the spatial estimator and adds a deterministic no-RF boot acceptance gate. It is not a calibrated spatial measurement system or an activity classifier. No automatic RF start or configuration was added.

## What changed

- Explicit scalar component stores replace the normalization loop's complex aggregate-return assignment.
- Boot runs a synthetic estimator self-test after UART setup, before acquisition/radio initialization.
- Failure leaves UART status/stop service available, reports fault 482, and blocks all acquisition/radio initialization and start paths.
- Identity/version is `0.2.1-rc1`. Identity and status include a generated `build_id`. Status adds `firmware_version`, `selftest_version`, `selftest_ok` and `selftest_mask`.
- Build identity is generated from application source/config and pinned tool versions; the release BIN SHA-256 is the authoritative artifact checksum.

## First hardware acceptance, before any sensing

Flashing and physical-board actions require the user's separate authorization and supported device procedure. This repository does not flash automatically.

1. Verify the release BIN's SHA-256 against `RELEASE_MANIFEST.json` before using it. Flash only the intended IWR6843AOP image through an approved workflow.
2. Boot normally and open the correct UART at **115200 baud**. Do not send `start`, load a start-containing configuration, or use an auto-start viewer.
3. Send only `status` followed by a newline. Require the current response to report:
   - `firmware_version`: `0.2.1-rc1`
   - `build_id`: exactly the value in `BUILD_IDENTITY.json` / release manifest
   - `selftest_version`: `1`
   - `selftest_ok`: `true`
   - `selftest_mask`: `0`
   - `state`: `idle`, all fault fields zero, `configured`: `false` after a fresh boot
4. Save the identity, status and corresponding status ACK. If the board is silent, reboots, reports a nonzero mask/fault, lacks fields or has the wrong version/build, **stop there**. Do not start RF to troubleshoot it.
5. A passing code self-test permits planning the next separately approved bounded RF test. It does not prove antenna calibration, axis signs, position accuracy, stack high-water under acquisition, or physical RF behavior.
6. Because configuration is volatile, a power cycle/unplug requires fresh configuration later. Do not infer configuration from a previous session. The existing published preview with RF Start disabled remains the safe interface until its separately reviewed acceptance gate is available.

The status query reports this boot's cached self-test result. It does not rerun the test. Every boot reruns the test before acquisition/radio initialization. Discard status receipts on disconnect, reboot/new identity, transport error or reconnect. The line protocol has no request nonce; status/ACK correlation is best-effort freshness, not cryptographic attestation.

## Self-test coverage

CPU-only synthetic estimator inputs test boresight, known +pi/4 phase differences on both axes, per-RX correction rotations with unequal amplitudes, zero-input rejection, NaN rejection, incoherent-pair rejection, and caller guard words. No private capture is embedded. The normal firmware does not include diagnostic internal-scratch instrumentation; that variant was independently tested offline.

Failure mask bits: 1 boresight; 2 phase plane; 4 correction/amplitudes; 8 zero rejection; 16 NaN rejection; 32 coherence rejection; 64 caller guard. Diagnostic-only builds additionally use bit 128 for internal scratch guards.

SoC/clock/MPU/UART startup occurs before the synthetic test. This integrated image is **not** a hardware-free standalone diagnostic firmware: it is a sensing-capable image whose RF prepare/start path still requires an explicit `start` after configuration and a passed boot gate.

## Public package scope

This public package includes the unchanged RC BIN, source, configs, tests, vendor notices, relative-path manifests and build instructions. Optional debug artifacts (ELF, map, link XML) and raw build/test logs are retained separately for private audit and are not included here. `VALIDATION.md` summarizes the completed checks without build-machine paths.

## Offline verification completed

- Unchanged v0.2.0 rebuilt and repackaged with exact official TI tools: entire public BIN SHA-256 reproduced.
- TI O2 baseline emitted the defective double increment; scalar candidate emits one correct 8-byte progression with paired stores.
- Source-listing and ordinary-build estimator instruction sections match exactly.
- TI-generated code in QEMU Cortex-R5F instruction emulation: baseline O2 fails positive tests (mask 7), baseline O0 and scalar/guarded candidates O0/O2 pass. Final RC estimator + self-test objects also pass.
- Host suite: 937 spatial plane-wave cases, 706 DSP checks, ASan/UBSan, 11 collector tests; generated main.c peripheral-mock harness checks pending/failing boot gate and no auto-start.
- BIN CRC32 and ELF memory/dependency structural checks pass.

Emulation is not IWR6843/Cortex-R4F hardware validation. Physical boot/UART/RF/acquisition and calibration remain pending.

## Current mounting guidance

The current user-reported mount reference is **4 feet (1.2192 m) from floor to the flat TOP of the bracket, excluding screws**. The chip-center offset is only an estimate, **+0.003175 to +0.00635 m**, giving an estimated antenna height interval **1.222375–1.22555 m** (midpoint about **1.224 m**, not a calibrated measurement). Actual downward tilt is unconfirmed. The earlier 2 m / 15-degree geometry is a historical example, not the current setup or a universal requirement. `config/room64_v1.cfg` retains that historical reference; do not use it unchanged for the current mount. See `config/current_mount.cfg.template` and `config/current_mount_metadata.json`; measure tilt before completing the template. RX phase and world-axis orientation are also unverified. Thus world coordinates and posture stay unknown, even if a numeric offset is later provided without the other calibration.

## Rebuild

Use the Makefile with SDK 03.06.02.00-LTS, TI ARM CGT 16.9.6.LTS, SYS/BIOS 6.73.01.01 and XDC 3.50.08.24. Override tool roots as documented in the Makefile. `make all` generates the source/config build identity and ELF/map; `make test` runs host checks. `tools/package_firmware.sh` produces a BIN using official TI image tools and refuses to overwrite existing outputs. It never flashes hardware.

Keep the ELF, map, link XML, compiler logs, source hashes and BIN checksum together. Changes require a new build identity and renewed review/testing.

## Licensing and history

`LICENSE`, `LICENSING.md` and `THIRD_PARTY_NOTICES/` are preserved. No TI demo algorithm or proprietary toolchain is bundled. Historical v0.2.0 documentation/manifest are retained under `HISTORY/` and do not describe this candidate's changed BIN.
