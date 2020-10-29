#include "common.hpp"
#include <iostream>
//if numunrolls > 16 there will be an error, consider this in future versions, it's a quick fix, i am using 16 for convenience

extern "C" {
void symgs(
    const v_dt *in1,             // Read-Only Vector 1
    const v_dt *in2,             // Read-Only Vector 2
    synt_type *out_symgs,         // Output Result for symgs
    const unsigned int size,     // Size in integer
    const unsigned int non_zero,  // Running the same kernel operations num_times
    const unsigned int n_times 	
) {
#pragma HLS INTERFACE m_axi port = in1 offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = in2 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = out_symgs offset = slave bundle = gmem2

#pragma HLS INTERFACE s_axilite port = in1 bundle = control
#pragma HLS INTERFACE s_axilite port = in2 bundle = control
#pragma HLS INTERFACE s_axilite port = out_symgs bundle = control

#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = non_zero bundle = control
#pragma HLS INTERFACE s_axilite port = in_times bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS data_pack variable = in1
#pragma HLS data_pack variable = in2

// #pragma HLS dataflow

    unsigned int nrows = ((size - 1) / non_zero) + 1; //number of rows
    unsigned int num_unrolls = ((non_zero - 1) / VDATA_SIZE) + 1;
    unsigned int vSize = ((size - 1) / VDATA_SIZE) + 1;

    v_dt tmpIn1, tmpIn2;
    v_dt tmpOutsymgs, tmpRed1, tmpRed2, tmpRed3, tmpRed4;
    v_dt buffer_input1[256*256*256];

#pragma HLS array_partition variable=tmpIn1.data complete dim=1
#pragma HLS array_partition variable=tmpIn2.data complete dim=1
#pragma HLS array_partition variable=tmpOutsymgs.data complete dim=1
#pragma HLS array_partition variable=tmpRed1.data complete dim=1 
#pragma HLS array_partition variable=tmpRed2.data complete dim=1
#pragma HLS array_partition variable=tmpRed3.data complete dim=1    
#pragma HLS array_partition variable=tmpRed4.data complete dim=1 
    for(int n = 0; n < n_times; n++){
    //run for the total number of rows and in reverse order
        for (int count = 0; count < nrows; count++) {
        vops1:
            for (int i = 0; i < num_unrolls; i++) {//unroll as much as we can
               #pragma HLS PIPELINE II=1
                tmpIn1 = in1[i+num_unrolls*count];
                tmpIn2 = in2[i+num_unrolls*count];

            vops2:
                for (int k = 0; k < VDATA_SIZE; k++) {
                    #pragma HLS UNROLL
                    tmpOutsymgs.data[k] = tmpIn1.data[k] * tmpIn2.data[k];
                }
            red1:
                for (int k = 0; k < VDATA_SIZE; k+=2) {
                    #pragma HLS UNROLL
                    tmpRed1.data[k] = tmpOutsymgs.data[k] + tmpOutsymgs.data[k+1];
                }
            red2:
                for (int k = 0; k < VDATA_SIZE; k+=4) {
                    #pragma HLS UNROLL
                    tmpRed2.data[k] = tmpRed1.data[k] + tmpRed1.data[k+2];
                }
                tmpRed3.data[i] = tmpRed2.data[0] + tmpRed2.data[4];
            redRes1:
                for (int k = 0; k < VDATA_SIZE; k+=2) {
                    #pragma HLS UNROLL
                    tmpRed4.data[k] = tmpRed3.data[k] + tmpRed3.data[k+1];
                }
                out_symgs[i+num_unrolls*count] = tmpRed4.data[0]+tmpRed4.data[2];//because we know that we unroll 4 at most
            }
        }
        for (int count = nrows-1; count >= 0; count--) {
        vops3:
            for (int i = 0; i < num_unrolls; i++) {//unroll as much as we can
               #pragma HLS PIPELINE II=1
                tmpIn1 = in1[i+num_unrolls*count];
                tmpIn2 = in2[i+num_unrolls*count];

            vops4:
                for (int k = 0; k < VDATA_SIZE; k++) {
                    #pragma HLS UNROLL
                    tmpOutsymgs.data[k] = tmpIn1.data[k] * tmpIn2.data[k];
                }
            red3:
                for (int k = 0; k < VDATA_SIZE; k+=2) {
                    #pragma HLS UNROLL
                    tmpRed1.data[k] = tmpOutsymgs.data[k] + tmpOutsymgs.data[k+1];
                }
            red4:
                for (int k = 0; k < VDATA_SIZE; k+=4) {
                    #pragma HLS UNROLL
                    tmpRed2.data[k] = tmpRed1.data[k] + tmpRed1.data[k+2];
                }
                tmpRed3.data[i] = tmpRed2.data[0] + tmpRed2.data[4];
            redRes2:
                for (int k = 0; k < VDATA_SIZE; k+=2) {
                    #pragma HLS UNROLL
                    tmpRed4.data[k] = tmpRed3.data[k] + tmpRed3.data[k+1];
                }
                out_symgs[i+num_unrolls*count] = tmpRed4.data[0]+tmpRed4.data[2];//because we know that we unroll 4 at most
            }
        }
    }

    
}
}
