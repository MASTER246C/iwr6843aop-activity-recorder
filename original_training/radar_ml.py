#!/usr/bin/env python3
"""Private, local original-radar dataset baseline. Python standard library only."""
import argparse
from datetime import datetime
import hashlib
import json
import math
from pathlib import Path
import statistics

TOOL_VERSION = '0.1.0-preview.3'
VERSION = 1
WINDOW_US = 3_000_000
LABELS = {'activity': ['sitting', 'standing', 'walking'],
          'transition': ['stand_up', 'sit_down', 'motion_onset', 'none']}
FEATURE_NAMES = ['mean_range_m', 'std_range_m', 'mean_abs_radial_mps',
                 'std_radial_mps', 'mean_sensor_z_m', 'sensor_z_change_m',
                 'mean_detection_count']
MAX_BYTES = 96 * 1024 * 1024


def fail(message):
    raise ValueError(message)


def require(condition, message):
    if not condition:
        fail(message)


def finite(x):
    return type(x) in (int, float) and math.isfinite(x) and abs(x) <= 1e15


def canonical(x):
    return json.dumps(x, sort_keys=True, separators=(',', ':'), allow_nan=False)


def digest(x):
    return hashlib.sha256(canonical(x).encode()).hexdigest()


def read(path):
    require(Path(path).stat().st_size <= MAX_BYTES, 'Input exceeds 96 MiB')
    return json.loads(Path(path).read_text(), parse_constant=lambda _: fail('Non-finite JSON'))


def write(path, value):
    with open(path, 'x', encoding='utf-8') as out:
        out.write(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')


def text(x):
    return isinstance(x, str) and bool(x.strip()) and len(x) <= 4096


def validate(data):
    require(isinstance(data, dict) and data.get('schema') == 'original-radar-dataset'
            and data.get('version') == VERSION, 'Unsupported dataset schema/version')
    source, session = data.get('source', {}), data.get('session', {})
    require(isinstance(source,dict) and isinstance(session,dict), 'Invalid source/session objects')
    require(source.get('protocol') == 'original-radar-v2', 'Unsupported source protocol')
    require(source.get('coordinate_frame') == 'sensor_xyz_m' and source.get('timing_basis') == 'sensor_relative_us', 'Explicit sensor coordinates/time basis required')
    require(text(source.get('sha256')) and len(source['sha256']) == 64
            and all(c in '0123456789abcdef' for c in source['sha256']), 'Invalid source SHA-256')
    require(text(session.get('id')), 'Missing session id')
    require(session.get('participant_group') is None or text(session['participant_group']), 'Invalid participant group')
    require(type(session.get('synthetic')) is bool, 'Explicit session.synthetic required')
    require(text(session.get('config_identity')), 'Explicit config_identity required')
    segments, labels = data.get('segments'), data.get('labels')
    require(isinstance(segments, list) and 0 < len(segments) <= 2000, 'Invalid segments')
    require(isinstance(labels, list) and len(labels) <= 20000, 'Invalid labels')
    ids, total, windows = set(), 0, 0
    for seg in segments:
        require(isinstance(seg,dict), 'Invalid segment object')
        require(text(seg.get('id')) and seg['id'] not in ids, 'Duplicate/missing segment id')
        ids.add(seg['id'])
        require(type(seg.get('session_id')) in (str, int), 'Missing firmware session id')
        frames = seg.get('frames')
        require(isinstance(frames, list) and bool(frames), 'Empty/invalid frames')
        total += len(frames)
        require(total <= 100000, 'Too many frames')
        last = -1
        for f in frames:
            require(isinstance(f,dict), 'Invalid frame object')
            t = f.get('relative_us')
            require(type(t) is int and last < t <= 10**12, 'Non-increasing/invalid relative time')
            last = t
            require(type(f.get('sequence')) is int and f['sequence'] >= 0, 'Invalid sequence')
            require(isinstance(f.get('detections'), list) and len(f['detections']) <= 3 and all(isinstance(d,dict) for d in f['detections']), 'Invalid detections')
        windows += (frames[-1]['relative_us']-frames[0]['relative_us'])//WINDOW_US + 2
        require(windows <= 10000, 'Aggregate window limit exceeded')
    seen = set()
    for label in labels:
        require(isinstance(label,dict), 'Invalid annotation object')
        require(text(label.get('id')) and label['id'] not in seen, 'Duplicate/missing label id')
        seen.add(label['id'])
        require(label.get('segment_id') in ids, 'Unknown annotation segment')
        uncertainty = label.get('timing_uncertainty_ms')
        require(uncertainty is None or (finite(uncertainty) and 0 <= uncertainty <= 60000), 'Invalid timing uncertainty')
        require(label.get('task') in LABELS, 'Invalid annotation task')
        require(label.get('label') in LABELS[label['task']] + ['unknown', None], 'Invalid label vocabulary')
        require(label.get('review_status') in ['reviewed', 'unreviewed', 'excluded'], 'Invalid review status')
        require(label.get('source') in ['human', 'machine'], 'Invalid annotation source')
        a, b = label.get('start_us'), label.get('end_us')
        require(type(a) is int and type(b) is int and 0 <= a < b <= 10**12, 'Invalid label interval')
        segment = next(s for s in segments if s['id']==label['segment_id'])
        require(b <= segment['frames'][-1]['relative_us'], 'Label extends beyond observations')
        if label['review_status'] == 'reviewed':
            require(label['source'] == 'human' and text(label.get('reviewer'))
                    and text(label.get('reviewed_at')), 'Reviewed labels require human provenance')
            try:
                reviewed = datetime.fromisoformat(label['reviewed_at'].replace('Z','+00:00'))
                require(reviewed.tzinfo is not None, 'Review timestamp needs timezone')
            except ValueError:
                fail('Invalid review timestamp')
    return data


def frame_reason(f):
    if f.get('complete') is not True or any(type(f.get(k)) is not int or f.get(k) != 0 for k in ['capture_fault', 'missed_frames', 'deadline_misses']):
        return 'acquisition_fault_or_missing_status'
    if type(f.get('quality')) is not int or not 0 <= f['quality'] <= 65535 or f['quality'] & 19:
        return 'frame_quality'
    if f.get('truncated') is not False:
        return 'truncated_or_missing_coverage'
    if len(f['detections']) != 1:
        return 'requires_single_unambiguous_detection'
    d = f['detections'][0]
    xyz = d.get('sensor_xyz_m')
    if not isinstance(xyz, list) or len(xyz) != 3 or not all(finite(v) for v in xyz):
        return 'missing_spatial_measurement'
    if type(d.get('spatial_flags')) is not int or not 0 <= d['spatial_flags'] <= 65535 or d['spatial_flags'] & 3 != 3:
        return 'invalid_spatial_flags'
    if not finite(d.get('range_m')) or not 0 <= d['range_m'] <= 1000 or not finite(d.get('velocity_mps')):
        return 'invalid_measurement'
    if xyz[1] < 0 or abs(math.dist(xyz, [0, 0, 0]) - d['range_m']) > .1:
        return 'inconsistent_sensor_coordinates'
    return None


def features(frames):
    ds = [f['detections'][0] for f in frames]
    ranges = [d['range_m'] for d in ds]
    radial = [d['velocity_mps'] for d in ds]
    z = [d['sensor_xyz_m'][2] for d in ds]
    n = max(1, len(z) // 3)
    return [statistics.mean(ranges), statistics.pstdev(ranges),
            statistics.mean(map(abs, radial)), statistics.pstdev(radial),
            statistics.mean(z), statistics.mean(z[-n:])-statistics.mean(z[:n]), 1.0]


def prepare(data):
    validate(data)
    rows = []
    label_index = {}
    for label in data['labels']:
        label_index.setdefault((label['segment_id'],label['task']),[]).append(label)
    require(sum(len(label_index.get((s['id'],t),[]))*((s['frames'][-1]['relative_us']-s['frames'][0]['relative_us'])//WINDOW_US+2) for s in data['segments'] for t in LABELS) <= 2000000, 'Annotation/window work limit exceeded')
    for seg in data['segments']:
        fs = seg['frames']
        origin = fs[0]['relative_us'] + 250000
        require((fs[-1]['relative_us']-origin)//WINDOW_US <= 10000, 'Segment too long')
        buckets = {}
        for f in fs:
            buckets.setdefault(max(-1,(f['relative_us']-origin)//WINDOW_US), []).append(f)
        # Continuity belongs to the segment, including pairs across bucket boundaries.
        # Reject every window touched by a gap, including empty windows it spans.
        gap_windows = set()
        for left, right in zip(fs, fs[1:]):
            if right['relative_us']-left['relative_us'] > 500000 or right['sequence'] != left['sequence']+1:
                first = max(-1,(left['relative_us']-origin)//WINDOW_US)
                last = max(-1,(right['relative_us']-origin)//WINDOW_US)
                gap_windows.update(range(first,last+1))
        for i in range(-1,max(0,int((fs[-1]['relative_us']-origin)//WINDOW_US)+1)):
            a, b = (fs[0]['relative_us'],origin) if i==-1 else (origin+i*WINDOW_US,origin+(i+1)*WINDOW_US)
            frames = buckets.get(i, [])
            reasons = set(filter(None, (frame_reason(f) for f in frames)))
            if i==-1:
                reasons.add('boundary_context_only')
            if len(frames) < 12:
                reasons.add('insufficient_frames')
            if not frames or frames[0]['relative_us']-a > 500000 or b-frames[-1]['relative_us'] > 500000 or fs[-1]['relative_us'] < b:
                reasons.add('incomplete_window')
            if i in gap_windows:
                reasons.add('frame_gap')
            vector = None if reasons else features(frames)
            for task in LABELS:
                touching = [l for l in label_index.get((seg['id'],task),[]) if l['start_us'] < b and l['end_us'] > a]
                # A reference label must cover the complete window, including event none.
                admitted = [l for l in touching if l['start_us'] + (250 if l.get('timing_uncertainty_ms') is None else l['timing_uncertainty_ms'])*1000 <= a and l['end_us'] - (250 if l.get('timing_uncertainty_ms') is None else l['timing_uncertainty_ms'])*1000 >= b]
                if task=='transition':
                    admitted = [l for l in touching if (l['label']=='none' and l in admitted) or (l['label'] in ['stand_up','sit_down','motion_onset'] and l['start_us']-(250 if l.get('timing_uncertainty_ms') is None else l['timing_uncertainty_ms'])*1000 >= a and l['end_us']+(250 if l.get('timing_uncertainty_ms') is None else l['timing_uncertainty_ms'])*1000 <= b)]
                label = admitted[0] if len(touching)==1 and len(admitted)==1 else None
                approved = bool(label and label['review_status']=='reviewed' and label['source']=='human' and label['label'] in LABELS[task])
                rows.append({'segment_id':seg['id'], 'start_us':a,'end_us':b,'task':task,
                             'features':vector,'rejection_reasons':sorted(reasons),
                             'label':label['label'] if approved else None,
                             'annotation_id':label['id'] if label else None,
                             'label_status':'reviewed' if approved else 'unknown_unreviewed_excluded_or_conflicting'})
    return {'schema':'original-radar-features','version':VERSION,'feature_names':FEATURE_NAMES,
            'window_us':WINDOW_US,'dataset_sha256':digest(data),
            'embedded_export_hash_verified':False,
            'integrity_note':'Producer digest and claimed original-capture source hash are not verified; preserve originals. This tool freezes its own canonical full-input hash.',
            'physical_sha256':digest(sorted(digest([physical_frame(f) for f in s['frames']]) for s in data['segments'])),
            'frame_fingerprints':sorted(set(digest({k:v for k,v in physical_frame(f).items() if k not in ['relative_us','sequence']}) for s in data['segments'] for f in s['frames'] if f['detections'])),
            'source_sha256':data['source']['sha256'],'session':data['session'],
            'source_metadata':data['source'],'sync_markers':data.get('sync_markers',[]),
            'source_diagnostics':data.get('diagnostics',{}),'source_provenance':data.get('provenance',{}),
            'records':rows}



def physical_frame(f):
    # Exclude arbitrary metadata. Preserve sensor-relative time/sequence for overlap checks.
    return {k:f.get(k) for k in ['relative_us','sequence','quality','complete','capture_fault','missed_frames','deadline_misses','truncated']} | {'detections':[{k:d.get(k) for k in ['range_m','velocity_mps','sensor_xyz_m','spatial_flags']} for d in f['detections']]}

def load_many(paths):
    require(0 < len(paths) <= 100, 'Input batch needs 1..100 files')
    require(sum(Path(p).stat().st_size for p in paths) <= MAX_BYTES, 'Input batch exceeds 96 MiB')
    prepared = []
    total = 0
    for path in paths:
        item = prepare(read(path))
        item['input_file_sha256'] = hashlib.sha256(Path(path).read_bytes()).hexdigest()
        total += len(item['records'])
        require(total <= 20000, 'Input batch exceeds 20000 task rows')
        prepared.append(item)
    for key in ['dataset_sha256', 'physical_sha256', 'source_sha256']:
        require(len({p[key] for p in prepared})==len(prepared), 'Duplicate capture: '+key)
    require(len({p['session']['id'] for p in prepared})==len(prepared), 'Repeated session id: combine segments in one export')
    return sorted(prepared, key=lambda p:p['session']['id'])


def split_guard(data, split):
    require(isinstance(split,dict), 'Split must be an object')
    require(set(split)=={'train','heldout'}, 'Split needs train and heldout lists')
    require(all(isinstance(split[k],list) and split[k] and all(text(x) for x in split[k]) for k in split), 'Invalid split lists')
    ids = split['train']+split['heldout']
    require(len(set(ids))==len(ids), 'Session leakage or duplicate split id')
    for key in ['dataset_sha256','physical_sha256','source_sha256']:
        require(len({p[key] for p in data})==len(data), 'Duplicate capture: '+key)
    require(len({p['session']['id'] for p in data})==len(data), 'Duplicate session id')
    seen_frames = set()
    for p in data:
        fps = set(p['frame_fingerprints'])
        require(not seen_frames & fps, 'Shared physical frame content; possible same-take subset or duplicate')
        seen_frames.update(fps)
    sessions = {p['session']['id']:p for p in data}
    require(set(ids)==set(sessions), 'Split must assign every supplied session exactly once')
    groups = {}
    for side in ['train','heldout']:
        groups[side] = {sessions[x]['session'].get('participant_group') for x in split[side]}
        require(None not in groups[side] and all(text(x) for x in groups[side]), 'Known pseudonymous participant groups required')
    require(not groups['train'] & groups['heldout'], 'Participant leakage across split')
    require(len({p['session']['config_identity'] for p in data})==1, 'Incompatible configuration identities')
    require(len({p['session']['synthetic'] for p in data})==1, 'Cannot mix synthetic and real sessions')
    return sessions


def distance(a,b):
    return math.sqrt(statistics.mean((x-y)**2 for x,y in zip(a,b)))


def avg(vectors):
    return [statistics.mean(col) for col in zip(*vectors)]


def train(data, split, task):
    require(task in LABELS, 'Unknown task')
    sessions = split_guard(data,split)
    rows = [(sid,r) for sid in sorted(split['train']) for r in sessions[sid]['records']
            if r['task']==task and r['features'] is not None and r['label'] in LABELS[task]]
    for label in LABELS[task]:
        require(len({sid for sid,r in rows if r['label']==label})>=2, 'Each class needs two training sessions: '+label)
    # Equal session contribution: scale based on session means/second moments.
    session_vectors = [[r['features'] for sid,r in rows if sid==s] for s in sorted(split['train'])]
    session_vectors = [vs for vs in session_vectors if vs]
    mean = avg([avg(vs) for vs in session_vectors])
    second = avg([avg([[v*v for v in row] for row in vs]) for vs in session_vectors])
    scale = [max(1e-6,math.sqrt(max(0,s-m*m))) for s,m in zip(second,mean)]
    standardized = [(sid,r['label'],[(x-m)/s for x,m,s in zip(r['features'],mean,scale)]) for sid,r in rows]
    centroids = {}
    for label in LABELS[task]:
        groups = {sid for sid,l,v in standardized if l==label}
        centroids[label] = avg([avg([v for sid,l,v in standardized if sid==g and l==label]) for g in sorted(groups)])
    radii = {}
    for label in LABELS[task]:
        within = [distance(v,centroids[label]) for sid,l,v in standardized if l==label]
        separation = min(distance(centroids[label],c) for l,c in centroids.items() if l!=label)
        require(separation > 1e-6, 'Indistinguishable training centroids')
        radii[label] = max(1e-6,min(max(.35,max(within)*1.25),.45*separation))
    return {'schema':'original-radar-baseline','version':VERSION,'algorithm':'session-balanced-centroid-v1',
            'task':task,'feature_names':FEATURE_NAMES,'window_us':WINDOW_US,
            'mean':mean,'scale':scale,'centroids':centroids,'radii':radii,
            'split':{k:sorted(v) for k,v in split.items()},
            'dataset_hashes':{sid:p['dataset_sha256'] for sid,p in sessions.items()},
            'synthetic_only':data[0]['session']['synthetic'],
            'warning':'Experimental review proposals only. No demonstrated recognition or physical accuracy.'}


def predict(model,row):
    if row['features'] is None:
        return 'unknown'
    v = [(x-m)/s for x,m,s in zip(row['features'],model['mean'],model['scale'])]
    ranked = sorted((distance(v,c),l) for l,c in model['centroids'].items())
    d,label = ranked[0]
    return label if d <= model['radii'][label] and ranked[1][0]-d >= .1 else 'unknown'


def evaluate(model,data):
    require(isinstance(model,dict), 'Model must be an object')
    # Refit deterministically and compare all parameters before trusting model input.
    expected = train(data,model.get('split',{}),model.get('task'))
    require(canonical(model)==canonical(expected), 'Model/data mismatch; supplied model must match frozen reproducible training')
    labels = LABELS[model['task']]
    matrix = {l:{p:0 for p in labels+['unknown']} for l in labels}
    counts = {'total':0,'reference_labeled':0,'unlabeled':0,'quality_rejected':0,'predicted':0,'correct':0}
    reasons = {}
    for p in data:
        if p['session']['id'] not in model['split']['heldout']:
            continue
        for r in p['records']:
            if r['task'] != model['task']:
                continue
            counts['total'] += 1
            if r['features'] is None:
                counts['quality_rejected'] += 1
            for reason in r['rejection_reasons']:
                reasons[reason] = reasons.get(reason,0)+1
            prediction = predict(model,r)
            if r['label'] is None:
                counts['unlabeled'] += 1
                continue
            counts['reference_labeled'] += 1
            matrix[r['label']][prediction] += 1
            counts['predicted'] += prediction!='unknown'
            counts['correct'] += prediction==r['label']
    n = counts['reference_labeled']
    return {'schema':'original-radar-evaluation','version':VERSION,'synthetic_only':model['synthetic_only'],
            'model_sha256':digest(model),'counts':counts,'confusion_matrix':matrix,'rejection_reasons':reasons,
            'accuracy_including_unknown':counts['correct']/n if n else None,
            'conditional_accuracy':counts['correct']/counts['predicted'] if counts['predicted'] else None,
            'labeled_prediction_coverage':counts['predicted']/n if n else None,
            'missing_heldout_classes':[l for l in labels if not sum(matrix[l].values())],
            'warning':'Synthetic tests prove software behavior only; no real-world accuracy established.' if model['synthetic_only'] else model['warning']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command',choices=['validate','prepare','train','evaluate'])
    parser.add_argument('inputs',nargs='+')
    parser.add_argument('--out')
    parser.add_argument('--split')
    parser.add_argument('--model')
    parser.add_argument('--task',choices=list(LABELS),default='activity')
    args=parser.parse_args()
    try:
        data=load_many(args.inputs)
        if args.command=='validate':
            result={'structurally_valid':True,'embedded_export_hash_verified':False,'claimed_original_capture_hash_verified':False,'sessions':len(data),'records':sum(len(p['records']) for p in data)}
        elif args.command=='prepare':
            result={'schema':'original-radar-prepared-bundle','version':VERSION,'datasets':data}
        elif args.command=='train':
            require(args.split is not None,'--split required')
            result=train(data,read(args.split),args.task)
        else:
            require(args.model is not None,'--model required')
            result=evaluate(read(args.model),data)
        if args.out:
            write(args.out,result)
        else:
            require(args.command=='validate','--out required (new path only)')
            print(json.dumps(result))
    except (ValueError,KeyError,TypeError,OSError) as exc:
        parser.exit(2, 'Error: '+str(exc)+'\n')


if __name__=='__main__':
    main()
