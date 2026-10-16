#!/usr/bin/env python3
"""Original offline structural checks; deliberately not a claim of board execution."""
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

root = Path(__file__).resolve().parents[1]


def require(condition, message):
    """Explicit check: python -O / PYTHONOPTIMIZE must never turn a failure into success."""
    if not condition:
        sys.exit(f'verify_sensing: FAILED: {message}')


elf = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'build/original_radar_sensing.xer4f'
map_path = Path(sys.argv[2]) if len(sys.argv) > 2 else elf.with_suffix('.map')
data = elf.read_bytes()
require(data[:4] == b'\x7fELF', 'Not an ELF file')
require(data[4:6] == b'\x01\x01', 'Expected 32-bit little-endian ELF')
header = struct.unpack_from('<HHIIIIIHHHHHH', data, 16)
kind, machine, version, entry, phoff, shoff, flags, ehsize, phsize, phcount, shsize, shcount, shnames = header
require(kind == 2 and machine == 40 and version == 1, 'Expected executable ARM ELF')
sections = [struct.unpack_from('<IIIIIIIIII', data, shoff + i * shsize) for i in range(shcount)]
names = sections[shnames]
name_table = data[names[4]:names[4] + names[5]]
def name_of(section):
    start = section[0]
    return name_table[start:name_table.index(b'\0', start)].decode('ascii')
allocated = []
for section in sections:
    name = name_of(section)
    if section[2] & 2 and section[5]:
        address, size = section[3], section[5]
        require((0 <= address and address + size <= 0x80000) or (
            0x08000000 <= address and address + size <= 0x08030000) or (
            0x51000000 <= address and address + size <= 0x510c0000),
                f'Unexpected allocated memory region: {name} {address:#x}+{size:#x}')
        allocated.append({'name': name, 'address': hex(address), 'size': size,
                          'load_bytes': 0 if section[1] == 8 else size})
require(any(s['name'] == '.vecs' and s['address'] == '0x0' for s in allocated), 'No reset vectors at zero')
require(0 < entry < 0x80000, 'Entry point outside program RAM')
map_text = map_path.read_text()
require(any(s['name'] == '.capture_frames' and s['size'] == 65536 for s in allocated), 'Expected two full L3 ADC frames')
for forbidden in ('libmmwavelib_', 'libdpm_', 'libdpedma_', 'libdpu_', 'libdsplib',
                  'MmwDemo_', 'DPU_', 'DPC_', 'DPM_', 'gtrack_'):
    require(forbidden not in map_text, f'Unexpected demo/pipeline dependency: {forbidden}')
for required in ('main', 'or_radio_prepare', 'or_rf_physical', 'or_acquisition_arm',
                 'or_profile_energy', 'radar_dsp_process', 'os_estimate', 'or_frame_json',
                 'MMWave_start', 'EDMA_configChannel', 'ADCBuf_open', 'UART_write', 'SOC_init'):
    require(required in map_text, f'Missing expected app/driver symbol: {required}')
result = {
    'status': 'offline_checks_passed', 'hardware_tested': False,
    'elf': elf.name, 'elf_sha256': hashlib.sha256(data).hexdigest(),
    'size_bytes': len(data), 'architecture': 'ARM ELF32 little-endian',
    'entry_point': hex(entry), 'allocated_sections': allocated,
    'checks': ['ARM executable format', 'reset vector at zero', 'MSS TCMA/TCMB and L3 bounds',
               'two 32KiB capture frames', 'expected original sensing and low-level driver symbols',
               'no listed TI demo, DPU/DPC, tracker or signal-processing libraries'],
    'limitations': ['No physical boot/UART/RF/acquisition validation', 'No RF emissions measurement',
                    'No measured processing timing or stack high-water', 'No activity classifier',
                    'Structural checks cannot establish hardware correctness']
}
out = elf.with_name('elf-verification.json')
out.write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
