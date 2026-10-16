#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Every host-test output stays under build/host-tests; no other build/ content is touched.
OUT=build/host-tests
mkdir -p "$OUT"
export HOST_TEST_OUTPUT="$PWD/$OUT"
export PYTHONDONTWRITEBYTECODE=1
# Python test scripts check with assert; PYTHONOPTIMIZE must not silently disable them.
unset PYTHONOPTIMIZE
CC=${HOST_CC:-cc}
FLAGS=(-std=c99 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -Isrc -Idsp -Ispatial)
# Real radio/acquisition sources against tests/stubs host fakes; the acquisition
# source keeps its TI section pragmas and 32-bit address casts, so only those
# two warnings are disabled for that test.
STUBS=(-Itests/stubs)
"$CC" "${FLAGS[@]}" tests/test_sensing.c src/sensing_config.c src/rf_profile.c src/serial_line.c src/capture_model.c src/json_output.c src/link_session.c src/energy_summary.c spatial/original_spatial.c -lm -o "$OUT/test_sensing"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test_sensing"
make -C dsp DSP_BUILD="$PWD/$OUT/dsp" test sanitize
"$CC" "${FLAGS[@]}" tests/test_original_spatial.c spatial/original_spatial.c -lm -o "$OUT/test_spatial"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test_spatial"
echo '== LEGACY original-radar-v1 collector tests (host/): historical v1 replay only; this collector cannot control current original-radar-v2 firmware =='
python3 -m unittest discover -s host -p 'test_*.py' -v
echo '== End of LEGACY v1 collector tests =='

"$CC" "${FLAGS[@]}" -DOS_SELFTEST_HOST_MAIN spatial/spatial_selftest.c spatial/original_spatial.c -lm -o "$OUT/test_boot_selftest"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test_boot_selftest"
python3 tests/test_boot_gate.py
"$CC" "${FLAGS[@]}" tests/test_record_transport.c src/record_transport.c -o "$OUT/test_record_transport"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test_record_transport"

"$CC" "${FLAGS[@]}" tests/test_link_session.c src/link_session.c -o "$OUT/test_link_session"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test_link_session"

python3 tests/test_independent_dual_framing.py .

"$CC" "${FLAGS[@]}" tests/test_aop_polarity_grid.c spatial/original_spatial.c -lm -o "$OUT/test_aop_polarity_grid"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test_aop_polarity_grid"

"$CC" "${FLAGS[@]}" "${STUBS[@]}" tests/test_radio.c src/radio.c src/sensing_config.c src/rf_profile.c -lm -o "$OUT/test_radio"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test_radio"
"$CC" "${FLAGS[@]}" "${STUBS[@]}" -Wno-unknown-pragmas -Wno-pointer-to-int-cast tests/test_acquisition.c src/acquisition.c src/capture_model.c -o "$OUT/test_acquisition"
ASAN_OPTIONS=detect_leaks=0 "$OUT/test_acquisition"
python3 tests/test_tool_checks.py
