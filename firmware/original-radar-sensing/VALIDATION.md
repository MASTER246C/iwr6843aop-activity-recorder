# 0.3.1-rc1 verification record

Status: offline-verified fixed-AOP-polarity candidate; physical acceptance pending.

## Artifacts

- Version: `0.3.1-rc1`
- Build ID: `abb27b48c1bdb956c0de704b`
- BIN: 344,836 bytes
- BIN SHA-256: `668476d4130f659cef4c1fea3b8eb436d5025f20853dc30ca83380f9bd38a6bf`
- Appended CRC32: `0x3ab0f587`
- Audit ELF SHA-256: `9a0ee0978ebac27b07918628f81952eb670c652cd7a0f6feda46d3793eced7ca` (debug artifact retained privately, not distributed)

## Corrected omission

The prior production config used unit corrections on every physical RX. Official TI AOP hardware documentation requires alternating receiver-feed polarity compensation. The new AOP-specific initializer applies (+1,−1,+1,−1) and is called by the production spatial setup; the generic estimator function and rejection thresholds are unchanged. This known wiring convention is not measured phase calibration. No phase/axis/world-height calibration flags are asserted.

Selftest_version2 adds synthetic inverted known-angle input, checked against the same production initializer, with failure bit256. Existing failure bits and Start gating remain.

Additional validation: an independent synthetic grid covers3,228 angle/carrier/common-phase cases, unequal amplitudes, common-sign invariance, preserved FOV/disk/NaN rejection and unknown world coordinates. Private input-derived regression (kept outside the package) checks3 supplied phasor sets: the prior host and actual TI estimator reject all3, while the corrected host and final TI estimator/config accept all3 with numeric/in-FOV flags only. No private phasors or captures are distributed, and these results do not establish physical coordinate accuracy.

## Checks completed

- Official TI compiler/SDK serial compile and link, warnings treated as errors; official image conversion and independent appended CRC check.
- 1,874 spatial direction cases (937 generic and 937 AOP-inverted synthetic waves), 706 DSP checks in ordinary and sanitized builds, bounded config/ownership/JSON checks and 11 historical collector regression tests. The legacy collector is not the new v2 controller.
- Host ASan/UBSan and strict compiler checks for the actual main source with mocked peripherals: all 32 individual boot-failure bits, pending self-test, application fault and both transport latches prevent Start hardware calls. No auto RF/config.
- Actual-main pairing tests: no RF from pair/probe/confirm; malformed/wrong confirmation revokes; configuration/stop revoke; Start consumes permission before mocked RF, cannot be repeated; data transport timeout while running invokes watchdog stop; control status still works. Data-open failure creates only control command service and no RF/acquisition initialization.
- Actual main adapter checks select separate SCI-A/SCI-B readiness registers. Independent pending-TD states, locks and UART handles model both lanes.
- Independently authored exact-source dual framing test: 1,000 distinct nonce sessions, 4,000 control records and 2,000 data records, all individually parsed and confined to the right lane with matching nonce/config/session tuples. No physical peripheral behavior is implied by that model.
- Portable transport tests cover pre/post readiness timeout, short/negative writes, permanently latched no-retry, one-byte writes, wrap, frozen ticks, and scheduling delay beyond the deadline. Unguarded control model reproduces missing LF; guarded model preserves records.
- Portable link tests cover strict lowercase 32-hex nonce syntax, probe-before-confirm, one-shot Start, revocation and refusal to wrap uint32 config/session counters.
- Final production TI objects executed with synthetic tests under QEMU Cortex-R5F instruction emulation: spatial/boot self-test, record transport and link session all exit 0. Emulator ELF code/data bytes are preserved at original virtual addresses when repacked for the user-mode loader. Emulator test images are never flash images.
- Structural ELF checks: ARM memory/reset-vector layout, two full ADC buffers, required original application/low-level driver symbols, no listed TI demo/DPU/DPC/tracker/signal-processing pipeline libraries.
- Worst-case numeric frame serialization: 1,515 bytes within the 1,800-byte cap, including 32-byte nonce and maximum uint32 generation/session. Invalid/overflowed records are never partly sent.

## Inherited spatial and framing evidence

The earlier exact official rebuild reproduced the complete original v0.2.0 BIN and defective estimator instructions. The scalar estimator remains the reviewed correction. The SDK UART interrupt path can acknowledge the last queued byte before it has left the transmit data register, while a following write stores its first byte without checking TXRDY. The guarded transport retains both readiness checks on both lanes. This is an evidence-backed software mechanism, not a hardware logic-analyzer proof.

## Explicit limits and first acceptance

No physical boot, UART routing/baud, peripheral timing, interrupt schedule, RF/acquisition, stack high-water, geometry or tracking accuracy has been validated for this build. Cortex-R5F user-mode instruction execution is not a full IWR6843 Cortex-R4F board emulator. A passing boot CPU self-test does not establish physical correctness or calibration.

Readiness is TD capacity, not full wire idle. A wake later than five 1-ms ticks fails closed even if ready; callback limits are not a wall-clock guarantee if the RTOS stops returning. The UART write timeout remains 200 ticks. TX-only data cannot detect a disconnected USB reader merely from successful writes. The host must stop via control on data disconnect/error/staleness; finite 1–5-second RF frame configuration is a fallback. A failed control transport may be unable to report its own fault.

First user-board acceptance is repeated strictly framed status/ACK and a fresh no-RF cross-port nonce probe/confirm/status capture only. Reject wrong identity, missing/merged/malformed records, nonzero diagnostics or self-test failure. The candidate streams at most three radar peaks with activity=null; it does not establish people count, identity, posture or a validated classifier.
