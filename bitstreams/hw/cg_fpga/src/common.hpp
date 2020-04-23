#ifndef COMMON_HPP
#define COMMON_HPP

#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "hls_stream.h"

#define HALF_W 16
#define SINGLE_W 32
#define DOUBLE_W 64
#define BLOCK 512
#define BLOCK_HBM 256

#define HIGH 100
#define LOW -100

#define KERNEL_NAME "cg"
#define STREAMS 3

// Change here to adapt the architecture to different precision
#define TYPE DOUBLE_W
typedef double synt_type;
#define VDATA_SIZE 8
//

//TRIPCOUNT indentifier
const unsigned int c_dt_size = VDATA_SIZE;

typedef struct v_datatype {
    synt_type data[VDATA_SIZE];
} v_dt;


#endif
