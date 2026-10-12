"""Run unmodified official TI Windows image tools; no ports or hardware access.

Outputs must be new files. The installed SDK supplies all image converters and
the RadarSS firmware. This script independently verifies the appended CRC32.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import subprocess
import tempfile
import zlib

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sdk-root', type=Path, default=Path('C:/ti/mmwave_sdk_03_06_02_00-LTS'))
parser.add_argument('--elf', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
elf = args.elf.resolve(strict=True)
output = args.output.resolve()
rprc = output.with_suffix('.rprc')
if output.exists() or rprc.exists():
    parser.error('Refusing to overwrite an existing BIN or RPRC output')
sdk = args.sdk_root.resolve(strict=True)
image_tools = sdk / 'packages/scripts/ImageCreator'
radarss = sdk / 'firmware/radarss/xwr6xxx_radarss_rprc.bin'
tools = [image_tools / name for name in (
    'out2rprc/out2rprc.exe',
    'multicore_image_generator/MulticoreImageGen.exe',
    'crc_multicore_image/crc_multicore_image.exe',
    'append_bin_crc/gen_bincrc32.exe',
)]
for required in [radarss, *tools]:
    if not required.is_file():
        parser.error(f'Missing official SDK input: {required}')
with tempfile.TemporaryDirectory(prefix='original-radar-image-') as temporary:
    work = Path(temporary)
    mss = work / 'mss.rprc'
    image = work / 'image.bin'
    subprocess.run([str(tools[0]), str(elf), str(mss)], check=True, cwd=work)
    subprocess.run([str(tools[1]), 'LE', '37', '0x00000006', str(image),
                    '0x35510000', str(mss), '0xb5510000', str(radarss)], check=True, cwd=work)
    subprocess.run([str(tools[2]), str(image), str(work / 'unused-be-image.bin')], check=True, cwd=work)
    subprocess.run([str(tools[3]), str(image)], check=True, cwd=work)
    binary = image.read_bytes()
    stored = struct.unpack('<I', binary[-4:])[0]
    computed = zlib.crc32(binary[:-4]) & 0xffffffff
    if stored != computed:
        raise RuntimeError(f'Appended CRC mismatch: {stored:#x} versus {computed:#x}')
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('xb') as stream:
        stream.write(binary)
    with rprc.open('xb') as stream:
        stream.write(mss.read_bytes())
print(json.dumps({
    'status': 'packaged_not_hardware_tested', 'hardware_accessed': False,
    'bin': str(output), 'bin_bytes': len(binary),
    'bin_sha256': hashlib.sha256(binary).hexdigest(), 'appended_crc32': f'{stored:#010x}',
    'elf_sha256': hashlib.sha256(elf.read_bytes()).hexdigest(),
    'radarss_sha256': hashlib.sha256(radarss.read_bytes()).hexdigest(),
}, indent=2))
