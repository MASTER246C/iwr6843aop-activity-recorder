# IWR6843AOP Activity Recorder

**Experimental v0.5-bundle1 — original reusable recorder and motion modules.**

The reusable collection component of Radar Studio Chromebook, built for the
**Texas Instruments IWR6843AOPEVM (IWR6843AOP), Rev G**. Planned initial input is
processed UART points/tracks from the standalone board, not raw ADC capture.
This repository supplies reusable JavaScript modules, not a standalone browser UI
or a pretrained activity model. Use Radar Studio for the integrated application.
Real-browser rendering, hardware throughput and real-world classification
accuracy remain unverified. No participant captures are included.

## Three related projects

- [Radar Studio Chromebook: visualizer and easy-to-run package](https://github.com/MASTER246C/radar-studio-chromebook)
- [IWR6843AOP Activity Recorder: reusable collection component](https://github.com/MASTER246C/iwr6843aop-activity-recorder)
- [IWR6843AOP Activity Dataset: collection/review plans and future data](https://github.com/MASTER246C/iwr6843aop-activity-dataset)

## Use and reproduce checks

See [RECORDER_API.md](RECORDER_API.md) for browser/CommonJS APIs and
[MOTION_LAB.md](MOTION_LAB.md) for capture, calibration and experimental limits.
Run `node tests/motion.test.js` and `node tests/capture.test.js` from this folder:
29 motion logic tests and 24 capture tests pass using synthetic inputs.

`SHARED_MODULES.json` pins 14 exact module/test/support files by SHA-256. These
same bytes ship in Radar Studio v0.5, without runtime remote loading. Changes must
be reconciled and tested before either copy is updated; do not develop silent
forks. Source: Radar Studio v0.5 build, ZIP SHA-256
`599ef0068e198c5ae5756e2936b28fe9a8a9393d4c3e7fadcc61f67677d04e03`.

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
