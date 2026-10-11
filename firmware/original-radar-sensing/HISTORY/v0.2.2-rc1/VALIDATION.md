# 0.2.2-rc1 verification record

Status: offline-verified transport repair candidate; new-build physical acceptance pending.

## Artifacts

- Version: `0.2.2-rc1`
- Build ID: `e085ac80bef63e23cb8185f4`
- BIN: 342,788 bytes
- BIN SHA-256: `277bddab7afa0ad937065082b1e137b3724e31024edf28367e58c4c57384ce85`
- Appended CRC32: `0x7dba14a7`

## Defect evidence and correction

The pinned SDK interrupt UART writer can signal completion immediately after placing the final byte in the transmit data register. Its next interrupt-driven write puts the first byte into that register without first testing TXRDY. Source inspection and a pending-register overwrite model support a mechanism for losing the final newline between consecutive JSON records. This is a strong software explanation, not a physical logic-analyzer measurement.

The application now checks volatile SCI-A TXRDY before and after each write while holding the existing transmit mutex. Timeout/short write latches transmission off until reboot and sets fault 401. Start rejects that latch or any application fault before radio preparation. No newline parser repair or synthetic acceptance was added.

## Checks completed

- Clean final serial compile/link with exact official TI tools, warnings treated as errors.
- All existing host checks: 937 spatial directions, 706 DSP checks in ordinary and sanitized builds, config/ownership/JSON tests, 11 collector tests.
- Actual-main mocked-peripheral test: all 32 individual boot-failure bits and pending self-test block hardware calls; transport latch and application fault block Start even with valid committed configuration; no auto RF/config.
- Transport tests: unguarded model loses the record terminator; guarded repeated records preserve bytes. Covers pre-existing pending bytes, pre/post readiness timeout, short/negative writes, permanently latched no-retry, one-byte records, tick wrap, frozen ticks and scheduling delay beyond deadline.
- Independently authored exact-source framing harness: 1,000 synthetic status/ACK pairs, 2,000 parsed records/newlines, 403,000 bytes, no byte loss under the stated model; ASan/UBSan and strict compiler warnings pass.
- Final TI-generated transport object linked with deterministic peripheral model: QEMU Cortex-R5F instruction emulation exit 0.
- Final spatial estimator/boot-self-test objects: QEMU Cortex-R5F instruction emulation exit 0.
- ELF architecture/memory/dependency structural checks and independently recomputed BIN CRC32 pass. RadarSS vendor payload is unchanged.

## Explicit limits

The new image has not been physically booted or tested on the user's board. QEMU Cortex-R5F user-mode execution does not model the IWR6843 Cortex-R4F, its UART/peripherals, real interrupt scheduling, physical RF, timing or stack high-water. TXRDY is data-register readiness, not full wire-idle indication.

A wake later than the 5-tick deadline fails closed, even if TXRDY is then ready. Finite polling limits repeated returning callbacks; no wall-clock claim applies if the scheduler or Task_sleep itself stops returning. Fault 401 may be unreportable once the transport is latched off. A new Start is blocked, and existing running-session watchdog handling remains in place.

Follow README: after separately authorized reflash, send only repeated status queries, waiting for each separate complete status/ACK. Wrong build, malformed/merged/missing records, nonzero diagnostics or self-test failure block configuration and RF. Do not bypass this gate to test tracking or visualization. Physical geometry, axes/IQ/phase calibration and activity/classification remain unverified.
