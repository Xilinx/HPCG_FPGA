#include "common.hpp"
#include <iostream>

typedef ap_axiu<BLOCK, 0, 0, 0> pkt;

extern "C" {
void precond0(
    const v_dt *r,              // Vector r
    hls::stream<pkt> &out,      // Output Result (Internal stream)
    const unsigned int size,    // Size in integer
) {
#pragma HLS INTERFACE m_axi port = r offset = slave bundle = gmem0
#pragma HLS INTERFACE axis port = out

#pragma HLS INTERFACE s_axilite port = r bundle = control

#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = r
#pragma HLS DATA_PACK variable = out

    unsigned int vSize = ((size - 1) / VDATA_SIZE) + 1;
    // unsigned int Va = alpha, Vb = beta;

    v_dt tmpIn1;
    v_dt tmpOut;

vops1:
    for (int i = 0; i < vSize; i++) {
       #pragma HLS PIPELINE II=1
        tmpIn1 = r[i];

        // Creating temp block for the result
        ap_uint<512> res_tmp_block;

        // Copy vector to the stream
    vops2:    
        split_block_loop:for(unsigned int j = 0; j < BLOCK/TYPE; j++){
        #pragma HLS unroll
            synt_type res_val = tmpIn1.data[j];
            res_tmp_block.range((j+1)*TYPE-1, j*TYPE) = *(ap_uint<TYPE> *)&res_val; 
        }
        
        // Writing packet to output stream
        res_tmp.set_data(res_tmp_block);
        res_tmp.set_last(i == (vSize-1));
        res_tmp.set_keep(-1); // Enabling all bytes
        
        // Writing packet to output stream
        res.write(res_tmp);

    }
}
}
