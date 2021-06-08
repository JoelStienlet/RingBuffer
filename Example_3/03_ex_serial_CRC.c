/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.
   
   
 
  In this example, we simulate a stream coming for example from the serial port.
  This stream contains packets.
  Each packet has a header, a know byte, followed by the size, then the packet content, and finally a CRC checksum.
  
  Sometimes the stream is polluted by random data added in between packets, that the receiver thus has to discard.
  
  Here we don't simulate missing bytes nor altered bytes within packets, so packet loss must be zero in this simulation.
 
 Note about the CRC calculation: here we use crc32() from zlib.
 
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#include <zlib.h>

#include "circ_buf.h"

#include "03_ex_pack.h"
#include "03_ex_parser.h"

struct shared_thd_data_str
{
uint8_t *buf; // containing the raw data, managed by "RingBuf"

CircBuf_t *p_RingBuf; // more convenient

size_t MaxDataSize; // maximum size of data in a data packet. minimum is zero

int Ninsertions; // iterations of the test: nber of packets sent
volatile uint8_t reader_ready;
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

int k = 0;
int m;

CCBFsize_t BufWrInd[2][2];

size_t data_size, full_size;

size_t Nremaining, NtoCopy, NCopied;

double TimeSincePrinted = 0; // in ms
double timeoutPrint = 300; // in ms
struct timeval t_startPrint, t_endPrint, t_deltaPrint;

while(p_data -> reader_ready == 0) {}

gettimeofday( &t_startPrint, NULL);

while( k < p_data -> Ninsertions) { 
    
    // check if we have enough space:
    
    data_size  = (p_data -> MaxDataSize + 1) *drand48();
    if(data_size > p_data -> MaxDataSize) data_size = p_data -> MaxDataSize;
       
    full_size = data_size + sizeof(pack_hdr_t) + sizeof(uint32_t); // crc32
        
    ret = CircBufWrInd(CIRCBUF(p_data -> p_RingBuf), &BufWrInd);
    if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
       
       
    gettimeofday( &t_endPrint, NULL);
    timersub(&t_endPrint, &t_startPrint,  &t_deltaPrint);
    TimeSincePrinted =  (1000. * t_deltaPrint.tv_sec) + ((double)t_deltaPrint.tv_usec / 1000.);
    if(TimeSincePrinted > timeoutPrint) {
            t_startPrint = t_endPrint;
            printf("indexes in writer:   %u : %u  ;  %u : %u\n", BufWrInd[0][0], BufWrInd[0][1], BufWrInd[1][0], BufWrInd[1][1]);
           }
       
    if(CircBufSzSum(BufWrInd) >= full_size) {
        // we have enough size to write our packet
        
        void *packbuf = malloc(full_size);
        if(packbuf == NULL) {fprintf(stderr,"ERROR malloc failed. F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
    
        pack_hdr_t *hdr = packbuf;
        hdr -> magic = hdrMAGIC;
        hdr -> size = full_size;
        
        if(data_size > 0) {
            //memset(packbuf + sizeof(pack_hdr_t), 0, data_size);
            randomize_struct(packbuf + sizeof(pack_hdr_t), data_size);
           }
        
        uLong crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, packbuf, sizeof(pack_hdr_t) + data_size);
    
        uint32_t *p_crc = packbuf + sizeof(pack_hdr_t) + data_size;
        *p_crc = crc;
                
        NCopied = 0; 
        for(m=0; m<2; m++) { 
            if(NCopied < full_size) { 
                Nremaining = full_size - NCopied;
                NtoCopy = CircBufSz(m,BufWrInd) < Nremaining ? CircBufSz(m,BufWrInd) : Nremaining;
                memcpy(p_data -> buf + BufWrInd[m][0], packbuf + NCopied, NtoCopy ); 
                NCopied += NtoCopy;
              }
            }

        if(NCopied != full_size) {fprintf(stderr,"incorrect nber of items copied. F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
        
        ret = CircBufUpdtWr(CIRCBUF(p_data -> p_RingBuf), full_size);
        if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
        
        free(packbuf);
    
        // see if we can add some "noise" here: write useless bytes:
        
        ret = CircBufWrInd(CIRCBUF(p_data -> p_RingBuf), &BufWrInd);
        if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
        if(CircBufSzSum(BufWrInd) > 0) {
        
        int max_bad_bytes = 20;
        if(max_bad_bytes > CircBufSzSum(BufWrInd)) max_bad_bytes = CircBufSzSum(BufWrInd);
        double P_AddBad = 0.3;
        int Nbad = 0;
        if(drand48() < P_AddBad) Nbad = max_bad_bytes * drand48();
           
        if(Nbad > 0) {
            NCopied = 0; 
            for(m=0; m<2; m++) { 
                for(size_t i=BufWrInd[m][0]; i<= BufWrInd[m][1]; i++) {
                    if(NCopied < Nbad) { 
                        p_data -> buf[i] = 255 * drand48();
                        NCopied++;
                        }
                   } // i
               } // m
            } // Nbad > 0
           
           printf("(%d bad bytes written)\n", (int)NCopied);
           
           } // CircBufSzSum(BufWrInd) > 0
        
        k++;
        } else {
          //  printf("no space to write\n");
        }
  } // while  k < Ninsertions

fprintf(stderr,"Writer finished. k = %d\n", k);

return NULL;
}


// ==============================================================================

//
// callback for parser : checks the magic bytes at the beginning of a frame
//
// 1 : different
// 0 : identical up to size_to_check
// 
int check_magic(void *to_check, int size_to_check)
{
    if(size_to_check > sizeof(magic_t)) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}
    
    magic_t ref = hdrMAGIC;
    
    uint8_t *p_ref = (void *)&ref;
    uint8_t *p_to_check = to_check;
    
int i;
for(i=0; i< size_to_check; i++) {
    if(p_ref[i] != p_to_check[i]) return 1;
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

CCBFsize_t RdInd[2][2]; // always 2x2
size_t cur_size, last_size = 0;

double TimeSinceReceived = 0; // in ms
double timeout = 3000; // in ms
struct timeval t_start, t_end, t_delta;

double TimeSincePrinted = 0; // in ms
double timeoutPrint = 300; // in ms
struct timeval t_startPrint, t_endPrint, t_deltaPrint;

gettimeofday( &t_start, NULL);

size_t max_full_pack_size;
max_full_pack_size = p_data -> MaxDataSize + sizeof(pack_hdr_t) + sizeof(uint32_t);

parser_t *p_parser;
ret = init_parser(&p_parser, 
                  sizeof(magic_t), 
                  sizeof(PackSize_t),  
                  sizeof(uint32_t), 
                  max_full_pack_size, 
                  check_magic );
if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}


int nber_bad = 0, nber_good = 0; // count the number of good and bad packets

// the loop here is a little different from the other examples: we use a timeout to stop waiting for new data  
size_t iter = 0, iter_buf_empty = 0;
p_data -> reader_ready = 1;

gettimeofday( &t_startPrint, NULL);

while( TimeSinceReceived < timeout) { 
   
    // we keep the data in the buffer until they form a correct packet, or are discarded
        
    // check is new data are available:

    ret = CircBufRdInd(CIRCBUF(p_data -> p_RingBuf), &RdInd);
    if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__);exit(1);}
    cur_size = CircBufSzSum(RdInd);
          
    if(cur_size == 0) iter_buf_empty++;

    gettimeofday( &t_endPrint, NULL);
    timersub(&t_endPrint, &t_startPrint,  &t_deltaPrint);
    TimeSincePrinted =  (1000. * t_deltaPrint.tv_sec) + ((double)t_deltaPrint.tv_usec / 1000.);
    if(TimeSincePrinted > timeoutPrint) {
         t_startPrint = t_endPrint;
         printf("indexes in reader:   %u : %u  ;  %u : %u\n", RdInd[0][0], RdInd[0][1], RdInd[1][0], RdInd[1][1]);
        }
       
    if(cur_size > last_size) {
        
        // we have received new data: reset timeout
        gettimeofday( &t_start, NULL);
           
        size_t Nread = 0;
        int m = 0;
        size_t to_remove = 0;
        
        while( m<2 ) {
            size_t i=RdInd[m][0];
            while( i <= RdInd[m][1] ) {
                Nread++;
                if(Nread > last_size) {
                    printf("%02x ", p_data -> buf[i]);
                    ret = parser_add_byte(p_parser, p_data -> buf[i]);
                    if(ret == error) {fprintf(stderr," ERROR F:%s L:%d\n",__FILE__,__LINE__);exit(1);}

                    if(ret == finished_good_pack) {
                        printf(" GOOD packet received! Nread = %lu / %lu; m = %d ; i = %lu\n", Nread,cur_size, m,i);
                        nber_good ++;
                        to_remove = Nread;
                        reset_parser(p_parser);
                        }
                    if(ret == finished_bad_pack) {
                        printf(" BAD packet received... m = %d ; i = %lu\n", m,i);
                        printf("   %u : %u  ;  %u : %u\n", RdInd[0][0], RdInd[0][1], RdInd[1][0], RdInd[1][1]);
                        nber_bad ++;
                        to_remove = Nread;
                        reset_parser(p_parser);
                        }
                   }// Nread > last_size
             i++;
            } // i
            m++;
        } // m
        printf("\n Nread : %lu    last : %lu\n", Nread, last_size);
        
        if(to_remove > 0) {
             printf("removing %lu from %lu (last : %lu) (looped on %lu)\n", to_remove, cur_size, last_size, Nread);
             ret = CircBufUpdtRd(CIRCBUF(p_data -> p_RingBuf), to_remove);
             if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__);exit(1);}
             last_size = cur_size - to_remove;
             cur_size = cur_size - to_remove;
            }
        
        last_size = cur_size;
                    
        printf("cur size : %lu\n", cur_size);
      } // if(cur_size > last_size)
       

       /*
       if( cur_size > 100*max_full_pack_size) {
             printf("trash removing %lu from %u\n", 50*max_full_pack_size, cur_size);
             ret = CircBufUpdtRd(CIRCBUF(p_data -> p_CrcBufData), 50*max_full_pack_size);
             if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__);exit(1);}
             last_size = cur_size - 50*max_full_pack_size;
             cur_size = cur_size - 50*max_full_pack_size;
          }
       */

    gettimeofday( &t_end, NULL);
    timersub(&t_end, &t_start,  &t_delta);
    TimeSinceReceived =  (1000. * t_delta.tv_sec) + ((double)t_delta.tv_usec / 1000.);
    
    iter++;
   } // while not timeout
    
clear_parser(&p_parser);
    
printf("indexes at exit from reader:   %u : %u  ;  %u : %u\n", RdInd[0][0], RdInd[0][1], RdInd[1][0], RdInd[1][1]);
printf("cur size : %lu\n", cur_size);
printf("last size : %lu\n", last_size);
printf("good packets: %d\n", nber_good);
printf("bad packets: %d\n",nber_bad );
printf("iter buf empty: %lu / %lu\n", iter_buf_empty, iter);
fprintf(stderr,"Reader finished.\n");
return NULL;    
}

// ==============================================================================

//
// data shared between the reader and writer thread
//
int init_shared_data(struct shared_thd_data_str *p_data, 
                     const int Nbufsz,
                     const size_t MaxDataSize, 
                     const int Ninsertions 
                    )
{
int ret;
 
 p_data -> buf = malloc(Nbufsz * sizeof(*(p_data -> buf))); // raw buffer containing the data of the circular buffer
 if( p_data -> buf == NULL)  {fprintf(stderr,"ERROR: malloc failed. F:%s L:%d\n",__FILE__,__LINE__); return 1;}
 randomize_struct(p_data -> buf, Nbufsz * sizeof(*(p_data -> buf)));
  
 p_data -> p_RingBuf = malloc(sizeof(*(p_data -> p_RingBuf))); // data structure of the circular buffer manager
if( p_data -> p_RingBuf == NULL) {fprintf(stderr,"ERROR: malloc failed. F:%s L:%d\n",__FILE__,__LINE__); return 1;}
 randomize_struct(p_data -> p_RingBuf, sizeof(*(p_data -> p_RingBuf)));
 ret = CircBufInit(CIRCBUF(p_data -> p_RingBuf), Nbufsz);    
 if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); return 1;}
 
 p_data -> MaxDataSize = MaxDataSize;
 p_data -> Ninsertions = Ninsertions;
 
 p_data -> reader_ready = 0;
return 0;
}

// ==============================================================================

int clear_shared_data(struct shared_thd_data_str *p_data)
{
 free(p_data -> buf);
 free(p_data -> p_RingBuf);
 return 0;
}

// ==============================================================================

void init_drand48()
{
    srand48(time(NULL));
}

// ==============================================================================

int main(void)
{
int ret;

pthread_t writer_thd, reader_thd;
struct shared_thd_data_str shrd_data;

const size_t MaxDataSize = 10; // maximum size of the data in a packet
const int Ninsertions = 10;
const size_t Nbufsz = 100; // size of the circular buffer, in bytes (elements are uint8_t here)

fprintf(stderr,"Simulation of serial connection with CRC integrity\n");

init_drand48();

ret = init_shared_data(&shrd_data, Nbufsz, MaxDataSize, Ninsertions );
if(ret != 0) {fprintf(stderr,"ERROR F:%s L:%d\n",__FILE__,__LINE__); return 1;}

ret = pthread_create(&writer_thd, NULL, writer, &shrd_data);
if(ret != 0) {fprintf(stderr,"ERROR: cannot create thread. F:%s L:%d\n",__FILE__,__LINE__); return 1;}
ret = pthread_create(&reader_thd, NULL, reader, &shrd_data);
if(ret != 0) {fprintf(stderr,"ERROR: cannot create thread. F:%s L:%d\n",__FILE__,__LINE__); return 1;}

pthread_join(writer_thd, NULL);
pthread_join(reader_thd, NULL);

clear_shared_data(&shrd_data);

fprintf(stderr,"OK.\n");

return 0;    
}


