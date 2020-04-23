#include "common.hpp"
#include <iostream>

typedef ap_axiu<BLOCK, 0, 0, 0> pkt;

extern "C" {
void waxpby0(
    hls::stream<pkt> &z,
    hls::stream<pkt> &p,      // Output Result (Internal stream)
    const unsigned int size,    // Size in integer
) {

#pragma HLS INTERFACE axis port = z
#pragma HLS INTERFACE axis port = p

#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    unsigned int vSize = ((size - 1) / VDATA_SIZE) + 1;
    // unsigned int Va = alpha, Vb = beta;

    v_dt tmpIn1, tmpIn2;
    v_dt tmpOut;

vops1:
    for (int i = 0; i < vSize; i++) {
       #pragma HLS PIPELINE II=1
        // Reading streaming into packets
        pkt z_tmp = z.read();
        // Writing packet to output stream
        p.write(z_tmp);
    }
}
}
