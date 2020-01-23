//@HEADER
// ***************************************************
//
// HPCG: High Performance Conjugate Gradient Benchmark
//
// Xilinx U250 vesion
//
// Alberto Zeni, Kenneth O'Brien - albertoz,kennetho{@xilinx.com}
// ***************************************************
//@HEADER

#ifndef COMPUTEWAXPBY_FPGA_HPP
#define COMPUTEWAXPBY_FPGA_HPP
#include "Vector.hpp"
#include "common.h"

int ComputeWAXPBY_FPGA(const local_int_t n, const double alpha, const Vector & x,
    const double beta, const Vector & y, Vector & w);
#endif // COMPUTEWAXPBY_FPGA_HPP