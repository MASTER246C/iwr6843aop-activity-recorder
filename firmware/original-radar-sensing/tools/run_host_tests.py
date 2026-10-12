"""Portable offline behavioral tests, with explicitly selected sanitizers.

Does not open serial devices or operate hardware. Use --zig for a Zig cc host
compiler; --sanitize address,undefined requests those runtimes explicitly.
"""
from pathlib import Path
import argparse
import json
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--zig', type=Path)
parser.add_argument('--cc', default='cc')
parser.add_argument('--sanitize', default='')
parser.add_argument('--output', type=Path, default=Path('build/0.3.2-rc1/host-tests'))
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
compiler = [str(args.zig.resolve()), 'cc'] if args.zig else [args.cc]
flags = ['-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-pedantic']
if args.sanitize:
    flags.append('-fsanitize=' + args.sanitize)
includes = ['-I' + str(root / folder) for folder in ('src', 'dsp', 'spatial')]
tests = {
    'sensing': ['tests/test_sensing.c', 'src/sensing_config.c', 'src/rf_profile.c',
                'src/serial_line.c', 'src/capture_model.c', 'src/json_output.c',
                'src/link_session.c', 'src/energy_summary.c', 'spatial/original_spatial.c'],
    'dsp': ['dsp/tests/test_radar_dsp.c', 'dsp/radar_dsp.c'],
    'spatial': ['tests/test_original_spatial.c', 'spatial/original_spatial.c'],
    'boot_selftest': ['spatial/spatial_selftest.c', 'spatial/original_spatial.c'],
    'record_transport': ['tests/test_record_transport.c', 'src/record_transport.c'],
    'link_session': ['tests/test_link_session.c', 'src/link_session.c'],
    'aop_polarity_grid': ['tests/test_aop_polarity_grid.c', 'spatial/original_spatial.c'],
}
environment = {**os.environ, 'HOST_CC': compiler[0],
               'HOST_CC_SUBCOMMAND': 'cc' if args.zig else '',
               'HOST_SANITIZERS': args.sanitize, 'ASAN_OPTIONS': 'detect_leaks=0'}
for name, sources in tests.items():
    executable = output / (name + ('.exe' if os.name == 'nt' else ''))
    definitions = ['-DOS_SELFTEST_HOST_MAIN'] if name == 'boot_selftest' else []
    subprocess.run(compiler + flags + definitions + includes +
                   [str(root / source) for source in sources] + ['-lm', '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True, env=environment)
subprocess.run([sys.executable, '-m', 'unittest', 'discover', '-s', str(root / 'host'),
                '-p', 'test_*.py', '-v'], check=True, env=environment)
subprocess.run([sys.executable, str(root / 'tests/test_boot_gate.py')], check=True, env=environment)
subprocess.run([sys.executable, str(root / 'tests/test_independent_dual_framing.py'), str(root)],
               check=True, env=environment)
print(json.dumps({'result': 'PASS', 'hardware_accessed': False,
                  'compiler': compiler, 'sanitizers': args.sanitize or 'none',
                  'portable_c_programs': list(tests), 'actual_main_mock': True,
                  'independent_dual_framing_model': True}, indent=2))
