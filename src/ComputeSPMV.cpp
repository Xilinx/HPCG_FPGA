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
