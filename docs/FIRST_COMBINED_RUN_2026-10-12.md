# First combined continuous radar and camera run — 2026-10-12

This is the first reviewed combined run of the new continuous firmware and C920e recording workflow. It is a brief physical acquisition/Stop test, not activity-recognition validation or dataset admission.

## Exact software and hardware

- TI IWR6843AOPEVM (IWR6843AOP); owner setup.
- Firmware: **0.3.2-rc1**, build **87589b2c32638ae119334c72**; onboard self-test version 2 reported passing.
- BIN SHA-256: `162db11aa7472d939b1039557bcad95253752c6562261d5ed851d6216c574fb9`.
- Viewer: **v0.9.0-preview.5**, [hosted Radar Studio](https://radar-studio.master246c.chatgpt.site), deployed Site version 14.
- Camera: Logitech C920e; recorded files decode at **1920 × 1080**; browser settings report 30 fps; audio disabled.
- Any browser-supported camera remains selectable; this run tests the C920e only.
- The earlier 0.3.1-rc1 BIN remains available for rollback. No new BIN was produced for this report.

## Observed results

| Check | Evidence and result |
| --- | --- |
| Configuration / pairing | Matching data-only nonce probe and paired control status for the same configuration and session; separate Start ACK and running status |
| Continuous acquisition | **283 complete consecutive frames**, sequence 0–282, spanning **56.4 seconds**; every sensor interval was 200,000 microseconds (5 Hz) |
| Fault reporting | Zero reported missed frames, capture faults, RF faults/errors and deadline misses; final transmit-fault flags false |
| Control heartbeat | 53 successful keepalive ACKs for the matching run; largest observed gap 1,257 ms |
| Manual Stop | Successful Stop ACK followed by fault-free idle reporting 283 frames |
| Stop timing | Browser command-to-ACK 85.8 ms; command-to-confirmed-idle 173.1 ms in this single trial. These are host observations, not measured RF shutdown latency or worst-case guarantees |
| Local recording | Complete final receipt; 68.2695 seconds of session time; three video segments and separate original control/data lanes |
| File integrity | All completed-file hashes, byte lengths, contiguous lane offsets and final lane totals matched; all three video segments fully decoded without errors |
| Independent export comparison | Downloaded data JSONL was byte-identical to the concatenated recording data lane; the control download included the recording's control bytes plus earlier setup replies |
| Camera segmentation | Recorder stop-to-next-start gaps of approximately 218.9 and 249.4 ms were recorded, not hidden or treated as continuous exposure |

An earlier camera-only take had zero radar measurement bytes. It was distinguished from this combined take rather than counted as successful radar acquisition.

## Activity observations and real camera-model check

The owner described: seated, rolled the chair back, stood, sat, rolled. Assistant visual inspection of sampled decoded frames supports that sequence. Rolling a chair is **not walking**. These observations have approximate video-local timing and are not human-reviewed, synchronized training labels.

The hosted Google MediaPipe Pose Landmarker Lite workflow was run on 29 seconds of the saved segment containing the sit–stand movement, using the existing five-samples-per-second analysis. It completed and produced one **unknown / unreviewed** interval with the reason “Insufficient visible single-person pose.” The view crops lower legs/feet; the current activity rules require visible shoulders, hips, knees and ankles. The result does not establish camera activity accuracy.

The exported review retained null camera/radar alignment and unreviewed machine labels. The existing importer rejected it with “Human-matched timing alignment is required.” No anchors or reviewed labels were invented.

Model: Pose Landmarker Lite, Tasks Vision runtime 1.1.0, task SHA-256 `59929e1d1ee95287735ddd833b19cf4ac46d29bc7afddbbf6753c459690d574a`; activity rules `pose-temporal-rules-v1`. Google supplies the pose model; the project's separate experimental rules propose activities.

## Radar training outcome

The existing single-detection baseline admitted **zero of 283 frames**. Its first rejection reasons were:

- 227 frames: truncated serialized detection coverage.
- 56 frames: multiple detections rather than a single unambiguous measurement.

These are reflection peaks, not a people count. Rejection at an earlier check does not prove that every later spatial-quality check would pass. Raw data was preserved. No trained radar model, held-out accuracy or admitted dataset samples resulted from this run.

A future separately versioned representation of serialized range/radial-velocity reflections may help study this data. It must retain truncation and association uncertainty and be evaluated with suitable independent recordings; the current baseline and its exclusions were not weakened.

## Remaining physical and learning checks

- Deliberate keepalive-loss/disconnect stop and long-duration endurance on hardware.
- Physical timestamp rollover, calibration, position and activity accuracy.
- Wider/full-body camera framing, reviewed labels and measured camera/radar alignment.
- Eligible independent acquisition sessions and participant groups for training/held-out evaluation.

The observed quality flags retain uncalibrated phase/power and unverified motion-sign caveats. A stable serial stream does not remove them.

## Provenance and publication boundary

This report summarizes local raw UART logs, camera-file receipts, complete video decoding, visual inspection and an actual hosted camera-model pass. It is an owner-run test reviewed by the development agent, not an independent laboratory validation.

Private recordings, video, device identifiers, local paths and participant metadata are not published here. Only this aggregate report and documentation links are added; released ZIPs, BINs, model assets and hosted code are unchanged. The dataset remains **0 released samples**.

Locally retained export digests for future reconciliation:

- Control JSONL: `d0a6c72dbabdadf0f2f311f9261980ebe5ccb8cd81cfb834aaee2effbc989d95`.
- Data JSONL: `038e00b53fd6299469756aacf4ad4d8f22418dc6ed1960458833c0ef4056cc1f`.

Hashes establish byte consistency; the underlying private recordings are not publicly reproducible from this report.
