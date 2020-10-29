#include "common.hpp"
#include <iostream>

extern "C" {
void waxpby(
    v_dt *in1,             // Read-Only Vector 1
    v_dt *in2,             // Read-Only Vector 2
    v_dt *out_waxpby,            // Output Result for alpha*x+beta*y
    synt_type alpha,
    synt_type beta,
    unsigned int size     // Size in integer
) {
#pragma HLS INTERFACE m_axi port = in1 offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = in2 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = out_waxpby offset = slave bundle = gmem2

#pragma HLS INTERFACE s_axilite port = in1 bundle = control
#pragma HLS INTERFACE s_axilite port = in2 bundle = control
#pragma HLS INTERFACE s_axilite port = out_waxpby bundle = control

#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = alpha bundle = control
#pragma HLS INTERFACE s_axilite port = beta bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = in1
#pragma HLS DATA_PACK variable = in2
#pragma HLS DATA_PACK variable = out_waxpby

    unsigned int vSize = ((size - 1) / VDATA_SIZE) + 1;
    // unsigned int Va = alpha, Vb = beta;

    v_dt tmpIn1, tmpIn2;
    v_dt tmpOutWaxpby;
vops1:
    for (int i = 0; i < vSize; i++) {
       #pragma HLS PIPELINE II=1
        tmpIn1 = in1[i];
        tmpIn2 = in2[i];
    	
    vops2:
        for (int k = 0; k < VDATA_SIZE; k++) {
            tmpOutWaxpby.data[k] = tmpIn1.data[k]*alpha + tmpIn2.data[k]*beta;
        }
        out_waxpby[i] = tmpOutWaxpby;
    }
}
}
