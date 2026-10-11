# v0.2.1-rc1 provenance and verification

The historical independently authored application and vendor dependency provenance is preserved in `HISTORY/v0.2.0_PROVENANCE.md`. Its frozen-BIN claims apply only to v0.2.0.

New independently authored work in this candidate consists of the scalar spatial component-store rewrite, deterministic synthetic spatial self-test, boot acceptance gate/status fields, and build-identity generator. No TI algorithm source was imported for these changes. Original licenses and vendor notices remain unchanged.

The official SDK installer was downloaded from TI and SHA-256 verified as `904ba6555e2c6bf6a3256c35e9b4155e6b1d43eaf88a2df025b7fea69d1fd674`. It supplies SDK 03.06.02.00-LTS, ARM CGT 16.9.6.LTS, SYS/BIOS 6.73.01.01 and XDC 3.50.08.24. Legacy tool executables were kept byte-unmodified and launched via shell wrappers using official Debian QEMU 10.0.13 and libc6-i386 2.41 compatibility. XDC runs natively. Official Debian Mono 6.12.0.199 runs TI out2rprc for packaging. These tools are external prerequisites and are not redistributed in this release.

The unchanged baseline rebuilt to the **exact entire original BIN** SHA-256 `ef945c247cb5fe6bb7dacbf0e6dba5431b57418544bbaf2cf81673be94de7841` (341,508 bytes). This proves source/build-byte reproducibility for the distributed image in this environment; it does not read back or authenticate any board's flash.

The malformed normalization-loop stores were reproduced by that build. O2 TI-generated baseline code fails three valid synthetic cases in ARM instruction emulation; the scalar correction and guarded variant pass. This is a reproducible code-generation defect in the tested build flow. It does not isolate compiler behavior from all host-emulation influences and is not a vendor-confirmed compiler issue.

The final package includes no private capture/phasors, hardware credentials or compiler installations. All spatial calibration flags remain unverified by default. Source/config identity appears in `BUILD_IDENTITY.json`; final release artifact hashes and test status appear in `RELEASE_MANIFEST.json`.
