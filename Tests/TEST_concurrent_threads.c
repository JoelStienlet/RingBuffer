/*

   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.

 
 One writer, one reader
 
 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>

#include "circ_buf.h"


typedef uint32_t elem_t;
#define MAX_VAL_ELEM UINT32_MAX


struct shared_thd_data_str
{
elem_t *buf;
CircBuf_t *p_CrcBuf; // more convenient
CircBuf_t CrcBuf;
size_t max_val; // insert "max_val+1" elements in the ring buffer, with values from 0 to max_val included
};


void *writer(void *p_usr_in)
{
size_t cur_val = 0; // next value to insert, not yet inserted
struct shared_thd_data_str *p_data = p_usr_in;
CCBFsize_t WrInd[2][2]; // always 2x2

size_t max_add, NtoAdd;
CCBFsize_t NCopied;
int m;
size_t i;

while(cur_val < p_data -> max_val) {
    //fprintf(stderr,"%u\n", cur_val);
    
    // get space available for writing:
    CircBufWrInd(CIRCBUF(p_data -> p_CrcBuf), &WrInd);
    max_add = CircBufSzSum(WrInd);
    if(max_add > (1 + p_data -> max_val - cur_val)) max_add = 1 + p_data -> max_val - cur_val;
    
    // choose the number of bytes to actually write:
    NtoAdd = (max_add + 1) * drand48();
    if(NtoAdd > max_add) NtoAdd = max_add;
    
    // write:
    NCopied = 0; for(m=0; m<2; m++) { 
        for(i=WrInd[m][0]; i<= WrInd[m][1]; i++) {
            if(NCopied < NtoAdd) {
                p_data -> buf[i] = cur_val;
                cur_val++;
                NCopied++;
               }
            else break;
        } // i
    }// m
    
    CircBufUpdtWr(CIRCBUF(p_data -> p_CrcBuf), NCopied);
    
  } // end of while
  
fprintf(stderr,"Writer finished.\n");
return NULL;
}


void *reader(void *p_usr_in)
{
int ret;
struct shared_thd_data_str *p_data = p_usr_in;
size_t cur_val = 0; // next value to be read

CCBFsize_t RdInd[2][2]; // always 2x2

size_t Nread, max_read, NtoRead;

int m;
size_t i;

//
// Simplest approach: just read everything available.
// The problem is, we may not hit all corner cases very well. Better read a randomized amount of data.
//
while(cur_val < p_data -> max_val) {
//     fprintf(stderr,"%u\n", cur_val);
    
    CircBufRdInd(CIRCBUF(p_data -> p_CrcBuf), &RdInd);
    
    //if(CircBufSzSum(RdInd) > 0) fprintf(stderr,"%u -> %u   &   %u -> %u\n", RdInd[0][0], RdInd[0][1], RdInd[1][0], RdInd[1][1]);
    max_read = CircBufSzSum(RdInd);
    NtoRead = (max_read + 1) * drand48();
    if(NtoRead > max_read) NtoRead = max_read;
    
    Nread = 0;
    for(m=0; m<2; m++) { 
        for(i=RdInd[m][0]; i<= RdInd[m][1]; i++) { 
            if(Nread < NtoRead) {
                if(p_data -> buf[i] != cur_val) {
                    // ERROR!
                    fprintf(stderr,"ERROR: bad value. F:%s L:%d\n",__FILE__,__LINE__);
                   }
                cur_val ++;
                Nread++;
              } // check Nread 
            }// i
       }// m
       
    ret = CircBufUpdtRd(CIRCBUF(p_data -> p_CrcBuf), Nread);
    if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
       
} // while

fprintf(stderr,"Reader finished: all values OK.\n");
return NULL;    
}



void randomize_struct(void *mem, size_t size)
{
uint8_t *tab_bytes = mem;
size_t i;
for(i=0; i< size; i++) tab_bytes[i] = 256 * drand48();
}

int init_shared_data(struct shared_thd_data_str *p_data, const size_t max_val, const size_t Nbufsz )
{
int ret;
 
 p_data -> buf = malloc(Nbufsz * sizeof(*(p_data -> buf)));
 if( p_data -> buf == NULL)  {fprintf(stderr,"ERROR: malloc failed. F:%s L:%d\n",__FILE__,__LINE__); return 1;}
  
 randomize_struct(p_data -> buf, Nbufsz * sizeof(*(p_data -> buf)));
  
 p_data -> p_CrcBuf = &(p_data -> CrcBuf);
 randomize_struct(p_data -> p_CrcBuf, sizeof(p_data -> CrcBuf));
 
 ret = CircBufInit(CIRCBUF(p_data -> p_CrcBuf), Nbufsz);    
 if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); return 1;}
 
 p_data -> max_val = max_val;
 
return 0;
}


int clear_shared_data(struct shared_thd_data_str *p_data)
{
 free(p_data -> buf);
 
    return 0;
}

void init_drand48()
{
    srand48(time(NULL));
}

int main(int argc, char *argv[])
{
size_t MaxVal; // ex: 4000000000
size_t Nbufsz; // ex: 10
 
int ret;
pthread_t writer_thd, reader_thd;
struct shared_thd_data_str shrd_data;

if(argc != 3) {
    fprintf(stderr,"ERROR: pass the buffer size and the number of elements to insert\n");
    return 1;
   }
ret = sscanf(argv[1],"%zu",&Nbufsz);
if(ret != 1) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
ret = sscanf(argv[2],"%zu",&MaxVal);
if(ret != 1) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}

printf("Randomized test of the ring buffer with %llu insertions\n", (long long unsigned)MaxVal);

init_drand48();

ret = init_shared_data(&shrd_data, MaxVal, Nbufsz);
if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}

ret = pthread_create(&writer_thd, NULL, writer, &shrd_data);
if(ret != 0) {fprintf(stderr,"ERROR: cannot create thread. F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
ret = pthread_create(&reader_thd, NULL, reader, &shrd_data);
if(ret != 0) {fprintf(stderr,"ERROR: cannot create thread. F:%s L:%d\n",__FILE__,__LINE__); exit(1);}

pthread_join(writer_thd, NULL);
pthread_join(reader_thd, NULL);

clear_shared_data(&shrd_data);

fprintf(stderr,"OK.\n");

return 0;    
}


