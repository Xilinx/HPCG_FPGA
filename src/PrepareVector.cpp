#include "PrepareVector.hpp"
#include "Vector.hpp"
#include <fstream>
#include <iostream>
#include "common.h"

void prepareVector(Vector &x,const SparseMatrix & A, int nonzero){
  // std::cout<<"HERE IN PREP"<<std::endl;
  int nonzeroAdjusted = ((nonzero + sizeof(synt_type) - 1 )/ sizeof(synt_type))*sizeof(synt_type);
  unsigned int arr_size = nonzeroAdjusted * A.localNumberOfRows;
  // free(x.val_spmv);
  double *prep_v = new double[arr_size];
  // std::cout<<nonzeroAdjusted<<std::endl;
  // std::cout<<"HERE IN PREP CREATED"<<std::endl;
  for(unsigned int i = 0; i < A.localNumberOfRows; i++){
    // std::cout<<i<<std::endl;
    for(unsigned int j = 0; j < nonzeroAdjusted; j++){
      // std::cout<<j<<std::endl;
      if(j<A.nonzerosInRow[i])
        prep_v[i*nonzeroAdjusted+j]=x.values[A.flat_mtxIndL[i*nonzeroAdjusted+j]];
      else
        prep_v[i*nonzeroAdjusted+j]=0;
    }
  }

  x.val_spmv = prep_v;
  // std::cout<<"HERE IN PREP FINISH"<<std::endl;
}