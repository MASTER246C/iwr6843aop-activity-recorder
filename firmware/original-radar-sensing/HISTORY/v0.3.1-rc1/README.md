# Original Radar Sensing 0.3.1-rc1

Temporary **dual-UART hardware-acceptance candidate**. The original sensing/DSP/spatial implementation remains independently authored. TI supplies the low-level device support, RF link and runtime, not the application processing or tracker. This is a three-peak exploratory radar stream, not a validated people/activity classifier or full-session recorder.

## Fixed AOP receiver polarity

This candidate corrects an omitted, documented AOP antenna-feed polarity pattern. The physical RX0..3 spatial configuration now uses (+1, −1, +1, −1), matching the official SDK AOP profile and the TI Rev G guide Figure 4-8. This is a fixed hardware convention, not fitted calibration or a TI processing algorithm. The generic estimator, coherence threshold, ±0.85 direction limit and physical direction-disk rejection are unchanged. Raw phasors remain raw; the compensation is applied only inside spatial estimation.

Boot selftest_version is now 2, adding failure bit256 for an analytic wave with the AOP feed inversion. A passing test does not validate physical direction signs, per-unit phase calibration, mount geometry or people tracking. The phase/axis/world-height validity flags remain unasserted without their separate evidence. The earlier v0.3.0-rc1 omission made the supplied phase patterns fail the conservative vertical-direction check; the correction does not relax that check.

## Changes

- Control/status/ACK: MSS SCI-A, Enhanced COM, 115200 baud, 8N1.
- Frames and explicit no-RF pairing probes: MSS SCI-B TX-only, Standard COM, 921600 baud, 8N1.
- Protocol `original-radar-v2`; each record is one strict LF-terminated JSON object. The two lanes must never be merged or decoded as TI TLV.
- Each UART retains the bounded TX-ready guard before and after writes, with an independent mutex and failure latch. Data transport failure blocks new starts and stops an active run through the watchdog while the control channel remains available where possible.
- A fresh host-generated 128-bit nonce, config generation and session number bind both ports before each run. A data-only probe must be observed before control `pairConfirm`; this is correlation, not authentication.
- Firmware now rejects durations outside **1–5 seconds**, with a 5-second staged default. Historical 60-second configs are deliberately rejected. This tighter limit is specific to this acceptance RC.
- The reviewed scalar spatial correction and boot no-RF synthetic self-test remain unchanged. Failure prevents RF/acquisition initialization and Start.

## First hardware acceptance

1. Use only the matching accepted BIN/build identity and matching dual-UART viewer. Actual flashing and board operation are user actions; cloud tests do not establish board acceptance.
2. After reset, open Enhanced control at 115200 and Standard data at 921600. COM numbers depend on the user's computer. Do not send commands on data.
3. Query control `status` repeatedly, one request at a time. Every status and ACK must be individually LF-framed, channel=control, exact version/build/protocol, selftest_ok=true, selftest_mask=0, idle and fault-free. Malformed or merged JSON is failure, never a repaired receipt.
4. Configure measured geometry explicitly using `config/dual_uart_acceptance.cfg.template`, choosing 1–5 seconds. It is intentionally invalid until the tilt placeholder is replaced. Config/commit do not start RF.
5. The viewer generates a fresh nonce, sends `pair <nonce>` on control, observes the matching data_probe exclusively on data, sends `pairConfirm <nonce>` on control, and queries fresh control status. Exact build/protocol/nonce/config/session must all match and paired must be true.
6. Stop here for the initial **no-RF dual-port acceptance capture**. Only after those receipts are reviewed should a separately authorized bounded sensing test be considered. No automatic configuration, pairing-driven RF, or automatic Start.
7. During a later explicit run, a data disconnect, stale stream, malformed record or correlation mismatch must make the viewer stop via control. Firmware's TX-only UART cannot prove the USB receiver is still reading. The finite RF frame count and watchdog remain fallback limits.

`stop` and `status` remain control commands. Every configuration change, successful commit, stop/end, failed pair, or transport fault revokes pairing; every Start consumes its one-shot permission before RF preparation. A new run requires a fresh pairing sequence. Reset clears configuration and pairing.

## Transport and runtime limits

Readiness waits use wrap-safe elapsed ticks, reject elapsed >5 configured 1 ms ticks, and perform at most 6 polls. A late scheduler wake fails closed even if TX is now ready. The existing driver write timeout remains 200 ticks. A short/failed write or readiness timeout latches that lane until reset; it never blindly retries a possibly partial line. Fault 401 means control transport; 483 means data setup; 484 means data transport; 485 means config generation exhausted. A failed control lane may be unable to report its own fault. These callback bounds assume the RTOS and callbacks return; no test proves a hard wall-clock bound under a frozen scheduler.

SOC/clock/MPU/UART startup occurs before the synthetic self-test. The integrated image is not hardware-free. A passing CPU test establishes neither calibration nor physical UART/RF behavior.

## Current mounting guidance

The current user-reported mount reference is **4 feet (1.2192 m) from floor to the flat TOP of the bracket, excluding screws**. The chip-center offset is only an estimate, **+0.003175 to +0.00635 m**, giving an estimated antenna height interval **1.222375–1.22555 m** (midpoint about **1.224 m**, not a calibrated measurement). Actual downward tilt is unconfirmed. The earlier 2 m / 15-degree geometry is a historical example, not the current setup or a universal requirement. `config/room64_v1.cfg` retains that historical reference; do not use it unchanged for the current mount. See `config/current_mount.cfg.template` and `config/current_mount_metadata.json`; measure tilt before completing the template. RX phase and world-axis orientation are also unverified. Thus world coordinates and posture stay unknown, even if a numeric offset is later provided without the other calibration.

## Build and verification

See PROTOCOL.md for the exact contract, VALIDATION.md for evidence and limits, and BUILD_IDENTITY.json for source/toolchain identity. Run `bash tests/run_tests.sh` for host checks. Rebuild with the official SDK 03.06.02.00-LTS, TI ARM compiler 16.9.6.LTS, SYS/BIOS 6.73.01.01 and XDCtools 3.50.08.24:

```
make TI_ROOT=/opt/ti CGT_ROOT=/opt/ti/ti-cgt-arm_16.9.6.LTS all
SDK_ROOT=/opt/ti/mmwave_sdk_03_06_02_00-LTS bash tools/package_firmware.sh
python3 tools/verify_sensing.py
```

The package script documents Mono and optional official compatibility-tool variables. Never flash emulator-only test ELFs. The public install/source package omits private debug ELF/map/link XML and raw build logs; those are retained separately for audit. No private board capture is included.

The legacy `host/` collector and its tests are retained for historical v1 replay; they are not a dual-port v2 controller. Use the specifically accepted v2 viewer for this candidate.

## Licensing and history

All vendor notices and licenses are retained. Read LICENSE, LICENSING.md and THIRD_PARTY_NOTICES/ before redistribution. Earlier source documentation is preserved in HISTORY; earlier released firmware has not been replaced.

## Range represented by the locked profile

At Fs=2.5 Msps, 64 samples and the quantized 29.981732 MHz/µs slope, range-bin spacing is about0.195296m. Default bins4–30 represent approximately0.781–5.859m. The implementation permits at most positive bin31 (about6.054m), below the positive-half-spectrum Nyquist interval of about6.249m. A20m display scale does not change this profile or establish20m human detection. Usable human range remains unmeasured.
