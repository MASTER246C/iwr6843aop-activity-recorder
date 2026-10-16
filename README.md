> **First combined physical run, 2026-10-12:** [results and remaining checks](https://github.com/MASTER246C/iwr6843aop-activity-recorder/blob/main/docs/FIRST_COMBINED_RUN_2026-10-12.md). Firmware 0.3.2-rc1 with Radar Studio v0.9.0-preview.5 recorded 283 consecutive radar frames over 56.4 seconds, acknowledged manual Stop and returned to fault-free idle. The C920e recording passed file-integrity and video-decoding checks. The tested camera-label segment stayed unknown; timing alignment and radar training eligibility remain unresolved. No recordings or dataset samples were published.

> **Current release path:** [sensing-v0.3.2-rc1](https://github.com/MASTER246C/iwr6843aop-activity-recorder/releases/tag/sensing-v0.3.2-rc1), [viewer v0.9.0-preview.5](https://github.com/MASTER246C/radar-studio-chromebook/releases/tag/v0.9.0-preview.5), and [current camera/training workflow](original_training/README.md). Version-specific 0.3.1-rc1 sections below describe the retained earlier timed-test release; they do not supersede the current continuous-run report.

> **Firmware source status:** [`firmware/original-radar-sensing/`](firmware/original-radar-sensing/README.md) now holds unreleased **0.3.3-dev** source (build ID `30528454ec253ca4d5710162`). It is host-tested only; no 0.3.3-dev BIN exists. The **0.3.2-rc1** BIN (build `87589b2c32638ae119334c72`) remains the accepted binary, and 0.3.1-rc1 (`abb27b48c1bdb956c0de704b`) remains for timed tests and rollback.

# IWR6843AOP Activity Recorder

## Radar + camera enclosure CAD

[Download and inspect the R02 enclosure prototype](hardware/enclosure-r02/) for the IWR6843AOPEVM and Logitech C920e: editable CadQuery source, STEP/STL exports, BOM, assembly notes, validation reports and SHA-256 checksums. Physical fit, retention, weather resistance, RF, thermal and load performance remain unvalidated; review the documented fit gaps before fabrication.

The [enclosure-specific license](hardware/enclosure-r02/LICENSE) permits personal and internal company use/research and customer-directed paid fabrication. Independent retail/resale of the design or enclosure copies is restricted. Users retain their datasets, measurements, models and independent research outputs and may commercialize those outputs; using the enclosure grants Chris no rights to them.

## Hosted viewer

**[Open Radar Studio](https://radar-studio.master246c.chatgpt.site)** · [Hosted help](https://radar-studio.master246c.chatgpt.site/help) · [Local offline ML](https://radar-studio.master246c.chatgpt.site/offline) · [Versioned viewer downloads](https://github.com/MASTER246C/radar-studio-chromebook/releases)

Use a full tab in **Chrome on desktop or ChromeOS**, with Web Serial available and permitted by your browser/device administrator. USB access requires your explicit browser port selection. The hosted viewer does not need a Linux file server, Python, Node.js or Developer Mode.

Use only the viewer workflow matching the firmware already on your IWR6843AOP board and its exact configuration. TI demos and the original JSON firmware are different protocols. Hosting does not flash firmware or establish compatibility with every demo or device.

Recordings are handled in your browser and saved through explicit local downloads; opening the site does not submit recordings to GitHub or the dataset. Review consent and privacy before sharing any export. The software remains **source-available under its existing no-resale license**; required notices and separate third-party terms still apply.

The hosted viewer must match the installed firmware and its exact reviewed build. **Radar Studio v0.8.0-preview.2 and later accept the 0.3.1-rc1 AOP-polarity candidate for timed tests only; continuous runs need 0.3.2-rc1 and v0.9.0-preview.4 or later.** Use the [current release](https://github.com/MASTER246C/radar-studio-chromebook/releases/tag/v0.9.0-preview.5) or the [hosted viewer](https://radar-studio.master246c.chatgpt.site), and check the displayed version when using the hosted site. Do not use the older single-port viewer or its 0.2.1-rc1 gate for either image. Initial physical acceptance is a no-RF control/data pairing capture, followed by review before any separately authorized 1–5 second sensing test. Hosting does not establish hardware acceptance or measurement accuracy.


**Mount height is configurable.** Radar Studio pre-fills editable 1219 mm / 0° starting values that must be measured and confirmed before use; they are not a default calibration, and 4 ft (1.2192 m) to the flat top of the bracket is a historical project reference. See [MOUNTING.md](https://github.com/MASTER246C/radar-studio-chromebook/blob/main/MOUNTING.md) for antenna height, its uncertainty and the earlier 2.00 m / 15-degree arrangement.

**Experimental v0.6-bundle3 — original reusable recorder and motion modules.**

The reusable collection component of Radar Studio Chromebook, built for the
**Texas Instruments IWR6843AOPEVM (IWR6843AOP), Rev G**. Planned initial input is
processed UART points/tracks from the standalone board, not raw ADC capture.
This repository supplies reusable JavaScript modules, not a standalone browser UI
or a pretrained activity model. Use Radar Studio for the integrated application.
Real-browser rendering, hardware throughput and real-world classification
accuracy remain unverified. No participant captures are included.

## Historical sensing firmware 0.3.1-rc1

The [0.3.1-rc1 source documentation](firmware/original-radar-sensing/HISTORY/v0.3.1-rc1/README.md) and its BIN (`firmware/original-radar-sensing/build/original_radar_sensing.bin`) are retained; the current source tree is later. Download the complete [sensing-v0.3.1-rc1 prerelease](https://github.com/MASTER246C/iwr6843aop-activity-recorder/releases/tag/sensing-v0.3.1-rc1) and verify its checksum. Radar Studio v0.8.0-preview.2 and later, including the [current source](https://github.com/MASTER246C/radar-studio-chromebook) and the [hosted viewer](https://radar-studio.master246c.chatgpt.site), accept this image for timed tests; the existing single-port viewer and legacy collector are not compatible controllers for this image.

This candidate adds the documented physical RX0–3 AOP feed-polarity correction **+1, −1, +1, −1**. It is a fixed hardware convention, not per-device calibration. Generic estimator thresholds, direction-disk rejection, profile and safety gates remain unchanged. Boot self-test version **2** adds an analytic AOP-inversion case; the exact build is **abb27b48c1bdb956c0de704b**. Raw phasors remain raw; compensation is confined to spatial estimation. New-build physical validation remains pending.

This candidate uses **original-radar-v2**, with Enhanced control at **115200 baud** and Standard data at **921600 baud**. Strict newline framing, lane separation and a fresh per-run nonce bind control receipts to a data-only probe. Pairing never starts RF. Firmware and viewer acceptance are restricted to separately initiated **1–5 second** tests; longer sessions are rejected.

**Physical no-RF dual-port acceptance is pending.** After a separately authorized flash, first verify fresh control status, exact build identity, passing boot self-test, idle state and zero faults. Use the measured-geometry acceptance template and matching viewer for configuration and the no-RF pairing sequence. Save the capture JSON and both control/data raw JSONL lanes, then **stop here for review of the initial no-RF receipts** before considering a later separately authorized manual 1–5 second RF test. Startup, configuration and pairing do not implicitly start RF. Never use RF start to troubleshoot malformed framing or failed pairing.

The stream contains at most three exploratory peaks per frame. Peaks and any host-side candidate IDs are not validated people counts, human identities, calibrated positions or activity classifications. Current mounting tilt and calibration remain unverified.

The BIN includes original application code plus identified TI/runtime support. Project no-resale terms cover original portions only; third-party components retain their own licenses and TI-device restrictions. Full notices accompany the package. Earlier tags and release downloads are unchanged.

## Three related projects

- [Radar Studio Chromebook: visualizer and easy-to-run package](https://github.com/MASTER246C/radar-studio-chromebook)
- [IWR6843AOP Activity Recorder: reusable collection component](https://github.com/MASTER246C/iwr6843aop-activity-recorder)
- [IWR6843AOP Activity Dataset: collection/review plans and future data](https://github.com/MASTER246C/iwr6843aop-activity-dataset)

## Original-protocol host training tools

[Original radar host training tools](original_training/README.md) provide a separate Python 3.10+ standard-library workflow for original-radar dataset export v1: validate, prepare, train and evaluate. The published [v0.1.0-preview.3 prerelease](https://github.com/MASTER246C/iwr6843aop-activity-recorder/releases/tag/original-training-v0.1.0-preview.3) is unchanged. The source in `original_training/` is unreleased **v0.1.0-preview.4**, with atomic output writes and camera-import rounding and label-source fixes. The [v0.1.0-preview.1 prerelease ZIP](https://github.com/MASTER246C/iwr6843aop-activity-recorder/releases/tag/original-training-v0.1.0-preview.1) remains a historical archive and does not contain the cross-window continuity fix.

**These are host analysis tools, not firmware to flash.** Python is not a prerequisite for the browser viewer. No recordings, trained weights, cloud inference, uploads or camera hardware are included. Training requires suitable human-reviewed exports with explicit session and participant-group splits; synthetic tests do not establish recognition accuracy. Labels cannot repair rejected sensor data. The tool independently hashes input bytes and frozen canonical inputs but does not authenticate the producer's embedded hash or an absent original capture. Read its admission, split and resource limits before use.

The shared TI offline feature extractor now excludes windows overlapped by unfinished or incomplete human events. Both repositories use the same v0.6-bundle3 exact pins. The firmware pin, no-RF pairing and manual 1–5 second sensing gates remain separate and unchanged. The current viewer supplies explicit local camera recording and MediaPipe-assisted review; the C920e was physically tested in the linked first-run report. Alignment and activity accuracy remain unvalidated. Software licensing and separate capture-consent requirements still apply.

## Use and reproduce checks

See [RECORDER_API.md](RECORDER_API.md) for browser/CommonJS APIs and
[MOTION_LAB.md](MOTION_LAB.md) for capture, calibration and experimental limits.
Run `node tests/motion.test.js` and `node tests/capture.test.js` from this folder:
29 motion logic tests and 24 capture tests pass using synthetic inputs.

`SHARED_MODULES.json` pins 32 exact module/test/support/doc files by SHA-256 in
current **v0.6-bundle3**, matched byte-for-byte to the unreleased Radar Studio
v0.9.0-preview.6 source. There is no runtime remote loading. Both copies must be
reconciled and tested before updates; `node tools/check-shared-modules.js`
verifies the pins. The historical v0.6-bundle1 originated in the Radar Studio v0.6
archive, ZIP SHA-256
`0253759439843f3452aa0c8d5cd152990d3ce8ee0cf08a50921db335cd32a805`;
that archive is unchanged and does not contain the current event-overlap fix.

Planned capture labels include sitting, standing and walking. Sit-to-stand and
motion-onset events belong on a separate annotation axis with boundaries and
uncertainty; movement onset alone is not proof of walking. Original labels and
AI proposals must remain distinguishable from final human-reviewed annotations.

## Development checks

CI ([`.github/workflows/ci.yml`](.github/workflows/ci.yml)) runs these commands on
every push and pull request, with Node 22 and Python 3.12. It uses no TI
toolchain, board, RF, camera or credentials. Passing checks show software
behavior on synthetic inputs only.

Firmware host tests, from `firmware/original-radar-sensing/` (gcc with
AddressSanitizer and UBSan):

```sh
bash tests/run_tests.sh
python3 -B tools/update_build_identity.py
git diff --exit-code -- BUILD_IDENTITY.json src/build_identity.h
git diff --exit-code -- build/ RELEASE_MANIFEST.json RELEASE_MANIFEST_0.3.2-rc1.json HISTORY/
```

The identity tool regenerates the identity files; the first `git diff` must show
no change. The second confirms the tests left release artifacts untouched.

Shared software, from the repository root:

```sh
node tests/run-all.js
(cd original_training && python3 -B -m unittest discover -s tests -v && python3 -B -m unittest test_camera_import -v)
node tools/check-shared-modules.js
python3 -B tools/release_manifest.py original_training/RELEASE_MANIFEST.json
(cd hardware/enclosure-r02 && sha256sum --check --strict SHA256SUMS.txt)
python3 -B tools/check_links.py
```

The dataset JSON Schema check also needs `jsonschema` (`pip install 'jsonschema>=4.18,<5'`):

```sh
OUT=$(mktemp -d)
python3 -B schemas/make_samples.py "$OUT"
python3 -B schemas/check_schema.py schemas/original-radar-dataset.v1.schema.json "$OUT"/*.json
```

## Privacy and data

Collection/export should be explicitly initiated and local. Publishing recordings
is a separate consent, privacy, quality and authorization decision. Do not upload
captures or participant details in issues. Camera-assisted review requires measured timing and human decisions before training admission. Raw-ADC capture is not supplied by this workflow.

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
