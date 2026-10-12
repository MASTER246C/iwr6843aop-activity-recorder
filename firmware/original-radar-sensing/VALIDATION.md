# 0.3.2-rc1 verification record

Status: built and tested offline; physical acceptance pending. No board, serial device, flasher, deployment or publication was accessed.

The actual candidate receipt is `build/0.3.2-rc1/BUILD_RECEIPT.json`. Build ID: `87589b2c32638ae119334c72`. Candidate BIN:346,436 bytes, SHA-256 `162db11aa7472d939b1039557bcad95253752c6562261d5ed851d6216c574fb9`, appended CRC32 `0xd2704b46`. Debug ELF SHA-256: `2b08810c8f9b9c31d9f81039c334d269cc468f63d63894641fc295d622647ff6` (local audit artifact).

Official native Windows TI ARM CGT 16.9.6.LTS compiled and linked against SDK 03.06.02.00-LTS, SYS/BIOS 6.73.01.01 and XDCtools 3.50.08.24 with warnings treated as errors. Official Windows ImageCreator tools packaged the BIN. Independent CRC32, exact MSS RPRC inclusion, embedded version/build identity, source identity and structural ELF/memory/driver-symbol checks passed. A repeat official packaging run produced byte-identical BIN output.

The SDK CRC tool printed `Failed to remove CRC temp file` and returned 0 in both packaging runs. This warning is preserved in the receipt; independent output checks passed. Converters now run in a disposable temporary working directory, so the SDK's temporary crcFile does not remain in the source tree. Successful packaging does not establish board behavior.

Zig 0.15.2 host C tests passed with strict warnings and effective undefined-behavior checks; a deliberate signed-overflow probe trapped. AddressSanitizer is unavailable: an address-only probe could not link the required __asan_* runtime. Combined address/undefined flag acceptance must not be called ASan proof.

Behavioral checks passed for bounded/continuous configuration; capture ownership and 64-bit time extension across raw-clock wrap; DSP 706 checks; spatial 1874 cases; boot self-test; UART record transport; tuple/rolling-lease/revocation/exhaustion; and 3228 synthetic AOP cases. The actual-main mocked harness sustains a 72-minute synthetic continuous run with keepalives, then verifies expiry Stop, rejected wrong/stale tuples, manual Stop and pairing-revocation Stop while retaining the inherited no-RF/self-test/fault gates. These are simulated time and mocked peripherals, not wall-clock radar operation. All 13 historical v1 collector tests pass; that collector does not control this v2 firmware.

An exact-source dual-UART framing model independently parses 1000 sessions,5000 control records (including tuple-bearing keepalive ACKs) and 2000 data records on separate simulated lanes. Maximum tested numeric frame serialization is 1625 bytes within the 1800 byte cap. No physical peripheral behavior is implied.

The accepted prior 0.3.1 BIN SHA-256 remains `668476d4130f659cef4c1fea3b8eb436d5025f20853dc30ca83380f9bd38a6bf`; its root RELEASE_MANIFEST is unchanged. Prior documentation/identity is preserved under HISTORY/v0.3.1-rc1. Current candidate files have a separate manifest. No 0.3.1 artifact has been relabeled.

Physical boot, UART routing/baud/framing, RF and EDMA operation, target processing deadlines, stack headroom, temperature/endurance, geometry and tracking remain unvalidated. This new build has not been executed in target instruction emulation. RTOS or callback stalls still prevent a hard wall-clock Stop guarantee. Exact identity/self-test/no-RF pairing, manual Stop and heartbeat-loss Stop require board acceptance before a supervised continuous sensing run. No human count, posture classifier, camera labeling or radar ML acceptance is claimed.
