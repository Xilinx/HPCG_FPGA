
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
 @file ComputeSPMV.cpp

 HPCG routine
 */

#include "ComputeSPMV.hpp"
#include "ComputeSPMV_ref.hpp"
#include "ComputeSPMV_FPGA.hpp"
#include "PrepareVector.hpp"
#define MAXNONZEROELEMENTS 32
#include <iostream>

// using namespace std;
/*!
  Routine to compute sparse matrix vector product y = Ax where:
  Precondition: First call exchange_externals to get off-processor values of x

  This routine calls the reference SpMV implementation by default, but
  can be replaced by a custom, optimized routine suited for
  the target system.

  @param[in]  A the known system matrix
  @param[in]  x the known vector
  @param[out] y the On exit contains the result: Ax.

  @return returns 0 upon success and non-zero otherwise

  @see ComputeSPMV_ref
*/

// int ComputeSPMV_FPGA(const SparseMatrix & A, Vector & x, Vector & y){
//   for(unsigned long i = 0; i < A.localNumberOfRows; i++){
//       // sw_results[i] = 0;
//       y.values[i] = 0;

//       for(int j = 0; j < /*A.nonzerosInRow[i]*/MAXNONZEROELEMENTS; j++){

//           //hw_results[i] += m[j+MAXNONZEROELEMENTS*i]*x[j+MAXNONZEROELEMENTS*i];
//           //cout<<i<<" "<<m[j+ROW_LEN*i]<<"*"<<x[j+ROW_LEN*i]<<endl;
//           //std::cout<<m[j+MAXNONZEROELEMENTS*i]<<std::endl;
//           // std::cout<<j<<std::endl;
//           y.values[i] +=  A.flat_matrixValues[j+MAXNONZEROELEMENTS*i]*x.val_spmv[j+MAXNONZEROELEMENTS*i];
//       }
//       // std::cout<<i<<std::endl;
//   }
//   return 0;
// }



int ComputeSPMV( const SparseMatrix & A, Vector & x, Vector & y) {

  // This line and the next two lines should be removed and your version of ComputeSPMV should be used.
  // FlattenMatrix(A,  27);
#ifdef FPGA
  A.isSpmvOptimized = true;
  // prepareVector(x,A,27);
  return ComputeSPMV_FPGA(A, A.flat_matrixValues, x, y);
  //return ComputeSPMV_FPGA(A,A.flat_matrixValues, A.flat_mtxIndL, x, y);
#else
  A.isSpmvOptimized = false;
  return ComputeSPMV_ref(A, x, y);
#endif

}
