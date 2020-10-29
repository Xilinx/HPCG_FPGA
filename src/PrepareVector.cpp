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

#include "PrepareVector.hpp"
#include "Vector.hpp"
#include <fstream>
#include <iostream>
#include "common.hpp"

void PrepareVector(Vector &x,const SparseMatrix & A){

  int nonzero = 27;
  int nonzeroAdjusted = ((nonzero + sizeof(synt_type) - 1 )/ sizeof(synt_type))*sizeof(synt_type);
  unsigned int arr_size = nonzeroAdjusted * A.localNumberOfRows;

  double *prep_v = new double[arr_size];

  for(unsigned int i = 0; i < A.localNumberOfRows; i++){
    
    for(unsigned int j = 0; j < nonzeroAdjusted; j++){

      if(j<A.nonzerosInRow[i])
        prep_v[i*nonzeroAdjusted+j]=x.values[A.flat_mtxIndL[i*nonzeroAdjusted+j]];
      else
        prep_v[i*nonzeroAdjusted+j]=0;
    }
  }

  x.val_spmv = prep_v;

}