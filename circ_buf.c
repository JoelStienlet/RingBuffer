/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.
 
 */

#include <stddef.h>
#include "circ_buf.h"

// ==============================================================================

// SizeOfBuf : in elements (NOT bytes!) must be >= 2
int CircBufInit(CircBuf_t *p_circ, CCBFsize_t SizeOfBuf)
{
// check that the defined maximum is indeed the maximum:
if(((CCBFsize_t)CCBFsizeMAX + 1) != 0) {
    return __LINE__;
    }
if(SizeOfBuf < 2) {
    return __LINE__; // error!
    }
if(CCBFbigsizeMAX / 2 < SizeOfBuf) {
    return __LINE__; // error!  we need that type to hold up to twice the value of ElemInBuf
    }
if(p_circ == NULL){
    return __LINE__;
    }
p_circ -> ElemInBuf = SizeOfBuf;
p_circ ->RdPos = 0;
p_circ ->WrPos = 0;
return 0;
}

// ==============================================================================

//
// returns the buffers where data can be written
//
// returns 0 if no error.
//
int CircBufWrInd(CircBuf_t *p_circ, CCBFsize_t (*p)[2][2])
{
CCBFsize_t Rd, Wr; 

// some sefety checks and initialization:
if(p == NULL) {
    return __LINE__;
    }
(*p)[0][0] = 1; // set empty ranges
(*p)[0][1] = 0;
(*p)[1][0] = 1;
(*p)[1][1] = 0;
if(p_circ == NULL){
    return __LINE__;
    }

Rd = p_circ -> RdPos;
Wr = p_circ -> WrPos;

if(Rd == 0) {
    if(Wr == 0) {
       // buffer is empty
       // [0]: empty range 
       (*p)[1][0] = 0; 
       (*p)[1][1] = p_circ -> ElemInBuf - 2;
    } else if(Wr == p_circ -> ElemInBuf - 1) {
       // buffer is full! -> empty range
    } else {
       // only one buffer to write to
       (*p)[1][0] = Wr; // 1: write between Wr included, and Size-2 included
       (*p)[1][1] = p_circ -> ElemInBuf - 2;   
    }
    
} else if(Rd == 1) {
    if(Wr == 0) {
       // buffer is full! -> empty range
    } else if(Wr == 1) {
       // buffer is empty
       (*p)[1][0] = 1;
       (*p)[1][1] = p_circ -> ElemInBuf - 1;
    } else { // Wr > Rd (=1)
       // only one buffer to write to as with Rd == 0, but this time we can go up to Size-1 instead of Size-2 included
       (*p)[1][0] = Wr; // 1: write between Wr included, and Size-1 included
       (*p)[1][1] = p_circ -> ElemInBuf - 1;
    }
} else {
    if(Wr < Rd - 1) {
       // one zone
       (*p)[1][0] = Wr; // 1: write between Wr included, and Rd-2 included
       (*p)[1][1] = Rd - 2;
    } else if(Wr == Rd - 1){
       // buffer is full! -> empty range
    } else if(Wr == Rd) {
       // buffer is empty
       (*p)[0][0] = Wr;
       (*p)[0][1] = p_circ -> ElemInBuf - 1;
       (*p)[1][0] = 0;
       (*p)[1][1] = Wr - 2;
    } else { // Wr > Rd
       // two buffers
       (*p)[0][0] = Wr; // 1: write between Wr included, and Size-1 included
       (*p)[0][1] = p_circ -> ElemInBuf - 1;
       (*p)[1][0] = 0;
       (*p)[1][1] = Rd - 2;
    }
}

return 0;
}

// ==============================================================================

//
// returns the buffers where data can be read
//
// returns 0 if no error.
//
int CircBufRdInd(CircBuf_t *p_circ, CCBFsize_t (*p)[2][2])
{
CCBFsize_t Rd, Wr; 

// some sefety checks and initialization:
if(p == NULL) {
    return __LINE__;
   }
(*p)[0][0] = 1; // set empty ranges
(*p)[0][1] = 0;
(*p)[1][0] = 1;
(*p)[1][1] = 0;
if(p_circ == NULL){
    return __LINE__;
   }

Rd = p_circ -> RdPos;
Wr = p_circ -> WrPos;

// special case: buffer is empty:
if(Rd == Wr) {
    // empty buffer: p already filled
} else if(Rd < Wr) {
    // only one block  containing data
    (*p)[1][0] = Rd;
    (*p)[1][1] = Wr - 1;
} else {
  // Rd > Wr
  if(Wr == 0) {
    // only one block  containing data
    (*p)[1][0] = Rd;
    (*p)[1][1] = p_circ -> ElemInBuf - 1;
  } else {
      // Wr > 0
     (*p)[0][0] = Rd; // 0: empty ranges
     (*p)[0][1] = p_circ -> ElemInBuf - 1;
     (*p)[1][0] = 0;
     (*p)[1][1] = Wr - 1;
   }
  }

return 0;
}

// ==============================================================================

//
// Updates the buffer as Nconsumed items have been inserted in the buffer:
//
// Note: there may be a difference between the number of items actually used, and that passed here.
// because DMA for example requires contiguous buffers, sometimes some space may be left unused at the end of the buffer.
// See example 2.
//
int CircBufUpdtWr(CircBuf_t *p_circ, CCBFsize_t Nconsumed)
{   
// TODO: add tests on Nconsumed
CCBFbigsize_t Wr;  // here we need to store up to almost twice the maximum buffer size !
Wr =  p_circ -> WrPos; 
Wr += Nconsumed;
if(Wr >= p_circ -> ElemInBuf) Wr = Wr - p_circ -> ElemInBuf;
p_circ -> WrPos = Wr;
return 0;
}

// ==============================================================================

//
// Updates the buffer as Nconsumed items have been deleted from the buffer:
//
// Note: same remarks as for CircBufUpdtWr()
//
int CircBufUpdtRd(CircBuf_t *p_circ, CCBFsize_t Nconsumed)
{
// TODO: add tests on Nconsumed
CCBFbigsize_t Rd;  // here we need to store up to almost twice the maximum buffer size !
Rd =  p_circ -> RdPos; 
Rd += Nconsumed;
if(Rd >= p_circ -> ElemInBuf) Rd = Rd - p_circ -> ElemInBuf;
p_circ -> RdPos = Rd;
return 0;
}





