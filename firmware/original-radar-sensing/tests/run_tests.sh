#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
CC=${HOST_CC:-cc}
FLAGS=(-std=c99 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -Isrc -Idsp -Ispatial)
"$CC" "${FLAGS[@]}" tests/test_sensing.c src/sensing_config.c src/rf_profile.c src/serial_line.c src/capture_model.c src/json_output.c src/link_session.c src/energy_summary.c spatial/original_spatial.c -lm -o build/test_sensing
ASAN_OPTIONS=detect_leaks=0 build/test_sensing
make -C dsp test sanitize
"$CC" "${FLAGS[@]}" tests/test_original_spatial.c spatial/original_spatial.c -lm -o build/test_spatial
ASAN_OPTIONS=detect_leaks=0 build/test_spatial
python3 -m unittest discover -s host -p 'test_*.py' -v

"$CC" "${FLAGS[@]}" -DOS_SELFTEST_HOST_MAIN spatial/spatial_selftest.c spatial/original_spatial.c -lm -o build/test_boot_selftest
ASAN_OPTIONS=detect_leaks=0 build/test_boot_selftest
python3 tests/test_boot_gate.py
"$CC" "${FLAGS[@]}" tests/test_record_transport.c src/record_transport.c -o build/test_record_transport
ASAN_OPTIONS=detect_leaks=0 build/test_record_transport

"$CC" "${FLAGS[@]}" tests/test_link_session.c src/link_session.c -o build/test_link_session
ASAN_OPTIONS=detect_leaks=0 build/test_link_session

python3 tests/test_independent_dual_framing.py .

"$CC" "${FLAGS[@]}" tests/test_aop_polarity_grid.c spatial/original_spatial.c -lm -o build/test_aop_polarity_grid
ASAN_OPTIONS=detect_leaks=0 build/test_aop_polarity_grid
