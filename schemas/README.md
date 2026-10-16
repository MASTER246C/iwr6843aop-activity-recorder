# original-radar-dataset v1 schema

`original-radar-dataset.v1.schema.json` is a JSON Schema (draft 2020-12) for the
real `original-radar-dataset` version 1 file. It covers:

| Role | Code |
| --- | --- |
| Producer | viewer `Review.dataset()` in `radar-studio-chromebook/original_sessions.js` |
| Producer | recorder `convert()` in `original_training/camera_import.py` |
| Consumer | `original_training/radar_ml.py` `validate()` / `prepare()` (prose: `original_training/INTEGRATION_CONTRACT.md`) |
| Consumer | viewer `validateDataset()`, used by Resume (`Review.restore`) |

The root, `source`, `session`, `segments[]` and `labels[]` objects are closed:
they list the union of both producers' keys, and any other key is rejected.
`source.kind` selects a producer profile. A file with `source.kind` is checked
against `camera_import_profile`. Any other file is checked against
`viewer_profile`, where every key is closed and every label is human-sourced.
Each profile forbids the other producer's keys. Camera frames and detections are
raw firmware records that camera_import copies verbatim, so they stay open: the
known firmware fields are typed and any other field passes through.

Units are given in the field descriptions and follow the field-name suffixes:
`_us` microseconds, `_ms` milliseconds, `_m` metres, `_mps` metres per second,
`_mm` millimetres, `_mdeg` millidegrees, `_seconds` seconds.

JSON Schema cannot express some cross-field rules, so the descriptions state
them and the producers' validators plus `radar_ml.validate()` enforce them:

- `segment_id` must name an existing segment.
- Ids must be unique.
- `start_us` must be less than `end_us`.
- Label and marker times must lie within the segment's observed frames.
- Frame time and sequence must strictly increase.
- A reviewed label needs human source, a reviewer and `reviewed_at`.

## Running the check

The check needs Python 3 with `jsonschema` (`pip install jsonschema`). The
schema and `check_schema.py` are pinned in `SHARED_MODULES.json`, so Radar Studio
carries byte-identical copies and each repository checks its own producer.

In this repository (camera_import):

    OUT=$(mktemp -d)
    python3 -B schemas/make_samples.py "$OUT"
    python3 -B schemas/check_schema.py schemas/original-radar-dataset.v1.schema.json "$OUT"/*.json

In Radar Studio (viewer exports; Node 18 or newer):

    OUT=$(mktemp -d)
    node tests/make_dataset_samples.js "$OUT"
    python3 -B schemas/check_schema.py schemas/original-radar-dataset.v1.schema.json "$OUT"/*.json

Both run in CI.

`check_schema.py` first checks the schema with
`Draft202012Validator.check_schema`. It then prints `OK` or `FAIL` with JSON
paths for each file and exits non-zero if any file fails. Like radar_ml, it
accepts only true integers (`1.0` is rejected) and rejects NaN and Infinity.

- `make_samples.py` builds bundles with the fixture helpers in
  `original_training/test_camera_import.py`. It writes three camera_import
  exports: reviewed, unreviewed-machine, and mixed with full firmware frames.
- `tests/make_dataset_samples.js` in Radar Studio writes three viewer
  exports through `loadCapture`, the Review edits and `dataset()`:
  - every task/label, reviewed, excluded and unreviewed labels, a sync marker
    and undo;
  - two segments with a null mount;
  - a resumed and extended review.

  Each viewer export is restored once to prove that Resume accepts it.

## Human/machine source boundary

- The viewer records only operator intervals, so its labels are always
  `source: "human"`.
- camera_import keeps the source declared in the review file. Camera review
  exports model proposals as `"machine"` until a person reviews them, and
  manually added intervals as `"human"` even while unreviewed. A label marked
  `review_status: "reviewed"` requires `reviewed: true` and `source: "human"`.
- radar_ml accepts both sources. Only `reviewed` + `human` labels from the
  task vocabulary (not `unknown`) become training truth.
- Viewer Resume accepts only viewer-produced exports of the exact source
  capture that is loaded. It rejects camera_import files: they have
  `source.kind`, machine labels, `camera_proposal_index`, and no
  `dataset_sha256`, `annotation_audit` or `sync_markers`.

## Planned fields to export fields

This table maps the planned fields in `iwr6843aop-activity-dataset/SCHEMA.md`
to the export fields. "V" means the viewer only and "C" means camera_import
only. "not exported yet" means v1 has no equivalent field.

| Planned field (SCHEMA.md) | v1 export field |
| --- | --- |
| schema_version | `schema` + `version` (`original-radar-dataset`, 1) |
| dataset_version | not exported yet |
| session_id | `session.id` |
| pseudonymous_participant_id | `session.participant_group` (a group pseudonym, not per person) |
| board_model, board_revision | not exported yet |
| firmware_name | not exported yet (V: `source.firmware_version`, `source.build_id`) |
| firmware_sha256 | not exported yet |
| cfg_sha256 | `session.config_identity` (V: canonical config JSON text; C: sha256 of firmware/build/mount/bin sizes) |
| protocol_profile | `source.protocol` (V: the profile `room64_v1` appears only inside `config_identity`) |
| parser_version, capture_tool_version | V: `session.viewer_version`; C: not exported yet |
| UART baud rates | not exported yet |
| start_time_basis | `source.timing_basis` (`sensor_relative_us`) |
| frame_period_s, duration_s | not exported yet (derivable from `segments[].frames[].relative_us`) |
| mounting_height_m | V: `session.mount_input.bracket_top_mm` (declared); C: `frames[].mount.bracket_top_m` |
| sensor_orientation | V: `session.mount_input.down_tilt_mdeg`; C: `frames[].mount.down_tilt_deg` |
| coordinate_frame | `source.coordinate_frame` (`sensor_xyz_m`) |
| environment_summary | not exported yet |
| calibration_method, calibration_result, calibration_time_relative | not exported yet (V `provenance.coordinates` states no calibration) |
| consent_scope_reference, public_release_review_status | not exported yet |
| recording_id | `segments[].id` (one timebase); `source.sha256` identifies the capture or bundle |
| relative_path | C: `provenance.raw_receipts[].file`, `provenance.video_receipt.file`; V: not exported yet |
| media_type | not exported yet (C receipt `kind` only) |
| byte_size | C: `provenance.*receipt*.byte_count`; V: not exported yet |
| sha256 | `source.sha256`; C: also each receipt's `sha256` and `provenance.review_sha256` |
| processed_uart_or_decoded_format | `source.protocol` + `segments[].frames[]` |
| units | not a field; declared by suffixes and schema descriptions |
| timestamp_basis | `source.timing_basis` |
| frame_count | not exported yet (length of `segments[].frames`) |
| missing_or_invalid_frame_count | V: `diagnostics.missing_sequence_frames`, `fault_frames`, `truncated_frames`, `invalid_coordinate_detections`, `rejected_admission_entries`; C: not exported yet |
| quality_flags | `segments[].frames[].quality` (with `complete`, `capture_fault`, `truncated`) |
| exclusion_reason | not exported yet (`review_status: "excluded"` carries no reason) |
| split_assignment | not exported yet (radar_ml groups by `participant_group`) |
| derivation_parent_ids | `source.sha256`; C: also `provenance.review_sha256` |
| processing_steps and software versions | V: `session.viewer_version`, `provenance`; C: `diagnostics.measurement_gates` (prose) |
| annotation_id | `labels[].id` |
| interval_start_s, interval_end_s | `labels[].start_us`, `labels[].end_us` (integer µs from segment origin) |
| captured_label | `labels[].label`; V: prior values in `annotation_audit[].details.previous` |
| ai_proposed_label | C: `provenance.machine_proposals[].label`, and labels with `source: "machine"` |
| ai_confidence | C: `provenance.machine_proposals[].activity_confidence` |
| ai_model_name, ai_model_version | not exported yet (the review's `model` is not copied) |
| review_flag, human_review_status | `labels[].review_status` |
| disagreement_flag, ambiguity_reason, decision_reason | not exported yet |
| human_final_label | `labels[].label` where `review_status` is `reviewed` and `source` is `human` |
| reviewer_pseudonym | `labels[].reviewer` |
| reviewed_at | `labels[].reviewed_at` |
| prior_annotation_id | not exported yet |
| annotation_version | V: `annotation_audit[].revision` (log-wide, not per label); C: not exported yet |
| video_relative_path / private_reference | C: `provenance.video_receipt.file` |
| video_sha256 | C: `provenance.video_receipt.sha256` |
| frame_rate_hz | not exported yet |
| radar_to_video_time_offset_s | C: `provenance.alignment.video_to_sensor_offset_us` (video to sensor, µs) |
| drift_estimate | C: `provenance.alignment.video_to_sensor_scale`, `drift_ppm` |
| synchronization_method | C: `provenance.alignment.method`, `status` |
| synchronization_uncertainty_s | C: `provenance.alignment.uncertainty_us`; per label `timing_uncertainty_ms` |
| extrinsic_transform, transform_direction, coordinate_frames, calibration method/version/residuals | not exported yet |
| video_consent_scope, privacy_review_status, public_video_release_approved | not exported yet (V `provenance.raw_and_video_included` is false) |
| event_id | `labels[].id` with `task: "transition"` |
| event_type | `labels[].label` with `task: "transition"` (`stand_up` for sit_to_stand, `sit_down`, `motion_onset`, `none`, `unknown`) |
| event_start_s, event_end_s | `labels[].start_us`, `labels[].end_us` (v1 requires end > start; point events need a nonzero span) |
| boundary_uncertainty_s | `labels[].timing_uncertainty_ms` |
| operational_definition_version | not exported yet |
| human_reviewer, review_time | `labels[].reviewer`, `labels[].reviewed_at` |

The planned activity vocabulary (sitting, standing, walking, transition,
unknown) maps to two v1 tasks. `activity` is walking/standing/sitting/unknown;
`transition` is a separate task with stand_up/sit_down/motion_onset/none/unknown.
