<p align="center">
  <img width="600" height="313.5" src="https://gitenterprise.xilinx.com/albertoz/hpcg_fpga/blob/master/media/xilinx.jpg">
</p>

# High Performance Conjugate Gradient Benchmark (HPCG) - FPGA #


FPGA version by Alberto Zeni and Kenneth O'Brien - Xilinx Inc.

Based on HPCG:

Original HPCG Revision: 3.1 Date: March 28, 2019

## Introduction ##

HPCG is a software package that performs a fixed number of multigrid preconditioned
(using a symmetric Gauss-Seidel smoother) conjugate gradient (PCG) iterations using double
precision (64 bit) floating point values.

The HPCG rating is is a weighted GFLOP/s (billion floating operations per second) value
that is composed of the operations performed in the PCG iteration phase over
the time taken.  The overhead time of problem construction and any modifications to improve
performance are divided by 500 iterations (the amortization weight) and added to the runtime.

Integer arrays have global and local
scope (global indices are unique across the entire distributed memory system,
local indices are unique within a memory image).  Integer data for global/local
indices have three modes:

* 32/32 - global and local integers are 32-bit
* 64/32 - global integers are 64-bit, local are 32-bit
* 64/64 - global and local are 64-bit.

These various modes are required in order to address sufficiently big problems
if the range of indexing goes above 2^31 (roughly 2.1B), or to conserve storage
costs if the range of indexing is less than 2^31.

The  HPCG  software  package requires the availibility on your system of an
implementation of the  Message Passing Interface (MPI) if enabling the MPI
build of HPCG, and a compiler that supports OpenMP syntax. An implementation
compliant with MPI version 1.1 is sufficient.

## FPGA Implementation ##

The FPGA implementation of HPCG has been developed keeping the same calculations as 
the original software version of HPCG implementing specific optimizations for multiple FPGA execution.
All the source code of the optimized version of HPCG is located in the `src-fpga` directory.
All the code has been optimized for the Alveo U280 FPGA, and can be run using 1 or multiple boards.
The 

## Results ##

Performance obtained using the **Alveo U280**, and the Intel(R) Xeon(R) Gold 6234 CPU @ 3.30GHz with various dataset sizes and different data precision.

| Problem Size 	| Double GFlop/s 	| Single GFlop/s 	| Half GFlop/s 		|
|----------:	|---------------:	|---------------:	|-----------------:	|
|   16^3  		|   106.1			|   215.4 			|	409.4		 	|
|   32^3  		|   107.1			|   215.7 			|	418.0		 	|
|   64^3  		|   103.9			|   213.5 			|	430.9		 	|
|   128^3  		|   107.2			|   217.9			|	422.3		 	|
|   256^3  		|   108.3			|   211.3		 	|	426.6		 	|


Performance comparison of HPCG across different platforms, using the same dataset size with different data precision.

| Platform 				| Problem Size 	| Double GFlop/s 	|
|------------------:	|----------:	|---------------:	|
|   Intel Xeon 6234		|   256^3  		|   1.03			|
|   NVIDIA Tesla K40 	|   256^3 		|   33.0			|
|  	NVIDIA Tesla M40	|   256^3 		|   41.5			|
|  	NVIDIA Tesla P100	|   256^3  		|   100.0			|
|  	NVIDIA Tesla V100	|   256^3  		|   145.9			|
|  **Alveo U280**		|   **256^3**	|   **108.3**		|


## Installation ##

See the file `INSTALL` in this directory.

## Valid Runs ##

HPCG can be run in just a few minutes from start to finish.  However, official
runs must be at least 1800 seconds (30 minutes) as reported in the output file.
The Quick Path option is an exception for machines that are in production mode
prior to broad availability of an optimized version of HPCG 3.0 for a given platform.
In this situation (which should be confirmed by sending a note to the HPCG Benchmark
owners) the Quick Path option can be invoked by setting the run time parameter equal
to 0 (zero).

A valid run must also execute a problem size that is large enough so that data
arrays accessed in the CG iteration loop do not fit in the cache of the device
in a way that would be unrealistic in a real application setting.  Presently this
restriction means that the problem size should be large enough to occupy a
significant fraction of *main memory*, at least 1/4 of the total.

Future memory system architectures may require restatement of the specific memory
size requirements.  But the guiding principle will always be that the problem
size should reflect what would be reasonable for a real sparse iterative solver.
