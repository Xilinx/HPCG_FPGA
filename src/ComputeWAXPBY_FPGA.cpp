/**********
Copyright (c) 2020, Xilinx, Inc.
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

//@HEADER
// ***************************************************
//
// HPCG: High Performance Conjugate Gradient Benchmark
//
// Xilinx Alveo U280 vesion
//
// Alberto Zeni, Kenneth O'Brien - albertoz,kennetho{@xilinx.com}
// ***************************************************
//@HEADER

/**********
Copyright (c) 2020, Xilinx, Inc.
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <chrono>

// This file is required for OpenCL C++ wrapper APIs
#include "xcl2.hpp"


#include "ComputeWAXPBY_FPGA.hpp"

#ifndef HPCG_NO_MPI
#include <mpi.h>
#include "mytimer.hpp"
#endif
#ifndef HPCG_NO_OPENMP
#include <omp.h>
#endif
#define NUM_KERNEL 8
//HBM Banks requirements
#define MAX_HBM_BANKCOUNT 32
#define BANK_NAME(n) n | XCL_MEM_TOPOLOGY
const int bank[MAX_HBM_BANKCOUNT] = {
        BANK_NAME(0),  BANK_NAME(1),  BANK_NAME(2),  BANK_NAME(3),  BANK_NAME(4),
        BANK_NAME(5),  BANK_NAME(6),  BANK_NAME(7),  BANK_NAME(8),  BANK_NAME(9),
        BANK_NAME(10), BANK_NAME(11), BANK_NAME(12), BANK_NAME(13), BANK_NAME(14),
        BANK_NAME(15), BANK_NAME(16), BANK_NAME(17), BANK_NAME(18), BANK_NAME(19),
        BANK_NAME(20), BANK_NAME(21), BANK_NAME(22), BANK_NAME(23), BANK_NAME(24),
        BANK_NAME(25), BANK_NAME(26), BANK_NAME(27), BANK_NAME(28), BANK_NAME(29),
        BANK_NAME(30), BANK_NAME(31)};


int ComputeWAXPBY_FPGA(const local_int_t n, const double alpha, const Vector & x,
    const double beta, const Vector & y, Vector & w, double & time_FPGA) {

  	std::string binaryFile = PATH_TO_BINARY_WAXPBY;
    cl_int err;
    cl::CommandQueue q;
    std::string krnl_name = "waxpby";
    std::vector<cl::Kernel> krnls(NUM_KERNEL);
    cl::Context context;
    std::vector<synt_type, aligned_allocator<synt_type>> source_in1[NUM_KERNEL];
    std::vector<synt_type, aligned_allocator<synt_type>> source_in2[NUM_KERNEL];
    std::vector<synt_type, aligned_allocator<synt_type>> source_hw_waxpby_results[NUM_KERNEL];
 	unsigned int dataSize = n/NUM_KERNEL;

    for(size_t i = 0; i < NUM_KERNEL; i++){
    	source_in1[i].resize(dataSize);
        source_in2[i].resize(dataSize);
        source_hw_waxpby_results[i].resize(dataSize);
    }

    for (unsigned long i = 0; i < NUM_KERNEL; i++) {
        for(long j = 0; j < dataSize; j++){
        	source_in1[i][j] = x.values[j+i*dataSize];
        	source_in2[i][j] = y.values[j+i*dataSize];
        }
    }

    // OPENCL HOST CODE AREA START
    // The get_xil_devices will return vector of Xilinx Devices
    auto devices = xcl::get_xil_devices();

    // read_binary_file() command will find the OpenCL binary file created using the
    // V++ compiler load into OpenCL Binary and return pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);

    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    int valid_device = 0;
    for (unsigned int i = 0; i < devices.size(); i++) {//skip slot zero bc of machine
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, NULL, NULL, NULL, &err));
        OCL_CHECK(err,
                q = cl::CommandQueue(context,
                                     device,
                                     CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                                     CL_QUEUE_PROFILING_ENABLE,
                                     &err));
        cl::Program program(context, {device}, bins, NULL, &err);
        if (err == CL_SUCCESS) {

            for (int i = 0; i < NUM_KERNEL; i++) {
                std::string cu_id = std::to_string(i + 1);
                std::string krnl_name_full =
                        krnl_name + ":{" + "waxpby_" + cu_id + "}";

                //Here Kernel object is created by specifying kernel name along with compute unit.
                //For such case, this kernel object can only access the specific Compute unit

                OCL_CHECK(err,
                krnls[i] = cl::Kernel(
                program, krnl_name_full.c_str(), &err));
            }
            valid_device++;
            break; // we break because we found a valid device
        }
    }
    if (valid_device == 0) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }

    std::vector<cl_mem_ext_ptr_t> inBufExt1(NUM_KERNEL);
    std::vector<cl_mem_ext_ptr_t> inBufExt2(NUM_KERNEL);
    std::vector<cl_mem_ext_ptr_t> outWAXPBYBufExt(NUM_KERNEL);

    std::vector<cl::Buffer> buffer_input1(NUM_KERNEL);
    std::vector<cl::Buffer> buffer_input2(NUM_KERNEL);
    std::vector<cl::Buffer> buffer_output_waxpby(NUM_KERNEL);

    // For Allocating Buffer to specific Global Memory Bank, user has to use cl_mem_ext_ptr_t
    // and provide the Banks
    for (int i = 0; i < NUM_KERNEL; i++) {
        inBufExt1[i].obj = source_in1[i].data();
        inBufExt1[i].param = 0;
        inBufExt1[i].flags = bank[i * 3];

        inBufExt2[i].obj = source_in2[i].data();
        inBufExt2[i].param = 0;
        inBufExt2[i].flags = bank[(i * 3) + 1];

        outWAXPBYBufExt[i].obj = source_hw_waxpby_results[i].data();
        outWAXPBYBufExt[i].param = 0;
        outWAXPBYBufExt[i].flags = bank[(i * 3) + 2];
    }

    // These commands will allocate memory on the FPGA. The cl::Buffer objects can
    // be used to reference the memory locations on the device.
    //Creating Buffers
    for (int i = 0; i < NUM_KERNEL; i++) {
        OCL_CHECK(err,
                    buffer_input1[i] =            
                    cl::Buffer(context,
                    CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX |
                    CL_MEM_USE_HOST_PTR,
                    sizeof(synt_type) * dataSize,
                    &inBufExt1[i],
                    &err));
        OCL_CHECK(err,
                    buffer_input2[i] =
                    cl::Buffer(context,
                    CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX |
                    CL_MEM_USE_HOST_PTR,
                    sizeof(synt_type) * dataSize,
                    &inBufExt2[i],
                    &err));
        OCL_CHECK(err,
                    buffer_output_waxpby[i] =
                    cl::Buffer(context,
                    CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX |
                    CL_MEM_USE_HOST_PTR,
                    sizeof(synt_type) * dataSize,
                    &outWAXPBYBufExt[i],
                    &err));
    }
    // Copy input data to Device Global Memory
    for (int i = 0; i < NUM_KERNEL; i++) {
        OCL_CHECK(err,
        err = q.enqueueMigrateMemObjects(
        {buffer_input1[i], buffer_input2[i]},
        0 /* 0 means from host*/));
    }
    q.finish();
    double kernel_time_in_sec = 0;

    std::chrono::duration<double> kernel_time(0);

    auto kernel_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_KERNEL; i++) {
        //Setting the k_vadd Arguments
        OCL_CHECK(err, err = krnls[i].setArg(0, buffer_input1[i]));
        OCL_CHECK(err, err = krnls[i].setArg(1, buffer_input2[i]));
        OCL_CHECK(err, err = krnls[i].setArg(2, buffer_output_waxpby[i]));
        OCL_CHECK(err, err = krnls[i].setArg(3, alpha));
        OCL_CHECK(err, err = krnls[i].setArg(4, beta));
        OCL_CHECK(err, err = krnls[i].setArg(5, dataSize));
        //Invoking the kernel
        OCL_CHECK(err, err = q.enqueueTask(krnls[i]));
    }
    q.finish();
    auto kernel_end = std::chrono::high_resolution_clock::now();

    kernel_time = std::chrono::duration<double>(kernel_end - kernel_start);

    kernel_time_in_sec = kernel_time.count();
    // std::cout<< "TIME: "<<kernel_time_in_sec <<std::endl;

    // Copy Result from Device Global Memory to Host Local Memory
    for (int i = 0; i < NUM_KERNEL; i++) {
        OCL_CHECK(err,
            err = q.enqueueMigrateMemObjects(
            {buffer_output_waxpby[i]},
            CL_MIGRATE_MEM_OBJECT_HOST));
    }
    q.finish();

    for (unsigned long i = 0; i < NUM_KERNEL; i++) {
        for(long j = 0; j < dataSize; j++){
        	w.values[j+i*dataSize] = source_hw_waxpby_results[i][j]; 
        }
    }
    
    time_FPGA += kernel_time_in_sec;

    return 0;
}