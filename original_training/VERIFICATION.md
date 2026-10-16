# Host-tool v0.1.0-preview.4 verification (unreleased source)

2026-10-16, synthetic software checks. Schema versions remain 1.

- preview.4 changes: output files are written atomically (a failed write leaves no
  partial file and an existing file is never replaced); camera import skips a label
  that rounds to zero length instead of aborting, and keeps the review file's
  declared label source, so manually added unreviewed intervals stay human-sourced.
- `tests/test_radar_ml.py`: **34 passed** (33 from preview.2 plus the atomic-write check).
  `test_camera_import.py`: **9 passed** (7 plus the rounding and label-source checks).
- preview.2 baseline (2026-10-11): Python standard-library tests: **33 passed**. The previous 28 cases remain, with added whole-segment timestamp/sequence checks across adjacent windows, gaps in initial boundary context, a current five-second take, and clean later-window admission.
- The existing cases cover activity/transition labels, deterministic training, source/model tamper checks, person/session split separation, quality rejection, timing uncertainty, sparse-window/resource bounds, CLI end-to-end and overwrite refusal.
- Cross-window faults now reject every window touched by the adjacent-frame interval. A reviewed annotation cannot repair missing or discontinuous sensor data.
- This host-tool package contains source, documentation and synthetic generators. It contains no private recording or trained weights and performs no serial/RF or upload operation.
- No recognition accuracy, physical calibration, camera synchronization or hardware performance is validated. Publication and hosted verification are recorded separately.

Reproduce from this folder with Python 3.10+:

    python -m unittest discover -s tests -v
    python -m unittest test_camera_import -v
