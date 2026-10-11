# Original dataset v1 integration contract

Producer: candidate viewer `original_sessions.js`, `Review.dataset()`.
Consumer: `radar_ml.py`; `validate()` is the executable strict admission contract.
No existing TI file format was changed. Unknown additional JSON fields are retained
in full dataset identity but do not silently become features or label truth.

Root: `schema: original-radar-dataset`, integer `version: 1`, objects `source` and
`session`, arrays `segments`, `labels`. Optional `sync_markers`, `diagnostics`,
`provenance`, `dataset_sha256` remain local source context.

Source: protocol `original-radar-v2`, lowercase 64-hex `sha256` of the exact raw
capture JSON text, coordinate_frame `sensor_xyz_m`, timing_basis
`sensor_relative_us`. Producer also supplies capture_version, firmware_version,
and build_id. Preserve the original raw-byte-corroborated capture beside export.
The consumer cannot re-verify raw bytes absent from this derivative.

Session: nonempty id, participant_group pseudonym or null, explicit boolean
synthetic, nonempty config_identity. Producer config_identity is canonical JSON
of firmware/build/protocol/mount/profile, excluding random session IDs. Participant
null is valid export metadata but blocks a split. All segments of this outer
session are one indivisible split group.

Segment: unique id, firmware `session_id` (not outer session ID), frames array.
Firmware connection_epoch, data_generation, config_generation and origin time
remain in metadata. Frame relative_us must be strictly increasing nonnegative
integer, sequence nonnegative integer; detections an array. Feature admission also
requires complete true, capture_fault/missed_frames/deadline_misses 0, integer
quality with no mask-19 failure, truncated false. Exactly one detection must have
finite sensor_xyz_m[3], nonnegative forward y, valid flags & 3, finite radial
velocity_mps, range_m in [0,1000], and xyz magnitude within 0.1 m of range.
Other malformed measurements reject windows rather than inventing values.

Label: unique id, existing segment_id, integer start_us < end_us within observed
segment, task activity or transition, permitted label, review_status
unreviewed/reviewed/excluded, source human/machine. Reviewed status requires human
source, reviewer and reviewed_at. Provenance is asserted, not authenticated.
Activity labels: sitting, standing, walking. Transition labels: stand_up, sit_down,
motion_onset, none. Unknown/null never supplies training truth. A reviewed positive event interval including its timing buffer must be fully
contained in the window; activity and event none must cover the window after
inward buffering. This is narrower than exporting arbitrary intervals.
Timing_uncertainty_ms: null or finite nonnegative milliseconds, maximum 60,000;
null uses 250 ms inward buffer. Camera sync does not change this automatically.

Outputs have independent schema/version tags: original-radar-features (per input),
original-radar-prepared-bundle, original-radar-baseline and
original-radar-evaluation, all version 1. Models freeze task, feature names/order,
window duration, training-only parameters, source dataset hashes and exact split.
An incompatible feature/schema change requires a version change.

## Integrity status

Producer embedded dataset_sha256 is NOT verified by this Python implementation,
because JavaScript/Python canonical number serialization differs. Structural
validation and prepare explicitly report embedded_export_hash_verified:false.
The source.sha256 original-capture identity is likewise an assertion unless
checked against the separately retained capture. The CLI computes exact imported
file bytes SHA-256 separately as input_file_sha256; the canonical full-input hash
freezes all export fields for model/evaluation linkage. None authenticate authors.
