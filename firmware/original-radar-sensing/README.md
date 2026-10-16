# Original Radar Sensing 0.3.3-dev (unreleased source)

## Status of this source tree

This source is unreleased **0.3.3-dev**. It has **not** been built with the TI toolchain, **not** flashed and **not** physically tested; no 0.3.3-dev BIN exists, so no binary carries its version or its `BUILD_IDENTITY.json` build ID. The **0.3.2-rc1** candidate in `build/0.3.2-rc1/` (build `87589b2c32638ae119334c72`) remains the binary the matching viewer accepts. The rollback chain is unchanged: 0.3.2-rc1 candidate, then the 0.3.1-rc1 `build/original_radar_sensing.bin`. The 0.3.2-rc1 source documentation and identity are preserved verbatim under `HISTORY/v0.3.2-rc1/`.

0.3.2-rc1 later ran on hardware on 2026-10-12 ([first combined run](../../docs/FIRST_COMBINED_RUN_2026-10-12.md)): one owner-run IWR6843AOPEVM paired, streamed 283 complete consecutive 5 Hz frames over 56.4 seconds with zero reported faults, accepted 53 keepalives and returned to fault-free idle after a manual Stop. That brief acquisition/Stop test was reviewed by the development agent; it is not independent validation. Deliberate keepalive-loss/disconnect Stop, endurance, timestamp rollover, calibration, position and activity accuracy remain untested on hardware. `build/0.3.2-rc1/BUILD_RECEIPT.json` (and `RELEASE_MANIFEST_0.3.2-rc1.json`) are unmodified build-time records: their pending physical-acceptance fields describe the state when they were written and were deliberately not edited after that run.

0.3.3-dev changes, host-tested only, with the wire protocol, commands, ACK formats and fault-code meanings unchanged:

- A rejected RF stop, or a stop with no frame-end event, now leaves the radio marked started and open; the next `stop` (accepted in fault state) retries, and RF is closed only after the frame end. The success path is unchanged.
- The status `fault` field keeps the first fault cause. A later failure (for example a revoked lease, failed RF stop or busy DSP) no longer replaces it.
- `make clean` removes only generated intermediates and refuses a receipted release folder. `CGT_ROOT` follows `TI_ROOT`, `tools/package_firmware.sh` shares the Makefile's `TI_ROOT`/`SDK_ROOT` defaults, and builds check `armcl --compiler_revision` against 16.9.6 (override `ALLOW_UNPINNED_CGT=1` for a non-release experiment). The guard has not been run on a TI host.
- Release-tool checks no longer use Python `assert`, so `python -O` cannot turn a failure into success. Host-test outputs go to `build/host-tests/`. New host tests compile the real `src/radio.c` and `src/acquisition.c` against host fakes in `tests/stubs/`, and the frame-size test covers the true worst case.

Supervised **continuous-session dual-UART candidate**. Explicit `session continuous` runs the existing 5 Hz FMCW frames until manual Stop, a fault, or loss of a matching active-session control keepalive. Successful keepalives impose no fixed duration cap. Bounded `session 1` through `session 5` remain available. See [CONTINUOUS_SESSION.md](CONTINUOUS_SESSION.md) for the exact keepalive and timestamp contract, and [WINDOWS_BUILD.md](WINDOWS_BUILD.md) for the actual installed official build chain. The original sensing/DSP/spatial implementation remains independently authored; TI supplies device support, RF link and runtime. This is a three-peak exploratory stream, not a validated people/activity classifier or full-session recorder.

## Fixed AOP receiver polarity

This candidate corrects an omitted, documented AOP antenna-feed polarity pattern. The physical RX 0..3 spatial configuration now uses (+1, −1, +1, −1), matching the official SDK AOP profile and the TI Rev G guide Figure 4-8. This is a fixed hardware convention, not fitted calibration or a TI processing algorithm. The generic estimator, coherence threshold, ±0.85 direction limit and physical direction-disk rejection are unchanged. Raw phasors remain raw; the compensation is applied only inside spatial estimation.

Boot selftest_version is now 2, adding failure bit 256 for an analytic wave with the AOP feed inversion. A passing test does not validate physical direction signs, per-unit phase calibration, mount geometry or people tracking. The phase/axis/world-height validity flags remain unasserted without their separate evidence. The earlier v0.3.0-rc1 omission made the supplied phase patterns fail the conservative vertical-direction check; the correction does not relax that check.

## Changes

- Control/status/ACK: MSS SCI-A, Enhanced COM, 115200 baud, 8 N 1.
- Frames and explicit no-RF pairing probes: MSS SCI-B TX-only, Standard COM, 921600 baud, 8 N 1.
- Protocol `original-radar-v2`; each record is one strict LF-terminated JSON object. The two lanes must never be merged or decoded as TI TLV.
- Each UART retains the bounded TX-ready guard before and after writes, with an independent mutex and failure latch. Data transport failure blocks new starts and stops an active run through the watchdog while the control channel remains available where possible.
- A fresh host-generated 128-bit nonce, config generation and session number bind both ports before each run. A data-only probe must be observed before control `pairConfirm`; this is correlation, not authentication.
- Bounded durations remain **1–5 seconds**, with a 5-second staged default. `session continuous` explicitly selects recurring frames with a rolling 5000 ms control lease; `session 0` and historical 60-second numeric configs remain rejected.
- The reviewed scalar spatial correction and boot no-RF synthetic self-test remain unchanged. Failure prevents RF/acquisition initialization and Start.

## First hardware acceptance

1. Use only the matching accepted BIN/build identity and matching dual-UART viewer. Actual flashing and board operation are user actions; cloud tests do not establish board acceptance.
2. After reset, open Enhanced control at 115200 and Standard data at 921600. COM numbers depend on the user's computer. Do not send commands on data.
3. Query control `status` repeatedly, one request at a time. Every status and ACK must be individually LF-framed, channel=control, exact version/build/protocol, selftest_ok=true, selftest_mask=0, idle and fault-free. Malformed or merged JSON is failure, never a repaired receipt.
4. Configure measured geometry explicitly using `config/dual_uart_acceptance.cfg.template`, choosing 1–5 seconds. It is intentionally invalid until the tilt placeholder is replaced. Config/commit do not start RF.
5. The viewer generates a fresh nonce, sends `pair <nonce>` on control, observes the matching data_probe exclusively on data, sends `pairConfirm <nonce>` on control, and queries fresh control status. Exact build/protocol/nonce/config/session must all match and paired must be true.
6. Stop here for the initial **no-RF dual-port acceptance capture**. Only after those receipts are reviewed should a separately authorized bounded sensing test be considered. No automatic configuration, pairing-driven RF, or automatic Start.
7. During a later explicit run, a data disconnect, stale stream, malformed record or correlation mismatch must make the viewer stop via control and cease keepalives. Firmware's TX-only UART cannot prove the USB receiver is still reading. Bounded runs retain finite RF frame counts; continuous runs stop on lease expiry while all existing fault/watchdog paths remain enabled.

`stop` and `status` remain control commands. Every configuration change, successful commit, stop/end, failed pair, or transport fault revokes pairing; every Start consumes its one-shot permission before RF preparation. A new run requires a fresh pairing sequence. Reset clears configuration and pairing.

## Transport and runtime limits

Readiness waits use wrap-safe elapsed ticks, reject elapsed >5 configured 1 ms ticks, and perform at most 6 polls. A late scheduler wake fails closed even if TX is now ready. The existing driver write timeout remains 200 ticks. A short/failed write or readiness timeout latches that lane until reset; it never blindly retries a possibly partial line. Fault 401 means control transport; 483 means data setup; 484 means data transport; 485 means config generation exhausted. A failed control lane may be unable to report its own fault. These callback bounds assume the RTOS and callbacks return; no test proves a hard wall-clock bound under a frozen scheduler.

SOC/clock/MPU/UART startup occurs before the synthetic self-test. The integrated image is not hardware-free. A passing CPU test establishes neither calibration nor physical UART/RF behavior.

## Current mounting guidance

The historical project mount reference (not a default or calibration; see [MOUNTING.md](https://github.com/MASTER246C/radar-studio-chromebook/blob/main/MOUNTING.md)) is **4 feet (1.2192 m) from floor to the flat TOP of the bracket, excluding screws**. The chip-center offset is only an estimate, **+0.003175 to +0.00635 m**, giving an estimated antenna height interval **1.222375–1.22555 m** (midpoint about **1.224 m**, not a calibrated measurement). Actual downward tilt is unconfirmed. The earlier 2 m / 15-degree geometry is a historical example, not the current setup or a universal requirement. `config/room64_v1.cfg` retains that historical reference; do not use it unchanged for the current mount. See `config/current_mount.cfg.template` and `config/current_mount_metadata.json`; measure tilt before completing the template. RX phase and world-axis orientation are also unverified. Thus world coordinates and posture stay unknown, even if a numeric offset is later provided without the other calibration.

## Build and verification

See PROTOCOL.md for the exact contract, VALIDATION.md for evidence and limits, and BUILD_IDENTITY.json for source/toolchain identity. Run `bash tests/run_tests.sh` for host checks; every output goes to `build/host-tests/`. The 0.3.2-rc1 candidate was built with the official SDK 03.06.02.00-LTS, TI ARM compiler 16.9.6.LTS, SYS/BIOS 6.73.01.01 and XDCtools 3.50.08.24 using:

```
make TI_ROOT=/opt/ti CGT_ROOT=/opt/ti/ti-cgt-arm_16.9.6.LTS BUILD=build/0.3.2-rc1 all
ELF="$PWD/build/0.3.2-rc1/original_radar_sensing.xer4f" SDK_ROOT=/opt/ti/mmwave_sdk_03_06_02_00-LTS bash tools/package_firmware.sh "$PWD/build/0.3.2-rc1/original_radar_sensing.bin"
python3 tools/verify_sensing.py build/0.3.2-rc1/original_radar_sensing.xer4f build/0.3.2-rc1/original_radar_sensing.map
```

These commands record how the candidate was made; do not rerun them into `build/0.3.2-rc1/`. A future 0.3.3-dev build must use a new folder, for example `BUILD=build/0.3.3-dev` with matching ELF/BIN paths. `make clean` never removes BIN/RPRC, receipts or verification files and refuses a folder holding `BUILD_RECEIPT.json`. The compiler-revision guard runs before configuration and compilation.

The package script documents Mono and optional official compatibility-tool variables. Never flash emulator-only test ELFs. The public install/source package omits private debug ELF/map/link XML and raw build logs; those are retained separately for audit. No private board capture is included.

The 0.3.2 candidate is kept separately under `build/0.3.2-rc1/`. The historical `build/original_radar_sensing.bin` and `RELEASE_MANIFEST.json` remain 0.3.1-rc1 bytes, preserved with their source documentation/identity under `HISTORY/v0.3.1-rc1/`. Do not relabel the prior BIN. The candidate has its own receipt and `RELEASE_MANIFEST_0.3.2-rc1.json`; both still read pending because they are unmodified build-time records. The candidate later ran on hardware on 2026-10-12 within the limits stated above. Build/packaging never flash a board. Retain the prior accepted BIN and matching viewer as the rollback artifacts.

The legacy `host/` collector and its tests are retained for historical v1 replay; they are not a dual-port v2 controller, and `tests/run_tests.sh` labels them as legacy v1 tests. Use the specifically accepted v2 viewer for this candidate.

## Licensing and history

All vendor notices and licenses are retained. Read LICENSE, LICENSING.md and THIRD_PARTY_NOTICES/ before redistribution. Earlier source documentation is preserved in HISTORY; earlier released firmware has not been replaced.

## Range represented by the locked profile

At Fs=2.5 Msps, 64 samples and the quantized 29.981732 MHz/µs slope, range-bin spacing is about 0.195296 m. Default bins 4–30 represent approximately 0.781–5.859 m. The implementation permits at most positive bin 31 (about 6.054 m), below the positive-half-spectrum Nyquist interval of about 6.249 m. A 20 m display scale does not change this profile or establish 20 m human detection. Usable human range remains unmeasured.
