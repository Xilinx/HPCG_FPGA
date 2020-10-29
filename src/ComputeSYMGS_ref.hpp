
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

#ifndef COMPUTESYMGS_REF_HPP
#define COMPUTESYMGS_REF_HPP
#include "SparseMatrix.hpp"
#include "Vector.hpp"
// static int counter_ex = 1;
int ComputeSYMGS_ref( const SparseMatrix  & A, const Vector & r, Vector & x);

#endif // COMPUTESYMGS_REF_HPP
