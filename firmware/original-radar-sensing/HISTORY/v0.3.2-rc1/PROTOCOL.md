# Original Radar v2 dual-UART contract

Version 0.3.2-rc1. Every record is one LF-terminated JSON object, with protocol `original-radar-v2`. Unknown numeric quantities are null, never NaN/Infinity. Strict framing is required; concatenated JSON is an error, not a successful receipt. Exact version/build matching selects a viewer that implements the extensions in [CONTINUOUS_SESSION.md](CONTINUOUS_SESSION.md).

## Physical channels

- Control: UART_open(0), MSS SCI-A, SDK N 5/N 4 PADBE/PADBD aliases corresponding to AOP U 16/V 16; Enhanced COM; 115200 baud, 8 N 1.
- Data: UART_open(1), MSS SCI-B TX-only, SDK F 14/PADAJ alias corresponding to AOP E 2, UARTB_TX function 6; Standard COM; 921600 baud, 8 N 1. No DSS startup is needed.
- The official [TI Rev G EVM guide section 4.10.1](https://www.ti.com/lit/ug/swru 546 e/swru 546 e.pdf#page=59) identifies Enhanced=user/application and Standard=data. COM numbers are not fixed identities.

Control carries only identity/status/ACK. Data carries only data_probe/frame. All control records include channel="control", dual_uart=true, control_baud=115200 and data_baud=921600. All data records include channel="data". Data records never count as control acceptance receipts.

## Configuration

Commands are ASCII lines, at most 191 bytes before LF. Wait for the matching control ACK after each nonempty/noncomment line. `orConfig 1`, `profile room64_v1`, `mount <bracket_mm> <down_tilt_mdeg> <signed_offset_mm|unknown>`, `detect <first_bin> <last_bin> <minimum_power> <noise_multiplier>`, `session <seconds>` and `commit` retain their meanings. Bounded seconds remain 1–5. `session continuous` explicitly selects recurring frames with a 5000 ms active-host control lease; numeric 0 and historical 60-second configs remain rejected. Reset defaults to 5 seconds, and all required fields must be explicitly supplied before commit. Configuration remains idle-only; the template requires measured tilt.

No config or pairing command starts RF. Explicit `start` performs calibration and finite acquisition only after all gates pass. `stop` and `status` remain available on control. Configuration is rejected while non-idle.

## Cross-port proof and one-shot Start

1. After a fault-free boot self-test and successful config commit, the host generates a fresh 32-character lowercase hexadecimal nonce (128 random bits) for this attempt.
2. Control command: `pair <nonce>`. Allowed only idle, not processing, valid committed config, passing boot self-test, neither transport latch failed, no app fault. It revokes prior permission and reserves a new session_id. A successful full data transmission emits:

```
{"type":"data_probe","channel":"data","protocol":"original-radar-v2","firmware_version":"0.3.2-rc1","build_id":"<24 hex>","pair_nonce":"<32 lowercase hex>","config_generation":1,"session_id":1,"session_mode":"continuous","session_seconds":0,"keepalive_lease_ms":5000}
```

3. The control ACK for pair has code 0 only after successful data transmission. It does not prove the host received the probe. The host must separately observe the exact probe on its chosen data connection, matching the accepted build/protocol/nonce and generation/session.
4. Only then send control `pairConfirm <same nonce>`. It must match the current successfully sent probe exactly. This enables one Start permission. Query fresh control `status`, verify the complete status and matching ACK, and require paired=true plus the same nonce/generation/session. A control status alone cannot replace data proof.
5. Start consumes the internal permission before touching RF. paired stays true during the active session for correlation. Stop/end revokes it. A second Start requires a new pair/probe/confirm sequence.

Successful commit increments config_generation; every probe increments session_id. Both are unsigned 32-bit integers and refuse overflow instead of wrapping. Reset clears them and the nonce, so the host must use a fresh random nonce on every connection/attempt. Configuration edits, commit, stop/end, failed pairing and transport faults revoke permission. A failed or mismatched pairConfirm also revokes it. Invalidated status may retain the old tuple for diagnostics, but paired=false is never authorization.

The host must invalidate proof on either disconnect/reconnect/reset, config changes, faults, read errors, malformed data or stale data. Every frame must match the active tuple. This binds current streams by a host challenge; it is not authentication or a stable hardware device ID. TX-only SCI-B cannot detect whether USB/host software is consuming bytes, so host-side stop-on-loss is essential.

## Control schema

Identity includes type=identity, app=original-radar-sensing, version, build_id, baud=115200, sensing=true, classifier=false and the common control fields.

ACK includes type=ack, command (canonical recognized token or invalid), integer code, state and configured, plus common control fields. Code 0 is successful handling; negative is rejection/failure. Comments/empty lines get no ACK. ACKs have no request ID; hosts must serialize requests and check fresh corresponding records.

Status includes type=status, state (idle/starting/running/stopping/fault), configured:boolean, fault/capture_fault/rf_fault:uint32, rf_error:int32, frames/deadline_misses:uint32, firmware_version:string, build_id:24-hex string, selftest_version:2, selftest_ok:boolean, selftest_mask:uint32, paired:boolean, pair_nonce:string (empty before first probe), config_generation/session_id:uint32, control_tx_fault/data_tx_fault:boolean, and common control fields.

Start error -25: no passing boot self-test; -26: application/transport fault; -20: invalid state/config; -27: no unconsumed paired permission. Pair errors -29: state/gate failure, -30: malformed nonce, -31: missing/mismatched probe confirmation, -32: exhausted session counter, -33: failed probe transmission. Faults 401/483/484/485 mean control transport/data setup/data transport/config-counter exhaustion. Boot self-test failure is fault 482.

## Data frames

The 0.3.2 extension adds `session_mode` (`continuous`/`bounded`), `session_seconds` (0/1–5) and `keepalive_lease_ms` (5000/0) to status, probes and frames. Frames carry `timestamp_basis:"session_elapsed"`: `timestamp_us` is 64-bit elapsed time since acquisition arm, extended from per-frame unsigned deltas of the 32-bit 1 ms clock before multiplication by 1000. Timestamps reset on arm and are interpreted within the nonce/session tuple. See CONTINUOUS_SESSION.md for the keepalive command, matching tuple ACK, code-34 rejection, fault 463 expiry/revocation Stop and late/canceled-ACK rules. Continuous operation skips only the total-duration watchdog; all existing fault/stale/deadline gates remain enabled.

The original frame payload is retained, with protocol v2 and mandatory channel="data", pair_nonce, config_generation and session_id. No frame serves as identity/self-test/ACK proof. Existing payload fields include sequence, timestamp_us, complete, missed_frames, capture_fault, quality, frame_events, processing_us, previous_tx_us, deadline_misses, mount, range_bin_m, velocity_bin_mps, profile_energy, detected_count, count, truncated and detections. Up to three detections are serialized; each has range_m, velocity_mps, power, four complex phasors, sensor_xyz_m, world_xyz_m, spatial_flags and activity=null. Axis/phase calibration remain unverified, so world height is not claimed. Record cap 1800 bytes; overflow/invalid values suppress the whole record and fault the run.

## RF/acquisition layout

- Physical RX 0,1,2,3; TX 0 only; 64 ADC samples ×32 chirps ×4 RX.
- Little-endian signed 16 bit pairs `{real,imag}`, ADCBUF sampleInterleave 0, channelInterleave 1.
- Contiguous `[chirp][receiver][sample]` frame:32,768 bytes. Hardware EDMA copies 1,024 bytes per chirp event into alternating L 3 frames.
- Start 60.25 GHz; nominal slope 30 MHz/us quantized DOWN to the valid even 60 GHz code; ADC 2.5 Msps; ADC start 6 us; ramp 40 us; idle 210 us; TX start 1 us; oneTX with 6 dB backoff; RX gain 30 dB;5 frames/s.
- Actual quantized slope/start frequency, not rounded nominal values, feed DSP/spatial calculations. No VCO forcing, beamforming phase steering, continuous-wave mode, or custom calibration bypass.
- About 0.195 m range-bin spacing and evaluated bins 4..30, roughly 0.78–5.85 m. Analog HPF 175/350 kHz attenuates close returns;350 kHz corresponds to about 1.75 m at 30 MHz/us. These are design numbers, not verified detection range or accuracy.

## Failure and no-RF acceptance

Each lane has an independent mutex, SDK SCI register readiness check and latched failure. Readiness uses at most six polls and five elapsed 1-ms ticks, with wrap-safe subtraction. Existing writeTimeout is 200 ticks. Failure releases the lock and blocks subsequent writes on that lane until reset. Data failure sets app fault and the command watchdog stops the run while control remains usable if healthy. Control failure can prevent any fault receipt. UART completion does not prove USB receipt.

First board acceptance is status/ACK framing plus no-RF cross-port probes only. Require exact accepted build and passing boot self-test before configuration, then matching data probe and fresh paired control status before considering a separately authorized 1–5 second run. Never synthesize success from malformed or missing records.

## v0.3.1-rc1 fixed AOP polarity

The physical RX 0..3 estimator corrections are (+1,−1,+1,−1), the fixed AOP feed-polarity relationship documented in [TI SWRU 546 E Figure 4-8](https://www.ti.com/lit/ug/swru 546 e/swru 546 e.pdf#page=51) and SDK 03.06.02.00-LTS AOP profile. The serialized phasors remain uncompensated input measurements. selftest_version 2 adds bit 256 for known-angle synthetic input with this inversion. No new status fields or commands; original-radar-v 2, both UART gates and 1–5 second cap remain. Fixed polarity does not set measured phase calibration, axes verification or world-height validity flags.
