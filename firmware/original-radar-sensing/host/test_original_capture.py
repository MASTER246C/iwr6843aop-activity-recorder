import copy
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
import types
from unittest import mock

spec=importlib.util.spec_from_file_location('original_capture',Path(__file__).with_name('original_capture.py'))
m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)

def frame(seq=0):
    return {'protocol':m.PROTOCOL,'type':'frame','sequence':seq,'timestamp_us':seq*200000,
        'complete':True,'missed_frames':0,'capture_fault':0,'quality':44,'processing_us':200,
        'previous_tx_us':500,'deadline_misses':0,'mount':{'bracket_top_m':2.0,'down_tilt_deg':15.0,
        'antenna_offset_m':None,'antenna_height_m':None,'world_height_valid':False},
        'range_bin_m':0.195,'velocity_bin_mps':0.3,'detected_count':1,'count':1,'truncated':False,
        'detections':[{'range_m':2.3,'velocity_mps':0.0,'power':100.0,
        'phasors':[[1,0],[1,0],[1,0],[1,0]],'sensor_xyz_m':[0,2.3,0],
        'world_xyz_m':None,'spatial_flags':3,'activity':None}]}

def line(obj): return json.dumps(obj).encode()+b'\n'

class FakeSerial:
    def __init__(self,responses): self.responses=list(responses);self.writes=[]
    def write(self,value): self.writes.append(value);return len(value)
    def flush(self): pass
    def read(self,n): return self.responses.pop(0) if self.responses else b''

class CollectorTest(unittest.TestCase):
    def test_valid_and_unknown_height(self):
        x=m.decode_record(line(frame()));self.assertIsNone(x['mount']['antenna_height_m'])
        self.assertIsNone(x['detections'][0]['world_xyz_m'])
    def test_refuses_ti_and_nonfinite(self):
        for raw in (b'\x02\x01\x04\x03', b'{"type":"frame"}',b'{"protocol":"original-radar-v1","type":"frame","sequence":NaN}'):
            with self.assertRaises(m.CaptureError):m.decode_record(raw)
    def test_additive_field_exponents_and_deep_json(self):
        for raw in (b'{"protocol":"original-radar-v1","type":"status","extra":1e999}',
                    b'{"protocol":"original-radar-v1","type":"status","extra":'+b'['*1200+b'0'+b']'*1200+b'}'):
            with self.assertRaises(m.CaptureError):m.decode_record(raw)

    def test_optional_profile_energy(self):
        f=frame();f['profile_energy']=[100,60,40]
        self.assertEqual(m.decode_record(line(f))['profile_energy'],[100,60,40])
        for value in (None,[],[1,2],[1,2,3,4],[1,-1,2],[True,1,2]):
            f['profile_energy']=value
            with self.subTest(value=value),self.assertRaises(m.CaptureError):m.decode_record(line(f))

    def test_inconsistent_geometry_and_counts(self):
        for mutation in ('height','world','count','truncated','incomplete','activity','boolseq','huge'):
            f=frame()
            if mutation=='height':f['mount']['antenna_height_m']=2
            elif mutation=='world':f['detections'][0]['world_xyz_m']=[0,1,2]
            elif mutation=='count':f['count']=2
            elif mutation=='truncated':f['truncated']=True
            elif mutation=='incomplete':f['complete']=False
            elif mutation=='activity':f['detections'][0]['activity']='standing'
            elif mutation=='boolseq':f['sequence']=True
            elif mutation=='huge':f['detections'][0]['power']=10**400
            with self.subTest(mutation=mutation),self.assertRaises(m.CaptureError):m.decode_record(line(f))
    def test_every_chunk_split_and_overlong_recovery(self):
        raw=line(frame())+line(frame(1))
        for cut in range(len(raw)+1):
            f=m.Framer();records=f.feed(raw[:cut])+f.feed(raw[cut:]);self.assertEqual(records,[line(frame()),line(frame(1))])
        f=m.Framer();self.assertEqual(f.feed(b'x'*(m.MAX_LINE+20)+b'\n'+line(frame())),[line(frame())]);self.assertEqual(f.oversized,1)
    def test_configuration_cannot_start_rf(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'x.cfg';p.write_text('# test\norConfig 1\nprofile room64_v1\ncommit\n')
            self.assertEqual(m.load_config(p),['orConfig 1','profile room64_v1','commit'])
            for dangerous in ('start','stop','sensorStart','x\x00y'):
                p.write_text('orConfig 1\n'+dangerous+'\ncommit\n')
                with self.assertRaises(m.CaptureError):m.load_config(p)
    def test_replay_no_serial_and_exclusive_output(self):
        with tempfile.TemporaryDirectory() as td:
            output=Path(td)/'out.jsonl';r=m.Recorder(output,'sitting','replay')
            stream=io.BytesIO(line(frame(0xffffffff))+line(frame(0))+line(frame(3))+b'bad\n')
            f=m.replay(stream,r);r.finish('test',f.oversized,len(f.buffer))
            rows=[json.loads(x) for x in output.read_text().splitlines()]
            self.assertEqual(r.frames,3);self.assertEqual(r.sequence_gaps,2);self.assertEqual(r.invalid,1)
            self.assertEqual(rows[0]['source_kind'],'replay');self.assertFalse(rows[0]['classifier'])
            with self.assertRaises(FileExistsError):m.Recorder(output,'unknown','replay')
    def test_correlated_ack_and_rejected_command(self):
        def ack(command,code=0):return {'protocol':m.PROTOCOL,'type':'ack','command':command,'code':code,'state':'idle','configured':True}
        with tempfile.TemporaryDirectory() as td:
            r=m.Recorder(Path(td)/'x.jsonl','unknown','fake_serial')
            s=FakeSerial([line(ack('commit')),line(ack('status'))])
            got=m.command(s,'status',m.Framer(),r,timeout=0.1)
            self.assertEqual(got['command'],'status');self.assertEqual(s.writes,[b'status\n'])
            s=FakeSerial([line(ack('start',-3))])
            with self.assertRaises(m.CaptureError):m.command(s,'start',m.Framer(),r,timeout=0.1)
            r.finish('test')
    def test_main_passive_never_writes_and_explicit_start_stops(self):
        class Port(FakeSerial):
            instances=[]
            def __init__(self, **kwargs):
                super().__init__([]); self.is_open=False; self.dtr=True; self.rts=True
                self.__class__.instances.append(self)
            def open(self): self.is_open=True
            def close(self): self.is_open=False
            def write(self, value):
                self.writes.append(value); token=value.decode().strip().split()[0]
                state='running' if token=='start' else 'idle'
                self.responses.append(line({'protocol':m.PROTOCOL,'type':'ack','command':token,
                                            'code':0,'state':state,'configured':True}))
                return len(value)
        with tempfile.TemporaryDirectory() as td, mock.patch.dict(m.sys.modules, {'serial':types.SimpleNamespace(Serial=Port)}):
            with mock.patch('builtins.print'):
                self.assertEqual(m.main(['--port','FAKE','--output',str(Path(td)/'passive.jsonl'),'--duration','0.001']),0)
                self.assertEqual(Port.instances[-1].writes,[])
                self.assertFalse(Port.instances[-1].dtr);self.assertFalse(Port.instances[-1].rts)
                self.assertEqual(m.main(['--port','FAKE','--output',str(Path(td)/'start.jsonl'),'--duration','0.001','--start']),0)
                self.assertEqual(Port.instances[-1].writes,[b'status\n',b'start\n',b'stop\n'])
                self.assertFalse(Port.instances[-1].is_open)
                rows=[json.loads(x) for x in (Path(td)/'start.jsonl').read_text().splitlines()]
                self.assertEqual(rows[-1]['reason'],'complete')

    def run_with_stop_failure(self, failure, capture_error=None):
        class Port(FakeSerial):
            def __init__(self):
                super().__init__([]);self.is_open=False
            def open(self):self.is_open=True
            def close(self):self.is_open=False
            def write(self, value):
                self.writes.append(value);token=value.decode().strip()
                if token=='stop':
                    if failure=='disconnect':raise OSError('simulated disconnect during stop')
                    if failure=='timeout':return len(value)
                self.responses.append(line({'protocol':m.PROTOCOL,'type':'ack','command':token,
                    'code':-3 if token=='stop' and failure=='rejected' else 0,
                    'state':'running' if token=='start' else 'idle','configured':True}))
                return len(value)
            def read(self,n):
                if self.responses:return self.responses.pop(0)
                if capture_error is not None:raise capture_error
                if failure=='closed':self.is_open=False
                return b''
        port=Port();original_command=m.command
        # Exercise the real acknowledgement timeout without a 15-second test delay.
        def bounded_command(stream, command, framer, recorder):
            return original_command(stream,command,framer,recorder,timeout=0.002)
        with tempfile.TemporaryDirectory() as td:
            output=Path(td)/'out.jsonl'
            with mock.patch.dict(m.sys.modules,{'serial':types.SimpleNamespace(Serial=lambda **kwargs:port)}), \
                 mock.patch.object(m,'command',side_effect=bounded_command),mock.patch('builtins.print'):
                result=m.main(['--port','FAKE','--output',str(output),'--duration','0.001','--start'])
            rows=[json.loads(x) for x in output.read_text().splitlines()]
        self.assertFalse(port.is_open)
        return result,rows,port.writes

    def test_unconfirmed_cleanup_stop_fails_otherwise_successful_capture(self):
        for failure in ('disconnect','rejected','timeout','closed'):
            with self.subTest(failure=failure):
                result,rows,writes=self.run_with_stop_failure(failure)
                self.assertEqual(result,1)
                self.assertEqual(rows[-1]['reason'],'stop_not_confirmed')
                self.assertEqual(sum(r['type']=='stop_not_confirmed' for r in rows),1)
                self.assertEqual(writes,[b'status\n',b'start\n']+([] if failure=='closed' else [b'stop\n']))

    def test_cleanup_stop_failure_preserves_existing_failure_or_interrupt(self):
        for error,expected_result,expected_reason in ((OSError('simulated capture failure'),1,'error: simulated capture failure'),
                                                      (KeyboardInterrupt(),130,'interrupted')):
            with self.subTest(error=type(error).__name__):
                result,rows,writes=self.run_with_stop_failure('disconnect',capture_error=error)
                self.assertEqual(result,expected_result)
                self.assertEqual(rows[-1]['reason'],expected_reason)
                self.assertEqual(sum(r['type']=='stop_not_confirmed' for r in rows),1)
                self.assertEqual(writes,[b'status\n',b'start\n',b'stop\n'])

    def test_timeout_does_not_send_followup(self):
        with tempfile.TemporaryDirectory() as td:
            r=m.Recorder(Path(td)/'x.jsonl','unknown','fake_serial');s=FakeSerial([])
            with self.assertRaises(m.CaptureError):m.command(s,'status',m.Framer(),r,timeout=0.002)
            self.assertEqual(s.writes,[b'status\n']);r.finish('test')

if __name__=='__main__':unittest.main()
