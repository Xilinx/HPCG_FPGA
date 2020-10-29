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

/********************************************************************************************
 * Description:
 * Contains data types used in the hardware kernels
 ******************************************************************************************/



#ifndef COMMON_HPP
#define COMMON_HPP

#define HALF_W 16
#define SINGLE_W 32
#define DOUBLE_W 64
#define BLOCK 512
#define BLOCK_HBM 256

#define HIGH 100
#define LOW -100

#define STREAMS 3

// Change here to adapt the architecture to different precision
#define TYPE DOUBLE_W
typedef double synt_type;
#define VDATA_SIZE 8
#define NON_ZERO 32
#define PATH_TO_BINARY_SPMV "../spmv/hbm_u280/double/build_dir.hw.xilinx_u280_xdma_201920_3/spmv.xclbin"
#define PATH_TO_BINARY_DP "../dp/hbm_u280/double/build_dir.hw.xilinx_u280_xdma_201920_3/dp.xclbin"
#define PATH_TO_BINARY_WAXPBY "../waxpby/hbm_u280/double/build_dir.hw.xilinx_u280_xdma_201920_3/waxpby.xclbin"
//#define PATH_TO_BINARY_WAXPBY "/home/albertoz/final_repos/waxpby/hbm_u280/double/build_dir.hw.xilinx_u280_xdma_201920_3/waxpby.xclbin"
#define PATH_TO_BINARY_SYMGS "../symgs/hbm_u280/double/build_dir.hw.xilinx_u280_xdma_201920_3/symgs.xclbin"
//

//TRIPCOUNT indentifier
const unsigned int c_dt_size = VDATA_SIZE;

typedef struct v_datatype {
    synt_type data[VDATA_SIZE];
} v_dt;


#endif
