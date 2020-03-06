#ifndef PREPAREVECTOR_HPP
#define PREPAREVECTOR_HPP

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

#include "Vector.hpp"
#include "SparseMatrix.hpp"

void prepareVector(Vector &x,const SparseMatrix & A, int nonzero);

#endif // PREPAREVECTOR_HPP
