/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.
 
 Check the error handling when a wrong buffer size is passed
 
 */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>

#include "circ_buf.h"


typedef uint32_t elem_t;
#define MAX_VAL_ELEM UINT32_MAX

int main(void)
{
int ret;
const size_t BufSize = 100;
elem_t *buf;
CircBuf_t CrcBuf;
CircBuf_t *p_CrcBuf = &CrcBuf;

CCBFsize_t WrInd[2][2]; // always 2x2
CCBFsize_t RdInd[2][2]; // always 2x2

fprintf(stderr,"Test with bad (NULL) pointers\n");

buf = malloc(BufSize * sizeof(*buf));
if(buf == NULL) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}

ret = CircBufInit(CIRCBUF(NULL), 0);    
if(ret == 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}

ret = CircBufInit(CIRCBUF(NULL), 1);    
if(ret == 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}

fprintf(stderr,"OK.\n");

return 0;    
}
