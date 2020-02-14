#include "FlattenMatrix.hpp"
#include <fstream>
#include <iostream>

void FlattenMatrix(SparseMatrix &A, int numberOfNonzerosPerRow){

	int locNumRows = A.localNumberOfRows;
    int arr_size = locNumRows*numberOfNonzerosPerRow;
	local_int_t  *flat_mtxIndL = new local_int_t[arr_size];
	double *flat_matrixValues  = new double[arr_size];
	double *flat_matrixDiagonal = new double[arr_size];

	#ifndef HPCG_NOOPENMP
		#pragma omp parallel for
	#endif

	for(int x = 0; x < locNumRows; x++){

		for(int y = 0; y < numberOfNonzerosPerRow; y++ ){

			flat_mtxIndL[x*numberOfNonzerosPerRow+y] = A.mtxIndL[x][y];
			flat_matrixValues[x*numberOfNonzerosPerRow+y] = A.matrixValues[x][y];
			flat_matrixDiagonal[x*numberOfNonzerosPerRow+y] = A.matrixDiagonal[x][y];
			
		}
	}
	A.flat_mtxIndL = flat_mtxIndL;
	A.flat_matrixValues = flat_matrixValues;
	A.flat_matrixDiagonal = flat_matrixDiagonal;

}
