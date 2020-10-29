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

/*********************************************************
 * Host for the computation of MG
 *********************************************************/

#include "ComputeMG_FPGA.hpp"
#include "ComputeSYMGS_FPGA.hpp"
#include "ComputeSPMV_FPGA.hpp"
#include "ComputeRestriction_ref.hpp"
#include "ComputeProlongation_ref.hpp"
#include <cassert>

/*!

  @param[in] A the known system matrix
  @param[in] r the input vector
  @param[inout] x On exit contains the result of the multigrid V-cycle with r as the RHS, x is the approximation to Ax = r.

  @return returns 0 upon success and non-zero otherwise

  @see ComputeMG
*/
int ComputeMG_FPGA(const SparseMatrix & A, const Vector & r, Vector & x, double & time) {
  assert(x.localLength==A.localNumberOfColumns); // Make sure x contain space for halo values

  ZeroVector(x); // initialize x to zero

  int ierr = 0;
  if (A.mgData!=0) { // Go to next coarse level if defined
    int numberOfPresmootherSteps = A.mgData->numberOfPresmootherSteps;
    ierr += ComputeSYMGS_FPGA(A, r, x, numberOfPresmootherSteps, time);

    if (ierr!=0) return ierr;
    ierr = ComputeSPMV_FPGA(A, x, *A.mgData->Axf, time); if (ierr!=0) return ierr;
    ierr = ComputeRestriction_ref(A, r);  if (ierr!=0) return ierr;
    ierr = ComputeSYMGS_FPGA(*A.Ac,*A.mgData->rc, *A.mgData->xc, 1, time);  if (ierr!=0) return ierr;
    ierr = ComputeProlongation_ref(A, x);  if (ierr!=0) return ierr;
    int numberOfPostsmootherSteps = A.mgData->numberOfPostsmootherSteps;
    ierr += ComputeSYMGS_FPGA(A, r, x, numberOfPostsmootherSteps, time);

    if (ierr!=0) return ierr;
  }
  return 0;
}

