#!/usr/bin/env python3
"""Write sample original-radar-dataset v1 files produced by camera_import.convert().

Usage: python3 schemas/make_samples.py OUTPUT_DIR

The bundles are built with the fixture helpers in
original_training/test_camera_import.py (setUp/save/convert), so the samples
follow camera_import's own tests. Each dataset also goes through
radar_ml.prepare, as the camera_import CLI does, and is written with
radar_ml.write. The data is synthetic.
"""
import sys

sys.dont_write_bytecode = True  # do not leave __pycache__ in the repository

from pathlib import Path

TRAINING = Path(__file__).resolve().parent.parent / 'original_training'
sys.path.insert(0, str(TRAINING))

import radar_ml as ml  # noqa: E402
import test_camera_import as fixtures  # noqa: E402


def reviewed(case):
    """The fixture as written: one reviewed human 'standing' label."""


def unreviewed_machine(case):
    """test_machine_proposals_do_not_train: the label stays an unreviewed machine proposal."""
    case.review['labels'][0].update(reviewed=False, review_status='unreviewed', source='machine')


def mixed_full_frames(case):
    """Reviewed and unreviewed labels in every camera activity, machine proposals,
    camera metadata, the review tool's extra alignment fields, and frames with
    every field the firmware emits (json_output.c)."""
    case.start['camera'] = {'actual_label': 'synthetic camera', 'settings': {'width': 640, 'height': 480, 'frameRate': 30}, 'audio': False}
    case.review['alignment'].update(drift_ppm=0, max_residual_us=0, physical_sync_accuracy_independently_verified=False)
    proposal = {'source': 'camera-model-proposal', 'review_status': 'unreviewed', 'method': 'pose-temporal-rules-v1',
                'activity_accuracy_validated': False, 'activity_confidence': None, 'reason': 'synthetic'}
    case.review['machine_proposals'] = [dict(proposal, label='standing', start_seconds=0, end_seconds=4),
                                        dict(proposal, label='walking', start_seconds=4, end_seconds=8)]
    human = {'boundary_uncertainty_ms': 100, 'review_status': 'reviewed', 'reviewed': True, 'source': 'human',
             'reviewer': 'synthetic-tester', 'reviewed_at': '2026-10-12T00:00:00.000Z'}
    machine = {'boundary_uncertainty_ms': 100, 'review_status': 'unreviewed', 'reviewed': False, 'source': 'machine',
               'reviewer': None, 'reviewed_at': None}
    case.review['labels'] = [
        dict(human, start_seconds=0, end_seconds=4, label='standing', proposal_index=0),
        dict(machine, start_seconds=4, end_seconds=8, label='walking', proposal_index=1),
        dict(human, start_seconds=8, end_seconds=10, label='sitting', proposal_index=None),
        dict(machine, start_seconds=10, end_seconds=12, label='unknown', proposal_index=None),
    ]
    for frame in case.frames:
        frame.update(frame_events=1, processing_us=900, previous_tx_us=1200, session_mode='continuous', session_seconds=0,
                     keepalive_lease_ms=5000, timestamp_basis='session_elapsed', range_bin_m=0.195, velocity_bin_mps=0.3,
                     profile_energy=[1.0, 1.0, 1.0],
                     mount={'bracket_top_m': 1.219, 'down_tilt_deg': 0, 'antenna_offset_m': None, 'antenna_height_m': None, 'world_height_valid': False})
        for detection in frame['detections']:
            detection.update(power=10.5, phasors=[[1, 0], [1, 0], [1, 0], [1, 0]], world_xyz_m=None, activity=None)


SAMPLES = {'camera_import_reviewed': reviewed,
           'camera_import_unreviewed_machine': unreviewed_machine,
           'camera_import_mixed_full_frames': mixed_full_frames}


def build(change):
    case = fixtures.CameraImportTest()
    case.setUp()
    try:
        change(case)
        case.save()
        dataset = case.convert()  # camera_import.convert() runs radar_ml.validate()
        ml.prepare(dataset)       # the CLI runs this before writing
        return dataset
    finally:
        case.tearDown()


def main(argv):
    if len(argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    out = Path(argv[1])
    out.mkdir(parents=True, exist_ok=True)
    for name, change in SAMPLES.items():
        dataset = build(change)
        path = out / (name + '.json')
        path.unlink(missing_ok=True)
        ml.write(path, dataset)
        states = sorted({l['review_status'] + '/' + l['source'] for l in dataset['labels']})
        print(str(path) + ': ' + str(len(dataset['labels'])) + ' label(s) ' + ', '.join(states))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
