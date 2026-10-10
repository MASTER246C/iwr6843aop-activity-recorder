# Exact selected ARM runtime notice audit

## Result

The frozen sensing image selects **40 members** from `rtsv7R4_T_le_v3D16_eabi.lib`, plus `boot.oer4ft` and `auto_init.oer4ft` from the TI target startup archives. The generic memcpy TI BSD notice in the private bundle does **not** cover all their notice-preservation requirements. Ten selected math members also carry a Sun Microsystems permission notice. Preserve those notices, the TI copyrights/conditions/disclaimers, and startup notices with any redistributed image.

The actual installed source notices for this selection show **TI BSD-3-Clause and Sun's notice-preservation permission**. No selected member was found to require AFL-3.0, Hewlett-Packard, BSD-4-Clause-UC or TSPA conditions. This is a selection-specific technical inventory, not a claim that the whole compiler/runtime package is BSD, and not full legal or firmware redistribution clearance.

The ten selected members with Sun notices are:

- `e_atan2f.obj`, `e_sqrtf.obj`, `k_rem_pio2.obj`, `s_atanf.obj`, `s_copysign.obj`
- `s_cosf.obj`, `s_floor.obj`, `s_frexp.obj`, `s_scalbn.obj`, `s_sinf.obj`

`s_cosf.c` and `s_sinf.c` each have both a TI BSD notice and the Sun notice. Their included implementation files `e_rem_pio2f.c`, `k_cosf.c`, `k_sinf.c`, and shared `math.h`/`math_private.h` likewise carry Sun notices. `k_rem_pio2.c` names SunSoft; the others name SunPro. Exact blocks, including attribution and differing copyright years, are retained rather than replaced with one generic license label.

## Evidence and files

- `runtime-notice-audit.json`: every selected member mapped to installed source, SHA-256, exact notice line range, extracted notice file/hash and available vendor SPDX classification. It also identifies transitive and inactive conditional headers separately.
- `selected-runtime-map-excerpt.txt`: the exact 40-member allocation table from the frozen linker map.
- `arm_rts__*.notice.txt`: verbatim notice text from the installed TI CGT 16.9.6 sources. Only notice text is copied, not implementation bodies.
- `ti_target__boot.asm.notice.txt` and `ti_target__auto_init.asm.notice.txt`: exact TI BSD notices for the selected startup modules.
- `extract_runtime_notices.py`: reproducible local extraction and mapping, requiring the installed SDK paths; it does not change firmware or publish files.

Checks are tied to:

- ELF SHA-256 `32ea6c3861dde198a7e3513647d3e1b8aaa5bc4411041122c51c2bb9560ae6b8`
- BIN SHA-256 `ef945c247cb5fe6bb7dacbf0e6dba5431b57418544bbaf2cf81673be94de7841`

The JSON records the linker-map, RTS archive and supplied SPDX file hashes. The source mapping uses selected object names, the installed matching source names, and the vendor build recipes; it is not a reconstruction or reproducible compilation of the proprietary prebuilt library.

## Why the compiler manifest alone is insufficient

The supplied `ARMCodeGenerationTools_16.9.0.LTS_manifest.html` RTS row applies to the entire 16.9.x runtime and enumerates many licenses. It refers to `arm_rts_16_9_0_LTS-5D08A7C6-FD5E-4de4-8222-962F213D6F88.spdx` for per-file information. That SPDX snapshot is older than the installed 16.9.6 source and explicitly warns that checksums/content can differ. In this selection:

- `hostopen.c`, `hostwrite.c` and `trgdrv.h` are present in 16.9.6 with TI 2017 BSD notices but absent from the older SPDX snapshot.
- Several Sun-permission files are classified `NOASSERTION` in SPDX; their actual installed source contains the permission grant and notice-preservation condition.
- SPDX calls `s_cosf.c` and `s_sinf.c` BSD-3-Clause, but their actual source also contains the Sun notice. Both blocks are preserved.

Thus the audit relies on actual installed source notices as well as map and SPDX, not a blanket license inference from a package row.

## Conditional BSD-2 header notices

The conservative textual include closure contains `fenv.h` and `fenv-softfloat.h`, which carry David Schultz BSD-2-Clause notices. They are included here conservatively and clearly marked `inactive_conditional_include_conservatively_retained` in the JSON, not counted as selected archive members. The installed `math_private.h` includes `fenv.h` only if `math_errhandling & MATH_ERREXCEPT`; installed `math.h` lines81–83 define `MATH_ERRNO=1`, `MATH_ERREXCEPT=2`, and `math_errhandling=MATH_ERRNO`, making that branch inactive for this source. There is no fenv archive member in the actual map. Retaining these two notices is harmless coverage, not a claim that their code is present in the image.

## Target startup

The actual selected `boot.aer4ft:boot.oer4ft` and `auto_init.aer4ft:auto_init.oer4ft` map to `bios_6_73_01_01/packages/ti/targets/arm/rtsarm/boot.asm` and `auto_init.asm`. `package.bld` lines29–30 builds those named source units into those archives. Their source notices are TI BSD-3-Clause with copyright years1996–2014 and2007–2011 respectively; both complete verbatim notices are supplied. No AFL/TSPA/HP condition appears in those two files.

## Remaining redistribution boundary

These notices supplement, rather than replace, the SDK/BIOS/XDC and opaque RadarSS license files. They do not authorize publishing the compiler, entire RTS library, SDK sources, tool executables or otherwise restricted vendor materials. The original frozen private ZIP and firmware were not altered. The publisher should attach applicable notices to the public distribution and keep the TI-only RadarSS restrictions and other independent dependency conditions intact.
