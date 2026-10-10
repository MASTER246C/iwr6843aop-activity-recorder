CC ?= cc
CPPFLAGS += -I.
CFLAGS ?= -O2 -g -std=c99 -Wall -Wextra -Wpedantic -Werror
LDLIBS += -lm
.PHONY: all test sanitize clean
all: test
build/test_radar_dsp: radar_dsp.c radar_dsp.h tests/test_radar_dsp.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) radar_dsp.c tests/test_radar_dsp.c $(LDFLAGS) $(LDLIBS) -o $@
test: build/test_radar_dsp
	./build/test_radar_dsp
sanitize:
	mkdir -p build
	$(CC) $(CPPFLAGS) -O1 -g -std=c99 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer radar_dsp.c tests/test_radar_dsp.c -lm -o build/test_radar_dsp_sanitize
	ASAN_OPTIONS=detect_leaks=0 ./build/test_radar_dsp_sanitize
clean:
	rm -rf build

# Compile-only portability check; does not claim the object ran on hardware.
TI_CGT_ROOT ?= /opt/ti/ti-cgt-arm_16.9.6.LTS
.PHONY: arm-check
arm-check:
	mkdir -p build
	$(TI_CGT_ROOT)/bin/armcl -mv7R4 --code_state=16 --float_support=VFPv3D16 --abi=eabi -me --c99 -O2 --emit_warnings_as_errors --include_path=$(TI_CGT_ROOT)/include --include_path=. --compile_only radar_dsp.c --obj_directory=build
