//@HEADER
// ***************************************************
//
// HPCG: High Performance Conjugate Gradient Benchmark
//
// Xilinx U250/U280 vesion
//
// Alberto Zeni, Kenneth O'Brien - albertoz,kennetho{@xilinx.com}
// ***************************************************
//@HEADER

#ifndef COMPUTESPMV_FPGA_HPP
#define COMPUTESPMV_FPGA_HPP
#include "Vector.hpp"
#include "common.h"
#include "SparseMatrix.hpp"

int ComputeSPMV_FPGA( const SparseMatrix & A, Vector & x, Vector & y);

#endif // COMPUTESPMV_FPGA_HPP