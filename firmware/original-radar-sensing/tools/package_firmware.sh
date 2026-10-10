#!/usr/bin/env bash
# Original orchestration of unmodified, official TI image-conversion tools.
# Creates files only. No serial devices, flasher, network or hardware access.
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
SDK_ROOT=${SDK_ROOT:-/opt/ti/mmwave_sdk_03_06_02_00-LTS}
MONO=${MONO:-mono}
ELF=${ELF:-$ROOT/build/original_radar_sensing.xer4f}
OUTPUT=${1:-$ROOT/build/original_radar_sensing.bin}
RADARSS="$SDK_ROOT/firmware/radarss/xwr6xxx_radarss_rprc.bin"
IMAGE_TOOLS="$SDK_ROOT/packages/scripts/ImageCreator"
if [[ -e "$OUTPUT" || -e "${OUTPUT%.bin}.rprc" ]]; then
    echo "Refusing to overwrite existing BIN/RPRC outputs: $OUTPUT" >&2
    exit 2
fi
[[ -r "$ELF" && -r "$RADARSS" ]] || { echo 'ELF or TI RadarSS input missing' >&2; exit 2; }
WORK=$(mktemp -d)
trap 'rm -rf -- "$WORK"' EXIT
run_i386() {
    if [[ -n "${QEMU_I386:-}" ]]; then
        : "${QEMU_SYSROOT:?Set QEMU_SYSROOT when using QEMU_I386}"
        "$QEMU_I386" -L "$QEMU_SYSROOT" -E "LD_LIBRARY_PATH=$QEMU_SYSROOT/usr/lib32" "$@"
    else
        "$@"
    fi
}
"$MONO" "$IMAGE_TOOLS/out2rprc/out2rprc.exe" "$ELF" "$WORK/mss.rprc"
# TI's generateMetaImage.sh uses token 37 (hex), six L3 banks, MSS and BSS core IDs.
run_i386 "$IMAGE_TOOLS/multicore_image_generator/MulticoreImageGen" \
    LE 37 0x00000006 "$WORK/image.bin" 0x35510000 "$WORK/mss.rprc" 0xb5510000 "$RADARSS"
run_i386 "$IMAGE_TOOLS/crc_multicore_image/crc_multicore_image" "$WORK/image.bin" "$WORK/image.bin.tmp"
"$IMAGE_TOOLS/append_bin_crc/gen_bincrc32" "$WORK/image.bin"
python3 - "$WORK/image.bin" <<'PY'
from pathlib import Path
import struct, sys, zlib
p = Path(sys.argv[1]); data = p.read_bytes()
assert len(data) > 4, 'Empty image'
stored = struct.unpack('<I', data[-4:])[0]
expected = zlib.crc32(data[:-4]) & 0xffffffff
assert stored == expected, f'Final CRC mismatch: {stored:#x} != {expected:#x}'
print(f'Independent CRC32 check passed: {stored:#010x}; {len(data)} bytes')
PY
mkdir -p -- "$(dirname -- "$OUTPUT")"
cp -- "$WORK/image.bin" "$OUTPUT"
cp -- "$WORK/mss.rprc" "${OUTPUT%.bin}.rprc"
sha256sum -- "$ELF" "$RADARSS" "$OUTPUT"
echo 'Packaging complete. No board was accessed or flashed; boot and UART remain unvalidated.'
