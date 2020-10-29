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

#define N_BUFF 16

using namespace hls;
using namespace std;

typedef qdma_axis<BLOCK, 0, 0, 0> pkt;
typedef qdma_axis<TYPE, 0, 0, 0> pkt_synt_type;

template <typename T, unsigned int length>
void reduce(T *product) {
//decomment the commented lines in case of different data types

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
	// 	reduction_loop5:for(int j = 0; j < length; j+=32){
	// 	 	#pragma HLS unroll
	// 	 	product[j] = product[j] + product[j+16];
	// 	}
	
}

template <typename T, unsigned int length>
void reduce16(T *product) {

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
	reduction_loop4:for(int j = 0; j < length; j+=16){
	 	#pragma HLS unroll
	 	product[j] = product[j] + product[j+8];
	}
	
}

void initialize_buffers(synt_type v[N_BUFF][BLOCK*2/TYPE]){

	for(int i = 0; i < N_BUFF; i++){
		#pragma HLS pipeline
		for(int j = 0; j < BLOCK/TYPE; j++){
			v[i][j]=0;
		}
	}

}

void execute_partial_prod(hls::stream<pkt> &a, hls::stream<pkt> &b,synt_type v[N_BUFF][BLOCK*2/TYPE]){

	bool eos = false;
	unsigned int i = 0;

	execution_loop:do{
	#pragma HLS pipeline rewind II=1
		
		// Reading a and b streaming into packets
		pkt a_tmp = a.read();
		pkt b_tmp = b.read();

		ap_uint<512> a_tmp_block = a_tmp.get_data();
		ap_uint<512> b_tmp_block = b_tmp.get_data();
		split_block_loop_compute:for(unsigned int j = 0; j < BLOCK/TYPE; j++){
		#pragma HLS unroll
			ap_uint<TYPE> a_tmp_uint = a_tmp_block.range((j+1)*TYPE-1, j*TYPE);
			ap_uint<TYPE> b_tmp_uint = b_tmp_block.range((j+1)*TYPE-1, j*TYPE);
			synt_type a_val = *(synt_type *)&a_tmp_uint;
			synt_type b_val = *(synt_type *)&b_tmp_uint;
			v[i%16][j]+=a_val*b_val;
		}
		i++;
		if (a_tmp.get_last() || b_tmp.get_last()) {
            eos = true;
        }
	} while (eos == false);

}

void sum_partial_results(synt_type v[N_BUFF][BLOCK*2/TYPE], synt_type temp[BLOCK*2/TYPE]){
	
	sum_loop:for(unsigned int j = 0; j < 16; j++){
	#pragma HLS pipeline
		reduce<synt_type,BLOCK/TYPE>(v[j]);
		temp[j]=v[j][0];
	}
}

void write_back(synt_type temp[BLOCK*2/TYPE],hls::stream<pkt_synt_type> &res){

	// Packet for output
	pkt_synt_type res_tmp;
	ap_uint<TYPE> res_val = *(ap_uint<TYPE> *)&temp[0];
	// Setting data and configuration to output packet
	res_tmp.set_data(res_val);
	res_tmp.set_last(1);
	res_tmp.set_keep(-1); // Enabling all bytes
	res.write(res_tmp);

}

extern "C"{
void vecdotprod(hls::stream<pkt> &a, hls::stream<pkt> &b, hls::stream<pkt_synt_type> &res){
	#pragma HLS interface axis register /*both*/ port=a //bundle = gmem0
	#pragma HLS interface axis register /*both*/ port=b //bundle = gmem1
	#pragma HLS interface axis register /*both*/ port=res //bundle = gmem2


	#pragma HLS interface s_axilite port = return bundle = control
	#pragma HLS interface ap_ctrl_chain port = return
	#pragma HLS inline recursive
	#pragma HLS dataflow


	// Creating local variables
	synt_type product[16][BLOCK*2/TYPE];
	#pragma HLS ARRAY_PARTITION variable=product complete dim=2
	synt_type temp[BLOCK*2/TYPE];
	#pragma HLS ARRAY_PARTITION variable=temp complete dim=1

	initialize_buffers(product);
	// Perform dot product
	execute_partial_prod(a,b,product);
	// Perform parallel reduction per every result array
	sum_partial_results(product,temp);
	// sum all the results from the buffers
	reduce16<synt_type,BLOCK*2/TYPE>(temp);
	//write the output back
	write_back(temp,res);
}
}
