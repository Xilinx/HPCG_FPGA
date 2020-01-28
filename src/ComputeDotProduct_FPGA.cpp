#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <chrono>

// This extension file is required for stream APIs
#include "CL/cl_ext_xilinx.h"
// This file is required for OpenCL C++ wrapper APIs
#include "xcl2.hpp"
// File containing common info
#include "common.h"

#include "ComputeDotProduct_FPGA.hpp"

#ifndef HPCG_NO_MPI
#include <mpi.h>
#include "mytimer.hpp"
#endif
#ifndef HPCG_NO_OPENMP
#include <omp.h>
#endif

// decltype(&clCreateStream) xcl::Stream::createStream = nullptr;
// decltype(&clReleaseStream) xcl::Stream::releaseStream = nullptr;
// decltype(&clReadStream) xcl::Stream::readStream = nullptr;
// decltype(&clWriteStream) xcl::Stream::writeStream = nullptr;
// decltype(&clPollStreams) xcl::Stream::pollStreams = nullptr;

int ComputeDotProduct_FPGA(const local_int_t n, const Vector & x, const Vector & y,
    double & result, double & time_allreduce){

	unsigned long size = n;

	auto binaryFile = "../bitstreams/hw/vecdotprod.xclbin";

	std::vector<synt_type, aligned_allocator<synt_type>> h_a(size);
    std::vector<synt_type, aligned_allocator<synt_type>> h_b(size);
    std::vector<synt_type, aligned_allocator<synt_type>> hw_results(size);

    for(unsigned int i = 0; i < size; i++){
    	h_a[i] = x.values[i];
    	h_b[i] = y.values[i];
    }

    hw_results[0] = 0.0;

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
                err, krnl = cl::Kernel(program, "vecdotprod", &err));
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
    size_t vector_size_bytes = size * sizeof(synt_type);

    // Streams
    // Device Connection specification of the stream through extension pointer
    cl_int ret;
    cl_mem_ext_ptr_t ext;
    ext.param = krnl.get();
    ext.obj = NULL;

    //Create write stream for argument 0 and 1 of kernel
    cl_stream write_stream_a, write_stream_b;
    ext.flags = 0;
    OCL_CHECK(ret,
              write_stream_a = xcl::Stream::createStream(
                  device.get(), CL_STREAM_WRITE_ONLY, CL_STREAM, &ext, &ret));
    ext.flags = 1;
    OCL_CHECK(ret,
              write_stream_b = xcl::Stream::createStream(
                  device.get(), CL_STREAM_WRITE_ONLY, CL_STREAM, &ext, &ret));

    //Create read stream for argument 2 of kernel
    cl_stream read_stream;
    ext.flags = 2;
    OCL_CHECK(ret,
              read_stream = xcl::Stream::createStream(
                  device.get(), CL_STREAM_READ_ONLY, CL_STREAM, &ext, &ret));

    // Launch the Kernel
    cl::Event nb_wait_event;
    OCL_CHECK(err, err = q.enqueueTask(krnl, NULL, &nb_wait_event));

    // Initiating the WRITE transfer
    cl_stream_xfer_req nb_wr_req{0};

    nb_wr_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
    nb_wr_req.priv_data = (void *)"nb_write_a";

    // Writing data to input stream 1 independently in case of non-blocking transfers.
    OCL_CHECK(
        ret,
        xcl::Stream::writeStream(
            write_stream_a, h_a.data(), vector_size_bytes, &nb_wr_req, &ret));

    nb_wr_req.priv_data = (void *)"nb_write_b";
    // Writing data to input stream 2 independently in case of non-blocking transfers.
    OCL_CHECK(
        ret,
        xcl::Stream::writeStream(
            write_stream_b, h_b.data(), vector_size_bytes, &nb_wr_req, &ret));

    // Initiating the READ transfer
    cl_stream_xfer_req nb_rd_req{0};
    nb_rd_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
    nb_rd_req.priv_data = (void *)"nb_read";
    // Reading the stream data independently in case of non-blocking transfers.
    OCL_CHECK(ret,
              xcl::Stream::readStream(read_stream,
                                      hw_results.data(),
                                      sizeof(synt_type),
                                      &nb_rd_req,
                                      &ret));

    // Checking the request completion
    cl_streams_poll_req_completions poll_req[3]{0, 0, 0}; // 3 Requests
    auto num_compl = 3;
    OCL_CHECK(ret,
              xcl::Stream::pollStreams(
                  device.get(), poll_req, 3, 3, &num_compl, 500000, &ret));

    q.finish();
    double local_result = hw_results[0];
    // Releasing Streams
    xcl::Stream::releaseStream(read_stream);
    xcl::Stream::releaseStream(write_stream_a);
    xcl::Stream::releaseStream(write_stream_b);
    #ifndef HPCG_NO_MPI
    // Use MPI's reduce function to collect all partial sums
        double t0 = mytimer();
        double global_result = 0.0;
        MPI_Allreduce(&local_result, &global_result, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        result = global_result;
        time_allreduce += mytimer() - t0;
    #else
        time_allreduce += 0.0;
        result = local_result;
    #endif  
    return 0;
}