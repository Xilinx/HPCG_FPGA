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

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <iostream>
#include <cmath>
#include "common.h"

#define MAXNONZEROELEMENTS 32
//#define N_ROWS 100

using namespace hls;
using namespace std;

typedef qdma_axis<BLOCK, 0, 0, 0> pkt;
typedef qdma_axis<TYPE, 0, 0, 0> pkt_synt_type;


void readX(ap_uint<512> *x,synt_type *x_local, int num_rows){
	read_loop:for(unsigned int i = 0; i < num_rows/(BLOCK/TYPE); i++){
	#pragma HLS pipeline	
		for(unsigned int j = 0; j < BLOCK/TYPE; j++){
		#pragma HLS unroll
			x_local[i*BLOCK/TYPE+j]= x[i].range((j+1)*TYPE-1, j*TYPE);
		}
	}

}


template <typename T, unsigned int length>
void reduce(T *product) {

	reduction_loop1:for(int j = 0; j < length; j+=2){
		#pragma HLS unroll
		product[j] = product[j] + product[j+1];
	}
	reduction_loop2:for(int j = 0; j < length; j+=4){
		#pragma HLS unroll
		product[j] = product[j] + product[j+2];
	}
	reduction_loop3:for(int j = 0; j < length; j+=8){
		#pragma HLS unroll
		product[j] = product[j] + product[j+4];
	}
	// reduction_loop4:for(int j = 0; j < length; j+=16){
	//  	#pragma HLS unroll
	//  	product[j] = product[j] + product[j+8];
	// }
	// reduction_loop5:for(int j = 0; j < length; j+=32){
	//  	#pragma HLS unroll
	//  	product[j] = product[j] + product[j+16];
	// }
	
}

void multiply_row(stream<pkt> &matrixValues, stream<pkt> &x, synt_type *res){

	synt_type partial_sums[MAXNONZEROELEMENTS];
	#pragma HLS ARRAY_PARTITION variable=partial_sums complete dim=1
	synt_type res_par[MAXNONZEROELEMENTS/(BLOCK/TYPE)];
	#pragma HLS ARRAY_PARTITION variable=res_par complete dim=1


	single_row_loop:for(long long j = 0; j < MAXNONZEROELEMENTS/(BLOCK/TYPE); j++){
	#pragma HLS pipeline II=1
	//reading matrix values and vector values
		// pkt indexes = AmtxIndL.read();
		pkt mat_values = matrixValues.read();
		pkt x_values = x.read();
		// ap_uint<512> indexes_block = indexes.get_data();
		ap_uint<512> mat_values_block = mat_values.get_data();
		ap_uint<512> x_values_block = x_values.get_data();	

		// #HLS resource 
		split_block_loop_compute:for (unsigned int k = 0; k < BLOCK/TYPE; k++){
		#pragma HLS unroll
			// ap_uint<TYPE> index = indexes_block.range((k+1)*TYPE-1, k*TYPE);
			ap_uint<TYPE> mat_values_tmp_uint = mat_values_block.range((k+1)*TYPE-1, k*TYPE);
			ap_uint<TYPE> x_values_tmp_uint = x_values_block.range((k+1)*TYPE-1, k*TYPE);
			synt_type mat_val = *(synt_type *)&mat_values_tmp_uint;
			synt_type x_val = *(synt_type *)&x_values_tmp_uint;
			// std::cout<<"HW: "<<matrixValues[i*MAXNONZEROELEMENTS+j]<<"*"<<x[AmtxIndL[i*MAXNONZEROELEMENTS+j]]<<std::endl;
			partial_sums[/*j*(BLOCK/TYPE)+*/k] =  mat_val*x_val;
		} 
		reduce<synt_type,32>(partial_sums);
		res_par[j]=partial_sums[0];
	}
	synt_type t1 = res_par[0]+res_par[1];
	synt_type t2 = res_par[2]+res_par[3];
	*res = t1+t2;
}

void write_back(synt_type res,stream<pkt_synt_type> &y, bool last){

	// Packet for output
	pkt_synt_type out_pkt;
	synt_type tmp=res;
	ap_uint<TYPE> out_val = *(ap_uint<TYPE> *)&tmp;
	// Setting data and configuration to output packet
	out_pkt.set_data(out_val);
	out_pkt.set_last(last);
	out_pkt.set_keep(-1); // Enabling all bytes
	y.write(out_pkt);
}

void execution(stream<pkt> &matrixValues, stream<pkt> &x, stream<pkt_synt_type> &y, unsigned int n_r){

	execution_loop: for(long long i = 0; i <n_r; i++){
		bool last = (i == (n_r - 1));
		synt_type res=0;
		multiply_row(/*AmtxIndL,*/matrixValues,x,&res);
		write_back(res,y,last);

	}
}


extern "C"{

void spmv(stream<pkt> &m, stream<pkt> &x, stream<pkt_synt_type> &y, int num_rows){

	#pragma HLS interface axis port=m //bundle = gmem1
	#pragma HLS interface axis port=x
	#pragma HLS interface axis port=y //bundle = gmem2

	// #pragma HLS interface m_axi port=x bundle=gmem0

	// #pragma HLS interface s_axilite port = x bundle = control
	#pragma HLS interface s_axilite port = num_rows bundle = control
	#pragma HLS interface s_axilite port = return bundle = control
	#pragma HLS interface ap_ctrl_chain port = return

	#pragma HLS dataflow

	#pragma HLS inline recursive
	
	
		
	// synt_type x_local[N_ROWS];
	// #pragma HLS ARRAY_PARTITION variable=x_local complete dim=1
	// #pragma HLS resource variable=x_local latency=1 

	unsigned n_r = num_rows;
	execution(m,x,y,n_r);
	// readX(x,x_local,n_r);	
}

}
