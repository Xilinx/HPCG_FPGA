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


/*******************************************************************************************
 * Test the correctness of the Preconditined CG implementation 
 * by using a system matrix with a dominant diagonal.
 * Added functions to flatten the matrix while computing this operations.
 *******************************************************************************************/

#include <fstream>
#include <iostream>
using std::endl;
#include <vector>
#include "hpcg.hpp"

#include "TestCG.hpp"
#include "CG.hpp"

int TestCG(SparseMatrix & A, CGData & data, Vector & b, Vector & x, TestCGData & testcg_data) {


  // Use this array for collecting timing information
  std::vector< double > times(8,0.0);
  // Temporary storage for holding original diagonal and RHS
  Vector origDiagA, exaggeratedDiagA, origB;
  InitializeVector(origDiagA, A.localNumberOfRows);
  InitializeVector(exaggeratedDiagA, A.localNumberOfRows);
  InitializeVector(origB, A.localNumberOfRows);
  CopyMatrixDiagonal(A, origDiagA);
  CopyVector(origDiagA, exaggeratedDiagA);
  CopyVector(b, origB);

  // Modify the matrix diagonal to greatly exaggerate diagonal values.
  // CG should converge in about 10 iterations for this problem, regardless of problem size
  for (local_int_t i=0; i< A.localNumberOfRows; ++i) {
    global_int_t globalRowID = A.localToGlobalMap[i];
    if (globalRowID<9) {
      double scale = (globalRowID+2)*1.0e6;
      ScaleVectorValue(exaggeratedDiagA, i, scale);
      ScaleVectorValue(b, i, scale);
    } else {
      ScaleVectorValue(exaggeratedDiagA, i, 1.0e6);
      ScaleVectorValue(b, i, 1.0e6);
    }
  }
  ReplaceMatrixDiagonal(A, exaggeratedDiagA);

  //Flatten Matrix
  FlattenMatrix(A);
  //
  int niters = 0;
  double normr = 0.0;
  double normr0 = 0.0;
  int maxIters = 50;
  int numberOfCgCalls = 2;
  double tolerance = 1.0e-12; // Set tolerance to reasonable value for grossly scaled diagonal terms
  testcg_data.expected_niters_no_prec = 12; // For the unpreconditioned CG call, we should take about 10 iterations, permit 12
  testcg_data.expected_niters_prec = 2;   // For the preconditioned case, we should take about 1 iteration, permit 2
  testcg_data.niters_max_no_prec = 0;
  testcg_data.niters_max_prec = 0;
  for (int k=0; k<2; ++k) { // This loop tests both unpreconditioned and preconditioned runs
    int expected_niters = testcg_data.expected_niters_no_prec;
    if (k==1) expected_niters = testcg_data.expected_niters_prec;
    for (int i=0; i< numberOfCgCalls; ++i) {
      ZeroVector(x); // Zero out x
      int ierr = CG(A, data, b, x, maxIters, tolerance, niters, normr, normr0, &times[0], k==1);
      if (ierr) HPCG_fout << "Error in call to CG: " << ierr << ".\n" << endl;
      if (niters <= expected_niters) {
        ++testcg_data.count_pass;
      } else {
        ++testcg_data.count_fail;
      }
      if (k==0 && niters>testcg_data.niters_max_no_prec) testcg_data.niters_max_no_prec = niters; // Keep track of largest iter count
      if (k==1 && niters>testcg_data.niters_max_prec) testcg_data.niters_max_prec = niters; // Same for preconditioned run
      if (A.geom->rank==0) {
        HPCG_fout << "Call [" << i << "] Number of Iterations [" << niters <<"] Scaled Residual [" << normr/normr0 << "]" << endl;
        if (niters > expected_niters)
          HPCG_fout << " Expected " << expected_niters << " iterations.  Performed " << niters << "." << endl;
      }
    }
  }

  // Restore matrix diagonal and RHS
  ReplaceMatrixDiagonal(A, origDiagA);
  CopyVector(origB, b);
  // Delete vectors
  DeleteVector(origDiagA);
  DeleteVector(exaggeratedDiagA);
  DeleteVector(origB);
  testcg_data.normr = normr;

  //Flatten Matrix
  FlattenMatrix(A);
  //

  return 0;
}
