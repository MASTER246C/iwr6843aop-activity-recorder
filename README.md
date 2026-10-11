# IWR6843AOP Activity Recorder

## Hosted viewer

**[Open Radar Studio](https://radar-studio.master246c.chatgpt.site)** · [Hosted help](https://radar-studio.master246c.chatgpt.site/help) · [Local offline ML](https://radar-studio.master246c.chatgpt.site/offline) · [Versioned viewer downloads](https://github.com/MASTER246C/radar-studio-chromebook/releases)

Use a full tab in **Chrome on desktop or ChromeOS**, with Web Serial available and permitted by your browser/device administrator. USB access requires your explicit browser port selection. The hosted viewer does not need a Linux file server, Python, Node.js or Developer Mode.

Use only the viewer workflow matching the firmware already on your IWR6843AOP board and its exact configuration. TI demos and the original JSON firmware are different protocols. Hosting does not flash firmware or establish compatibility with every demo or device.

Recordings are handled in your browser and saved through explicit local downloads; opening the site does not submit recordings to GitHub or the dataset. Review consent and privacy before sharing any export. The software remains **source-available under its existing no-resale license**; required notices and separate third-party terms still apply.

The hosted viewer must match the installed firmware and its exact reviewed build. The **0.3.0-rc1 dual-UART candidate requires [Radar Studio v0.8.0-preview.1](https://github.com/MASTER246C/radar-studio-chromebook/releases/tag/v0.8.0-preview.1)**; check the displayed version when using the hosted site. Do not use the older single-port viewer or its 0.2.1-rc1 gate for this image. Initial physical acceptance is a no-RF control/data pairing capture, followed by review before any separately authorized 1–5 second sensing test. Hosting does not establish hardware acceptance or measurement accuracy.


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

## Original sensing firmware 0.3.0-rc1

[Dual-UART acceptance candidate, source, configuration and notices](firmware/original-radar-sensing/README.md) are included. Download the complete [sensing-v0.3.0-rc1 prerelease](https://github.com/MASTER246C/iwr6843aop-activity-recorder/releases/tag/sensing-v0.3.0-rc1) and verify its checksum. The matching dual-port viewer is [Radar Studio v0.8.0-preview.1](https://github.com/MASTER246C/radar-studio-chromebook/releases/tag/v0.8.0-preview.1); the existing single-port viewer and legacy collector are not compatible controllers for this image.

This candidate uses **original-radar-v2**, with Enhanced control at **115200 baud** and Standard data at **921600 baud**. Strict newline framing, lane separation and a fresh per-run nonce bind control receipts to a data-only probe. Pairing never starts RF. Firmware and viewer acceptance are restricted to separately initiated **1–5 second** tests; longer sessions are rejected.

**Physical no-RF dual-port acceptance is pending.** After a separately authorized flash, first verify fresh control status, exact build identity, passing boot self-test, idle state and zero faults. Use the measured-geometry acceptance template and matching viewer for the no-RF pairing sequence, then stop for receipt review before considering a later RF test. Never use RF start to troubleshoot malformed framing or failed pairing.

The stream contains at most three exploratory peaks per frame. Peaks and any host-side candidate IDs are not validated people counts, human identities, calibrated positions or activity classifications. Current mounting tilt and calibration remain unverified.

The BIN includes original application code plus identified TI/runtime support. Project no-resale terms cover original portions only; third-party components retain their own licenses and TI-device restrictions. Full notices accompany the package. Earlier tags and release downloads are unchanged.

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
