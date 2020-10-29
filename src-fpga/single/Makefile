.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> SYSROOT=<sysroot_path>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH and SYSROOT is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""
	$(ECHO)  "  make test DEVICE=<FPGA platform>"
	$(ECHO)  "     Command to run the application. This is same as 'check' target but does not have any makefile dependency."
	$(ECHO)  ""
	$(ECHO) "  make sd_card TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> SYSROOT=<sysroot_path>"
	$(ECHO) "      Command to prepare sd_card files."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH and SYSROOT is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make check TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> SYSROOT=<sysroot_path>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH and SYSROOT is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make build TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> SYSROOT=<sysroot_path>"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH and SYSROOT is required for SoC shells"
	$(ECHO) ""

# Points to top directory of Git repository
COMMON_REPO = ../
PWD = $(shell readlink -f .)
ABS_COMMON_REPO = $(shell readlink -f $(COMMON_REPO))

TARGET := sw_emu
HOST_ARCH := x86
SYSROOT := 
DEVICE = /proj/xbuilds/2019.2_daily_latest/internal_platforms/xilinx_u280_xdma_201920_3/xilinx_u280_xdma_201920_3.xpfm

include ./utils.mk

XSA := $(call device2xsa, $(DEVICE))
TEMP_DIR := ./_x.$(TARGET).$(XSA)
BUILD_DIR := ./build_dir.$(TARGET).$(XSA)

VPP := v++
SDCARD := sd_card

#Include Libraries
include $(ABS_COMMON_REPO)/common/includes/opencl/opencl.mk
include $(ABS_COMMON_REPO)/common/includes/xcl2/xcl2.mk
CXXFLAGS += $(xcl2_CXXFLAGS)
LDFLAGS += $(xcl2_LDFLAGS)
HOST_SRCS += $(xcl2_SRCS)
CXXFLAGS += $(opencl_CXXFLAGS) -Wall -O0 -g -std=c++11
LDFLAGS += $(opencl_LDFLAGS)

HOST_SRCS += src/host.cpp

# Host compiler global settings
CXXFLAGS += -fmessage-length=0
LDFLAGS += -lrt -lstdc++ 

ifneq ($(HOST_ARCH), x86)
	LDFLAGS += --sysroot=$(SYSROOT)
endif

# Kernel compiler global settings
CLFLAGS += -t $(TARGET) --platform $(DEVICE) --save-temps 
ifneq ($(TARGET), hw)
	CLFLAGS += -g
endif



# Adding config files to linker
# LDCLFLAGS += --config spmv.ini --config symgs.ini --config waxpby.ini --config dp.ini 
CMD_ARGS = $(BUILD_DIR)/spmv.xclbin $(BUILD_DIR)/symgs.xclbin $(BUILD_DIR)/waxpby.xclbin $(BUILD_DIR)/dp.xclbin
EMCONFIG_DIR = $(TEMP_DIR)
EMU_DIR = $(SDCARD)/data/emulation

BINARY_CONTAINERS += $(BUILD_DIR)/spmv.xclbin $(BUILD_DIR)/symgs.xclbin $(BUILD_DIR)/waxpby.xclbin $(BUILD_DIR)/dp.xclbin
# BINARY_CONTAINER_OBJS +=  $(TEMP_DIR)/symgs.xo $(TEMP_DIR)/waxpby.xo $(TEMP_DIR)/dp.xo

# Setting higher frequency to achieve higher bandwidth
CLFLAGS+=--kernel_frequency 450

CP = cp -rf

.PHONY: all clean cleanall docs emconfig
all: check-devices $(BINARY_CONTAINERS) emconfig sd_card

# .PHONY: exe
# exe: $(EXECUTABLE)

.PHONY: build
build: $(BINARY_CONTAINERS)

# Building kernel
$(TEMP_DIR)/spmv.xo: src/spmv.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(CLFLAGS) --temp_dir $(TEMP_DIR) -c -k spmv -I'$(<D)' -o'$@' '$<'

$(BUILD_DIR)/spmv.xclbin: $(TEMP_DIR)/spmv.xo
	mkdir -p $(BUILD_DIR)
	$(VPP) $(CLFLAGS) --temp_dir $(BUILD_DIR) -l $(LDCLFLAGS) --config spmv.ini -o'$@' $(+)

$(TEMP_DIR)/symgs.xo: src/symgs.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(CLFLAGS) --temp_dir $(TEMP_DIR) -c -k symgs -I'$(<D)' -o'$@' '$<'

$(BUILD_DIR)/symgs.xclbin: $(TEMP_DIR)/symgs.xo
	mkdir -p $(BUILD_DIR)
	$(VPP) $(CLFLAGS) --temp_dir $(BUILD_DIR) -l $(LDCLFLAGS) --config symgs.ini -o'$@' $(+)

$(TEMP_DIR)/waxpby.xo: src/waxpby.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(CLFLAGS) --temp_dir $(TEMP_DIR) -c -k waxpby -I'$(<D)' -o'$@' '$<'

$(BUILD_DIR)/waxpby.xclbin: $(TEMP_DIR)/waxpby.xo
	mkdir -p $(BUILD_DIR)
	$(VPP) $(CLFLAGS) --temp_dir $(BUILD_DIR) -l $(LDCLFLAGS) --config waxpby.ini -o'$@' $(+)

$(TEMP_DIR)/dp.xo: src/dp.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(CLFLAGS) --temp_dir $(TEMP_DIR) -c -k dp -I'$(<D)' -o'$@' '$<'

$(BUILD_DIR)/dp.xclbin: $(TEMP_DIR)/dp.xo
	mkdir -p $(BUILD_DIR)
	$(VPP) $(CLFLAGS) --temp_dir $(BUILD_DIR) -l $(LDCLFLAGS) --config dp.ini -o'$@' $(+)

# # Building Host
# $(EXECUTABLE): check-xrt $(HOST_SRCS) $(HOST_HDRS)
# 	$(CXX) $(CXXFLAGS) $(HOST_SRCS) $(HOST_HDRS) -o '$@' $(LDFLAGS)

emconfig:$(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(DEVICE) --od $(EMCONFIG_DIR)

# Cleaning stuff
clean:
	-$(RMDIR) $(EXECUTABLE) $(XCLBIN)/{*sw_emu*,*hw_emu*} 
	-$(RMDIR) profile_* TempConfig system_estimate.xtxt *.rpt *.csv 
	-$(RMDIR) src/*.ll *v++* .Xil emconfig.json dltmp* xmltmp* *.log *.jou *.wcfg *.wdb fast*

cleanall: clean
	-$(RMDIR) build_dir* sd_card*
	-$(RMDIR) _x.* *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl* start_simulation.sh *.xclbin

