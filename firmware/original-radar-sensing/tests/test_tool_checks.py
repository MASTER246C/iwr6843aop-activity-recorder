"""Release-tool checks must fail closed under python -O / PYTHONOPTIMIZE (no assert here either).
Synthetic ELF/map/image bytes only; no TI tool, board or real artifact is used.
"""
from pathlib import Path
import os,re,struct,subprocess,sys,zlib
root=Path(__file__).resolve().parents[1]
dest=Path(os.environ.get('HOST_TEST_OUTPUT') or root/'build/host-tests')/'tool_checks';dest.mkdir(parents=True,exist_ok=True)
REQUIRED=('main','or_radio_prepare','or_rf_physical','or_acquisition_arm','or_profile_energy','radar_dsp_process',
          'os_estimate','or_frame_json','MMWave_start','EDMA_configChannel','ADCBuf_open','UART_write','SOC_init')
def elf(magic=b'\x7fELF',capture_size=65536):
    names=b'\0.vecs\0.capture_frames\0.shstrtab\0'
    sections=[(0,0,0,0,0,0),(1,1,6,0,52,64),(7,8,3,0x51000000,0,capture_size),(23,3,0,0,116,len(names))]
    shoff=116+len(names)
    data=magic+bytes([1,1,1])+bytes(9)+struct.pack('<HHIIIIIHHHHHH',2,40,1,0x100,0,shoff,0,52,0,0,40,len(sections),3)
    data+=bytes(64)+names
    for name,kind,flags,address,offset,size in sections:
        data+=struct.pack('<IIIIIIIIII',name,kind,flags,address,offset,size,0,0,4,0)
    return data
def check(condition,detail):
    if not condition:sys.exit(f'FAIL: {detail!r}')
def verify(label,elf_bytes,map_text):
    case=dest/label;case.mkdir(exist_ok=True)
    (case/'image.xer4f').write_bytes(elf_bytes);(case/'image.map').write_text(map_text)
    return subprocess.run([sys.executable,'-O',str(root/'tools/verify_sensing.py'),str(case/'image.xer4f'),str(case/'image.map')],
                          capture_output=True,text=True,env={**os.environ,'PYTHONOPTIMIZE':'1'})
good_map='\n'.join(REQUIRED)+'\n'
ok=verify('good',elf(),good_map)
check(ok.returncode==0 and '"offline_checks_passed"' in ok.stdout,ok)
for label,elf_bytes,map_text,reason in [
        ('bad_magic',elf(magic=b'\x7fBAD'),good_map,'Not an ELF file'),
        ('bad_capture',elf(capture_size=4096),good_map,'Expected two full L3 ADC frames'),
        ('forbidden',elf(),good_map+'MmwDemo_dataPathTask\n','Unexpected demo/pipeline dependency'),
        ('missing',elf(),good_map.replace('MMWave_start\n',''),'Missing expected app/driver symbol: MMWave_start')]:
    bad=verify(label,elf_bytes,map_text)
    check(bad.returncode!=0 and reason in bad.stderr and 'offline_checks_passed' not in bad.stdout,(label,bad))
    check(not (dest/label/'elf-verification.json').exists(),label)
script=(root/'tools/package_firmware.sh').read_text()
checker=re.search(r"<<'PY'\n(.*?)\nPY\n",script,re.S).group(1)
def crc(label,data):
    path=dest/(label+'.bin');path.write_bytes(data)
    return subprocess.run([sys.executable,'-O','-',str(path)],input=checker,capture_output=True,text=True,
                          env={**os.environ,'PYTHONOPTIMIZE':'1'})
body=bytes(range(256))*4;image=body+struct.pack('<I',zlib.crc32(body)&0xffffffff)
check(crc('crc_good',image).returncode==0,'good CRC rejected')
bad=crc('crc_bad',image[:-1]+bytes([image[-1]^1]))
check(bad.returncode!=0 and 'Final CRC mismatch' in bad.stderr and 'passed' not in bad.stdout,bad)
bad=crc('crc_empty',b'\0\0\0\0')
check(bad.returncode!=0 and 'Empty image' in bad.stderr,bad)
print('PASS: verify_sensing.py and the package CRC check fail closed under python -O on synthetic bad inputs.')
