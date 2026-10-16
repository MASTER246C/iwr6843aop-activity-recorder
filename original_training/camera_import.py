#!/usr/bin/env python3
"""Verify a local camera/radar bundle and map human-reviewed labels to dataset v1.

No upload, model inference or automatic acceptance. Uses the existing trainer's
measurement/label gates. File hashes establish byte integrity, not authenticity.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import radar_ml as ml


def load(path, limit=8*1024*1024):
    ml.require(path.stat().st_size <= limit, 'Metadata file too large: '+path.name)
    return json.loads(path.read_text(encoding='utf-8'), parse_constant=lambda _: ml.fail('Non-finite JSON'))


def file_hash(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            h.update(block)
    return h.hexdigest()


def checked_file(folder, name, session_id, kind):
    ml.require(Path(name).name == name and '/' not in name and '\\' not in name, 'Invalid bundle filename')
    path = folder/name
    ml.require(path.resolve().parent == folder.resolve(), 'File leaves recording folder')
    receipt = load(folder/(name+'.meta.json'))
    ml.require(receipt.get('schema') == 'radar-camera-file' and receipt.get('version') == 1
               and receipt.get('file') == name and receipt.get('session_id') == session_id
               and receipt.get('kind') == kind and receipt.get('complete_file') is True,
               'Missing or mismatched completed-file receipt: '+name)
    ml.require(receipt.get('byte_count') == path.stat().st_size and receipt.get('sha256') == file_hash(path),
               'File hash or byte count mismatch: '+name)
    return path, receipt


def lane_records(folder, kind, session_id, end, receipts):
    names = sorted(p.name for p in folder.glob(kind+'-*.jsonl') if re.fullmatch(kind+r'-\d{6}\.jsonl', p.name))
    total = 0
    tail = b''
    for i, name in enumerate(names):
        ml.require(name == f'{kind}-{i:06d}.jsonl', 'Missing or unordered raw segment')
        path, receipt = checked_file(folder, name, session_id, kind)
        ml.require(receipt.get('global_byte_start') == total, 'Raw segment offset mismatch')
        total += receipt['byte_count']
        ml.require(receipt.get('global_byte_end') == total, 'Raw segment end mismatch')
        receipts.append(receipt)
        with path.open('rb') as stream:
            while True:
                block = stream.read(65536)
                if not block:
                    break
                parts = (tail+block).split(b'\n')
                tail = parts.pop()
                ml.require(len(tail) <= 4096, 'Overlong raw record')
                for line in parts:
                    ml.require(0 < len(line) <= 4096, 'Invalid raw line length')
                    record = json.loads(line.decode('utf-8'), parse_constant=lambda _: ml.fail('Non-finite raw JSON'))
                    ml.require(isinstance(record, dict) and record.get('protocol') == 'original-radar-v2', 'Unknown raw protocol')
                    yield record
    ml.require(not tail, 'Incomplete final raw record; preserve recording for diagnosis')
    expected = end.get('lanes', {}).get('control' if kind == 'radar-control' else 'data', {})
    ml.require(expected.get('byte_count') == total and expected.get('completed_files') == len(names), 'Session end does not match raw lane')


def alignment_fit(a):
    ml.require(isinstance(a, dict) and a.get('status') == 'human-matched-events' and a.get('method') == 'affine-fit-v1', 'Human-matched timing alignment is required')
    anchors = a.get('anchors')
    ml.require(isinstance(anchors, list) and 3 <= len(anchors) <= 1000, 'At least three timing anchors are required')
    for p in anchors:
        ml.require(isinstance(p,dict) and ml.finite(p.get('video_seconds')) and p['video_seconds'] >= 0
                   and type(p.get('sensor_timestamp_us')) is int and p['sensor_timestamp_us'] >= 0
                   and ml.finite(p.get('uncertainty_ms')) and 0 < p['uncertainty_ms'] <= 60000, 'Invalid alignment anchor')
    points = sorted(anchors, key=lambda p:p['video_seconds'])
    ml.require(points[-1]['video_seconds']-points[0]['video_seconds'] >= 1, 'Anchors need at least one second of coverage')
    ml.require(all(q['video_seconds'] > p['video_seconds'] and q['sensor_timestamp_us'] > p['sensor_timestamp_us'] for p,q in zip(points,points[1:])), 'Anchor clocks must increase')
    xs = [p['video_seconds']*1e6 for p in points]
    ys = [p['sensor_timestamp_us'] for p in points]
    mx, my = sum(xs)/len(xs), sum(ys)/len(ys)
    scale = sum((x-mx)*(y-my) for x,y in zip(xs,ys))/sum((x-mx)**2 for x in xs)
    offset = my-scale*mx
    ml.require(abs(scale-1) <= .01, 'Alignment clock scale is outside 1%')
    uncertainty = max(abs(y-(offset+scale*x)) for x,y in zip(xs,ys))+max(p['uncertainty_ms']*1000 for p in points)
    for key, computed in [('video_to_sensor_scale',scale),('video_to_sensor_offset_us',offset),('uncertainty_us',uncertainty),('valid_video_start_seconds',points[0]['video_seconds']),('valid_video_end_seconds',points[-1]['video_seconds'])]:
        ml.require(ml.finite(a.get(key)) and math.isclose(a[key], computed, rel_tol=1e-9, abs_tol=1e-4), 'Exported alignment disagrees with its anchors: '+key)
    return scale, offset, uncertainty, points[0]['video_seconds'], points[-1]['video_seconds']


def convert(folder, review_path):
    folder, review_path = Path(folder), Path(review_path)
    start, end, review = load(folder/'session-start.json'), load(folder/'session-end.json'), load(review_path)
    sid = start.get('session_id')
    ml.require(start.get('schema') == 'radar-camera-session' and start.get('version') == 1 and ml.text(sid), 'Unsupported recording')
    ml.require(end.get('schema') == 'radar-camera-session-end' and end.get('session_id') == sid and end.get('complete') is True, 'Recording is incomplete or lacks a matching final receipt')
    ml.require(review.get('schema') == 'radar-camera-review' and review.get('version') == 1, 'Unsupported camera review')
    binding = review.get('recording') or {}
    ml.require(binding.get('session_id') == sid and binding.get('participant_group') == start.get('participant_group') and ml.text(start.get('participant_group')), 'Review needs matching session and participant group')
    video = review.get('video') or {}
    ml.require(isinstance(video.get('file'),str), 'Missing reviewed video filename')
    _, video_receipt = checked_file(folder, video['file'], sid, 'camera')
    ml.require(video.get('sha256') == video_receipt['sha256'] and video.get('bytes') == video_receipt['byte_count'], 'Review is for different video bytes')
    scale, offset, uncertainty, first, last = alignment_fit(review.get('alignment'))
    run = review['alignment'].get('radar_run') or {}
    ml.require(re.fullmatch('[0-9a-f]{32}',str(run.get('pair_nonce',''))) and type(run.get('config_generation')) is int and type(run.get('session_id')) is int, 'Invalid radar run identity')
    def matches(r):
        return all(r.get(k) == run[k] for k in ['pair_nonce','config_generation','session_id'])
    receipts, evidence = [], []
    accepted = {('0.3.1-rc1','abb27b48c1bdb956c0de704b'), ('0.3.2-rc1','87589b2c32638ae119334c72')}
    for r in lane_records(folder, 'radar-control', sid, end, receipts):
        if r.get('type') == 'status' and r.get('channel') == 'control' and matches(r) and r.get('state') == 'running':
            ml.require(r.get('paired') is True and r.get('configured') is True and r.get('selftest_ok') is True and r.get('selftest_version') == 2
                       and (r.get('firmware_version'),r.get('build_id')) in accepted
                       and all(type(r.get(k)) is int and r[k] == 0 for k in ['fault','selftest_mask','capture_fault','rf_fault','rf_error','deadline_misses'])
                       and r.get('control_tx_fault') is False and r.get('data_tx_fault') is False, 'Running status fails the existing firmware/self-test gate')
            evidence.append(r)
    ml.require(evidence, 'No matching accepted running status in the preserved control bytes')
    frames, probe = [], False
    lower, upper = offset+scale*first*1e6, offset+scale*last*1e6
    for r in lane_records(folder, 'radar-data', sid, end, receipts):
        if not matches(r):
            continue
        ml.require(r.get('channel') == 'data', 'Wrong raw data channel')
        if r.get('type') == 'data_probe':
            probe = (r.get('firmware_version'),r.get('build_id')) in accepted
        elif r.get('type') == 'frame':
            ml.require(probe, 'No preceding matching data-port identity probe; start camera recording before pairing')
            t = r.get('timestamp_us')
            ml.require(type(t) is int and t >= 0 and type(r.get('sequence')) is int, 'Invalid radar frame time/sequence')
            if lower <= t <= upper:
                ml.require(not frames or t > frames[-1]['timestamp_us'], 'Radar timestamps do not increase')
                ml.require(isinstance(r.get('detections'),list) and r.get('count') == len(r['detections']) and type(r.get('detected_count')) is int and r['detected_count'] >= r['count'] and r.get('truncated') == (r['detected_count'] > r['count']), 'Invalid radar frame coverage')
                frames.append(r)
                ml.require(len(frames) <= 100000, 'Review contains too many frames')
    ml.require(len(frames) >= 2, 'No usable time span overlaps the matched radar run')
    origin = frames[0]['timestamp_us']
    segment_id = 'camera-'+video['file']+'-'+str(run['session_id'])
    labels = []
    for i, label in enumerate(review.get('labels', [])):
        ml.require(isinstance(label,dict), 'Invalid review label')
        a,b,u = label.get('start_seconds'),label.get('end_seconds'),label.get('boundary_uncertainty_ms')
        ml.require(ml.finite(a) and ml.finite(b) and 0 <= a < b and ml.finite(u) and 0 < u <= 60000, 'Invalid camera label interval/uncertainty')
        ml.require(label.get('label') in ['sitting','standing','walking','unknown'], 'Unsupported camera activity')
        # Clip to actual matched observations. Nothing outside anchor coverage is mapped.
        left = max(origin, lower, offset+scale*a*1e6)
        right = min(frames[-1]['timestamp_us'], upper, offset+scale*b*1e6)
        start_us, end_us = math.ceil(left-origin), math.floor(right-origin)
        if end_us <= start_us:
            continue  # nothing left after clipping and integer rounding
        reviewed = label.get('review_status') == 'reviewed'
        ml.require(not reviewed or (label.get('reviewed') is True and label.get('source') == 'human'), 'Reviewed flag/source mismatch')
        # Keep the reviewer's declared origin: manual intervals are human even before review.
        source = label.get('source') if label.get('source') in ('human','machine') else ('human' if reviewed else 'machine')
        labels.append({'id':'camera-'+str(i),'segment_id':segment_id,'task':'activity','label':label['label'],
                       'start_us':start_us,'end_us':end_us,
                       'timing_uncertainty_ms':(uncertainty+u*1000*abs(scale))/1000,
                       'source':source,'review_status':'reviewed' if reviewed else 'unreviewed',
                       'reviewer':label.get('reviewer'),'reviewed_at':label.get('reviewed_at'),
                       'camera_proposal_index':label.get('proposal_index')})
    source_hash = ml.digest({'session_start_sha256':file_hash(folder/'session-start.json'),'session_end_sha256':file_hash(folder/'session-end.json'),'files':receipts,'video':video_receipt})
    output = {'schema':'original-radar-dataset','version':1,
              'source':{'protocol':'original-radar-v2','coordinate_frame':'sensor_xyz_m','timing_basis':'sensor_relative_us','sha256':source_hash,'kind':'verified-local-camera-radar-bundle'},
              'session':{'id':sid,'participant_group':start['participant_group'],'synthetic':start.get('synthetic',False),'config_identity':ml.digest({'firmware_version':evidence[0]['firmware_version'],'build_id':evidence[0]['build_id'],'mount':frames[0].get('mount'),'range_bin_m':frames[0].get('range_bin_m'),'velocity_bin_mps':frames[0].get('velocity_bin_mps')})},
              'segments':[{'id':segment_id,'session_id':run['session_id'],'frames':[{**f,'relative_us':f['timestamp_us']-origin} for f in frames]}],
              'labels':labels,'provenance':{'review_sha256':file_hash(review_path),'video_receipt':video_receipt,'raw_receipts':receipts,'alignment':review['alignment'],'machine_proposals':review.get('machine_proposals',[]),'camera':start.get('camera'),'source_bytes_verified':True,'authenticity_verified':False,'physical_sync_accuracy_independently_verified':False},
              'diagnostics':{'measurement_gates':'Unchanged radar_ml.prepare; no automatic admission of camera predictions'}}
    ml.validate(output)
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle',required=True,type=Path)
    parser.add_argument('--review',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args = parser.parse_args()
    try:
        dataset = convert(args.bundle,args.review)
        features = ml.prepare(dataset)
        ml.write(args.output,dataset)
        print(json.dumps({'output':str(args.output),'frames':sum(len(s['frames']) for s in dataset['segments']), 'reviewed_activity_windows':sum(r['task']=='activity' and r['label'] is not None and not r['rejection_reasons'] for r in features['records']), 'trained_model':False}))
    except (ValueError, OSError, KeyError, TypeError) as e:
        parser.exit(2,'Camera import failed: '+str(e)+'\n')


if __name__ == '__main__':
    main()
