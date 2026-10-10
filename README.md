# IWR6843AOP Activity Recorder

**Mount height is configurable. Current user-reported reference: 4 feet (1.2192 m) from floor to the flat TOP of the bracket, excluding screws.**

The chip center is above that reference in the photographed arrangement. Its signed vertical offset is **estimated at +0.003175 to +0.00635 m**, giving an **estimated antenna-height interval of 1.222375–1.22555 m**. The midpoint is about **1.224 m**; it is not a calibrated measurement. **Actual downward tilt is unconfirmed.** Measure the final geometry after setting the physical tilt, and preserve the uncertainty in capture metadata.

**Antenna-center height = measured mount-top reference height + measured signed vertical chip-center offset after final tilt.** The mount reference is not the chip height. Do not guess the offset, replace it with zero, or apply an extra cosine to an already vertical offset. Verify point and track coordinate conventions separately before applying a transform; do not infer a firmware `sensorPosition` or claim calibrated angles from these estimates.

The earlier **2.00 m / 15-degree** arrangement was a planned experimental reference and remains in historical examples and frozen releases. It is **not the current setup, a mandatory height, a universal optimum or a proven most-accurate setting**. Earlier underside-to-board-bottom measurements are not used for the chip-center offset. Physical calibration remains unverified.

**Experimental v0.6-bundle1 — original reusable recorder and motion modules.**

The reusable collection component of Radar Studio Chromebook, built for the
**Texas Instruments IWR6843AOPEVM (IWR6843AOP), Rev G**. Planned initial input is
processed UART points/tracks from the standalone board, not raw ADC capture.
This repository supplies reusable JavaScript modules, not a standalone browser UI
or a pretrained activity model. Use Radar Studio for the integrated application.
Real-browser rendering, hardware throughput and real-world classification
accuracy remain unverified. No participant captures are included.

## Original sensing firmware 0.2.0

[Custom BIN, matching configuration, original source and dedicated collector](firmware/original-radar-sensing/README.md) are now included. Download the complete Original Radar Sensing release ZIP with its required notices. This is a separate JSON protocol, not the TI demo CLI/TLV path used by v0.5/v0.6.

**Never flashed, booted or hardware-validated.** Movement/position measurements retain stationary returns; no heart/breath estimates, completed people-ID tracker, activity classifier or demonstrated tracking accuracy. The current-mount configuration template requires a measured tilt before use; its offset remains unknown. Read the firmware guide and validation gates before any manual flash/RF start.

The BIN includes original application code plus identified TI/runtime support. Project no-resale terms cover original portions only; third-party components retain their own licenses and TI-device restrictions. Full notices accompany the package.

## Three related projects

- [Radar Studio Chromebook: visualizer and easy-to-run package](https://github.com/MASTER246C/radar-studio-chromebook)
- [IWR6843AOP Activity Recorder: reusable collection component](https://github.com/MASTER246C/iwr6843aop-activity-recorder)
- [IWR6843AOP Activity Dataset: collection/review plans and future data](https://github.com/MASTER246C/iwr6843aop-activity-dataset)

## Use and reproduce checks

See [RECORDER_API.md](RECORDER_API.md) for browser/CommonJS APIs and
[MOTION_LAB.md](MOTION_LAB.md) for capture, calibration and experimental limits.
Run `node tests/motion.test.js` and `node tests/capture.test.js` from this folder:
29 motion logic tests and 24 capture tests pass using synthetic inputs.

`SHARED_MODULES.json` pins 23 exact module/test/support files by SHA-256. These
same bytes ship in Radar Studio v0.6, without runtime remote loading. Changes must
be reconciled and tested before either copy is updated; do not develop silent
forks. Source: Radar Studio v0.6 build, ZIP SHA-256
`0253759439843f3452aa0c8d5cd152990d3ce8ee0cf08a50921db335cd32a805`.

Planned capture labels include sitting, standing and walking. Sit-to-stand and
motion-onset events belong on a separate annotation axis with boundaries and
uncertainty; movement onset alone is not proof of walking. Original labels and
AI proposals must remain distinguishable from final human-reviewed annotations.

## Privacy and data

Collection/export should be explicitly initiated and local. Publishing recordings
is a separate consent, privacy, quality and authorization decision. Do not upload
captures or participant details in issues. Future camera/AI review and raw-ADC
capture require their own validated workflows and metadata; none is supplied now.

## Software license

This repository is the recorder component of the Radar Studio Chromebook project.
Its original code/documentation that the licensor has authority to license uses
the same [Source-Available No-Resale License](LICENSE), reproduced unchanged.
It permits personal use, internal business use, paid teaching and free sharing
under its conditions; resale, paid bundles and premium hosted software access
are restricted. It is not OSI-approved open source. TI/third-party materials are
excluded and keep their own terms.

The separate dataset's intended commercial AI-training/model-sales policy does
not relicense this software. Final dataset terms remain pending in its repository.

## v0.6 local offline ML

Original offline feature extraction, model save/load, annotation proposals, human
review overlays and CLI are included. See [OFFLINE_ML.md](OFFLINE_ML.md) and
[RECORDER_API.md](RECORDER_API.md) for commands, data contracts and tests. The
browser offline page is supplied by Radar Studio. All processing is local;
no pretrained weights, participant captures or cloud inference are supplied.
Automated proposals stay separate from original annotations and human decisions.
Synthetic tests do not establish real-world accuracy, hardware behavior or
browser rendering. Dataset terms remain pending and separate from software.

## Submit recordings for review

The activity recorder is the collection tool; the [activity dataset repository](https://github.com/MASTER246C/iwr6843aop-activity-dataset) is for recordings Chris has reviewed and accepted. Contact **dataset@swarmcord.com** first with a short, non-sensitive summary. This owner-provided address has not been delivery-tested here. Do not attach unreviewed personal video or sensitive recordings to a public issue; arrange an appropriate private transfer and permissions first.

Include the export schema/version and firmware path (original JSONL versus TI-compatible capture), file hashes, session IDs, human-observed labels and event boundaries, quality flags, exact configuration/firmware identity, measured or explicitly uncertain mounting/timing metadata, collection context and consent/reuse scope. Keep original labels separate from AI proposals and final human decisions. Do not invent calibration, ground truth or participant permissions.

Chris reviews submissions and adds accepted recordings to the dataset himself. Nothing is uploaded or published automatically. Large recordings need an agreed storage/transfer location; GitHub is not unlimited storage. Accepted review contributions may receive opt-in name/handle credit; anonymous credit is available. Public inclusion and future reuse remain subject to review and final dataset terms.
