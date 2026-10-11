# 0.3.0-rc1 verification record

Status: offline-verified dual-UART acceptance candidate; physical acceptance pending.

## Artifacts

- Version: `0.3.0-rc1`
- Build ID: `e0ce9b95204dee4ecbf0b9e0`
- BIN: 344,644 bytes
- BIN SHA-256: `e945b06a120f63dcb9c76625983940686f3cecc5c69781d9a24eb02228ff5deb`
- Appended CRC32: `0x4c12f1de`
- Audit ELF SHA-256: `429cf27fe202df4237e63fd326495663b622aafa371ac23b1cb8493396b98e34` (debug artifact retained privately, not distributed)

## Checks completed

- Official TI compiler/SDK serial compile and link, warnings treated as errors; official image conversion and independent appended CRC check.
- 937 spatial direction cases, 706 DSP checks in ordinary and sanitized builds, bounded config/ownership/JSON checks and 11 historical collector regression tests. The legacy collector is not the new v2 controller.
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
