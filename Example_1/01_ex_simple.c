/*
  
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.

   
   
 Simple example using the ring buffer
   
 Here we create a ring buffer containing integers.
 
 NOTE: this is the simplest example. 
 !!! No error handling is done here for the sake of simplicity !!! (as this example is only here for teaching purposes) But it should obviously be done for any real-world project!
  Please refer to the header "circ_buf.h" for information on the returned error flags, that must be checked in production.
 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "circ_buf.h"

int main(void)
{
const int Nelem = 100; // size of the ring buffer
const int InsertSize = 20; // size of the table containing the elements to be inserted

CircBuf_t CrcBuf;
int *buf;
CCBFsize_t WrInd[2][2]; // always 2x2
CCBFsize_t RdInd[2][2]; // always 2x2

int to_insert[InsertSize];

int i, m;

size_t Nremaining, NtoCopy, NCopied, Nread, NtoRead; // in number of items, NOT bytes

// -----

for(i=0; i< InsertSize; i++) to_insert[i] = i; // fill the table that we wand to insert in our ring buffer

buf = malloc(Nelem * sizeof(*buf));// -> now we have the raw linear buffer that will be managed using using the circular buffer library

CircBufInit(CIRCBUF(&CrcBuf), Nelem); // initialize our ring buffer manager

CircBufWrInd(CIRCBUF(&CrcBuf), &WrInd); // get buffer indexes of free ranges for writing new data

// check if there is enough space available in the free ranges:  (not required if handled correctly in copy loop. May be useful though.)
if(CircBufSzSum(WrInd) < InsertSize) {fprintf(stderr,"no space in buffer. F:%s L:%d\n",__FILE__,__LINE__); exit(1);}

// copy the data where we have space left:
// Note: here we don't use the variable CrcBuf: because CrcBuf doesn't own the data, it is a helper to manage indexes.
NCopied = 0; for(m=0; m<2; m++) { 
    if(NCopied < InsertSize) { 
        Nremaining = InsertSize - NCopied;
        NtoCopy = CircBufSz(m,WrInd) < Nremaining ? CircBufSz(m,WrInd) : Nremaining;
        memcpy(buf + WrInd[m][0], to_insert + NCopied, NtoCopy*sizeof(*buf)); 
        NCopied += NtoCopy;
       } 
    }

// a little test (not necessary in production, this is just for illustration):
if(NCopied != InsertSize) {fprintf(stderr,"incorrect nber of items copied. F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
   

CircBufUpdtWr(CIRCBUF(&CrcBuf), NCopied); // update the ring buffer manager: tell it how many bytes have been consumed

// Now that the data are written to the buffer and the buffer manager is updated, we want to read the data

CircBufRdInd(CIRCBUF(&CrcBuf), &RdInd); // get buffer indexes of available data

// check & print the data: loop on the buffers
// two versions of the loop are shown:
//   - one where the number of items read is double-checked, 
//   - the other where we loop on the items returned by the circular buffer in the simplest way.
//   
// Double-checked version (ensured that we don't exceed the size of the other array):
Nread = 0; for(m=0; m<2; m++) { 
    if(Nread < InsertSize) { 
        Nremaining = InsertSize - Nread;
        NtoRead = CircBufSz(m,RdInd) < Nremaining ? CircBufSz(m,RdInd) : Nremaining;
        for(i=0; i< NtoRead; i++) {
             if(buf[RdInd[m][0] + i] != to_insert[i]) {fprintf(stderr,"bad data in buffer! F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
             }
        Nread += NtoRead;
       } 
    }

// Without double-check, simplest loop:
for(m=0; m<2; m++) { 
    for(i=RdInd[m][0]; i<= RdInd[m][1]; i++) {
        printf("%d\n", buf[i]);
        }
    }
    

CircBufUpdtRd(CIRCBUF(&CrcBuf), CircBufSzSum(RdInd)); // update the ring buffer manager to free the data, so they can be overwritten

// a little test: check that all space is available again (not necessary in production, this is just for illustration):
CircBufWrInd(CIRCBUF(&CrcBuf), &WrInd);
CircBufRdInd(CIRCBUF(&CrcBuf), &RdInd);
if( (CircBufSzSum(WrInd) != (Nelem-1)) || (CircBufSzSum(RdInd) != 0) ) {fprintf(stderr,"wrong amount of available space! F:%s L:%d\n",__FILE__,__LINE__); exit(1);}

free(buf);

return 0;   
}

