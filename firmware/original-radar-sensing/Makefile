# Original build recipe. Override all tool roots for another machine.
TI_ROOT ?= /opt/ti
SDK ?= $(TI_ROOT)/mmwave_sdk_03_06_02_00-LTS/packages
BIOS_ROOT ?= $(TI_ROOT)/bios_6_73_01_01
XDC_ROOT ?= $(TI_ROOT)/xdctools_3_50_08_24_core
CGT_ROOT ?= /opt/ti/ti-cgt-arm_16.9.6.LTS
CC := $(CGT_ROOT)/bin/armcl
HOST_CC ?= cc
BUILD := build
CFG := $(BUILD)/configPkg
OUT := $(BUILD)/original_radar_sensing.xer4f
ARCH := -mv7R4 --code_state=16 --float_support=VFPv3D16 --abi=eabi -me --enum_type=int
DEFS := --define=SUBSYS_MSS --define=SOC_XWR68XX --define=_LITTLE_ENDIAN --define=DebugP_ASSERT_ENABLED \
 --define=MMWAVE_L3RAM_NUM_BANK=6 --define=MMWAVE_SHMEM_TCMA_NUM_BANK=0 \
 --define=MMWAVE_SHMEM_TCMB_NUM_BANK=0 --define=MMWAVE_SHMEM_BANK_SIZE=0x20000
CFLAGS := $(ARCH) $(DEFS) --c99 -g -O2 --gen_func_subsections --display_error_number \
 --diag_warning=225 --diag_wrap=off --emit_warnings_as_errors -i$(SDK) -i$(CGT_ROOT)/include -isrc -idsp -ispatial
DRIVERS := osal esm soc pinmux uart dma adcbuf edma crc mailbox
LIBS := $(foreach driver,$(DRIVERS),-i$(SDK)/ti/drivers/$(driver)/lib -llib$(driver)_xwr68xx.aer4f) \
 -i$(SDK)/ti/control/mmwave/lib -llibmmwave_xwr68xx.aer4f \
 -i$(SDK)/ti/control/mmwavelink/lib -llibmmwavelink_xwr68xx.aer4f
HEADERS := $(wildcard src/*.h dsp/*.h spatial/*.h) src/build_identity.h
IDENTITY_INPUTS := $(filter-out src/build_identity.h,$(wildcard src/*.c src/*.h dsp/*.c dsp/*.h spatial/*.c spatial/*.h)) Makefile sensing.cfg application.cmd tools/update_build_identity.py
OBJECTS := $(addprefix $(BUILD)/,main.oer4f serial_line.oer4f sensing_config.oer4f capture_model.oer4f acquisition.oer4f radio.oer4f rf_profile.oer4f energy_summary.oer4f json_output.oer4f radar_dsp.oer4f original_spatial.oer4f spatial_selftest.oer4f record_transport.oer4f link_session.oer4f)
.PHONY: all configure test verify clean
all: $(OUT)
configure: $(CFG)/.configured
$(CFG)/.configured: sensing.cfg
	mkdir -p $(BUILD)
	$(XDC_ROOT)/xs --xdcpath="$(BIOS_ROOT)/packages;$(SDK)" xdc.tools.configuro \
	 -t ti.targets.arm.elf.R4Ft -p ti.platforms.cortexR:IWR68XX:false:200 \
	 -r release -c $(CGT_ROOT) --compileOptions "--enum_type=int" -o $(CFG) sensing.cfg
	touch $(CFG)/.configured
src/build_identity.h: $(IDENTITY_INPUTS)
	python3 tools/update_build_identity.py
vpath %.c src dsp spatial
$(BUILD)/%.oer4f: %.c $(HEADERS) $(CFG)/.configured Makefile
	$(CC) $(CFLAGS) --cmd_file=$(CFG)/compiler.opt -c $< --output_file=$@
$(OUT): $(OBJECTS) application.cmd $(CFG)/.configured
	$(CC) $(ARCH) --display_error_number --diag_wrap=off -z $(DEFS) --rom_model \
	 --reread_libs --warn_sections --unused_section_elimination --emit_warnings_as_errors \
	 $(OBJECTS) $(LIBS) -l$(CFG)/linker.cmd $(SDK)/ti/platform/xwr68xx/r4f_linker.cmd \
	 application.cmd -i$(CGT_ROOT)/lib -lrtsv7R4_T_le_v3D16_eabi.lib \
	 --map_file=$(BUILD)/original_radar_sensing.map --xml_link_info=$(BUILD)/link.xml -o $@
test:
	bash tests/run_tests.sh
verify: all test
	python3 tools/verify_sensing.py
clean:
	rm -rf $(BUILD) src/sysbios src/makefile.libs src/.exclude
