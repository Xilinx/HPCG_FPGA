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
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <chrono>
#include "Vector.hpp"
#include "SparseMatrix.hpp"
#include "common.h"

#define MAXNONZEROELEMENTS 32

#define NOW std::chrono::high_resolution_clock::now()

// This extension file is required for stream APIs
#include "CL/cl_ext_xilinx.h"
// This file is required for OpenCL C++ wrapper APIs
#include "xcl2.hpp"
// File containing common info
#include "common.h"

#include "ComputeSPMV_FPGA.hpp"

#ifndef HPCG_NO_MPI
#include "ExchangeHalo.hpp"
#endif

#ifndef HPCG_NO_OPENMP
#include <omp.h>
#endif
#include <cassert>
#include <iostream>

////////////////////RESET FUNCTION//////////////////////////////////
int reset(synt_type *m, synt_type *x, synt_type *hw_results, synt_type *m_v, Vector & x_v, unsigned long size) {
    //Fill the input vectors with data
    // std::cout<<"IN RESET"<<std::endl;

    for (unsigned long i = 0; i < size*MAXNONZEROELEMENTS; i++) {
        m[i] = m_v[i];//LOW + static_cast <synt_type> (rand()) /( static_cast <synt_type> (RAND_MAX/(HIGH-LOW)));
        x[i] = x_v.val_spmv[i];//x_v.val_spmv[i];//LOW + static_cast <synt_type> (rand()) /( static_cast <synt_type> (RAND_MAX/(HIGH-LOW)));
        // std::cout<<A.flat_matrixValues[i]<<std::endl;
        
        // std::cout<<i<<std::endl;
    }

    return 0;
}


int ComputeSPMV_FPGA( const SparseMatrix & A, synt_type *m_v, Vector & x, Vector & y){

    assert(x.localLength>=A.localNumberOfColumns); // Test vector lengths
    assert(y.localLength>=A.localNumberOfRows);


    unsigned size = A.localNumberOfRows;

    // I/O Data Vectors
    std::vector<synt_type, aligned_allocator<synt_type>> h_m(MAXNONZEROELEMENTS * size);//same size for testing convenience
    std::vector<synt_type, aligned_allocator<synt_type>> h_x(MAXNONZEROELEMENTS * size);
    std::vector<synt_type, aligned_allocator<synt_type>> hw_results(size,0);
    
    auto binaryFile = "../bitstreams/hw/single_bitstream/build_dir.hw.xilinx_u250_qdma_201920_1/cg.xclbin";


    // Reset the data vectors
    reset(h_m.data(), h_x.data(), hw_results.data(), m_v, x, size);

    // OpenCL Setup
    // OpenCL objects
    cl::Device device;
    cl::Context context;
    cl::CommandQueue q;
    cl::Program program;
    cl::Kernel krnl;

    // Error Status variable
    cl_int err;

    // get_xil_devices() is a utility API which will find the xilinx
    // platforms and will return list of devices connected to Xilinx platform
    auto devices = xcl::get_xil_devices();

    // Selecting the first available Xilinx device
    device = devices[0];
    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);

    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    int valid_device = 0;
    for (unsigned int i = 0; i < devices.size(); i++) {
        device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context({device}, NULL, NULL, NULL, &err));
        OCL_CHECK(err,
                  q = cl::CommandQueue(
                      context, {device}, CL_QUEUE_PROFILING_ENABLE, &err));

        cl::Program program(context, {device}, bins, NULL, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i
                      << "] with xclbin file!\n";
        } else {
            OCL_CHECK(
                err, krnl = cl::Kernel(program, "spmv", &err));
            valid_device++;
            break; // we break because we found a valid device
        }
    }

    if (valid_device == 0) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }

    auto platform_id = device.getInfo<CL_DEVICE_PLATFORM>(&err);

    //Initialization of streaming class is needed before using it.
    xcl::Stream::init(platform_id);
    size_t vector_input_bytes = MAXNONZEROELEMENTS * size * sizeof(synt_type);
    size_t vector_size_bytes = size * sizeof(synt_type);

    // Streams
    // Device Connection specification of the stream through extension pointer
    cl_int ret;
    cl_mem_ext_ptr_t ext;
    ext.param = krnl.get();
    ext.obj = NULL;

    //Create write stream for argument 0 and 1 of kernel
    cl_stream write_stream_m, write_stream_x;
    ext.flags = 0;
    OCL_CHECK(ret,
              write_stream_m = xcl::Stream::createStream(
                  device.get(), CL_STREAM_WRITE_ONLY, CL_STREAM, &ext, &ret));
    ext.flags = 1;
    OCL_CHECK(ret,
              write_stream_x = xcl::Stream::createStream(
                  device.get(), CL_STREAM_WRITE_ONLY, CL_STREAM, &ext, &ret));

    //Create read stream for argument 2 of kernel
    cl_stream read_stream;
    ext.flags = 2;
    OCL_CHECK(ret,
              read_stream = xcl::Stream::createStream(
                  device.get(), CL_STREAM_READ_ONLY, CL_STREAM, &ext, &ret));

    //Set n rows
    int s = size;
    OCL_CHECK(err, err = krnl.setArg(3, s));

    cl::Event nb_wait_event;
    OCL_CHECK(err, err = q.enqueueTask(krnl, NULL, &nb_wait_event));

    // Initiating the WRITE transfer
    cl_stream_xfer_req nb_wr_req{0};

    nb_wr_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
    nb_wr_req.priv_data = (void *)"nb_write_m";

    // Writing data to input stream 1 independently in case of non-blocking transfers.
    OCL_CHECK(
        ret,
        xcl::Stream::writeStream(
            write_stream_m, h_m.data(), vector_input_bytes, &nb_wr_req, &ret));

    nb_wr_req.priv_data = (void *)"nb_write_x";
    // Writing data to input stream 2 independently in case of non-blocking transfers.
    OCL_CHECK(
        ret,
        xcl::Stream::writeStream(
            write_stream_x, h_x.data(), vector_input_bytes, &nb_wr_req, &ret));

    // Initiating the READ transfer
    cl_stream_xfer_req nb_rd_req{0};
    nb_rd_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
    nb_rd_req.priv_data = (void *)"nb_read";
    // Reading the stream data independently in case of non-blocking transfers.
    OCL_CHECK(ret,
              xcl::Stream::readStream(read_stream,
                                      hw_results.data(),
                                      vector_size_bytes,
                                      &nb_rd_req,
                                      &ret));

    // Checking the request completion
    cl_streams_poll_req_completions poll_req[3]{0, 0, 0}; // 3 Requests
    auto num_compl = 3;
    OCL_CHECK(ret,
              xcl::Stream::pollStreams(
                  device.get(), poll_req, 3, 3, &num_compl, 5000000, &ret));

    // Ensuring all OpenCL objects are released.
    q.finish();
    // Releasing Streams
    xcl::Stream::releaseStream(read_stream);
    xcl::Stream::releaseStream(write_stream_m);
    xcl::Stream::releaseStream(write_stream_x);
    
    for(unsigned i = 0; i < A.localNumberOfRows; i++)
        y.values[i]=hw_results[i];
    
    return 0;

}
