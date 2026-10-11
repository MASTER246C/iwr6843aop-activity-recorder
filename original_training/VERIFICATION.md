# Host-tool v0.1.0-preview.1 verification

2026-10-11, isolated candidate; no RF or public/private-data upload.

- Python standard-library tests: **28 passed**. Includes activity and all transition
  classes, deterministic training, unknown/rejected/unreviewed handling, person and
  session separation, source/model tamper checks, gap/ambiguity/coordinate faults,
  finite/type bounds, aggregate sparse-window limits, five-second take admission,
  positive short-event containment, CLI train/evaluate and overwrite refusal.
- Cross-language smoke: actual synthetic export from candidate
  `original_sessions.js` validates and prepares unmodified. Six rows retained:
  boundary context activity/transition rejected, 0.25–3.25 s activity standing
  admitted, unlabeled transition, final activity/transition partial rows rejected.
- Independent read-only reviewer reran 26 then-current tests and the cross-language
  import and found no remaining principal training blocker. The final two added
  tests cover transition training and CLI end-to-end/no-overwrite. Final runner
  passes all 28. Version/retained optional metadata are non-model changes.
- Existing TI workflow inspected and left unchanged. No disguised TI adapter.
- No real labeled recordings, recognition accuracy, physical calibration, RF,
  camera synchronization, hardware performance or browser behavior is validated.
- No trained model or capture file is distributed. Fixtures are synthetic source
  generators inside tests. Release publication remains a separate approval step.

Reproduce from this folder:

    python -m unittest discover -s tests -v
    python -m py_compile radar_ml.py
