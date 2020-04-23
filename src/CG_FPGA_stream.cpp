
//@HEADER
// ***************************************************
//
// HPCG: High Performance Conjugate Gradient Benchmark
//
// Contact:
// Michael A. Heroux ( maherou@sandia.gov)
// Jack Dongarra     (dongarra@eecs.utk.edu)
// Piotr Luszczek    (luszczek@eecs.utk.edu)
//
// ***************************************************
//@HEADER

/*!
 @file CG.cpp

 HPCG routine
 */

#include <fstream>

#include <cmath>

#include "hpcg.hpp"
#include "Vector.hpp"
#include "CG_FPGA_stream.hpp"
#include "mytimer.hpp"
#include "ComputeSPMV.hpp"
#include "ComputeMG.hpp"
#include "ComputeDotProduct.hpp"
#include "ComputeWAXPBY.hpp"
#include "PrepareVector.hpp"
#include <iostream>

// This extension file is required for stream APIs
#include "CL/cl_ext_xilinx.h"
// This file is required for OpenCL C++ wrapper APIs
#include "xcl2.hpp"
// File containing common info
#include "common.h"

#define MAXNONZEROELEMENTS 32

// Use TICK and TOCK to time a code section in MATLAB-like fashion
#define TICK()  t0 = mytimer() //!< record current time in 't0'
#define TOCK(t) t += mytimer() - t0 //!< store time difference in 't' using time in 't0'

/*!
  Routine to compute an approximate solution to Ax = b

  @param[in]    geom The description of the problem's geometry.
  @param[inout] A    The known system matrix
  @param[inout] data The data structure with all necessary CG vectors preallocated
  @param[in]    b    The known right hand side vector
  @param[inout] x    On entry: the initial guess; on exit: the new approximate solution
  @param[in]    max_iter  The maximum number of iterations to perform, even if tolerance is not met.
  @param[in]    tolerance The stopping criterion to assert convergence: if norm of residual is <= to tolerance.
  @param[out]   niters    The number of iterations actually performed.
  @param[out]   normr     The 2-norm of the residual vector after the last iteration.
  @param[out]   normr0    The 2-norm of the residual vector before the first iteration.
  @param[out]   times     The 7-element vector of the timing information accumulated during all of the iterations.
  @param[in]    doPreconditioning The flag to indicate whether the preconditioner should be invoked at each iteration.

  @return Returns zero on success and a non-zero value otherwise.

  @see CG_ref()
*/

void execute_SPMV_FPGA(cl::CommandQueue q, cl::Device device, cl::Kernel krnl, unsigned int size, std::vector<synt_type, aligned_allocator<synt_type>> &h_m, std::vector<synt_type, aligned_allocator<synt_type>> &h_x, std::vector<synt_type, aligned_allocator<synt_type>> &hw_results){

  size_t vector_input_bytes = MAXNONZEROELEMENTS * size * sizeof(synt_type);
  size_t vector_size_bytes = size * sizeof(synt_type);

  // Streams
  // Device Connection specification of the stream through extension pointer
  cl_int ret;
  cl_int err;
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
                device.get(), poll_req, 3, 3, &num_compl, 500000, &ret));

  // Ensuring all OpenCL objects are released.
  q.finish();
  // Releasing Streams
  xcl::Stream::releaseStream(read_stream);
  xcl::Stream::releaseStream(write_stream_m);
  xcl::Stream::releaseStream(write_stream_x);
}

void execute_WAXPBY_FPGA(cl::CommandQueue q ,cl::Device device, cl::Kernel krnl, unsigned int size, double alpha, std::vector<synt_type, aligned_allocator<synt_type>> &h_a, double beta, std::vector<synt_type, aligned_allocator<synt_type>> &h_b, std::vector<synt_type, aligned_allocator<synt_type>> &hw_results){
  size_t vector_size_bytes = size * sizeof(synt_type);

  // Streams
  // Device Connection specification of the stream through extension pointer
  cl_int ret;
  cl_int err;
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

  //Set alpha and beta
  OCL_CHECK(err, err = krnl.setArg(3, alpha));
  OCL_CHECK(err, err = krnl.setArg(4, beta));

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
                                    vector_size_bytes,
                                    &nb_rd_req,
                                    &ret));

  // Checking the request completion
  cl_streams_poll_req_completions poll_req[3]{0, 0, 0}; // 3 Requests
  auto num_compl = 3;
  OCL_CHECK(ret,
            xcl::Stream::pollStreams(
                device.get(), poll_req, 3, 3, &num_compl, 500000, &ret));

  // Ensuring all OpenCL objects are released.
  q.finish();
  xcl::Stream::releaseStream(read_stream);
  xcl::Stream::releaseStream(write_stream_a);
  xcl::Stream::releaseStream(write_stream_b);
}

void execute_DOTPROD_FPGA(cl::CommandQueue q, cl::Device device, cl::Kernel krnl, unsigned int size, std::vector<synt_type, aligned_allocator<synt_type>> &h_a, std::vector<synt_type, aligned_allocator<synt_type>> &h_b, std::vector<synt_type, aligned_allocator<synt_type>> &hw_results){
  size_t vector_size_bytes = size * sizeof(synt_type);

  // Streams
  // Device Connection specification of the stream through extension pointer
  cl_int ret;
  cl_int err;
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
  // Releasing Streams
  xcl::Stream::releaseStream(read_stream);
  xcl::Stream::releaseStream(write_stream_a);
  xcl::Stream::releaseStream(write_stream_b);
}

int CG_FPGA_stream(const SparseMatrix & A, CGData & data, const Vector & b, Vector & x, const int max_iter, const double tolerance, int & niters, double & normr, double & normr0, double * times, bool doPreconditioning) {
  
  double t_begin = mytimer();  // Start timing right away
  normr = 0.0;
  double rtz = 0.0, oldrtz = 0.0, alpha = 0.0, beta = 0.0, pAp = 0.0;


  double t0 = 0.0, t1 = 0.0, t2 = 0.0, t3 = 0.0, t4 = 0.0, t5 = 0.0;
//#ifndef HPCG_NO_MPI
//  double t6 = 0.0;
//#endif
  local_int_t nrow = A.localNumberOfRows;
  Vector & r = data.r; // Residual vector
  Vector & z = data.z; // Preconditioned residual vector
  Vector & p = data.p; // Direction vector (in MPI mode ncol>=nrow)
  Vector & Ap = data.Ap;
  if (!doPreconditioning && A.geom->rank==0) HPCG_fout << "WARNING: PERFORMING UNPRECONDITIONED ITERATIONS" << std::endl;

#ifdef HPCG_DEBUG
  int print_freq = 1;
  if (print_freq>50) print_freq=50;
  if (print_freq<1)  print_freq=1;
#endif

  auto binaryFile = "../bitstreams/hw/single_bitstream/build_dir.hw.xilinx_u250_qdma_201920_1/cg.xclbin";

  // I/O Data Vectors for spmv
  unsigned size_spmv = A.localNumberOfRows;
  unsigned size_waxpby = A.localNumberOfRows;
  unsigned size_dotprod = A.localNumberOfRows;
  std::vector<synt_type, aligned_allocator<synt_type>> a_vector(MAXNONZEROELEMENTS * size_spmv);//same size for testing convenience
  std::vector<synt_type, aligned_allocator<synt_type>> vector_spmv(MAXNONZEROELEMENTS * size_spmv);
  std::vector<synt_type, aligned_allocator<synt_type>> spmv_results(size_spmv,0);

  std::vector<synt_type, aligned_allocator<synt_type>> vector_x_waxpby(size_waxpby);
  std::vector<synt_type, aligned_allocator<synt_type>> vector_y_waxpby(size_waxpby);
  std::vector<synt_type, aligned_allocator<synt_type>> waxpby_results(size_waxpby,0);

  std::vector<synt_type, aligned_allocator<synt_type>> vector_x_dotprod(size_dotprod);
  std::vector<synt_type, aligned_allocator<synt_type>> vector_y_dotprod(size_dotprod);
  std::vector<synt_type, aligned_allocator<synt_type>> dotprod_results(size_dotprod,0  );
  
  // OpenCL Setup
  // OpenCL objects
  cl::Device device;
  cl::Context context;
  cl::CommandQueue q;
  cl::Program program;
  cl::Kernel krnl_spmv, krnl_waxpby, krnl_vdot;

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
              err, krnl_spmv = cl::Kernel(program, "spmv", &err));
          OCL_CHECK(
              err, krnl_waxpby = cl::Kernel(program, "vecsumprod", &err));//consider changing the name to waxpby -- no change to the code but that implies rebuilding the kernel
          OCL_CHECK(
              err, krnl_vdot = cl::Kernel(program, "vecdotprod", &err));
          valid_device++;
          break; // we break because we found a valid device
      }
  }

  if (valid_device == 0) {
      std::cout << "Failed to program any device found, exit!\n";
      exit(EXIT_FAILURE);
  }

  auto platform_id = device.getInfo<CL_DEVICE_PLATFORM>(&err);

  xcl::Stream::init(platform_id);
  
  //fill a_vector and flattened p
  // p is of length ncols, copy x to p for sparse MV operation
  CopyVector(x, p);
  prepareVector(p,A,27);
  for (unsigned long i = 0; i < size_spmv*MAXNONZEROELEMENTS; i++) {
        a_vector[i] = A.flat_matrixValues[i];
        vector_spmv[i] = p.val_spmv[i];
  }

  TICK(); execute_SPMV_FPGA(q, device, krnl_spmv, size_spmv, a_vector, vector_spmv, spmv_results); TOCK(t3); // Ap = A*p --> Ap = spmv_results

  //Copying b into waxpby_x_vector
  for(unsigned int i = 0; i < size_waxpby; i++){
      vector_x_waxpby[i] = b.values[i];
  }
  TICK(); execute_WAXPBY_FPGA(q, device, krnl_waxpby, size_waxpby, 1.0, vector_x_waxpby, -1.0, spmv_results, waxpby_results);  TOCK(t2); // r = b - Ax (x stored in p) --> r = waxbpy_results

  //Copying r into vector_y_dotprod
  for(unsigned int i = 0; i < size_dotprod; i++){
      vector_y_dotprod[i] = r.values[i];
  }

  TICK(); execute_DOTPROD_FPGA(q, device, krnl_vdot, size_dotprod, waxpby_results, vector_y_dotprod, dotprod_results); TOCK(t1);

  normr = sqrt(dotprod_results[0]);

#ifdef HPCG_DEBUG
  if (A.geom->rank==0) HPCG_fout << "Initial Residual = "<< normr << std::endl;
#endif

  // Record initial residual for convergence testing
  normr0 = normr;

  // Start iterations

  for (int k=1; k<=max_iter && normr/normr0 > tolerance; k++ ) {
    TICK();
    if (doPreconditioning)
      ComputeMG(A, r, z); // Apply preconditioner
    else
      CopyVector (r, z); // copy r to z (no preconditioning)
    TOCK(t5); // Preconditioner apply time

    if (k == 1) {
      TICK(); ComputeWAXPBY(nrow, 1.0, z, 0.0, z, p, A.isWaxpbyOptimized); TOCK(t2); // Copy Mr to p
      TICK(); ComputeDotProduct (nrow, r, z, rtz, t4, A.isDotProductOptimized); TOCK(t1); // rtz = r'*z
    } else {
      oldrtz = rtz;
      TICK(); ComputeDotProduct (nrow, r, z, rtz, t4, A.isDotProductOptimized); TOCK(t1); // rtz = r'*z
      beta = rtz/oldrtz;
      TICK(); ComputeWAXPBY (nrow, 1.0, z, beta, p, p, A.isWaxpbyOptimized);  TOCK(t2); // p = beta*p + z
    }
    prepareVector(p,A,27);
    TICK(); ComputeSPMV(A, p, Ap); TOCK(t3); // Ap = A*p
    TICK(); ComputeDotProduct(nrow, p, Ap, pAp, t4, A.isDotProductOptimized); TOCK(t1); // alpha = p'*Ap
    alpha = rtz/pAp;
    TICK(); ComputeWAXPBY(nrow, 1.0, x, alpha, p, x, A.isWaxpbyOptimized);// x = x + alpha*p
            ComputeWAXPBY(nrow, 1.0, r, -alpha, Ap, r, A.isWaxpbyOptimized);  TOCK(t2);// r = r - alpha*Ap
    TICK(); ComputeDotProduct(nrow, r, r, normr, t4, A.isDotProductOptimized); TOCK(t1);
    normr = sqrt(normr);
#ifdef HPCG_DEBUG
    if (A.geom->rank==0 && (k%print_freq == 0 || k == max_iter))
      HPCG_fout << "Iteration = "<< k << "   Scaled Residual = "<< normr/normr0 << std::endl;
#endif
    niters = k;
  }

  // Store times
  times[1] += t1; // dot-product time
  times[2] += t2; // WAXPBY time
  times[3] += t3; // SPMV time
  times[4] += t4; // AllReduce time
  times[5] += t5; // preconditioner apply time
//#ifndef HPCG_NO_MPI
//  times[6] += t6; // exchange halo time
//#endif
  times[0] += mytimer() - t_begin;  // Total time. All done...
  return 0;
}
