"""Every measurement, participant and label in these tests is SYNTHETIC."""
import copy
import sys
import tempfile
import subprocess
import json
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import radar_ml as m


def fixture(sid, participant, delta=0):
    frames=[]
    for i in range(187):
        block=max(0,min(2,(i-5)//60))
        z=[.3,.8,1.2][block]+delta
        xyz=[0,2,z]
        frames.append({'index':i,'relative_us':i*50000,'sequence':i,'quality':0,
                       'complete':True,'capture_fault':0,'missed_frames':0,
                       'deadline_misses':0,'truncated':False,'detections':[
                           {'range_m':sum(x*x for x in xyz)**.5,'velocity_mps':[0,.1,.8][block]+delta,
                            'power':5,'sensor_xyz_m':xyz,'spatial_flags':3}]})
    labels=[]
    for i,l in enumerate(m.LABELS['activity']):
        labels.append({'id':'a'+str(i),'segment_id':'seg','start_us':250000+i*m.WINDOW_US,
                       'end_us':250000+(i+1)*m.WINDOW_US,'task':'activity','label':l,
                       'review_status':'reviewed','reviewer':'synthetic-test',
                       'reviewed_at':'2026-10-11T00:00:00Z','source':'human','timing_uncertainty_ms':0})
    return {'schema':'original-radar-dataset','version':1,
            'source':{'protocol':'original-radar-v2','sha256':m.digest(sid),'coordinate_frame':'sensor_xyz_m','timing_basis':'sensor_relative_us'},
            'session':{'id':sid,'participant_group':participant,'synthetic':True,'config_identity':'SYNTHETIC CONFIG'},
            'segments':[{'id':'seg','session_id':42,'frames':frames}],'labels':labels,'sync_markers':[]}


def dataset():
    return [m.prepare(fixture('train-a','person-a',0)),m.prepare(fixture('train-b','person-a',.01)),m.prepare(fixture('test-c','person-b',.005))]


class Tests(unittest.TestCase):
    split={'train':['train-a','train-b'],'heldout':['test-c']}
    def test_train_evaluate_reproducible(self):
        d=dataset(); a=m.train(d,self.split,'activity'); b=m.train(list(reversed(d)),self.split,'activity')
        self.assertEqual(m.canonical(a),m.canonical(b))
        result=m.evaluate(a,d)
        self.assertTrue(result['synthetic_only']); self.assertEqual(result['counts']['reference_labeled'],3)
        self.assertEqual(result['counts']['unlabeled'],2)
    def test_participant_leakage(self):
        d=dataset(); d[-1]['session']['participant_group']='person-a'
        with self.assertRaisesRegex(ValueError,'Participant leakage'): m.train(d,self.split,'activity')
    def test_session_leakage(self):
        with self.assertRaises(ValueError): m.train(dataset(),{'train':['train-a','train-b'],'heldout':['train-a']},'activity')
    def test_five_second_default_uncertainty_is_usable(self):
        d=fixture('a','b');d['segments'][0]['frames']=d['segments'][0]['frames'][:101]
        d['labels']=[dict(d['labels'][0],start_us=0,end_us=5000000,timing_uncertainty_ms=None)]
        rows=m.prepare(d)['records']
        self.assertEqual(rows[2]['label'],'sitting');self.assertIsNotNone(rows[2]['features'])
    def test_short_transition_contained(self):
        d=fixture('a','b');d['labels']=[dict(d['labels'][0],task='transition',label='stand_up',start_us=1000000,end_us=1500000,timing_uncertainty_ms=None)]
        self.assertEqual(m.prepare(d)['records'][3]['label'],'stand_up')
    def test_aggregate_sparse_expansion_rejected(self):
        d=fixture('a','b');d['labels']=[];d['segments'][0]['frames'][-1]['relative_us']=40000000000
        with self.assertRaisesRegex(ValueError,'Aggregate window'):m.prepare(d)
    def test_malformed_nested_rejected(self):
        for field in ['segments','labels']:
            d=fixture('a','b');d[field]=[None]
            with self.assertRaises(ValueError):m.prepare(d)
    def test_shared_subset_rejected(self):
        d=dataset();d[-1]['frame_fingerprints'].append(d[0]['frame_fingerprints'][0])
        with self.assertRaisesRegex(ValueError,'Shared physical'):m.train(d,self.split,'activity')
    def test_bad_counter_types_reject_features(self):
        for value in [False,0.0,-1]:
            d=fixture('a','b');d['segments'][0]['frames'][6]['capture_fault']=value
            self.assertIsNone(m.prepare(d)['records'][2]['features'])
    def test_bad_review_timestamp_rejected(self):
        d=fixture('a','b');d['labels'][0]['reviewed_at']='yesterday'
        with self.assertRaises(ValueError):m.prepare(d)
    def test_wrong_model_and_split_types_rejected(self):
        with self.assertRaises(ValueError):m.evaluate([],dataset())
        with self.assertRaises(ValueError):m.train(dataset(),[],'activity')
    def test_transition_training_all_classes(self):
        prepared=[]
        for sid,person,delta in [('a','p1',0),('b','p1',.01),('c','p2',.005)]:
            d=fixture(sid,person,delta)
            template=copy.deepcopy(d['segments'][0]['frames'][-1])
            for i in range(187,247):
                f=copy.deepcopy(template);f.update(index=i,sequence=i,relative_us=i*50000)
                if i>=185:
                    xyz=[0,2,1.8+delta];f['detections'][0].update(sensor_xyz_m=xyz,range_m=sum(x*x for x in xyz)**.5,velocity_mps=1.5+delta)
                d['segments'][0]['frames'].append(f)
            d['labels']=[dict(d['labels'][0],id=str(i),task='transition',label=label,start_us=250000+i*m.WINDOW_US,end_us=250000+(i+1)*m.WINDOW_US) for i,label in enumerate(m.LABELS['transition'])]
            prepared.append(m.prepare(d))
        model=m.train(prepared,{'train':['a','b'],'heldout':['c']},'transition')
        report=m.evaluate(model,prepared)
        self.assertEqual(report['missing_heldout_classes'],[])
    def test_cli_end_to_end_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);paths=[]
            for sid,person,delta in [('train-a','p1',0),('train-b','p1',.01),('test-c','p2',.005)]:
                p=root/(sid+'.json');p.write_text(json.dumps(fixture(sid,person,delta)));paths.append(str(p))
            split=root/'split.json';split.write_text(json.dumps(self.split))
            script=str(Path(m.__file__));model=root/'model.json';report=root/'report.json'
            cmd=[sys.executable,script,'train',*paths,'--split',str(split),'--out',str(model)]
            self.assertEqual(subprocess.run(cmd,capture_output=True).returncode,0)
            self.assertEqual(subprocess.run(cmd,capture_output=True).returncode,2)
            result=subprocess.run([sys.executable,script,'evaluate',*paths,'--model',str(model),'--out',str(report)],capture_output=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertTrue(json.loads(report.read_text())['synthetic_only'])
    def test_unknown_participant(self):
        d=dataset();d[-1]['session']['participant_group']=None
        with self.assertRaises(ValueError):m.train(d,self.split,'activity')
    def test_unreviewed_not_truth(self):
        d=fixture('a','b'); d['labels'][0]['review_status']='unreviewed'
        self.assertIsNone(m.prepare(d)['records'][2]['label'])
    def test_machine_cannot_claim_review(self):
        d=fixture('a','b'); d['labels'][0]['source']='machine'
        with self.assertRaises(ValueError):m.prepare(d)
    def test_conflicting_annotations(self):
        d=fixture('a','b');d['labels'].append(dict(d['labels'][0],id='conflict',label='walking'))
        self.assertIsNone(m.prepare(d)['records'][2]['label'])
    def test_missing_coordinate_rejected_retained(self):
        d=fixture('a','b');d['segments'][0]['frames'][6]['detections'][0]['sensor_xyz_m']=None
        r=m.prepare(d)['records'][2];self.assertIsNone(r['features']);self.assertEqual(r['label'],'sitting')
    def test_gap_rejected(self):
        d=fixture('a','b');d['segments'][0]['frames'][7]['sequence']=20
        self.assertIn('frame_gap',m.prepare(d)['records'][2]['rejection_reasons'])
    def test_two_detections_not_person_identity(self):
        d=fixture('a','b');f=d['segments'][0]['frames'][6];f['detections']*=2
        self.assertIsNone(m.prepare(d)['records'][2]['features'])
    def test_end_window_rejected(self):
        self.assertIn('incomplete_window',m.prepare(fixture('a','b'))['records'][-1]['rejection_reasons'])
    def test_holdout_does_not_fit_parameters(self):
        d=dataset();a=m.train(d,self.split,'activity');d[-1]['records'][2]['features']=[999]*7;b=m.train(d,self.split,'activity')
        for k in ['mean','scale','centroids','radii']:self.assertEqual(a[k],b[k])
    def test_model_tampering_rejected(self):
        d=dataset();model=m.train(d,self.split,'activity');model['mean'][0]+=1
        with self.assertRaises(ValueError):m.evaluate(model,d)
    def test_heldout_source_changes_rejected(self):
        d=dataset();model=m.train(d,self.split,'activity');d[-1]['dataset_sha256']='0'*64
        with self.assertRaises(ValueError):m.evaluate(model,d)
    def test_configuration_mismatch(self):
        d=dataset();d[-1]['session']['config_identity']='other'
        with self.assertRaises(ValueError):m.train(d,self.split,'activity')
    def test_nan_and_boolean_time(self):
        for invalid in [True,float('nan')]:
            d=fixture('a','b');d['segments'][0]['frames'][6]['relative_us']=invalid
            with self.assertRaises(ValueError):m.prepare(d)
    def test_excluded_and_unknown_not_truth(self):
        for status,label in [('excluded','sitting'),('reviewed','unknown')]:
            d=fixture('a','b');d['labels'][0].update(review_status=status,label=label)
            self.assertIsNone(m.prepare(d)['records'][2]['label'])
    def test_uncertain_boundary_not_truth(self):
        d=fixture('a','b');d['labels'][0]['timing_uncertainty_ms']=100
        self.assertIsNone(m.prepare(d)['records'][2]['label'])


if __name__=='__main__':unittest.main()
