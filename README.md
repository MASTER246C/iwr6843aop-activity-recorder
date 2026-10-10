# IWR6843AOP Activity Recorder

**JUST BEGINNING — documentation scaffold. Recorder modules and tests are awaiting a reviewed publication.**

The reusable collection component of Radar Studio Chromebook, built for the
**Texas Instruments IWR6843AOPEVM (IWR6843AOP), Rev G**. Planned initial input is
processed UART points/tracks from the standalone board, not raw ADC capture.
This repository does not yet supply a standalone runnable recorder, trained
activity classifier, completed camera/AI pipeline or released participant data.

## Three related projects

- [Radar Studio Chromebook: visualizer and easy-to-run package](https://github.com/MASTER246C/radar-studio-chromebook)
- [IWR6843AOP Activity Recorder: reusable collection component](https://github.com/MASTER246C/iwr6843aop-activity-recorder)
- [IWR6843AOP Activity Dataset: collection/review plans and future data](https://github.com/MASTER246C/iwr6843aop-activity-dataset)

## Publication plan

After review, publish the original capture/motion modules, documented API,
reproducible tests, provenance and known limitations here. Radar Studio's easy
package will bundle an identical pinned copy, with upstream commit and file hashes.
Avoid separate implementations: changes should be reconciled and tested before
updating the bundled version. No completed integration is implied by this plan.

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
