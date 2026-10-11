# 0.3.0-rc1 provenance

This release retains the project's independently authored radar acquisition, DSP, spatial estimator and no-RF boot self-test. The inherited `src/record_transport.c/.h` guard is now applied independently to both UARTs. New `src/link_session.c/.h` and the application adapter implement strict nonce correlation, generation/session binding and one-shot Start permission; no TI driver source was copied or modified. The adapter uses the pinned SDK's public register definitions and UART instance mapping. No TI demo processing/tracking/classifier was substituted.

Original licenses and third-party notices are retained byte-for-byte. Earlier release provenance is preserved in HISTORY. Vendor SDK/compiler/compatibility tools are external prerequisites, not redistributed in this package.

Build versions: TI mmWave SDK 03.06.02.00-LTS, ARM CGT 16.9.6.LTS, SYS/BIOS 6.73.01.01, XDC 3.50.08.24. Official SDK installer SHA-256: `904ba6555e2c6bf6a3256c35e9b4155e6b1d43eaf88a2df025b7fea69d1fd674`. Legacy vendor executable bytes remain unmodified and are launched through official Debian QEMU 10.0.13/libc6-i386 compatibility; XDC runs natively. Official Debian Mono 6.12.0.199 runs the vendor out2rprc packaging utility.

The inherited spatial correction was previously checked by rebuilding the entire original v0.2.0 BIN byte-for-byte and executing TI-generated synthetic tests in ARM instruction emulation. The new transport guard is tested with an explicit pending-TD-overwrite model, including actual application adapters/status/ACK/pair commands and frame serializer, and the final compiled guard and link-session objects is also executed in ARM emulation. Neither simulated peripheral state nor instruction emulation proves physical UART behavior.

This public distribution includes source, the new BIN, configs, tests, notices, relative-path manifests and sanitized aggregate validation. It excludes user capture data, ELF/map/link XML, raw logs, installed toolchains and private workspace identifiers. `BUILD_IDENTITY.json` identifies source/config plus pinned tool versions; `RELEASE_MANIFEST.json` identifies exact distributed bytes.
