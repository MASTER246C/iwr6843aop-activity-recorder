# Original radar local training scaffold (v0.1.0-preview.3)

Modified/new project component, 2026-10-11. Private local analysis only. This is
**software scaffolding tested with explicitly synthetic fixtures**, not a useful
trained activity classifier, validated person tracker, calibrated physical
measurement, medical tool, or safety detector. No model weights or participant
recordings ship. No network, uploads, RF configuration or camera is required.

## What exists already, and why this is separate

Radar Studio v0.6 has its own substantial TI people/vitals offline workflow in
`OFFLINE_ML.md`, `offline_features.js`, `offline_model.js`, and `offline_review.js`.
Keep using that workflow for its supported TI captures. This component does not
rename original firmware detections as TI tracks or reuse its TI extractor.
It accepts only the new original-radar dataset export v1 from the candidate
`original_sessions.js` session/replay/review module. Existing recorder and dataset
planning material was inspected; this is the concrete original-protocol bridge.
The current host-tool patch adds a verified local camera/radar bundle importer while retaining continuity checks. The matching viewer update is maintained separately in Radar Studio.

## Run locally

Python 3.10+ standard library only. No pip, npm, GPU, cloud service or downloads.
From this folder:

    python radar_ml.py validate take-a.json take-b.json take-c.json
    python radar_ml.py prepare take-a.json take-b.json take-c.json --out prepared.json
    python radar_ml.py train take-a.json take-b.json take-c.json --split split.json --task activity --out activity-model.json
    python radar_ml.py evaluate take-a.json take-b.json take-c.json --model activity-model.json --out activity-evaluation.json

Use `--task transition` for the separate transition vocabulary. Input filenames
are examples of your own private exports. `prepare` is an inspectable derived
artifact, not a replacement for recordings. Train/evaluate always re-read and
validate original dataset exports; they do not accept arbitrary prepared vectors.
All outputs use exclusive file creation and refuse to overwrite existing files.
Validation returns a structural-admission summary to stdout; errors exit nonzero.
It explicitly reports embedded_export_hash_verified:false and
claimed_original_capture_hash_verified:false. Success does not verify the
browser export's declared digest or the absent original capture. The tool records
an independently computed input_file_sha256 for exact imported file bytes and
a canonical full-input dataset hash for frozen evaluation.

`split.json` contains the exact session IDs from the dataset exports:

    {"train":["session-a","session-b"],"heldout":["session-c"]}

Every input session must appear exactly once. Adjacent windows, segments,
connection generations and firmware sessions in one export stay together. All
sessions for one pseudonymous participant must be on the same side. Unknown
participant identity blocks training. Have at least two independent training
sessions for every class, and different held-out participants. This guard is not
an adequate scientific sample-size recommendation. Repeated tuning on holdout
turns it into validation; collect new people/takes for final testing.

## Admission and labels

- Fixed, nonoverlapping 3-second sensor-relative windows starting 250 ms after each segment begins. The initial 250 ms is retained as rejected boundary context; a five-second take can supply one full window.
- Minimum 12 frames; maximum adjacent gap 500 ms; sequence continuity required across the full segment, including adjacent windows and initial boundary context. Every window touched by a discontinuity is rejected; clean later windows remain usable.
- Require explicit good acquisition status, no truncation, exactly one detection,
  valid spatial flags and finite sensor xyz/range/radial speed. Reject inconsistent
  range/xyz, missing coordinates and multiple detections. A single detection does
  not establish identity or prove that the scene contains only one person.
- End-of-recording partial windows and empty/gapped windows stay in the output
  with null features and rejection reasons. Human labels cannot repair quality.
- Only a single, human-reviewed annotation supplies a reference label. Activity and transition `none` must cover the window. Positive transitions must be entirely contained within it, including timing uncertainty.
  Machine labels, unreviewed/excluded/unknown labels, conflicts and uncovered
  intervals remain unlabeled. Stable activity never generates transition `none`.
- Unknown annotation timing uncertainty receives a **250 ms inward buffer** on
  both interval boundaries. A provided nonnegative timing uncertainty replaces
  that buffer. Zero is an assertion of exact timing, not a shortcut to admission.
  The default buffer is an unvalidated exclusion heuristic, not a measured
  uncertainty bound, synchronization guarantee or proof of ground-truth timing.
  Use independently reviewed timing uncertainty when available. Review genuinely observed intervals; never invent precision or pad an event
  interval just to obtain training examples. A transition crossing a window
  boundary stays unlabeled.
- Positive transition labels mean one reviewed event occurred inside the window;
  they do not assert the event lasted the full window or establish exact onset.
  No interpolation fills missing positions, labels, packets or camera alignment.

## Features and model

Features: mean/std range, mean absolute radial speed, radial-speed std, mean
sensor z, last-third minus first-third sensor z, and detection count. Sensor z
is **not floor-relative person height**. No calibrated coordinate transform is
invented; mount/configuration identity must match across every input. The single
accepted detection count is constant and retained as an explicit feature-schema
field; it is not evidence of occupancy.

Training fits scaling from equally weighted session first/second moments, then
class centroids from equally weighted per-session class means. Class rejection
radii use only training distances (max distance × 1.25, 0.35 floor, capped at 0.45
of nearest centroid separation); a 0.1 standardized distance margin is also
required. These heuristic thresholds are not calibrated probabilities. Zero
variance scaling is floored at 1e-6. Inseparable centroids fail training.

Models are deterministic JSON. Evaluation independently reconstructs the model
from its frozen split and exact dataset hashes, then checks every parameter.
Keep the original training and held-out exports to evaluate a saved model. This
intentionally does not support standalone inference on new captures yet.
Neither normalization, centroids nor rejection limits fit held-out rows. Synthetic
and real sessions cannot mix. A synthetic-only output is prominently marked.

Evaluation reports a confusion matrix including `unknown`, reviewed-label
accuracy with unknown retained in the denominator, conditional accuracy among
accepted predictions, labeled prediction coverage, missing held-out classes,
unlabeled counts, quality rejections and reason counts. Zero denominators produce
null. These are window-level metrics, not UART availability or real-world safety
performance. Quality-rejected but human-labeled windows count as unknown.

## Identity, privacy and limits

Duplicate exact source capture SHA-256 values and normalized physical-frame
hashes are blocked, even across renamed session files. Physical hashing uses an explicit measurement/status allowlist. Cross-input
measurement fingerprints ignore time/sequence/index metadata and reject any shared
physical frame content, including cropped or retimed copies. This is deliberately
conservative: separate static scenes can coincide and be refused. Do not edit
measurements to bypass a duplicate warning. Full dataset hashes freeze
annotations, metadata and unknown fields, so edits invalidate saved evaluations.
The export's JavaScript `dataset_sha256` is retained as source metadata; this
Python pipeline computes its own canonical JSON hash rather than pretending the
two number serializers are interchangeable. Preserve the exact raw capture and
viewer export for audit. Hashes and reviewer names are assertions/linkage, not
authentication. Edited measurements, false participant IDs or undisclosed derivation parents
cannot be proven independent by these guards.
Do not split one physical recording into multiple exports or disguise identity.

Each input batch is bounded to 100 files, 96 MiB of input bytes in total and
20,000 derived task rows in total. Separately, each individual dataset is limited
to 100,000 frames across its segments, 2,000 segments, 20,000 labels, 10,000
candidate windows across its segments and two million label/window comparisons.
These per-dataset limits do not become batch-wide frame, segment, label or
comparison limits. This is an in-memory bounded prototype. Private
participant labels, mount information and optional camera marker metadata may
remain in your local prepared exports. There is no public-export sanitization or
publication authorization in this CLI. Do not upload these files. Camera choice
is still open; optional sync metadata is preserved, never used as synchronized
truth, and no camera/video processing is implemented.

## Tests and licensing

    python -m unittest discover -s tests -v

Tests synthesize all measurements, people and labels in memory. Passing tests
establishes software behavior only. Run reviewer/integration acceptance before
release. See `INTEGRATION_CONTRACT.md` and `VERIFICATION.md`.

`LICENSE` preserves the Radar Studio Source-Available No-Resale terms, not an
open-source license. `UPSTREAM_LICENSE_COVERAGE.md` is preserved as upstream
context (its viewer file inventory is not this package's file inventory). New
Python code/docs here are original project material under LICENSE. No TI firmware,
SDK binaries, proprietary algorithms, model weights or private inputs are bundled.
TI and other third-party materials retain their own notices and rights. Source
licensing does not grant permission to publish participant datasets.

## Optional reviewed camera labels

`camera_import.py --bundle "recording-folder" --review "video.review.json" --output "new-dataset.json"` verifies exact video/raw file receipts, accepted radar identity, data-port pairing proof and human-matched clock anchors. Start local recording before configuring and pairing radar. Camera proposals stay unreviewed until explicitly reviewed; unknown or ambiguous activity remains excluded. The output is dataset v1 and uses the same quality, participant split and held-out evaluation gates above. This does not train a model automatically. See the viewer CAMERA_WORKFLOW.md.
