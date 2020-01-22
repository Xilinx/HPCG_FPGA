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

#ifndef COMPUTEDOTPRODUCT_FPGA_HPP
#define COMPUTEDOTPRODUCT_FPGA_HPP
#include "Vector.hpp"
#include "common.h"

int ComputeDotProduct_FPGA(const local_int_t n, const Vector & x, const Vector & y,
    double & result, double & time_allreduce);

#endif // COMPUTEDOTPRODUCT_FPGA_HPP