# IWR6843AOP Activity Recorder

**Mount reference height: 2.00 meters from floor to the flat TOP of the tripod mount, excluding screws.**

This is the selected experimental activity-collection setup, with a final **15-degree downward tilt**. The mount reference is not the antenna/chip-center height. After setting the final tilt, measure the signed **vertical** offset from the mount-top reference plane to the antenna/chip center:

**Antenna-center height = 2.00 m + measured signed vertical chip-center offset.**

The chip is above the mount in the current photographed arrangement, so its offset is positive, but its numeric value is still unknown. Do not guess it or substitute 2.00 m for chip height. The owner plans to adjust the setup when ready to load the software; that adjustment and calibration have not yet been completed. This is a repeatable experimental standard, not a universal optimum or a proven most-accurate setting. Verify and record the actual geometry before collection; no firmware `sensorPosition` or other configuration value is inferred here.

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
