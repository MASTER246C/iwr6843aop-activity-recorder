import copy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
import camera_import as ci
import radar_ml as ml


class CameraImportTest(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.folder=Path(self.temp.name)
        self.sid='radar-camera-synthetic'
        self.run={'pair_nonce':'a'*32,'config_generation':1,'session_id':1}
        self.start={'schema':'radar-camera-session','version':1,'session_id':self.sid,'participant_group':'synthetic-a','synthetic':True}
        self.identity={'firmware_version':'0.3.2-rc1','build_id':'87589b2c32638ae119334c72'}
        self.status={'protocol':'original-radar-v2','type':'status','channel':'control','state':'running','paired':True,'configured':True,'selftest_ok':True,'selftest_version':2,'control_tx_fault':False,'data_tx_fault':False,**self.run,**self.identity,**{k:0 for k in ['fault','selftest_mask','capture_fault','rf_fault','rf_error','deadline_misses']}}
        self.frames=[{'protocol':'original-radar-v2','type':'frame','channel':'data',**self.run,'sequence':i,'timestamp_us':i*200000,'complete':True,'capture_fault':0,'missed_frames':0,'deadline_misses':0,'quality':44,'count':1,'detected_count':1,'truncated':False,'detections':[{'range_m':2,'velocity_mps':0,'sensor_xyz_m':[0,2,0],'spatial_flags':3}]} for i in range(61)]
        self.review={'schema':'radar-camera-review','version':1,'recording':{'session_id':self.sid,'participant_group':'synthetic-a'},'video':{'file':'camera-000000.webm','bytes':9,'sha256':hashlib.sha256(b'synthetic').hexdigest()},'alignment':{'status':'human-matched-events','method':'affine-fit-v1','radar_run':self.run,'anchors':[{'video_seconds':t,'sensor_timestamp_us':t*1000000,'uncertainty_ms':50} for t in [0,6,12]],'video_to_sensor_scale':1,'video_to_sensor_offset_us':0,'uncertainty_us':50000,'valid_video_start_seconds':0,'valid_video_end_seconds':12},'labels':[{'start_seconds':0,'end_seconds':12,'label':'standing','boundary_uncertainty_ms':100,'review_status':'reviewed','reviewed':True,'source':'human','reviewer':'synthetic-tester','reviewed_at':'2026-10-12T00:00:00Z'}]}
        self.save()

    def tearDown(self): self.temp.cleanup()

    def write_json(self,name,value): (self.folder/name).write_text(json.dumps(value),encoding='utf-8')

    def lane(self,kind,parts):
        total=0
        for i,content in enumerate(parts):
            name=f'{kind}-{i:06d}.'+('webm' if kind=='camera' else 'jsonl')
            (self.folder/name).write_bytes(content)
            self.write_json(name+'.meta.json',{'schema':'radar-camera-file','version':1,'file':name,'kind':kind,'session_id':self.sid,'complete_file':True,'byte_count':len(content),'sha256':hashlib.sha256(content).hexdigest(),'global_byte_start':total,'global_byte_end':total+len(content)})
            total+=len(content)
        return {'byte_count':total,'completed_files':len(parts)}

    def save(self):
        self.write_json('session-start.json',self.start)
        control=self.lane('radar-control',[(json.dumps(self.status)+'\n').encode()])
        probe={'protocol':'original-radar-v2','type':'data_probe','channel':'data',**self.run,**self.identity}
        data=('\n'.join(json.dumps(r) for r in [probe,*self.frames])+'\n').encode()
        lane=self.lane('radar-data',[data[:901],data[901:]]) # split inside a JSON record
        self.lane('camera',[b'synthetic'])
        self.write_json('session-end.json',{'schema':'radar-camera-session-end','session_id':self.sid,'complete':True,'lanes':{'data':lane,'control':control}})
        self.write_json('review.json',self.review)

    def convert(self): return ci.convert(self.folder,self.folder/'review.json')

    def test_exact_bytes_and_reviewed_windows(self):
        data=self.convert()
        self.assertTrue(data['session']['synthetic'])
        self.assertEqual(len(data['segments'][0]['frames']),61)
        rows=ml.prepare(data)['records']
        self.assertTrue(any(r['task']=='activity' and r['label']=='standing' and not r['rejection_reasons'] for r in rows))
        self.assertEqual(data['labels'][0]['timing_uncertainty_ms'],150)

    def test_machine_proposals_do_not_train(self):
        self.review['labels'][0].update(reviewed=False,review_status='unreviewed',source='machine')
        self.save()
        self.assertTrue(all(r['label'] is None for r in ml.prepare(self.convert())['records']))

    def test_measurement_exclusions_remain(self):
        for frame in self.frames: frame.update(truncated=True,detected_count=2)
        self.save()
        rows=ml.prepare(self.convert())['records']
        self.assertTrue(all(r['rejection_reasons'] for r in rows))
        self.assertTrue(any('truncated_or_missing_coverage' in r['rejection_reasons'] for r in rows))

    def test_wrong_raw_hash_fails(self):
        with (self.folder/'radar-data-000001.jsonl').open('ab') as out:out.write(b' ')
        with self.assertRaisesRegex(ValueError,'hash or byte'):self.convert()

    def test_unbound_wrong_clock_and_run_fail(self):
        cases=[lambda r:r.update(recording=None),lambda r:r['alignment'].update(video_to_sensor_offset_us=999),lambda r:r['alignment']['radar_run'].update(pair_nonce='b'*32)]
        for change in cases:
            original=copy.deepcopy(self.review)
            change(self.review);self.write_json('review.json',self.review)
            with self.assertRaises(ValueError):self.convert()
            self.review=original

    def test_incomplete_recording_fails(self):
        end=ci.load(self.folder/'session-end.json');end['complete']=False;self.write_json('session-end.json',end)
        with self.assertRaisesRegex(ValueError,'incomplete'):self.convert()

    def test_faulted_status_fails(self):
        self.status['selftest_mask']=1;self.save()
        with self.assertRaisesRegex(ValueError,'gate'):self.convert()


if __name__=='__main__':unittest.main()
