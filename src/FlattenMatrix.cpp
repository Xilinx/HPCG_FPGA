#include "FlattenMatrix.hpp"
#include <fstream>
#include <iostream>
#include "common.h"

void FlattenMatrix(SparseMatrix &A, int numberOfNonzerosPerRow){//numberOfNonzerosPerRow is 32 (27 closest power of 2)

	int locNumRows = A.localNumberOfRows;
	int nonzeroAdjusted = ((numberOfNonzerosPerRow + sizeof(synt_type) - 1 )/ sizeof(synt_type))*sizeof(synt_type);
    int arr_size = locNumRows*nonzeroAdjusted;
	local_int_t  *flat_mtxIndL = new local_int_t[arr_size];
	double *flat_matrixValues  = new double[arr_size];
	double *flat_matrixDiagonal = new double[arr_size];

	#ifndef HPCG_NOOPENMP
		#pragma omp parallel for
	#endif

	for(int x = 0; x < locNumRows; x++){

		for(int y = 0; y < nonzeroAdjusted; y++ ){

			if(y < numberOfNonzerosPerRow){
				flat_mtxIndL[x*nonzeroAdjusted+y] = A.mtxIndL[x][y];
				flat_matrixValues[x*nonzeroAdjusted+y] = A.matrixValues[x][y];
				flat_matrixDiagonal[x*nonzeroAdjusted+y] = A.matrixDiagonal[x][y];
			}
			else{
				flat_mtxIndL[x*nonzeroAdjusted+y] = 0;
				flat_matrixValues[x*nonzeroAdjusted+y] = 0;
				flat_matrixDiagonal[x*nonzeroAdjusted+y] = 0;
			}
		}
	}
	A.flat_mtxIndL = flat_mtxIndL;
	A.flat_matrixValues = flat_matrixValues;
	A.flat_matrixDiagonal = flat_matrixDiagonal;
}
