# Original Radar v1 serial protocol

This original sensing firmware speaks **JSON Lines over MSS UARTA at115200 baud,8N1**. It is not the TI-TLV protocol. Each JSON object has `protocol:"original-radar-v1"`, one LF-terminated line. JSON never contains NaN/Infinity; unknown quantities are `null`. Do not send this configuration to a TI demo or decode this output as TI TLV.

## Configuration and RF commands

Send `config/room64_v1.cfg` line by line, waiting for an ack after each non-comment/non-empty line. Lines are at most191 bytes including any CR, excluding LF. Unknown words, malformed/overflowed numbers, excess tokens and overlong lines are rejected without partially changing configuration.

```
orConfig 1
profile room64_v1
mount 2000 15000 unknown
detect 4 30 64 8
session 60
commit
```

- `orConfig 1`: reset staged configuration, with unknown offset.
- `profile room64_v1`: the only supported RF profile. Arbitrary chirp/radio settings are not accepted.
- `mount`: floor-to-bracket-flat-top millimeters, downward tilt millidegrees, signed world-vertical antenna-center offset AFTER tilt in millimeters or `unknown`. A known offset must produce positive antenna height. Default bracket measurement is2m, tilt15degrees, offset unknown.
- `detect`: inclusive first/last positive range bin, absolute normalized-power floor, median-background multiplier. These are exploratory thresholds, not a calibrated CFAR false-alarm claim.
- `session`: finite run duration1..600seconds. RF also has a configured finite frame count.
- `commit`: validate all required configuration. Does not open, calibrate or start RF.
- `start`: separately and explicitly request radio calibration plus RF frames.
- `stop`: stop the current RF run.
- `status`: query status without changing configuration.

Boot and configuration loading do not issue RF configuration/calibration/start commands. RadioSS low-level APLL clock startup occurs at boot. The image has not been hardware-validated. During a run, configuration changes and another start are rejected; stop/status remain accepted.

## Identity and replies

```
{"type":"identity","protocol":"original-radar-v1","app":"original-radar-sensing","version":"0.2.0","baud":115200,"sensing":true,"classifier":false}
{"type":"ack","protocol":"original-radar-v1","command":"commit","code":0,"state":"idle","configured":true}
```

Ack `command` echoes the canonical recognized first token, or `invalid` for unknown input. Match this field to reject stale/unrelated replies. Ack code0 means successful handling. Negative values indicate rejection/failure. A comment or empty line gets no ack. Status contains `type:"status"`, `state`, `configured`, `fault`, `frames`, and additional diagnostics. States: `idle`, `starting`, `running`, `stopping`, `fault`. The firmware never labels a frame sitting/standing/walking without a validated classifier; `classifier:false` remains explicit.

## RF/acquisition layout

- Physical RX0,1,2,3; TX0 only; 64ADC samples ×32chirps ×4RX.
- Little-endian signed16bit pairs `{real,imag}`, ADCBUF sampleInterleave0, channelInterleave1.
- Contiguous `[chirp][receiver][sample]` frame:32,768bytes. Hardware EDMA copies1,024bytes per chirp event into alternating L3 frames.
- Start60.25GHz; nominal slope30MHz/us quantized DOWN to the valid even60GHz code; ADC2.5Msps; ADC start6us; ramp40us; idle210us; TX start1us; oneTX with6dB backoff; RX gain30dB;5frames/s.
- Actual quantized slope/start frequency, not rounded nominal values, feed DSP/spatial calculations. No VCO forcing, beamforming phase steering, continuous-wave mode, or custom calibration bypass.
- About0.195m range-bin spacing and evaluated bins4..30, roughly0.78–5.85m. Analog HPF175/350kHz attenuates close returns;350kHz corresponds to about1.75m at30MHz/us. These are design numbers, not verified detection range or accuracy.

## Frame schema

```
{
 "type":"frame","protocol":"original-radar-v1",
 "sequence":0,"timestamp_us":1234000,"complete":true,
 "missed_frames":0,"capture_fault":0,"quality":44,
 "frame_events":1,"processing_us":14000,"previous_tx_us":85000,"deadline_misses":0,
 "mount":{"bracket_top_m":2,"down_tilt_deg":15,"antenna_offset_m":null,
          "antenna_height_m":null,"world_height_valid":false},
 "range_bin_m":0.195,"velocity_bin_mps":0.305,
 "profile_energy":[150,120,30],
 "detected_count":1,"count":1,"truncated":false,
 "detections":[{"range_m":2.34,"velocity_mps":0,"power":100,
     "phasors":[[10,0],[10,0],[10,0],[10,0]],
     "sensor_xyz_m":[0,2.34,0],"world_xyz_m":null,
     "spatial_flags":3,"activity":null}]
}
```

The example is illustrative, not a captured board result. On wire the object is one line. `timestamp_us` is derived from the1ms BIOS tick at EDMA frame completion. Counters and sequence reset per session; a new start should begin a new host recording epoch. Capture completeness, generation and ownership are checked before and after DSP; incomplete/overwritten/stale frames are not emitted as valid frames.

`detected_count` is the DSP count (up to8). `count` is emitted detections (at most3), capped for115200-baud budget; `truncated` tells whether results were omitted. Per-RX phasors preserve physical orderRX0..3 and each pair is `[real,imag]`. Signed velocity follows mathematical IQ convention and is unverified on hardware. Sensor coordinates are exploratory unless phase and axis calibration flags are valid. World coordinates remain null until antenna offset, RX phase calibration and axis orientation are all known. A supplied antenna offset alone does not enable world-height/posture claims.

`profile_energy` is `[total, static, variance]`, summed over all configured range bins, independent of the top-three peak limit. Total is range-Hann weighted, RX-mean and slow-time-mean ADC-count-squared power. Static is the squared coherent slow-time mean. Variance sums `max(total[bin]-static[bin],0)`; no slow-time Hann is applied to these three profiles. Neighboring-bin leakage is included. These are exploratory stationary/varying measurements, not calibrated physical target energies, person probabilities, or sitting/standing/walking labels.

Quality bits are defined in `dsp/radar_dsp.h`; spatial bits in `spatial/original_spatial.h`. Host code should retain flags, nulls, counts, timestamps and unknown calibration rather than infer validity. The outgoing frame cap is1800bytes including newline, below the2304bytes/frame theoretical5Hz limit. `processing_us` measures DSP work before spatial serialization; the complete DSP+serialization+transmission path is also deadline-checked. The board measures DSP/previous-transmission duration and stops on capture/deadline faults rather than silently lowering acquisition rate. No raw ADC cube is streamed at115200.

## Hardware validation boundary

`complete:true` means the original software's event-count, frame-boundary and buffer-generation checks passed. It is not certification of ADCBUF bank mapping, IQ order, actual RF data, or target truth. Those require a physical known-reflector/acquisition test. Quality value44 marks uncalibrated phase/power and unverified velocity sign only. Fixed-window bank exposure and the positive-half FFT convention remain explicit hardware-validation gates; synthetic test tones cannot establish them.

## 0.2.1-rc1 boot self-test gate (additive)

No new command is introduced. Identity `version` is `0.2.1-rc1`; identity/status include `build_id` (24 lowercase hexadecimal characters). The ID identifies canonical source/config plus pinned tool versions, not the complete BIN bytes.

A `status` response additionally carries:
- `firmware_version`: string, `0.2.1-rc1`
- `selftest_version`: integer, `1`
- `selftest_ok`: JSON boolean, true only after completed boot self-test returns zero
- `selftest_mask`: unsigned 32-bit integer, zero for pass; `4294967295` indicates not completed

Synthetic test runs once per boot, after UART opens and before acquisition/radio initialization. Failure sets `state:"fault"`, `fault:482`; only command/status service starts. Status and stop remain usable without invoking radio/acquisition calls. Start cannot proceed. A fresh successful boot reports `configured:false`; users must query current status before proposing any separately approved sensing test.

Failure bits: 1 boresight; 2 known phase plane; 4 nontrivial corrections/unequal amplitudes; 8 zero-input rejection; 16 NaN rejection; 32 low-coherence rejection; 64 caller guards. Bit 128 is reserved for the separately compiled internal-scratch diagnostic variant and is not produced by the normal RC self-test.

Status responses use the existing trailing `ack` for `command:"status"`. There is no nonce; clear buffered/stale receipts after disconnect, reboot/identity, transport error or reconnection. A passed cached receipt concerns only the current boot and does not establish physical calibration or RF safety. Fault-free idle state and matching version/build are also required before later configuration/start consideration.
