/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.
 
 */

#include "custom_circ_buf.h"

typedef struct CircBuf_str
{
  CCBFsize_t ElemInBuf; // buffer size in elements (NOT bytes!)
  
  // Read and write indexes (NOT the offset in bytes!)
  // if(RdPos == WrPos) --> buffer empty: no data to be read.
  // WrPos == RdPos-1 (or ElemInBuf-1 when RdPos==0) --> buffer full
  CCBFvolsize_t RdPos; // start index of valid data in buffer
  CCBFvolsize_t WrPos; // next index to write
    
} CircBuf_t;

 

#define CIRCBUF(x) ((CircBuf_t *)x)


// ElemInBuf : in elements (NOT bytes!)
int CircBufInit(CircBuf_t *p_circ, CCBFsize_t SizeOfBuf);


//
// returns the buffers where data can be written
//
// returns 0 if no error.
//
int CircBufWrInd(CircBuf_t *p_circ, CCBFsize_t (*p)[2][2]);

//
// returns the buffers where data can be read
//
// returns 0 if no error.
//
int CircBufRdInd(CircBuf_t *p_circ, CCBFsize_t (*p)[2][2]);

//
// Computes the size from ranges
//
#define CircBufSz(y,x) (1+x[y][1] - x[y][0])
#define CircBufSzSum(x) ((1+x[0][1] - x[0][0]) + (1+x[1][1] - x[1][0]))

//
// Updates the buffer as Nconsumed items have been inserted in the buffer:
//
// Note: there may be a difference between the number of items actually used, and that passed here.
// because DMA for example requires contiguous buffers, sometimes some space may be left unused at the end of the buffer.
// See the example "_ex_DMA.c"
//
int CircBufUpdtWr(CircBuf_t *p_circ, CCBFsize_t Nconsumed);


//
// Updates the buffer as Nconsumed items have been deleted from the buffer:
//
// Note: same remarks as for CircBufUpdtWr()
//
int CircBufUpdtRd(CircBuf_t *p_circ, CCBFsize_t Nconsumed);




