/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.
 
The purpose of this example is to show how this library can be used to store blocks of contiguous memory in the ring buffer.

=> The keyword here is "contiguous".

In this example we suppose that the block size is somehow imposed (which is usually the case).
This means that sometimes a block won't fit in the space left at the end of the allocated buffer, so we have to start from the beginning of the buffer, and we leave some unused space at the end of it. 

To distinguish the data from "wasted" space, we choose to track the actual location of every DMA blocks in the circular buffer.

This is achieved using two circular buffers:
 - one containing the data
 - the other containing the indexes of the DMA blocks in the first buffer
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>

#include "circ_buf.h"


typedef uint32_t elem_t;
#define MAX_VAL_ELEM UINT32_MAX

typedef struct ind_buf_elem_str
{
CCBFsize_t start; // start of the data block in the buffer, as elem_t index
CCBFsize_t end; // end of the data block in the buffer, as elem_t index
CCBFsize_t size; // actual size to remove from the data buffer, as number of elem_t items
} ind_t;


struct shared_thd_data_str
{
elem_t *buf; // containing the raw data
ind_t *indbuf; // containing the indices

CircBuf_t *p_CrcBufData; // more convenient
CircBuf_t CrcBufData; // ring managing the data in buf

CircBuf_t *p_CrcBufInd; // more convenient
CircBuf_t CrcBufInd; // ring managing the indexes of the packets

size_t DMAsize; // size of a DMA block
int Ninsertions; // iterations of the test
};

// ==============================================================================

void randomize_struct(void *mem, size_t size)
{
uint8_t *tab_bytes = mem;
size_t i;
for(i=0; i< size; i++) tab_bytes[i] = 256 * drand48();
}

// ==============================================================================

//
//  Writer Thread 
//
void *writer(void *p_usr_in)
{
int ret;
struct shared_thd_data_str *p_data = p_usr_in;
CCBFsize_t BufWrInd[2][2]; // always 2x2
CCBFsize_t IndWrInd[2][2];

int k = 0;

size_t m;

CCBFsize_t imin, used;

ind_t ind;

while( k < p_data -> Ninsertions) { 
    
    // insert the DMA blocks one at a time
    
    // get space available for writing:
    ret = CircBufWrInd(CIRCBUF(p_data -> p_CrcBufData), &BufWrInd);
    if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
    
    ret = CircBufWrInd(CIRCBUF(p_data -> p_CrcBufInd), &IndWrInd);
    if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
    
    // do we have enough space to store the indexes?
    if(CircBufSzSum(IndWrInd) > 0) {
    
        // can we write to the first buffer?
        if(CircBufSz(0,BufWrInd) >= p_data -> DMAsize) {
            // first empty range is big enough
               imin = BufWrInd[0][0];
               used = p_data -> DMAsize;
        } else {
            // check the second range:
            if(CircBufSz(1,BufWrInd) >= p_data -> DMAsize) {
                imin = BufWrInd[1][0];
                used = CircBufSz(0,BufWrInd) + p_data -> DMAsize;
             } else {
                 // no suitable empty space... wait for the reader to free some space.
                 used = 0;
             }
         }
    
        if(used > 0) {
            elem_t *dma_buf = malloc(p_data -> DMAsize * sizeof(*dma_buf));
            if(dma_buf == NULL) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
            randomize_struct(dma_buf, p_data -> DMAsize * sizeof(*dma_buf));
        
            elem_t chksum = 0;
            for(m=0; m< p_data -> DMAsize - 1; m++) chksum ^= dma_buf[m];
            dma_buf[p_data -> DMAsize - 1] = chksum;
    
            memcpy(p_data -> buf + imin, dma_buf, p_data -> DMAsize*sizeof(*dma_buf)); 
            free(dma_buf);
        
            ret = CircBufUpdtWr(CIRCBUF(p_data -> p_CrcBufData), used);
            if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
    
            // now insert the indexes in the second circular buffer:
    
            ind.start = imin;
            ind.end = ind.start + (p_data -> DMAsize  -1);
            ind.size = used;
    
            if(CircBufSz(0,IndWrInd) > 0) {
                p_data -> indbuf[IndWrInd[0][0]] = ind;
            } else {
                p_data -> indbuf[IndWrInd[1][0]] = ind;
            }
    
            ret = CircBufUpdtWr(CIRCBUF(p_data -> p_CrcBufInd), 1);
            if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
            
            k++;
           } // if used
       }// if can store indexes
  } // while on k
  
fprintf(stderr,"Writer finished.\n");
return NULL;
}

// ==============================================================================

//
// Verifies size and Checksum
//
int check_DMA_block( CCBFsize_t start, CCBFsize_t end, struct shared_thd_data_str *p_data )
{
elem_t chksum = 0;
size_t m;

if((end - start + 1) != p_data -> DMAsize) {
    fprintf(stderr,"ERROR wrong packet size F:%s L:%d\n",__FILE__,__LINE__); 
    return 1;
   }
    
for(m=start; m<= end; m++) chksum ^= p_data -> buf[m];
if(chksum != 0) {
    fprintf(stderr,"ERROR wrong packet checksum. F:%s L:%d\n",__FILE__,__LINE__); 
    fprintf(stderr, "%x\n", chksum);
    return 1;
   }
return 0;
}

// ==============================================================================

//
//  Reader Thread 
//
void *reader(void *p_usr_in)
{
int ret;
struct shared_thd_data_str *p_data = p_usr_in;

CCBFsize_t IndRdInd[2][2];

int k = 0;

ind_t ind;

while( k < p_data -> Ninsertions) {
    
    // see if we can pop a DMA block:
    ret = CircBufRdInd(CIRCBUF(p_data -> p_CrcBufInd), &IndRdInd);
    if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
    
    if(CircBufSzSum(IndRdInd) > 0) { 
    
       if(CircBufSz(0,IndRdInd) > 0) {
           ind = p_data -> indbuf[IndRdInd[0][0]];
       } else {
           ind = p_data -> indbuf[IndRdInd[1][0]];
       }
    
       ret = check_DMA_block( ind.start, ind.end, p_data );
       if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}

       ret = CircBufUpdtRd(CIRCBUF(p_data -> p_CrcBufData), ind.size);
       if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
    
       ret = CircBufUpdtRd(CIRCBUF(p_data -> p_CrcBufInd), 1);
       if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
         
       k++;
     }
   }
   
fprintf(stderr,"Reader finished: all values OK.\n");
return NULL;    
}

// ==============================================================================

int init_shared_data(struct shared_thd_data_str *p_data, const size_t DMAsize, const int Nbufsz, const int Nindbufsz, const int Ninsertions )
{
int ret;
 
 p_data -> buf = malloc(Nbufsz * sizeof(*(p_data -> buf)));
 if( p_data -> buf == NULL)  {fprintf(stderr,"ERROR: malloc failed. F:%s L:%d\n",__FILE__,__LINE__); return 1;}
  
 p_data -> indbuf = malloc(Nindbufsz * sizeof(*(p_data -> indbuf)));
 if( p_data -> indbuf == NULL)  {fprintf(stderr,"ERROR: malloc failed. F:%s L:%d\n",__FILE__,__LINE__); return 1;}
 
 randomize_struct(p_data -> buf, Nbufsz * sizeof(*(p_data -> buf)));
  
 p_data -> p_CrcBufData = &(p_data -> CrcBufData);
 randomize_struct(p_data -> p_CrcBufData, sizeof(p_data -> CrcBufData));
 ret = CircBufInit(CIRCBUF(p_data -> p_CrcBufData), Nbufsz);    
 if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); return 1;}

  p_data -> p_CrcBufInd = &(p_data -> CrcBufInd);
 randomize_struct(p_data -> p_CrcBufInd, sizeof(p_data -> CrcBufInd));
  ret = CircBufInit(CIRCBUF(p_data -> p_CrcBufInd), Nindbufsz);    
 if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); return 1;}
 
 p_data -> DMAsize = DMAsize;
 p_data -> Ninsertions = Ninsertions;
return 0;
}

// ==============================================================================

int clear_shared_data(struct shared_thd_data_str *p_data)
{
 free(p_data -> buf);
 free(p_data -> indbuf);
return 0;
}

// ==============================================================================

void init_drand48()
{
    srand48(time(NULL));
}

// ==============================================================================

int do_test(const size_t DMAsize, const int Nbufsz, const int Nindbufsz, const int Ninsertions)
{
int ret;
pthread_t writer_thd, reader_thd;
struct shared_thd_data_str shrd_data;

ret = init_shared_data(&shrd_data, DMAsize, Nbufsz, Nindbufsz, Ninsertions);
if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); return 1;}

ret = pthread_create(&writer_thd, NULL, writer, &shrd_data);
if(ret != 0) {fprintf(stderr,"ERROR: cannot create thread. F:%s L:%d\n",__FILE__,__LINE__); return 1;}
ret = pthread_create(&reader_thd, NULL, reader, &shrd_data);
if(ret != 0) {fprintf(stderr,"ERROR: cannot create thread. F:%s L:%d\n",__FILE__,__LINE__); return 1;}

pthread_join(writer_thd, NULL);
pthread_join(reader_thd, NULL);

clear_shared_data(&shrd_data);

return 0;
}

// ==============================================================================

int main(void)
{
size_t DMAsize; // in elements. 
int Nbufsz; // size of the circular buffer, in elements
int Nindbufsz; // size of the index buffer. if >> Nbufsz / DMAsize then Nbufsz is limiting. if <<  Nbufsz / DMAsize then Nindbufsz is limiting
int Ninsertions; // number of DMA packets inserted (one DMA packet: DMAsize x elements)

int ret;

fprintf(stderr,"Randomized test of the ring buffer\n");

init_drand48();

DMAsize = 251; //  let's take a prime for example
Nbufsz = 1024; 
Nindbufsz = 2;
Ninsertions = 100000;

ret = do_test( DMAsize, Nbufsz, Nindbufsz, Ninsertions);
if(ret != 0) {fprintf(stderr,"ERROR. F:%s L:%d\n",__FILE__,__LINE__); return 1;}

Nindbufsz = 300;

ret = do_test( DMAsize, Nbufsz, Nindbufsz, Ninsertions);
if(ret != 0) {fprintf(stderr,"ERROR. F:%s L:%d\n",__FILE__,__LINE__); return 1;}

fprintf(stderr,"OK.\n");

return 0;    
}
