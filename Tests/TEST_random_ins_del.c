/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.
 
 
 The purpose: randomly insert/delete elements from the ringbuffer to test it. We check that we get the inserted elements back: that we get them back, and only once as expected, in the order expected. We also check that we don't get elements that were never inserted.
 - create a table with random integers to be inserted
 - randomly insert/delete elements from the buffer, and check the value of the deleted elements
  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <time.h>

#include "circ_buf.h"


typedef uint32_t elem_t;
#define MAX_VAL_ELEM UINT32_MAX


#define PRINTLEVEL 0
// currently used: 0, 5, 10


int rand_test(const size_t nber_test_elems, elem_t *tab_vals, CircBuf_t *p_CrcBuf, elem_t *buf);

// ==============================================================================

// allocates and fills the table that contains the test elements (elements inserted in the ring buffer, then retrieved)
//
int init_test_tab(const size_t N, elem_t **p_tab_vals)
{
size_t i;

*p_tab_vals = malloc(N * sizeof(**p_tab_vals));
    
if(*p_tab_vals == NULL) {
    fprintf(stderr,"malloc failed. F:%s L:%d\n",__FILE__,__LINE__);
    return 1;
    }

for(i=0; i< N; i++) {
    (*p_tab_vals)[i] = MAX_VAL_ELEM * drand48();
    }
    
    
return 0;
}

// ==============================================================================

void print_ind_table(CCBFsize_t IndTab[2][2], char *comment) // always 2x2
{
    fprintf(stderr,"-------\n");
    fprintf(stderr,"|   %s\n", comment);
    fprintf(stderr,"| %d %d\n", (int)IndTab[0][0], (int)IndTab[0][1] );
    fprintf(stderr,"| %d %d\n", (int)IndTab[1][0], (int)IndTab[1][1] );
    fprintf(stderr,"-------\n");
}

// ==============================================================================

void randomize_struct(void *mem, size_t size)
{
uint8_t *tab_bytes = mem;
size_t i;
for(i=0; i< size; i++) tab_bytes[i] = 256 * drand48();
}

// ==============================================================================

void init_drand48()
{
    srand48(time(NULL));
}

// ==============================================================================

int main(int argc, char *argv[])
{
int ret;
size_t tmpsz;
const CCBFsize_t MIN_BUFSIZE = 2; // corner case when only one element can be inserted
CCBFsize_t MAX_BUFSIZE = 5;

CCBFsize_t bufsize;

size_t nber_test_elems; // size of tab_vals (table that contains the test elements)
elem_t *tab_vals; // table that contains the test elements

elem_t *buf; // buffer containing the data, managed by the ring buffer library

CircBuf_t *p_CrcBuf; // data for the ring buffer library

fprintf(stderr,"Randomized test of the ring buffer\n");

if(argc != 3) {
    fprintf(stderr,"ERROR: pass the maximum buffer size and the number of elements to insert\n");
    return 1;
   }
ret = sscanf(argv[1],"%zu",&tmpsz);
if(ret != 1) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
MAX_BUFSIZE = tmpsz;
ret = sscanf(argv[2],"%zu",&nber_test_elems);
if(ret != 1) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}


init_drand48();

// check every size in the range MIN_BUFSIZE to MAX_BUFSIZE
// TODO positive control: change one element in the buffer (buf), and check that we do trigger an error
    
   
for(bufsize = MIN_BUFSIZE; bufsize <= MAX_BUFSIZE; bufsize++){
        fprintf(stderr,"buf size under test: %d\n", bufsize);
        
        ret = init_test_tab(nber_test_elems, &tab_vals);
        if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
    
        buf = malloc(bufsize * sizeof(*buf));
        if(buf == NULL) {fprintf(stderr,"malloc failed. F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
        randomize_struct(buf, bufsize * sizeof(*buf));
        
        p_CrcBuf = malloc(sizeof(*p_CrcBuf));
        if(p_CrcBuf == NULL) {fprintf(stderr,"malloc failed. F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
        randomize_struct(p_CrcBuf, sizeof(*p_CrcBuf));
        
        ret = CircBufInit(CIRCBUF(p_CrcBuf), bufsize);    
        if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
        
        ret = rand_test( nber_test_elems, tab_vals, p_CrcBuf, buf);
        if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
        
        free(p_CrcBuf);
        free(buf);
        free(tab_vals);
        } // loop on bufsize
   
fprintf(stderr,"OK.\n");

return 0;
}

// ==============================================================================

void copy_items(int printlev, elem_t * target, elem_t *src, size_t Nelems)
{
//memcpy(target, src, Nelems*sizeof(elem_t));
size_t i;
for(i=0; i< Nelems; i++) {
    if(printlev >= 10) fprintf(stderr,"   copied %2lu/%2lu : %u\n", (long unsigned int)i, (long unsigned int)Nelems-1, src[i]);
    target[i] = src[i];
   }
}

// ==============================================================================

//
// Adds and removes a random number of items
// 
int rand_test(const size_t nber_test_elems, elem_t *tab_vals, CircBuf_t *p_CrcBuf, elem_t *buf)
{
int ret;
size_t cur_check_ind = 0; // start of next values to check in tab_vals
size_t cur_write_ind = 0; // start of next values to insert in tab_vals

size_t cur_filling = 0; // size_t instead of CCBFsize_t because this is for checking

CCBFsize_t WrInd[2][2]; // always 2x2
CCBFsize_t RdInd[2][2]; // always 2x2
    
CCBFsize_t max_add, NtoAdd, max_read, NtoRead;
    
CCBFsize_t NCopied, Nremaining, NtoCopy, Nread; //, NtoRead;

int m;
size_t i;
    
while(nber_test_elems > cur_check_ind)
{
    if(PRINTLEVEL >= 5) fprintf(stderr,"rand test advancement: %d / %d\n", (int)cur_write_ind, (int)nber_test_elems );
// how many elements can be added?
    ret = CircBufWrInd(CIRCBUF(p_CrcBuf), &WrInd);
    if(ret != 0) {fprintf(stderr,"ERROR CircBufWrInd failed. Err %d F:%s L:%d\n",ret, __FILE__,__LINE__); return 1;}
    max_add = CircBufSzSum(WrInd);
    if(max_add > (nber_test_elems - cur_write_ind)) max_add = nber_test_elems - cur_write_ind;
    
// select the number of elements to add:
    
    NtoAdd = (max_add + 1) * drand48();
    if(NtoAdd > max_add) NtoAdd = max_add;
    
// add the elements:
    
    NCopied = 0; for(m=0; m<2; m++) { 
    if(NCopied < NtoAdd) { 
        Nremaining = NtoAdd - NCopied;
        NtoCopy = CircBufSz(m,WrInd) < Nremaining ? CircBufSz(m,WrInd) : Nremaining;
        if(NtoCopy > 0) copy_items(PRINTLEVEL, buf + WrInd[m][0], (tab_vals + cur_write_ind) + NCopied, NtoCopy ); 
        NCopied += NtoCopy;
        cur_filling += NtoCopy;
       } 
    }
    cur_write_ind += NCopied;
    
// update the buffer manager (we just added the elements):
    ret = CircBufUpdtWr(CIRCBUF(p_CrcBuf), NCopied);
    if(ret != 0) {fprintf(stderr,"ERROR CircBufUpdtWr failed. Err %d F:%s L:%d\n",ret,__FILE__,__LINE__); return 1;}
    
// how many elements can be read? (we know this already: cur_filling)
    
    ret = CircBufRdInd(CIRCBUF(p_CrcBuf), &RdInd);
    if(ret != 0) {fprintf(stderr,"ERROR CircBufRdInd failed. Err %d F:%s L:%d\n",ret,__FILE__,__LINE__); return 1;}
    max_read = CircBufSzSum(RdInd);
    
// check that the number given above is correct:
    if(max_read != cur_filling) {
        fprintf(stderr,"incorrect nber of items in buffer. F:%s L:%d\n",__FILE__,__LINE__); 
        fprintf(stderr,"max add: %u  NtoAdd: %u\n", max_add, NtoAdd);
        fprintf(stderr,"max can read %u != cur fill %u\n", (unsigned int)max_read, (unsigned int)cur_filling );
        ret = CircBufWrInd(CIRCBUF(p_CrcBuf), &WrInd);
        if(ret != 0) {fprintf(stderr,"ERROR CircBufWrInd failed. Err %d F:%s L:%d\n",ret,__FILE__,__LINE__); return 1;}
        ret = CircBufRdInd(CIRCBUF(p_CrcBuf), &RdInd);
        if(ret != 0) {fprintf(stderr,"ERROR CircBufRdInd failed. Err %d F:%s L:%d\n",ret,__FILE__,__LINE__); return 1;}
        print_ind_table(WrInd,"W");
        print_ind_table(RdInd,"R");
        return 1;
       }
    
// select the number of elements to read:
    NtoRead = (max_read + 1) * drand48();
    if(NtoRead > max_read) NtoRead = max_read;
    
// read & check the values of the elements:
    Nread = 0; for(m=0; m<2; m++) { 
        for(i=RdInd[m][0]; i<= RdInd[m][1]; i++) {
            if(Nread < NtoRead) {
                if(PRINTLEVEL >= 10) printf("checked %u\n", buf[i]);
                if(cur_check_ind >= nber_test_elems) {fprintf(stderr,"incorrect nber of items in buffer. F:%s L:%d\n",__FILE__,__LINE__); return 1; }
                if(buf[i] != tab_vals[cur_check_ind]) {
                    fprintf(stderr,"incorrect item(s) in buffer. F:%s L:%d\n",__FILE__,__LINE__); 
                    fprintf(stderr,"%d %d --> buf %u != ref val %u\n", (int)i, (int)cur_check_ind, buf[i], tab_vals[cur_check_ind] );
                    fprintf(stderr,"tab ref values:\n");
                    for(int k =0; k<= cur_check_ind; k++) fprintf(stderr,"    %u\n", tab_vals[k]);
                    return 1; 
                    }
                cur_check_ind ++;
                Nread ++;
               } // if Nread < NtoRead
           } // loop on i
        } // loop on m
    
// remove the elements read above:
    ret = CircBufUpdtRd(CIRCBUF(p_CrcBuf), NtoRead);
    if(ret != 0) {fprintf(stderr,"ERROR CircBufUpdtRd Err %d F:%s L:%d\n",ret, __FILE__,__LINE__); return 1;}
    cur_filling -= NtoRead;
    
} // end of while 

    return 0;
}
