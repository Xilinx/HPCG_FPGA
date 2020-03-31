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

#ifndef COMPUTEMG_FPGA_HPP
#define COMPUTEMG_FPGA_HPP
#include "SparseMatrix.hpp"
#include "Vector.hpp"

int ComputeMG_FPGA(const SparseMatrix  & A, const Vector & r, Vector & x);

#endif // COMPUTEMG_FPGA_HPP
