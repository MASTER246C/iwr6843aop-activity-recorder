# Original Radar v2 dual-UART contract

Version 0.3.0-rc1. Every record is one LF-terminated JSON object, with protocol `original-radar-v2`. Unknown numeric quantities are null, never NaN/Infinity. Strict framing is required; concatenated JSON is an error, not a successful receipt.

## Physical channels

- Control: UART_open(0), MSS SCI-A, SDK N5/N4 PADBE/PADBD aliases corresponding to AOP U16/V16; Enhanced COM; 115200 baud, 8N1.
- Data: UART_open(1), MSS SCI-B TX-only, SDK F14/PADAJ alias corresponding to AOP E2, UARTB_TX function 6; Standard COM; 921600 baud, 8N1. No DSS startup is needed.
- The official [TI Rev G EVM guide section 4.10.1](https://www.ti.com/lit/ug/swru546e/swru546e.pdf#page=59) identifies Enhanced=user/application and Standard=data. COM numbers are not fixed identities.

Control carries only identity/status/ACK. Data carries only data_probe/frame. All control records include channel="control", dual_uart=true, control_baud=115200 and data_baud=921600. All data records include channel="data". Data records never count as control acceptance receipts.

## Configuration

Commands are ASCII lines, at most 191 bytes before LF. Wait for the matching control ACK after each nonempty/noncomment line. `orConfig 1`, `profile room64_v1`, `mount <bracket_mm> <down_tilt_mdeg> <signed_offset_mm|unknown>`, `detect <first_bin> <last_bin> <minimum_power> <noise_multiplier>`, `session <seconds>`, `commit` retain their v1 meanings, except seconds must now be an integer 1–5. Historical 60-second configs are rejected. Reset/default duration is 5 seconds, but all required fields must still be explicitly supplied before commit. Use the new acceptance template with measured tilt.

No config or pairing command starts RF. Explicit `start` performs calibration and finite acquisition only after all gates pass. `stop` and `status` remain available on control. Configuration is rejected while non-idle.

## Cross-port proof and one-shot Start

1. After a fault-free boot self-test and successful config commit, the host generates a fresh 32-character lowercase hexadecimal nonce (128 random bits) for this attempt.
2. Control command: `pair <nonce>`. Allowed only idle, not processing, valid committed config, passing boot self-test, neither transport latch failed, no app fault. It revokes prior permission and reserves a new session_id. A successful full data transmission emits:

```
{"type":"data_probe","channel":"data","protocol":"original-radar-v2","firmware_version":"0.3.0-rc1","build_id":"<24 hex>","pair_nonce":"<32 lowercase hex>","config_generation":1,"session_id":1}
```

3. The control ACK for pair has code 0 only after successful data transmission. It does not prove the host received the probe. The host must separately observe the exact probe on its chosen data connection, matching the accepted build/protocol/nonce and generation/session.
4. Only then send control `pairConfirm <same nonce>`. It must match the current successfully sent probe exactly. This enables one Start permission. Query fresh control `status`, verify the complete status and matching ACK, and require paired=true plus the same nonce/generation/session. A control status alone cannot replace data proof.
5. Start consumes the internal permission before touching RF. paired stays true during the active session for correlation. Stop/end revokes it. A second Start requires a new pair/probe/confirm sequence.

Successful commit increments config_generation; every probe increments session_id. Both are unsigned 32-bit integers and refuse overflow instead of wrapping. Reset clears them and the nonce, so the host must use a fresh random nonce on every connection/attempt. Configuration edits, commit, stop/end, failed pairing and transport faults revoke permission. A failed or mismatched pairConfirm also revokes it. Invalidated status may retain the old tuple for diagnostics, but paired=false is never authorization.

The host must invalidate proof on either disconnect/reconnect/reset, config changes, faults, read errors, malformed data or stale data. Every frame must match the active tuple. This binds current streams by a host challenge; it is not authentication or a stable hardware device ID. TX-only SCI-B cannot detect whether USB/host software is consuming bytes, so host-side stop-on-loss is essential.

## Control schema

Identity includes type=identity, app=original-radar-sensing, version, build_id, baud=115200, sensing=true, classifier=false and the common control fields.

ACK includes type=ack, command (canonical recognized token or invalid), integer code, state and configured, plus common control fields. Code 0 is successful handling; negative is rejection/failure. Comments/empty lines get no ACK. ACKs have no request ID; hosts must serialize requests and check fresh corresponding records.

Status includes type=status, state (idle/starting/running/stopping/fault), configured:boolean, fault/capture_fault/rf_fault:uint32, rf_error:int32, frames/deadline_misses:uint32, firmware_version:string, build_id:24-hex string, selftest_version:1, selftest_ok:boolean, selftest_mask:uint32, paired:boolean, pair_nonce:string (empty before first probe), config_generation/session_id:uint32, control_tx_fault/data_tx_fault:boolean, and common control fields.

Start error -25: no passing boot self-test; -26: application/transport fault; -20: invalid state/config; -27: no unconsumed paired permission. Pair errors -29: state/gate failure, -30: malformed nonce, -31: missing/mismatched probe confirmation, -32: exhausted session counter, -33: failed probe transmission. Faults 401/483/484/485 mean control transport/data setup/data transport/config-counter exhaustion. Boot self-test failure is fault 482.

## Data frames

The original frame payload is retained, with protocol v2 and mandatory channel="data", pair_nonce, config_generation and session_id. No frame serves as identity/self-test/ACK proof. Existing payload fields include sequence, timestamp_us, complete, missed_frames, capture_fault, quality, frame_events, processing_us, previous_tx_us, deadline_misses, mount, range_bin_m, velocity_bin_mps, profile_energy, detected_count, count, truncated and detections. Up to three detections are serialized; each has range_m, velocity_mps, power, four complex phasors, sensor_xyz_m, world_xyz_m, spatial_flags and activity=null. Axis/phase calibration remain unverified, so world height is not claimed. Record cap 1800 bytes; overflow/invalid values suppress the whole record and fault the run.

## RF/acquisition layout

- Physical RX0,1,2,3; TX0 only; 64ADC samples ×32chirps ×4RX.
- Little-endian signed16bit pairs `{real,imag}`, ADCBUF sampleInterleave0, channelInterleave1.
- Contiguous `[chirp][receiver][sample]` frame:32,768bytes. Hardware EDMA copies1,024bytes per chirp event into alternating L3 frames.
- Start60.25GHz; nominal slope30MHz/us quantized DOWN to the valid even60GHz code; ADC2.5Msps; ADC start6us; ramp40us; idle210us; TX start1us; oneTX with6dB backoff; RX gain30dB;5frames/s.
- Actual quantized slope/start frequency, not rounded nominal values, feed DSP/spatial calculations. No VCO forcing, beamforming phase steering, continuous-wave mode, or custom calibration bypass.
- About0.195m range-bin spacing and evaluated bins4..30, roughly0.78–5.85m. Analog HPF175/350kHz attenuates close returns;350kHz corresponds to about1.75m at30MHz/us. These are design numbers, not verified detection range or accuracy.

## Failure and no-RF acceptance

Each lane has an independent mutex, SDK SCI register readiness check and latched failure. Readiness uses at most six polls and five elapsed 1-ms ticks, with wrap-safe subtraction. Existing writeTimeout is 200 ticks. Failure releases the lock and blocks subsequent writes on that lane until reset. Data failure sets app fault and the command watchdog stops the run while control remains usable if healthy. Control failure can prevent any fault receipt. UART completion does not prove USB receipt.

First board acceptance is status/ACK framing plus no-RF cross-port probes only. Require exact accepted build and passing boot self-test before configuration, then matching data probe and fresh paired control status before considering a separately authorized 1–5 second run. Never synthesize success from malformed or missing records.
