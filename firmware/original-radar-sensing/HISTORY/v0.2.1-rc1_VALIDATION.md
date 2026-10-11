# 0.2.1-rc1 verification record

Status: offline checks passed; physical-board acceptance pending.

## Artifacts

- Firmware version: `0.2.1-rc1`
- Source/config build ID: `846a3ae3184aa5d26e5d0ea3`
- BIN: 342,532 bytes; SHA-256 `d534e6b6da25520d911c25bb23593bc5a086becce962186c5e4ffa6cc38d19ec`
- ELF SHA-256: `9b3c411a123b655782e68eefcd8ba1f26148b1311e1c1996566886e3b8f8a48d`
- Final appended CRC32: `0x80079b4f`

## Reproduction and static checks

The exact official TI tool versions rebuilt and packaged the old v0.2.0 source into a byte-identical complete public BIN. The rebuilt old estimator at runtime address 0x2680 matches all 1,584 original bytes. Its normalization loop reproduces the wrong double pointer increment.

The scalar correction's ordinary-build executable object sections match its source-correlated listing build. Independent assembly review confirms a single 8-byte writeback, paired real/imaginary stores, same-address subsequent checks, and temporary arrays within the local stack frame. Remaining aggregate-return helpers use paired fixed slots. Diagnostic guards surround both complex and amplitude scratch arrays and are checked after stores.

Independent final review verified all four MSS BIN payloads against ELF bytes including alignment padding, unchanged RadarSS payload versus the old image, valid CRC32, current source-hash manifest, correct fail-first start gate and initialization ordering. These checks do not read back board flash.

## Tests

- Existing host suite: 937 spatial direction cases, 706 DSP checks in ordinary and ASan/UBSan builds, config/ownership/JSON tests, and 11 collector tests: pass.
- Deterministic self-test on GCC O0/O1/O2/O3: pass.
- Internal scratch guard diagnostic under ASan/UBSan: pass. Deliberately corrupted guard is detected.
- TI-generated ARM code under QEMU Cortex-R5F user-mode instruction emulation: baseline O0 passes; baseline O2 returns failure mask 7; scalar and guarded O0/O2 pass. Final RC estimator/self-test build objects also return zero.
- Host peripheral-mock harness compiles actual application functions: pending test and every individual failure bit (all 32 bits) block all acquisition/radio calls; status/stop remain available; successful boot emits status without RF start or configuration. Pass under ASan/UBSan (leak detection disabled for execution-environment compatibility).
- ELF structural checker: ARM architecture, reset vector, MSS/L3 allocation bounds, expected dependencies and absence of listed TI demo processing/tracking libraries: pass.

## Limits and next gate

No physical boot, UART, RF, ADCBUF/EDMA, timing, stack high-water, physical axis/IQ/phase/calibration or classification claim is made. Instruction emulation uses Cortex-R5F, not the IWR6843's real Cortex-R4F/peripherals. A passing code test cannot certify hardware or spatial accuracy.

Follow README's first acceptance step: after a separately authorized flash, query `status` only and verify exact version/build, self-test pass, zero faults and idle state before considering any separately approved bounded RF test. Fresh configuration is required after power loss. Keep all calibration/trust flags unverified until measured.

Historical implementation assumptions and original validation details are preserved in `HISTORY/v0.2.0_VALIDATION.md`; its historical artifact links do not identify this RC.
