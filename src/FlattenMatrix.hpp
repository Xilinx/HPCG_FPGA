#ifndef FLATTENMATRIX_HPP
#define FLATTENMATRIX_HPP

#ifndef HPCG_NO_MPI
#include <mpi.h>
#endif

#ifndef HPCG_NO_OPENMP
#include <omp.h>
#endif

#if defined(HPCG_DEBUG) || defined(HPCG_DETAILED_DEBUG)
#include <fstream>
using std::endl;
#include "hpcg.hpp"
#endif

#include "SparseMatrix.hpp"


void FlattenMatrix(SparseMatrix &A, int numberOfNonzerosPerRow);

#endif // FLATTENMATRIX_HPP
