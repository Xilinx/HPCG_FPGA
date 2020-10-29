#include "common.hpp"
#include <iostream>

extern "C" {
void dp(
    const v_dt *in1,             // Read-Only Vector 1
    v_dt *in2,             // Read-Only Vector 2
    // synt_type *out_dp,          // Output Result
    const unsigned int size     // Size in integer
) {
#pragma HLS INTERFACE m_axi port = in1 offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = in2 offset = slave bundle = gmem1
// #pragma HLS INTERFACE m_axi port = out_dp offset = slave bundle = gmem2

#pragma HLS INTERFACE s_axilite port = in1 bundle = control
#pragma HLS INTERFACE s_axilite port = in2 bundle = control

// #pragma HLS INTERFACE s_axilite port = out_dp bundle = control
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = in1
#pragma HLS DATA_PACK variable = in2

#pragma HLS dataflow 
    unsigned int vSize = ((size - 1) / VDATA_SIZE) + 1;

    v_dt tmpIn1, tmpIn2;
    v_dt tmpOutdp, tmpRed1, tmpRed2, tmpRes1, tmpRes2, tmpRes3;
    synt_type res;
    //Initialize tmpRes at zero
    for(int i = 0; i < VDATA_SIZE; i++){
    #pragma HLS UNROLL
        tmpRes1.data[i] = 0;
    }

    //Running same kernel operation num_times to keep the kernel busy for HBM bandwiv_dth testing 
vops1:
    for (int i = 0; i < vSize; i++) {
       #pragma HLS PIPELINE II=1
        tmpIn1 = in1[i];
        tmpIn2 = in2[i];

    vops2:
        for (int k = 0; k < VDATA_SIZE; k++) {
            tmpOutdp.data[k] = tmpIn1.data[k] * tmpIn2.data[k];
        }
    red1:
        for (int k = 0; k < VDATA_SIZE; k+=2) {
            tmpRed1.data[k] = tmpOutdp.data[k] + tmpOutdp.data[k+1];
        }
    red2:
        for (int k = 0; k < VDATA_SIZE; k+=4) {
            tmpRed2.data[k] = tmpRed1.data[k] + tmpRed1.data[k+2];
        }
        tmpRes1.data[i%VDATA_SIZE] += tmpRed2.data[0] + tmpRed2.data[4];
    }
    
    redRes1:
        for (int k = 0; k < VDATA_SIZE; k+=2) {
            #pragma HLS UNROLL
            tmpRes2.data[k] = tmpRes1.data[k] + tmpRes1.data[k+1];
        }
    redRes2:
        for (int k = 0; k < VDATA_SIZE; k+=4) {
            #pragma HLS UNROLL
            tmpRes3.data[k] = tmpRes2.data[k] + tmpRes2.data[k+2];
        }
    in2[0].data[0] = tmpRes3.data[0] + tmpRes3.data[4];
    // std::cout<< out_dp[0].data[0] <<std::endl;
}
}
