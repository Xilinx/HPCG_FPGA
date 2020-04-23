#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

// This extension file is required for stream APIs
#include "CL/cl_ext_xilinx.h"
// This file is required for OpenCL C++ wrapper APIs
#include "xcl2.hpp"

#include "common.hpp"
#define max_iter 55
#define NUM_KERNEL 10
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

///////////////////VERIFY FUNCTION///////////////////////////////////
bool verify(int *sw_results, int *hw_results, int size) {
    bool match = true;
    for (int i = 0; i < size; i++) {
        if (sw_results[i] != hw_results[i]) {
            match = false;
            break;
        }
    }
    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
    return match;
}
////////MAIN FUNCTION//////////
int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <XCLBIN> \n", argv[0]);
        return -1;
    }

    unsigned int dataSize = 32*1024*1024; //taking maximum possible data size value for an HBM bank

    //reducing the test data capacity to run faster in emulation mode
    if (xcl::is_emulation()) {
        dataSize = 1024;
    }

    std::vector<synt_type, aligned_allocator<synt_type>> A(dataSize);
    std::vector<synt_type, aligned_allocator<synt_type>> r(dataSize);
    std::vector<synt_type, aligned_allocator<synt_type>> source_sw_waxpby_results(dataSize);

    // Create the test data
    std::generate(A.begin(), A.end(), std::rand);
    std::generate(r.begin(), r.end(), std::rand);
    synt_type alpha = std::rand();
    synt_type beta = std::rand();
    for (size_t i = 0; i < dataSize; i++) {
        source_sw_waxpby_results[i] = A[i]*alpha + r[i]*beta;
    // source_sw_waxpby_results[i] = source_in1[i] + source_in2[i];
    }


    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    // OpenCL Host Code Begins.
    cl_int err;

    // OpenCL objects
    cl::Device device;
    cl::Context context;
    cl::CommandQueue q;
    cl::Program program;
    std::vector<cl::Kernel> krnl_precond_0(NUM_KERNEL);
    std::vector<cl::Kernel> krnl_dotprod_0(NUM_KERNEL);
    std::vector<cl::Kernel> krnl_dotprod(NUM_KERNEL);
    std::vector<cl::Kernel> krnl_spmv_0(NUM_KERNEL);
    std::vector<cl::Kernel> krnl_spmv(NUM_KERNEL);
    std::vector<cl::Kernel> krnl_waxpby_0(NUM_KERNEL);
    std::vector<cl::Kernel> krnl_waxpby(NUM_KERNEL);

    auto binaryFile = argv[1];

    // get_xil_devices() is a utility API which will find the xilinx
    // platforms and will return list of devices connected to Xilinx platform
    auto devices = xcl::get_xil_devices();

    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    int valid_device = 0;
    for (unsigned int i = 0; i < devices.size(); i++) {
        device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, NULL, NULL, NULL, &err));
        OCL_CHECK(
            err,
            q = cl::CommandQueue(context,
                                 device,
                                 CL_QUEUE_PROFILING_ENABLE |
                                     CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE,
                                 &err));
        std::cout << "Trying to program device[" << i
                  << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        cl::Program program(context, {device}, bins, NULL, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i
                      << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            // Creating Kernels


            for (int i = 0; i < NUM_KERNEL; i++) {
                std::string cu_id = std::to_string(i + 1);
                
                //Here Kernel object is created by specifying kernel name along with compute unit.
                //For such case, this kernel object can only access the specific Compute unit

                std::string krnl_name = "precond0";
                std::string krnl_name_full =
                        krnl_name + ":{" + "precond0_" + cu_id + "}";
                printf("Creating a kernel [%s] for CU(%d)\n",
                             krnl_name_full.c_str(),
                             i + 1);
                OCL_CHECK(err,
                krnl_precond_0[i] = cl::Kernel(
                program, krnl_name_full.c_str(), &err));

                krnl_name = "dotprod0";
                krnl_name_full =
                        krnl_name + ":{" + "dotprod0_" + cu_id + "}";
                printf("Creating a kernel [%s] for CU(%d)\n",
                             krnl_name_full.c_str(),
                             i + 1);
                OCL_CHECK(err,
                krnl_dotprod_0[i] = cl::Kernel(
                program, krnl_name_full.c_str(), &err));

                krnl_name = "dotprod";
                krnl_name_full =
                        krnl_name + ":{" + "dotprod_" + cu_id + "}";
                printf("Creating a kernel [%s] for CU(%d)\n",
                             krnl_name_full.c_str(),
                             i + 1);
                OCL_CHECK(err,
                krnl_dotprod[i] = cl::Kernel(
                program, krnl_name_full.c_str(), &err));

                krnl_name = "spmv0";
                krnl_name_full =
                        krnl_name + ":{" + "spmv0_" + cu_id + "}";
                printf("Creating a kernel [%s] for CU(%d)\n",
                             krnl_name_full.c_str(),
                             i + 1);
                OCL_CHECK(err,
                krnl_spmv_0[i] = cl::Kernel(
                program, krnl_name_full.c_str(), &err));

                krnl_name = "spmv";
                krnl_name_full =
                        krnl_name + ":{" + "spmv_" + cu_id + "}";
                printf("Creating a kernel [%s] for CU(%d)\n",
                             krnl_name_full.c_str(),
                             i + 1);
                OCL_CHECK(err,
                krnl_spmv[i] = cl::Kernel(
                program, krnl_name_full.c_str(), &err));

                krnl_name = "waxpby0";
                krnl_name_full =
                        krnl_name + ":{" + "waxpby0_" + cu_id + "}";
                printf("Creating a kernel [%s] for CU(%d)\n",
                             krnl_name_full.c_str(),
                             i + 1);
                OCL_CHECK(err,
                krnl_waxpby_0[i] = cl::Kernel(
                program, krnl_name_full.c_str(), &err));

                krnl_name = "waxpby";
                krnl_name_full =
                        krnl_name + ":{" + "waxpby_" + cu_id + "}";
                printf("Creating a kernel [%s] for CU(%d)\n",
                             krnl_name_full.c_str(),
                             i + 1);
                OCL_CHECK(err,
                krnl_waxpby[i] = cl::Kernel(
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

    std::chrono::duration<double> pcie_time(0);
    auto pcie_start = std::chrono::high_resolution_clock::now();

    //HBM data
    std::vector<cl_mem_ext_ptr_t> inBufExt1(NUM_KERNEL);
    std::vector<cl_mem_ext_ptr_t> inBufExt2(NUM_KERNEL);

    std::vector<cl::Buffer> buffer_input1(NUM_KERNEL);
    std::vector<cl::Buffer> buffer_input2(NUM_KERNEL);

    // For Allocating Buffer to specific Global Memory Bank, user has to use cl_mem_ext_ptr_t
    // and provide the Banks
    for (int i = 0; i < NUM_KERNEL; i++) {
        inBufExt1[i].obj = r.data();
        inBufExt1[i].param = 0;
        inBufExt1[i].flags = bank[i * 2];

        inBufExt2[i].obj = A.data();
        inBufExt2[i].param = 0;
        inBufExt2[i].flags = bank[(i * 2) + 1];
    }

    //Running the kernel

    std::cout << "Performing CG" << std::endl;

    // Allocate Buffer in Global Memory
    // Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and
    // Device-to-host communication
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
    }
    // Copy input data to Device Global Memory
    for (int i = 0; i < NUM_KERNEL; i++) {
        OCL_CHECK(err,
        err = q.enqueueMigrateMemObjects(
        {buffer_input1[i], buffer_input2[i]},
        0 /* 0 means from host*/));
    }
    q.finish();
    double kernel_time_in_sec = 0, pcie_time_in_sec = 0;

    std::chrono::duration<double> kernel_time(0);

    auto kernel_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_KERNEL; i++) {
        //Setting the kernels Arguments

        //The preconditioning kernel needs r, which resides in buffer_input1
        // and its size (per CU)
        OCL_CHECK(err, err = krnl_precond_0[i].setArg(0, buffer_input1[i]));
        OCL_CHECK(err, err = krnl_precond_0[i].setArg(0, dataSize));
        

        //spmv needs A
        OCL_CHECK(err, err = krnl_spmv[i].setArg(0, buffer_input2[i]));
    }
    for (int i = 0; i < NUM_KERNEL; i++) {
        for(int j = 0; j < max_iter; j++){
            //Invoking the kernels
            OCL_CHECK(err, err = q.enqueueTask(krnl_precond_0[i]));
            if(i==0){
                OCL_CHECK(err, err = q.enqueueTask(krnl_waxpby_0[i]));
                OCL_CHECK(err, err = q.enqueueTask(krnl_dotprod_0[i]));
                OCL_CHECK(err, err = q.enqueueTask(krnl_waxpby_0[i]));// I need this so I can have z in streaming
            }
            else{
                
            }
            OCL_CHECK(err, err = q.enqueueTask(krnl_spmv[i]));

        }
    }
    q.finish();

    //Time spent on board
    auto kernel_end = std::chrono::high_resolution_clock::now();
    kernel_time = std::chrono::duration<double>(kernel_end - kernel_start);
    kernel_time_in_sec = kernel_time.count();


    // Copy Result from Device Global Memory to Host Local Memory
    // OCL_CHECK(err,
    //           err = q.enqueueMigrateMemObjects({buffer_output},
    //                                            CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();

    auto pcie_end = std::chrono::high_resolution_clock::now();
    pcie_time = std::chrono::duration<double>(pcie_end - pcie_start);
    pcie_time_in_sec = pcie_time.count();
    bool match = true;
    // OpenCL Host Code Ends

    // Compare the device results with software results
    //bool match = verify(sw_results.data(), hw_results.data(), size);
    std::cout << "TIME HBM = " << kernel_time_in_sec << "s" << std::endl;
    std::cout << "TIME PCIe = " << pcie_time_in_sec << "s" << std::endl;
    //OPENCL HOST CODE AREA ENDS

    std::cout << (match ? "TEST PASSED" : "TEST FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);

}