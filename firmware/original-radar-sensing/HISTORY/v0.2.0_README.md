# Original Radar Sensing 0.2.0

**Scope: movement/activity measurements toward people tracking, not heart-rate or breathing estimation.** Static returns are retained for stationary subjects; the compiled application has no completed identity tracker or activity classifier and no demonstrated tracking accuracy.

An independently written sensing bring-up application for **IWR6843AOPEVM Rev G**, running on the MSS Cortex-R4F. The bundle contains a compiled sensing BIN, matching configuration, full original application/DSP/spatial source, a dedicated JSONL collector, build recipes and offline test evidence.

**Status: built and tested offline; never flashed, booted or measured on a board.** This advances the original firmware from telemetry to an implemented RF/acquisition/measurement pipeline. Sitting, walking and standing classification remains unfinished: there is no trained or validated classifier, and no demonstrated accuracy improvement. Every detection has `activity:null`.

## What is implemented

- Explicit, finite RF sessions through TI's low-level mmWave/mmWaveLink transport, with a fixed 60 GHz profile.
- ADCBUF plus original event-driven EDMA capture into two 32 KiB L3 buffers; independent frame-start checks, generation/ownership checks and fault reporting.
- Original, allocation-free range/Doppler FFT measurements that retain stationary energy, eight internal peaks, and uncalibrated per-RX complex coefficients.
- Original adjacent-pair spatial estimates, with calibration/height validity flags. No TI demo detection, tracker, classifier, DPU/DPC/DPM or signal-processing library is linked.
- Bounded JSON Lines at 115200 baud, with three transmitted peaks and aggregate total/static/varying measurements across the configured range bins. The cap is explicit; omitted peaks are counted.
- Original strict configuration parser and separate `start` command. The supplied configuration does not start RF. Boot includes RadarSS clock/APLL startup; RF open/calibration/profile/start only occurs when `start` is explicitly handled.
- A dedicated Python collector that preserves labels supplied by the recorder, unknown geometry, timing and quality flags. It does not feed this different schema into a TI-trained model.

## Files to start with

- `build/original_radar_sensing.bin`: packaged MSS plus unmodified TI RadarSS image.
- `build/original_radar_sensing.xer4f`: produced by a local build; debug ELF is not included in this public package.
- `config/room64_v1.cfg`: matching configure-only file.
- `host/README.md`: passive collection, configuration, explicit RF start and offline replay.
- `PROTOCOL.md`: wire format, command contract, ADC layout and field semantics.
- `VALIDATION.md`: exactly what was checked and the remaining physical validation gates.
- `PROVENANCE.md`, `PUBLIC_MANIFEST.json`, `THIRD_PARTY_NOTICES/`: origins, SHA-256 hashes and dependency notices.

This package is independent of the existing TI-compatible v0.5/v0.6 capture path. It is not required for that path; its config and JSON output must not be used as TI CLI/TLV data. The earlier telemetry-only build is preserved separately.

## Concrete profile and mounting

`room64_v1` uses TX0 and RX0–3, 64 complex 16-bit samples per chirp, 32 chirps per frame, 5 frames/s. Nominal start is 60.25 GHz, slope 30 MHz/us, ADC rate 2.5 Msps, ADC start 6 us, ramp 40 us, idle 210 us, and TX backoff 6 dB. Frequencies and slope are quantized using IWR6843's 60 GHz units; the derived measurements use those quantized values.

The range-bin spacing is about 0.1953 m; selected bins 4–30 correspond to about 0.78–5.86 m. This is a configured bin span, not proven usable range or accuracy. Analog high-pass filtering attenuates nearer returns, and a person may lie outside the span or field of view. Mounting height and tilt alone do not establish target distance.

The current user-reported mount reference is **4 feet (1.2192 m) from floor to the flat TOP of the bracket, excluding screws**. The chip-center offset is only an estimate, **+0.003175 to +0.00635 m**, giving an estimated antenna height interval **1.222375–1.22555 m** (midpoint about **1.224 m**, not a calibrated measurement). Actual downward tilt is unconfirmed. The earlier 2 m / 15-degree geometry is a historical example, not the current setup or a universal requirement. `config/room64_v1.cfg` retains that historical reference; do not use it unchanged for the current mount. See `config/current_mount.cfg.template` and `config/current_mount_metadata.json`; measure tilt before completing the template. RX phase and world-axis orientation are also unverified. Thus world coordinates and posture stay unknown, even if a numeric offset is later provided without the other calibration.

## Build and test

Use TI mmWave SDK 03.06.02.00-LTS, ARM CGT 16.9.6.LTS, SYS/BIOS 6.73.01.01 and XDC 3.50.08.24. No DSP compiler is needed. The public recipe uses conventional `/opt/ti` defaults; override paths for your machine:

```sh
make clean
make -j2 all TI_ROOT=/path/to/ti \
  SDK=/path/to/ti/mmwave_sdk_03_06_02_00-LTS/packages \
  BIOS_ROOT=/path/to/ti/bios_6_73_01_01 \
  XDC_ROOT=/path/to/ti/xdctools_3_50_08_24_core \
  CGT_ROOT=/path/to/ti/ti-cgt-arm_16.9.6.LTS
bash tests/run_tests.sh
python3 tools/verify_sensing.py
SDK_ROOT=/path/to/ti/mmwave_sdk_03_06_02_00-LTS \
  MONO=mono bash tools/package_firmware.sh
```

The host test script needs a C99 compiler with AddressSanitizer/UndefinedBehaviorSanitizer and Python 3.10+. Leak detection is disabled for sandbox compatibility; the portable signal-processing path has no heap allocations. The packaging script creates files only and refuses to overwrite an existing BIN/RPRC. A supported native TI build host can run its tools directly. The build here used user-space QEMU for legacy 32-bit tools and Mono for out2rprc; see provenance. Changing paths or host may change ELF debug metadata, so the manifest identifies this exact delivered build.

The source build regenerates TI runtime configuration; generated build caches are intentionally omitted from the ZIP. Retain the vendor dependencies and notices when redistributing a firmware image. No toolchain installers or proprietary compiler executable are included.

## Before physical use

This is a development artifact, not a claim that flashing is safe or that RF operation has passed validation. No flash command or switch-change automation is included. An independently approved board bring-up must establish boot/UART routing, the fixed ADCBUF completed-bank window, event count, RX/IQ order and FFT sign, real timing/stack usage, RF configuration acceptance, stopping behavior and actual emissions. Follow the applicable TI board documentation and local RF requirements. Do not make safety, occupancy, medical or other consequential decisions from this experimental output.

After acquisition is verified, record empty-room, sitting, standing, walking and transition sessions with independent human labels and mounting provenance. A dedicated original model can then be trained and evaluated on held-out people/sessions. This package does not yet establish that result.

## Public distribution and source license

This sensing component lives in the existing activity-recorder repository. See
[LICENSING.md](LICENSING.md), LICENSE and THIRD_PARTY_NOTICES for the distinction
between original project code and third-party support inside the BIN. The
project's no-resale software license does not relicense TI or other components.
Use the complete distribution with its notices. No SDK/compiler binaries are
included. The public file manifest identifies shipped bytes; omitted debug/build
outputs can be regenerated locally with the documented prerequisites.
