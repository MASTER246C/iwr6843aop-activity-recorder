# Offline validation and limits

## Completed checks

- Clean regeneration of SYS/BIOS/XDC support and Cortex-R4F compile/link with TI ARM CGT 16.9.6.LTS. All original application objects compile with warnings treated as errors; final linker warnings are also errors. See `build/clean-build.log`.
- Host C99 ASan/UBSan tests of strict staging/commit parsing, integer/line bounds, nonterminated input, embedded NUL/control-byte rejection, signed mount-height validity, finite RF physical units, capture ownership/generation and missing-frame-boundary rejection, bounded energy sums and JSON serialization. Overflowed or invalid required numeric output is rejected rather than transmitted partially.
- Original DSP: 706 checks covering FFT/DFT agreement, static and moving synthetic tones, physical scaling, thresholds, clipping, incomplete/stale/gapped frames and bounded workspace. Both ordinary and sanitizer builds pass. DSP workspace is 42,552 bytes; result is 560 bytes; each default ADC frame is 32,768 bytes.
- Original spatial module: 937 synthetic directions plus unknown/signed-offset/tilt/boundary/invalid-input cases, under ASan/UBSan. Synthetic phasors validate mathematics only, not AOP channel order or calibration.
- Dedicated collector: 11 unit tests including no-write passive mode, explicit start/stop, acknowledgement correlation, configuration that cannot embed start, malformed/oversized/non-finite JSON, unknown geometry and optional three-value energy summaries. No serial hardware was opened in testing.
- ELF checker verifies ARM ELF32 little-endian format, reset vectors at zero, program/data/L3 bounds, exactly 64 KiB L3 frame storage, expected original sensing/driver symbols, and absence of listed demo/DPU/DPC/DPM/tracker/signal-processing-library symbols.
- TI tools convert the ELF into an MSS RPRC and a multicore BIN with the unmodified supplied RadarSS image. The appended CRC32 is independently recomputed using Python zlib; exact artifact hashes are in `PUBLIC_MANIFEST.json`.
- The actual C serializer's synthetic frame was replayed through the actual Python collector. The original audit used synthetic serializer/replay fixtures, never board recordings. Those generated audit outputs are omitted from this public package; the included host tests reproduce synthetic checks.

Logs and structural checks are evidence about their stated scope, not proof of physical correctness. The separate private build audit covered code/map/collector behavior; it did not establish physical operation.

## UART and timing budget

At 115200 baud, 8N1, theoretical capacity is 11,520 bytes/s or 2,304 bytes per 5 Hz frame. Output is capped at 1,800 bytes including newline, with at most three peaks; the numeric-boundary stress frame including energy summaries is 1,395 bytes. The serializer checks every append and suppresses any record that exceeds its bound. This leaves margin for acknowledgements/status but does not justify flooding commands during acquisition.

On-board timing fields use 1 ms BIOS ticks, not a cycle counter. `processing_us` covers DSP and energy aggregation before spatial serialization. `previous_tx_us` describes the last UART write, possibly a status/ack. A separate check covers the complete DSP/spatial-serialization/transmission path and faults at 180 ms. UART has a finite write timeout. If acquisition, processing or transmission cannot keep up, the firmware attempts to stop and reports a fault; it does not silently change the radar frame period. Actual timing, deadline margin, UART behavior, CPU utilization and task-stack high-water have not been measured.

## Acquisition invariants and unverified hardware assumptions

The original double-buffer model has FREE/FILLING/READY/PROCESSING states and generation tokens. Shared interrupt/task state is volatile; claims, releases, stop and arm transitions use interrupt critical sections. Each hardware EDMA completion covers 32 A-sync chirp transfers. A separate SOC frame-start interrupt checks that the preceding frame completed exactly once. Missing completion before the next frame start faults and disables EDMA. Stop invalidates outstanding generations. The processing task's busy guard covers the entire claim/process/transmit lifecycle, including higher-priority command preemption.

This relies on ADCBUF presenting the completed ping/pong bank through the same fixed read window for each chirp event, RX offsets matching the selected format, and each event representing one chirp. The SDK driver returns a fixed channel base plus configured offset; its low-level ADCBUF unit test uses the same source address across repeated bank swaps. The SDK SOC MPU marks L3 at 0x51000000 as normal non-cached memory. These source facts support this design but are **not** a hardware demonstration of the full path. Independent frame boundaries make many lost-event cases fail closed; they cannot prove correct IQ data or detect every analog/digital corruption.

Positive-half range FFT convention and velocity sign must be tested with a known reflector and controlled motion. A wrong IQ/sign convention could suppress real returns even while all synthetic tests pass. Quality value 44 only indicates uncalibrated RX phase/power and unverified motion sign, not acquisition certification.

## Remaining physical validation gates

1. Verify correct board revision, power, boot mode and UART routing without assuming connector/switch state. SDK N5/N4 pad aliases mean AOP U16/V16; actual board routing/baud has not been measured.
2. Verify SOC/RadarSS APLL startup and identity/status output. Some low-level clock/startup calls may block before telemetry is available.
3. On an approved bounded RF run, verify accepted profile settings, one frame-start per frame, exactly 32 chirp events/transfers, alternating-buffer content, RX ordering and real/imag ordering with a known target or suitable driver test pattern.
4. Measure RF behavior/emissions, stop and finite-frame/session behavior, async fault responses, processing deadlines and task-stack high-water. Standard API ranges are not regulatory certification.
5. Establish range bias, positive-frequency/velocity conventions, inter-RX phase corrections, world axes and the signed antenna-center offset after tilt. Until then, spatial output remains exploratory and world height stays unknown.
6. Collect labeled real sessions, design/train an original posture/motion classifier, and evaluate on held-out sessions/people and difficult stationary cases. There is currently no classifier or accuracy result.

No board boot, UART capture, RF emissions, actual ADC acquisition, real point cloud, activity classification, flashing, or deployment was performed. The BIN is a compiled development implementation with these outstanding gates.
