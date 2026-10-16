CC ?= cc
CPPFLAGS += -I.
CFLAGS ?= -O2 -g -std=c99 -Wall -Wextra -Wpedantic -Werror
LDLIBS += -lm
# Output folder; tests/run_tests.sh points it at ../build/host-tests/dsp.
DSP_BUILD ?= build
.PHONY: all test sanitize clean
all: test
$(DSP_BUILD)/test_radar_dsp: radar_dsp.c radar_dsp.h tests/test_radar_dsp.c
	mkdir -p $(DSP_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) radar_dsp.c tests/test_radar_dsp.c $(LDFLAGS) $(LDLIBS) -o $@
test: $(DSP_BUILD)/test_radar_dsp
	$(DSP_BUILD)/test_radar_dsp
sanitize:
	mkdir -p $(DSP_BUILD)
	$(CC) $(CPPFLAGS) -O1 -g -std=c99 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer radar_dsp.c tests/test_radar_dsp.c -lm -o $(DSP_BUILD)/test_radar_dsp_sanitize
	ASAN_OPTIONS=detect_leaks=0 $(DSP_BUILD)/test_radar_dsp_sanitize
clean:
	rm -rf $(DSP_BUILD)

# Compile-only portability check; does not claim the object ran on hardware.
TI_CGT_ROOT ?= /opt/ti/ti-cgt-arm_16.9.6.LTS
.PHONY: arm-check
arm-check:
	mkdir -p $(DSP_BUILD)
	$(TI_CGT_ROOT)/bin/armcl -mv7R4 --code_state=16 --float_support=VFPv3D16 --abi=eabi -me --c99 -O2 --emit_warnings_as_errors --include_path=$(TI_CGT_ROOT)/include --include_path=. --compile_only radar_dsp.c --obj_directory=$(DSP_BUILD)
