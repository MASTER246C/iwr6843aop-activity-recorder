# Source and dependency provenance

## Original implementation

All application files in `src/` (excluding regenerated `src/sysbios`, `makefile.libs` and `.exclude`), `dsp/`, `spatial/`, `host/`, `tests/`, `tools/`, the Makefile, `sensing.cfg`, `application.cmd`, matching config and project documentation were independently authored for this work. The earlier original telemetry startup/build scaffold was reused and extended. The application uses its own configuration parser, lifecycle state machine, EDMA frame ownership, FFT measurements, peak selection, profile-energy summaries, phase-pair coordinates and JSON protocol. No TI demo detector, DPU/DPC/DPM pipeline, CFAR/tracker/classifier, TI CLI parser or TI signal-processing implementation was copied or linked.

The original algorithms are exploratory and straightforward, not a claim that FFT/interferometry itself is a new invention. User authorization allowed TI low-level drivers, radio firmware and compiler tools. This provenance intentionally identifies those dependencies rather than calling the whole binary original.

## Reused TI support, by reference

- mmWave SDK `ti/drivers/{osal,esm,soc,pinmux,uart,dma,adcbuf,edma,crc,mailbox}` libraries and their register/pad/API definitions.
- `ti/control/mmwave` and `ti/control/mmwavelink`: radio-control transport, API parameter handling, command serialization and calibration lifecycle. These do not supply this application's detection/processing pipeline.
- `ti/platform/xwr68xx/r4f_linker.cmd`: unmodified memory region definitions. Six 128 KiB L3 banks, no shared TCMA/TCMB expansion.
- SYS/BIOS kernel/tasks/semaphores/interrupts/clock/heap/GateMutex; XDC generated configuration/runtime; TI target boot/auto-init and ARM C/math runtime `rtsv7R4_T_le_v3D16_eabi.lib`.
- The packaged BIN includes the SDK's unmodified `firmware/radarss/xwr6xxx_radarss_rprc.bin`. The MSS ELF does not contain this separate opaque radio image. There is no DSP application image.

The map and link XML identify actual selected archive members. Generated vendor runtime/cache files are not original application source and are omitted from the public ZIP. Vendor toolchains and SDK archives are external prerequisites, not bundled deliverables.

## Low-level evidence consulted

Paths are relative to the SDK `packages` directory unless stated otherwise. Low-level unit tests were consulted for API contracts and hardware data layout; no TI demo processing pipeline was used as a template.

- `ti/drivers/soc/src/soc.c`: SOC clock/APLL startup, address translation and interrupt listeners. SOC startup releases RadarSS for APLL; `SOC_DSSCfg_HALT` means no action rather than actively halting DSP.
- `ti/drivers/soc/platform/soc_xwr68xx.c`: memory/interrupt configuration and non-cached L3 mapping.
- `ti/drivers/pinmux/include/pinmux_xwr68xx.h`: PADBE/PADBD N5/N4 definitions are equivalent to AOP U16/V16 pads.
- `ti/drivers/adcbuf/ADCBuf.h`, `src/ADCBuf_mmwave.c`, low-level `test/common`/xwr68xx tests: complex sample/channel format, channel offsets, threshold encoding and repeated fixed-window transfers across ping/pong swaps. This is source evidence for an assumption still requiring board verification.
- `ti/drivers/edma/edma.h` and driver source: A-sync transfer counts, event channel, linked PaRAM entries, completion codes and error monitoring.
- `ti/control/mmwave/mmwave.h` plus SDK radio-control API documentation: full/isolated MSS operation, profile/chirp/frame/calibration lifecycle and error handling.
- `ti/control/mmwavelink/include/rl_sensor.h`: 60 GHz frequency/slope/time units and field ranges. Even start/slope encodings are derived in original `rf_profile.c`; a unit test checks the physical scale to avoid confusing kHz/us with Hz/s.
- `ti/board/antenna_geometry.c`: AOP RX physical geometry [(1,1),(1,0),(0,1),(0,0)] and 2.5 mm spacing. Hardware geometry facts informed the independently written phase-pair math.
- `ti/common/mmwave_sdk.mak`, `mmwave_sdk_xwr68xx.mak`: compiler ABI, enum and memory-bank build requirements.
- `packages/scripts/unix/generateMetaImage.sh` and official image tools: packaging sequence/IDs. Device token `37` is parsed as hex, shared memory `0x00000006`, MSS core `0x35510000`, BSS core `0xb5510000`.

See `VALIDATION.md` for the distinction between API/source inspection and actual hardware validation. The existing TI-compatible host workflow remains a different project/schema.

## Tool provenance and reproduction

Versions: mmWave SDK 03.06.02.00-LTS, TI ARM CGT 16.9.6.LTS, SYS/BIOS 6.73.01.01, XDC 3.50.08.24. The official SDK installer from TI's SDK 3.6.2 page was 526,070,221 bytes, SHA-256 `904ba6555e2c6bf6a3256c35e9b4155e6b1d43eaf88a2df025b7fea69d1fd674`. Its required ARM/BIOS/XDC components were installed independently after unrelated DSP installers failed. This MSS-only application needs no DSP compiler.

The build host ran unmodified legacy vendor binaries using original shell launchers plus official Debian user-space QEMU (`qemu-user_10.0.13+ds-0+deb13u1_amd64.deb`, SHA-256 `ca6ede739327a20ae5a3498230c8a3a9a072c586e131bc6bcc1201eb1725e336`) and libc (`libc6-i386_2.41-12+deb13u4_amd64.deb`, SHA-256 `50c1653ce29519e1fcd8742f2c67f5edad8d4c3aed3d1ffbea2b7c3497a809d2`). Mono 6.12.0.199 ran the official out2rprc tool. These compatibility packages and proprietary tools are not in the ZIP and are unnecessary on a supported native TI environment.

The optional user-space packaging environment was:

```sh
export MONO_PATH=/opt/ti-compat/mono/usr/lib/mono/4.5
export MONO_CFG_DIR=/opt/ti-compat/mono/etc
export MONO=/opt/ti-compat/mono/usr/bin/mono-sgen
export QEMU_I386=/opt/ti-compat/qemu/usr/bin/qemu-i386
export QEMU_SYSROOT=/opt/ti-compat/sysroot
bash tools/package_firmware.sh
```

All target application paths can be overridden in the Makefile; no runtime code depends on build-host paths. The manifest records exact source, dependency and artifact hashes. The packaging script independently verifies appended CRC32 and refuses existing output paths. Firmware packaging itself does not access a serial port or flash a device.

## Notices and distribution

`THIRD_PARTY_NOTICES/` retains full component/SDK manifests, relevant source copyright notices and XDC EDL/EPL text. `TI_RadarSS_Text_License.txt` is the supplied TI Text license for the included opaque radio firmware, including TI-device use restrictions. The generic TI source BSD notice and SDK manifest cover low-level driver/control support; compiler/runtime and SYS/BIOS notices are also retained. Consult those original terms before redistribution. The initial private build was not published. This public package is separately prepared with distribution notices and a public file manifest.

## Public packaging changes

The BIN is byte-identical to the frozen 0.2.0 build. The historical CFG command bytes are unchanged except a clarifying comment; a separate incomplete current-mount template and estimated metadata were added. Public packaging omits ELF/RPRC/map/link XML, private build logs and generated runtime caches; these are reproducible build outputs, not required to download the supplied BIN. Build-path defaults/documentation were normalized to `/opt/ti` and `/opt/ti-compat`, with no application algorithm or firmware-byte changes. See PUBLIC_MANIFEST.json for exact distributed file hashes.
