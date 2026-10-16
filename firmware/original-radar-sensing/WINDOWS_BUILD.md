# Native Windows build

These commands create the separate 0.3.2-rc1 candidate and never access a board. Run from this firmware directory. The old `build/original_radar_sensing.bin` stays untouched.

Actual installed official roots: `C:\ti\mmwave_sdk_03_06_02_00-LTS`, `C:\ti\ti-cgt-arm_16.9.6.LTS`, `C:\ti\bios_6_73_01_01` and `C:\ti\xdctools_3_50_08_24_core`. ARM compiler revision reports 16.9.6; XDCtools includes native GNU Make 3.81, xs.exe and a small POSIX shell. WSL is unnecessary.

```powershell
$env:PATH = 'C:\ti\xdctools_3_50_08_24_core\bin;C:\ti\xdctools_3_50_08_24_core;' + $env:PATH
$radarPython = 'C:\Users\bobji\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& 'C:\ti\xdctools_3_50_08_24_core\gmake.exe' `
  'SHELL=C:/ti/xdctools_3_50_08_24_core/bin/sh.exe' `
  'TI_ROOT=C:/ti' 'CGT_ROOT=C:/ti/ti-cgt-arm_16.9.6.LTS' `
  "PYTHON=$($radarPython.Replace('\','/'))" 'BUILD=build/0.3.2-rc1' all
if ($LASTEXITCODE -ne 0) { throw 'TI compile/link failed' }
& $radarPython tools/package_firmware_windows.py `
  --elf build/0.3.2-rc1/original_radar_sensing.xer4f `
  --output build/0.3.2-rc1/original_radar_sensing.bin
if ($LASTEXITCODE -ne 0) { throw 'TI image packaging failed' }
& $radarPython tools/verify_sensing.py `
  build/0.3.2-rc1/original_radar_sensing.xer4f `
  build/0.3.2-rc1/original_radar_sensing.map
if ($LASTEXITCODE -ne 0) { throw 'ELF verification failed' }
Get-FileHash build/0.3.2-rc1/original_radar_sensing.bin -Algorithm SHA256
```

These are the recorded commands that produced the 0.3.2-rc1 candidate; do not rerun them into `build/0.3.2-rc1/`. The unreleased 0.3.3-dev source has not been built on this or any TI host. A future build should use a new folder, for example `build/0.3.3-dev` in place of every `build/0.3.2-rc1` above. The current Makefile keeps these explicit TI_ROOT/CGT_ROOT/BUILD/PYTHON/SHELL overrides working. It now also runs `armcl --compiler_revision` before configuring and compiling, expecting 16.9.6; `ALLOW_UNPINNED_CGT=1` skips that check for a non-release experiment only. The check has not been run under this XDCtools shell. `make clean` refuses `BUILD=build/0.3.2-rc1` because that folder holds a receipt. `tools/package_firmware_windows.py` is unchanged.

Image packaging runs unmodified SDK Windows out2rprc, MulticoreImageGen, crc_multicore_image and gen_bincrc32 executables, using the SDK RadarSS RPRC. It refuses existing BIN/RPRC output names and independently compares appended CRC32 with the computed value. Choose a new output filename for a repeat packaging attempt instead of deleting a prior receipt.

The Makefile permits BUILD/PYTHON overrides and retains the same compiler/SDK roots. Generated SYS/BIOS files and debug ELF/map/link XML remain local audit artifacts and are omitted from public source packages. Portable host tests additionally require a host C compiler; the TI ARM compiler cannot execute x86 tests. Passing tests or the official build does not establish physical radar operation.
