# -*- Makefile -*-

arch = UNKNOWN
setup_file = setup/Make.$(arch)

include $(setup_file)
# include ./utils.mk

# Points to top directory of Git repository
COMMON_REPO = .
PWD = $(shell readlink -f .)
ABS_COMMON_REPO = $(shell readlink -f $(COMMON_REPO))

TARGET := hw
HOST_ARCH := x86
SYSROOT := 
DEVICE = /proj/xbuilds/2020.1_daily_latest/internal_platforms/xilinx_u250_qdma_201910_1/xilinx_u250_qdma_201910_1.xpfm 

#Include Libraries
include src/common/includes/opencl/opencl.mk
include src/common/includes/xcl2/xcl2.mk
CXXFLAGSFPGA += $(xcl2_CXXFLAGS)
LDFLAGS += $(xcl2_LDFLAGS)
HOST_SRCS += $(xcl2_SRCS)
CXXFLAGSFPGA += -pthread
CXXFLAGSFPGA += $(opencl_CXXFLAGS) -Wall -O0 -g -std=c++11 
LDFLAGS += $(opencl_LDFLAGS) 

# HOST_SRCS += host.cpp 
# Host compiler global settings
CXXFLAGSFPGA += -fmessage-length=0
LDFLAGS += -lrt -lstdc++ 

EXECUTABLE = host
CMD_ARGS = $(BUILD_DIR)/vecdotprod.xclbin
EMCONFIG_DIR = $(TEMP_DIR)
EMU_DIR = $(SDCARD)/data/emulation

BINARY_CONTAINERS += $(BUILD_DIR)/vecdotprod.xclbin
BINARY_CONTAINER_vecdotprod_OBJS += $(TEMP_DIR)/vecdotprod.xo

CP = cp -rf

# FPGA_HOST_FLAGS = $(xcl2_CXXFLAGS) -pthread $(opencl_CXXFLAGS) -Wall -O0 -g -std=c++11 -fmessage-length=0
# LDFLAGS += $(xcl2_LDFLAGS) $(opencl_LDFLAGS) -lrt -lstdc++ --sysroot=$(SYSROOT)

HPCG_DEPS = src/xcl2.o \
		 src/CG.o \
		 src/CG_ref.o \
		 src/TestCG.o \
		 src/ComputeResidual.o \
		 src/ExchangeHalo.o \
		 src/GenerateGeometry.o \
		 src/GenerateProblem.o \
		 src/GenerateProblem_ref.o \
		 src/CheckProblem.o \
		 src/MixedBaseCounter.o \
		 src/OptimizeProblem.o \
		 src/ReadHpcgDat.o \
		 src/ReportResults.o \
		 src/SetupHalo.o \
		 src/SetupHalo_ref.o \
		 src/TestSymmetry.o \
		 src/TestNorms.o \
		 src/WriteProblem.o \
		 src/YAML_Doc.o \
		 src/YAML_Element.o \
		 src/ComputeDotProduct.o \
		 src/ComputeDotProduct_ref.o \
		 src/ComputeDotProduct_FPGA.o \
		 src/mytimer.o \
		 src/ComputeOptimalShapeXYZ.o \
		 src/ComputeSPMV.o \
		 src/ComputeSPMV_ref.o \
		 src/ComputeSYMGS.o \
		 src/ComputeSYMGS_ref.o \
		 src/ComputeWAXPBY.o \
		 src/ComputeWAXPBY_ref.o \
		 src/ComputeWAXPBY_FPGA.o \
		 src/ComputeMG_ref.o \
		 src/ComputeMG.o \
		 src/ComputeProlongation_ref.o \
		 src/ComputeRestriction_ref.o \
		 src/CheckAspectRatio.o \
		 src/OutputFile.o \
		 src/GenerateCoarseProblem.o \
		 src/init.o \
		 src/finalize.o

# These header files are included in many source files, so we recompile every file if one or more of these header is modified.
PRIMARY_HEADERS = src/Geometry.hpp src/SparseMatrix.hpp src/Vector.hpp src/CGData.hpp \
						src/MGData.hpp src/hpcg.hpp

all: bin/xhpcg

bin/xhpcg: src/main.o $(HPCG_DEPS)
	$(LINKER) $(LINKFLAGS) src/main.o $(HPCG_DEPS) $(HPCG_LIBS) -o bin/xhpcg $(LDFLAGS) 

clean:
	rm -f src/*.o bin/xhpcg

.PHONY: all clean

src/main.o: src/main.cpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/CG.o: src/CG.cpp src/CG.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/CG_ref.o: src/CG_ref.cpp src/CG_ref.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/TestCG.o: src/TestCG.cpp src/TestCG.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeResidual.o: src/ComputeResidual.cpp src/ComputeResidual.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ExchangeHalo.o: src/ExchangeHalo.cpp src/ExchangeHalo.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/GenerateGeometry.o: src/GenerateGeometry.cpp src/GenerateGeometry.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/GenerateProblem.o: src/GenerateProblem.cpp src/GenerateProblem.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/GenerateProblem_ref.o: src/GenerateProblem_ref.cpp src/GenerateProblem_ref.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/CheckProblem.o: src/CheckProblem.cpp src/CheckProblem.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/MixedBaseCounter.o: src/MixedBaseCounter.cpp src/MixedBaseCounter.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/OptimizeProblem.o: src/OptimizeProblem.cpp src/OptimizeProblem.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ReadHpcgDat.o: src/ReadHpcgDat.cpp src/ReadHpcgDat.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ReportResults.o: src/ReportResults.cpp src/ReportResults.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/SetupHalo.o: src/SetupHalo.cpp src/SetupHalo.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/SetupHalo_ref.o: src/SetupHalo_ref.cpp src/SetupHalo_ref.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/TestSymmetry.o: src/TestSymmetry.cpp src/TestSymmetry.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/TestNorms.o: src/TestNorms.cpp src/TestNorms.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/WriteProblem.o: src/WriteProblem.cpp src/WriteProblem.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/YAML_Doc.o: src/YAML_Doc.cpp src/YAML_Doc.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/YAML_Element.o: src/YAML_Element.cpp src/YAML_Element.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeDotProduct.o: src/ComputeDotProduct.cpp src/ComputeDotProduct.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeDotProduct_ref.o: src/ComputeDotProduct_ref.cpp src/ComputeDotProduct_ref.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/xcl2.o: common/includes/xcl2/xcl2.cpp common/includes/xcl2/xcl2.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGSFPGA) $(FPGA_HOST_FLAGS) -Isrc $< -o $@ 

src/ComputeDotProduct_FPGA.o: src/ComputeDotProduct_FPGA.cpp src/ComputeDotProduct_FPGA.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGSFPGA) -Isrc $< -o $@

src/ComputeWAXPBY_FPGA.o: src/ComputeDotProduct_FPGA.cpp src/ComputeDotProduct_FPGA.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGSFPGA) -Isrc $< -o $@

src/finalize.o: src/finalize.cpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/init.o: src/init.cpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/mytimer.o: src/mytimer.cpp src/mytimer.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeOptimalShapeXYZ.o: src/ComputeOptimalShapeXYZ.cpp src/ComputeOptimalShapeXYZ.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeSPMV.o: src/ComputeSPMV.cpp src/ComputeSPMV.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeSPMV_ref.o: src/ComputeSPMV_ref.cpp src/ComputeSPMV_ref.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeSYMGS.o: src/ComputeSYMGS.cpp src/ComputeSYMGS.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeSYMGS_ref.o: src/ComputeSYMGS_ref.cpp src/ComputeSYMGS_ref.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeWAXPBY.o: src/ComputeWAXPBY.cpp src/ComputeWAXPBY.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeWAXPBY_ref.o: src/ComputeWAXPBY_ref.cpp src/ComputeWAXPBY_ref.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeMG_ref.o: src/ComputeMG_ref.cpp src/ComputeMG_ref.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeMG.o: src/ComputeMG.cpp src/ComputeMG.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeProlongation_ref.o: src/ComputeProlongation_ref.cpp src/ComputeProlongation_ref.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/ComputeRestriction_ref.o: src/ComputeRestriction_ref.cpp src/ComputeRestriction_ref.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/GenerateCoarseProblem.o: src/GenerateCoarseProblem.cpp src/GenerateCoarseProblem.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/CheckAspectRatio.o: src/CheckAspectRatio.cpp src/CheckAspectRatio.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

src/OutputFile.o: src/OutputFile.cpp src/OutputFile.hpp $(PRIMARY_HEADERS)
	$(CXX) -c $(CXXFLAGS) -Isrc $< -o $@

