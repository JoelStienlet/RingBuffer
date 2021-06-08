/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.
 
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <zlib.h>


#include "03_ex_parser.h"

// for internal use by the parser
enum parser_status {
    expect_hdr, // just created, no header found yet
    expect_size, // we received a header, now reading the bytes containing the total packet size
    expect_data, // 
    expect_chksum, // 
    end_bad_chksum, // finished, but the checksum is bad
    end_good_packet // everything OK: we got a good packet
};

struct parser_str
{
    enum parser_status status;
   
    size_t magic_size;
    size_t size_size; 
    size_t chksum_size; 
    size_t max_packet_size; // maximum size of a packet!

    size_t skipped; // number of bytes skipped before the magic header    
    size_t cur_fill; // stored the number of bytes already received for the current field. ex: packet size
    uint8_t *tmp_buf; // stores the (yet incomplete) data of the current field. ex: header magic, packet size, chksum ...
    //size_t tmp_buf_size;
    size_t total_pack_size; // will be filled when the size bytes have been received
    
    int (*is_magic)(void *to_check, int size_to_check);
    uLong crc;
    
}; // parser_t;


// ==============================================================================

// amount of bytes skipped before the magic field was detected
size_t parser_query_skipped(parser_t *p_parser)
{
    return p_parser -> skipped;
}

// ==============================================================================

// states of the parser:
// 
//    - expecting a new start : no magic value has been found yet, discard everything that is not a magic
//                              if a magic is found: set the state to "exploring a current hypothesis: find size"
//    
//    - exploring a current hypothesis find size : wait 2 bytes to get the size of the data inside the packet
//           when the size is found:
//               - check if it is <= max size. if not: try to find another magic.... --> here we may make multiple tries!!
//               - if true: switch to "exploring a current hypothesis data"
//    
//    - exploring a current hypothesis data: wait until the size has been received. then switch to "receive checksum"
//         
//    - receive checksum : wait 4 bytes
//                   - if received: check the checksum. if bad: try to find another magic.... --> here we may make multiple tries!!
//
// Two cases: we receive the data one by one, OR we want to check data already in the buffer then the rest received one by one
//    ==> better code a machine that receives the data one by one.
//
int parser_add_byte(parser_t *p_parser, uint8_t byte)
{
    
if(p_parser -> status == expect_hdr) {
    int is_not_mgk;
    // check quickly for the first byte:
    if(p_parser -> cur_fill == 0) {
        is_not_mgk = (p_parser -> is_magic)(&byte, 1);
        if(is_not_mgk) {
            p_parser -> skipped += 1;
            return not_finished; // caller will send next byte!
           } else {
               // we may have a magic start..
               p_parser -> crc = crc32(0L, Z_NULL, 0); // call crc() through a pointer??
           }
       }
    
    // we may have a magic start..
       
    p_parser -> tmp_buf[p_parser -> cur_fill]= byte;
    p_parser -> cur_fill ++;
    
    p_parser -> crc = crc32(p_parser -> crc, &byte, 1); // call crc() through a pointer??
    
    is_not_mgk = (p_parser -> is_magic)(p_parser -> tmp_buf, p_parser -> cur_fill);
    if(is_not_mgk == 0) {
        if(p_parser -> cur_fill == p_parser -> magic_size ) {
            // it may be a header!
            p_parser -> status = expect_size;
            p_parser -> cur_fill = 0;
            return not_finished;
           } else {
            // stil some more bytes to check..
            return not_finished;
           }
       } else {
           // not a "magic" value. we can stop here
           p_parser -> skipped += 1; // p_parser -> cur_fill; BE CAREFUL: only skip one maybe-start byte at a time!!!
           p_parser -> cur_fill = 0;
           return finished_bad_pack; // caller will retry on next byte!
       }
  }
    
    
if(p_parser -> status == expect_size) {
       
    p_parser -> tmp_buf[p_parser -> cur_fill]= byte;
    p_parser -> cur_fill ++;
    p_parser -> crc = crc32(p_parser -> crc, &byte, 1); // call crc() through a pointer??
    
    if(p_parser -> cur_fill < p_parser -> size_size) {
        // waiting for more bytes..
        return not_finished;
    } else {
        // got all the bytes!
        if(p_parser -> size_size == sizeof(uint8_t)) {
            uint8_t *p_size = (void *)p_parser -> tmp_buf;
            p_parser -> total_pack_size = *p_size;
        } else if (p_parser -> size_size == sizeof(uint16_t)) {
            uint16_t *p_size = (void *)p_parser -> tmp_buf;
            p_parser -> total_pack_size = *p_size;
        } else if (p_parser -> size_size == sizeof(uint32_t)) {
            uint32_t *p_size = (void *)p_parser -> tmp_buf;
            p_parser -> total_pack_size = *p_size;
        } else if (p_parser -> size_size == sizeof(uint64_t)) {
            uint64_t *p_size = (void *)p_parser -> tmp_buf;
            p_parser -> total_pack_size = *p_size;
        } else {
            //fprintf(stderr,"unknown size %lu\n",p_parser -> size_size);
            return error;
        }
        
        p_parser -> cur_fill = 0;
        
        if(p_parser -> total_pack_size > p_parser -> max_packet_size){
            // Wrong packet size!
            p_parser -> skipped += 1; // BE CAREFUL: only skip one byte at a time!!!
            return finished_bad_pack; // caller will retry on next byte!
            }
        
        size_t datasize =   p_parser -> total_pack_size - ( p_parser -> magic_size + p_parser -> size_size + p_parser -> chksum_size );    
        if(datasize < 0)  {
            p_parser -> skipped += 1; // BE CAREFUL: only skip one byte at a time!!!
            return finished_bad_pack; // caller will retry on next byte!
           }
    
        p_parser -> status = datasize > 0? expect_data : expect_chksum;
        return not_finished;
    }
    
   } // status == expect_size
   
if(p_parser -> status == expect_data) {
    // don't store the data: only compute the checksum!
    size_t datasize =   p_parser -> total_pack_size - ( p_parser -> magic_size + p_parser -> size_size + p_parser -> chksum_size );  // < 0 already checked
        
    p_parser -> crc = crc32(p_parser -> crc, &byte, 1); // call crc() through a pointer??
    
    p_parser -> cur_fill ++;
    if(p_parser -> cur_fill == datasize) {
         p_parser -> cur_fill = 0;
         p_parser -> status = expect_chksum;
        }
    return not_finished;
  } // status == expect_data

if(p_parser -> status == expect_chksum) {

    p_parser -> tmp_buf[p_parser -> cur_fill]= byte;
    p_parser -> cur_fill ++;
    
    if(p_parser -> cur_fill < p_parser -> chksum_size) {
        // waiting for more bytes..
        return not_finished;
    } else {
        // we got the bytes!
        // check crc:
        uint32_t *p_crc = (void *)p_parser -> tmp_buf;
        
        //printf("%u %u\n", *p_crc, p_parser -> crc);
        
        if(p_parser -> crc == *p_crc) {
            // good packet!
            p_parser -> status = end_good_packet;
            return finished_good_pack;
         } else {
             p_parser -> status = end_bad_chksum;
             p_parser -> skipped += 1; // BE CAREFUL: only skip one byte at a time!!!
             return finished_bad_pack;
         }
    }
   } // status == expect_chksum

if(p_parser -> status == end_good_packet) {
    fprintf(stderr,"ERROR: parsing finished (good packet) but still got new data! F:%s L:%d\n",__FILE__,__LINE__);
  }
  
if(p_parser -> status == end_bad_chksum) {
    fprintf(stderr,"ERROR: parsing finished (bad checksum) but still got new data! F:%s L:%d\n",__FILE__,__LINE__);
  }
 
fprintf(stderr,"unknown status %d F:%s L:%d\n",p_parser -> status, __FILE__,__LINE__);
fprintf(stderr,"expect_hdr : %d\n",expect_hdr);
fprintf(stderr,"expect_size : %d\n",expect_size);
fprintf(stderr,"expect_data : %d\n",expect_data);
fprintf(stderr,"expect_chksum : %d\n",expect_chksum);
fprintf(stderr,"end_bad_chksum : %d\n",end_bad_chksum);
fprintf(stderr,"end_good_packet : %d\n",end_good_packet);
return error;
}

// ==============================================================================

void print_parser_status(FILE *f, parser_t *p_parser)
{
 switch(p_parser -> status)
 {
  case expect_hdr:
        fprintf(f,"expect_hdr\n");
        break;
  case expect_size:
        fprintf(f,"expect_size\n");
        break;   
  case expect_data:
        fprintf(f,"expect_data\n");
        break;   
  case expect_chksum:
        fprintf(f,"expect_chksum\n");
        break;
  case end_bad_chksum:
        fprintf(f,"end_bad_chksum\n");
        break;
  case end_good_packet:
        fprintf(f,"end_good_packet\n");
        break;
  default:
        fprintf(f,"- other unknown -\n");
 }
}

// ==============================================================================

int init_parser( parser_t **p_p_parser, 
                 size_t magic_size, 
                 size_t size_size, 
                 size_t chksum_size, 
                 size_t max_packet_size,
                 int (*is_magic)(void *to_check, int size_to_check))
{
*p_p_parser = malloc(sizeof(parser_t));
if(*p_p_parser == NULL) {fprintf(stderr,"ERROR: malloc failed. F:%s L:%d\n",__FILE__,__LINE__); return 1;}
parser_t *p_parser = *p_p_parser;

p_parser -> is_magic = is_magic;

p_parser -> status = expect_hdr;
p_parser -> skipped = 0;
p_parser -> cur_fill = 0;
p_parser -> total_pack_size = 0;

p_parser -> magic_size = magic_size;
p_parser -> size_size = size_size;
p_parser -> max_packet_size = max_packet_size;
p_parser -> chksum_size = chksum_size;

size_t min_tmpbufsize = 0;
if(p_parser -> magic_size  > min_tmpbufsize) min_tmpbufsize = p_parser -> magic_size;
if(p_parser -> size_size   > min_tmpbufsize) min_tmpbufsize = p_parser -> size_size;
if(p_parser -> chksum_size > min_tmpbufsize) min_tmpbufsize = p_parser -> chksum_size;
p_parser -> tmp_buf = malloc(min_tmpbufsize);
if(p_parser -> tmp_buf == NULL) {fprintf(stderr,"ERROR: malloc failed. F:%s L:%d\n",__FILE__,__LINE__); return 1;}
//p_parser -> tmp_buf_size = 0;

return 0;
}

// ==============================================================================

int reset_parser(parser_t *p_parser)
{
p_parser -> status = expect_hdr;
p_parser -> skipped = 0;
p_parser -> cur_fill = 0;
p_parser -> total_pack_size = 0;

return 0;
}

// ==============================================================================

int clear_parser(parser_t **p_p_parser)
{
parser_t *p_parser = *p_p_parser;
free(p_parser -> tmp_buf);
p_parser -> tmp_buf = NULL;
free(*p_p_parser);
*p_p_parser = NULL;
 return 0;   
}

