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

// Declaration of custom stream APIs that binds to Xilinx Streaming APIs.
// decltype(&clCreateStream) xcl::Stream::createStream = nullptr;
// decltype(&clReleaseStream) xcl::Stream::releaseStream = nullptr;
// decltype(&clReadStream) xcl::Stream::readStream = nullptr;
// decltype(&clWriteStream) xcl::Stream::writeStream = nullptr;
// decltype(&clPollStreams) xcl::Stream::pollStreams = nullptr;


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
    for(unsigned long i = 0; i < size; i++){
        // sw_results[i] = 0;
        hw_results[i] = 0;
    }
    // std::cout<<";-D"<<std::endl;


    return 0;
}


int ComputeSPMV_FPGA( const SparseMatrix & A, synt_type *m_v, Vector & x, Vector & y){
    // if (xcl::is_hw_emulation()) {
    //     size = 16; // 4KB for HW emulation
    // } else if (xcl::is_emulation()) {
    //     size = 16; // 4MB for sw emulation
    // }

    assert(x.localLength>=A.localNumberOfColumns); // Test vector lengths
    assert(y.localLength>=A.localNumberOfRows);


    unsigned size = A.localNumberOfRows;

    // I/O Data Vectors
    std::vector<synt_type, aligned_allocator<synt_type>> h_m(MAXNONZEROELEMENTS * size);//same size for testing convenience
    std::vector<synt_type, aligned_allocator<synt_type>> h_x(MAXNONZEROELEMENTS * size);
    std::vector<synt_type, aligned_allocator<synt_type>> hw_results(size);
    // std::vector<synt_type> sw_results(size);

    // std::cout<<"CREATED"<<std::endl;
    auto binaryFile = "../bitstreams/hw/spmv.xclbin";


    // Reset the data vectors
    reset(h_m.data(), h_x.data(), hw_results.data(), m_v, x, size);

    // std::cout << size << std::endl;
    // std::cout<<"OK"<<std::endl;
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

    // std::cout
    //     << "############################################################\n";
    // std::cout
    //     << "                  Non-Blocking Stream                       \n";
    // std::cout
    //     << "############################################################\n";
    // std::cout<<"HW input"<<std::endl;
    // Launch the Kernel
    // auto t1 = NOW;
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
                  device.get(), poll_req, 3, 3, &num_compl, 500000, &ret));
    // Blocking API, waits for 3 poll request completion or 50000ms, whichever occurs first.

    // auto t2 = NOW;
    // std::cout<<"DONE"<<std::endl;

    // for (auto i=0; i<STREAMS; ++i) {
    //     if(nb_rd_req.priv_data == poll_req[i].priv_data) { // Identifying the read transfer

    //         //Getting read size, data size from kernel is unknown
    //         ssize_t result_size=poll_req[i].nbytes;
    //         std::cout << "RESULT SIZE " << result_size << std::endl;
    //     }
    // }


    // Ensuring all OpenCL objects are released.
    q.finish();
    // Releasing Streams
    xcl::Stream::releaseStream(read_stream);
    xcl::Stream::releaseStream(write_stream_m);
    xcl::Stream::releaseStream(write_stream_x);
    
    for(unsigned i = 0; i < A.localNumberOfRows; i++)
        y.values[i]=hw_results[i];
    // std::cout<<"OK2"<<std::endl;
    return 0;

}
