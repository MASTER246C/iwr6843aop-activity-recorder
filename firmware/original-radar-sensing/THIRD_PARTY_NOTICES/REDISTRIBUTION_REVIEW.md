# Component redistribution review for original sensing v0.2.0

This is a review of the supplied component terms and build evidence, not legal advice or a guarantee of compliance in every jurisdiction. It applies to the frozen BIN whose SHA-256 is `ef945c247cb5fe6bb7dacbf0e6dba5431b57418544bbaf2cf81673be94de7841`.

## Evidence supporting redistribution

1. **TI RadarSS firmware:** The exact SDK 03.06.02.00-LTS manifest identifies the IWR68xx RadarSS firmware as TI Text licensed. That license permits unmodified binary redistribution and use for TI devices, with existing copyrights and the full license/disclaimer preserved. It prohibits reverse engineering, decompilation and disassembly of binary-only material and TI-name endorsement without permission. The original 247,416-byte RadioSS RPRC, SHA-256 `f47053e5dbe0fcb0bc6415ccb9625898b3c32526633674b409e018d823b091ff`, occurs byte-for-byte at offset 94,080 in the final BIN. The radio image was not modified.
2. **TI drivers and radio-control support:** The SDK manifest and component source notices identify the linked driver/platform/mmWave/mmWaveLink support as BSD-3-Clause. Binary distribution requires the copyright, conditions and disclaimer in accompanying materials; the non-endorsement condition remains. The application does not link TI detection/tracking pipelines.
3. **SYS/BIOS:** The installed 6.73.01.01 manifest and contributing kernel source notices permit source/binary redistribution under BSD-3-Clause, subject to notice retention and non-endorsement.
4. **XDC target runtime:** The installed XDC 3.50.08.24 manifest identifies target content as available under EDL 1.0; the actual linked runtime source headers cite the EPL/EDL dual licensing. Use the EDL option for distribution and retain `edl-v10.html` and contributor copyright notices. No XDC host tooling, Java runtime or GPL host tools are included.
5. **ARM compiler runtime:** The completed exact-member audit in `runtime-notices/` identifies 40 selected RTS members plus two target startup members. Applicable selected-source terms are TI BSD-3-Clause and Sun Microsystems notice-preservation permission; ten selected math members contain the Sun notice (two also contain TI BSD notices). All applicable notices, transitive source/header notices and startup notices are preserved. No selected member was found to require AFL-3.0, TSPA, Hewlett-Packard or BSD-4-Clause-UC conditions. Two inactive conditional BSD-2-Clause header notices are retained conservatively and labelled. The generic TI memcpy notice alone was insufficient; the supplemental runtime notices close that identified gap.

Official SDK source: https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-PIrUeCYr3X/03.06.02.00-LTS/mmwave_sdk_software_manifest.html

## Required packaging treatment

- Ship the full `THIRD_PARTY_NOTICES` directory with the BIN, including supplemental component notices and exact ARM runtime notices. A BIN-only download without accompanying notice materials is not the recommended distribution unit.
- Explicitly scope any custom project/no-resale license to the independently authored portions. Third-party components remain governed by their own terms; the custom license must not purport to revoke their independent rights.
- Do not add TI SDK/compiler installers, proprietary build tools, QEMU/Mono distributions or full SDK source trees to the release. They are build prerequisites, not parts of this public artifact.
- Preserve the TI-device-only restriction for the RadioSS part and avoid any TI endorsement claim.
- Do not describe the whole BIN as wholly original or open source. It contains independently authored application code plus identified vendor runtime/driver/radio components.
- Keep hardware-unverified status, no-classifier status and planned-versus-measured mounting facts accurate. These are engineering disclosure requirements, distinct from component licenses.

## Remaining scope limits

The SDK manifest states ECCN 5D992 and expressly leaves export classification/compliance responsibility with the exporter. This review does not determine all export, RF-emissions, product-safety or local-law requirements. No contrary distribution restriction was inferred merely from the previous private-only artifact label.

## Review outcome

The inspected terms support distributing the frozen BIN for TI devices with all identified third-party notices and the packaging boundaries above. No additional component-license permission request was identified for this selection. This conclusion is limited to the actual component evidence; it is not a blanket license for the whole SDK or a general legal clearance. The original private ZIP should not be the public release unchanged, because it lacked the supplemental Sun/runtime notices and current project licensing/public-facing corrections.
