/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.

   
The parser needed some tests, even is this is only an example.
   
   
*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <zlib.h>

#include "03_ex_pack.h"
#include "03_ex_parser.h"

typedef struct __attribute__((packed)) test_pack_str
{
    pack_hdr_t hdr;
    uint8_t data[10];
    uint32_t chksum;
} testpack_t;


// ==============================================================================

void fill_crc(testpack_t *p_pack)
{
 uLong crc = crc32(0L, Z_NULL, 0);
 p_pack -> chksum = crc32(crc, (void *)p_pack, sizeof(testpack_t) - sizeof(uint32_t) );
}

// ==============================================================================

void randomize_struct(void *mem, size_t size)
{
uint8_t *tab_bytes = mem;
size_t i;
for(i=0; i< size; i++) tab_bytes[i] = 256 * drand48();
}

// ==============================================================================

//
// callback for parser
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

int main(void)
{
int ret;
printf("test of the parser\n");

testpack_t TestPack;
TestPack.hdr.magic = hdrMAGIC;
TestPack.hdr.size = sizeof(testpack_t);
randomize_struct(&(TestPack.data), sizeof(TestPack.data));
fill_crc(&TestPack);

printf("in test: crc = %u\n", TestPack.chksum);

uint8_t *p_bytes = (void *)(&TestPack);

parser_t *p_parser;
ret = init_parser( &p_parser, 
                 sizeof(TestPack.hdr.magic), 
                 sizeof(TestPack.hdr.size), 
                 sizeof(uint32_t), 
                 10000,
                 check_magic);
if(ret != 0) {printf("ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);}

int i;

print_parser_status(stdout, p_parser);

// add some bad bytes first:
for(i=0; i< 10; i++) {
    ret = parser_add_byte(p_parser, i);
    
    switch(ret) {
        case  not_finished:
            printf("bad i = %d not_finished\n", i);
            break;
        case finished_bad_pack:
            printf("finished_bad_pack\n");
            break;
        case finished_good_pack:
            printf("finished_good_pack\n");
            break;
        case error:
        default:
            printf("ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);
       } // switch
   }
    
print_parser_status(stdout, p_parser);
    
for(i=0; i< sizeof(testpack_t); i++) {

    ret = parser_add_byte(p_parser, p_bytes[i]);
    
    switch(ret) {
        case  not_finished:
            printf("not_finished\n");
            break;
        case finished_bad_pack:
            printf("finished_bad_pack\n");
            break;
        case finished_good_pack:
            printf("finished_good_pack\n");
            break;
        case error:
        default:
            printf("ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);
       } // switch
    
    } // i

print_parser_status(stdout, p_parser);
reset_parser(p_parser);

printf("------ test pack 2  --------\n");

uint8_t testpack2[] = {0x55, 0x55, 0x08, 0x00, 0xdf, 0x46, 0x39, 0x7f}; // packet with 0 data size

print_parser_status(stdout, p_parser);

for(i=0; i< sizeof(testpack2); i++) {

    ret = parser_add_byte(p_parser, testpack2[i]);
    
    switch(ret) {
        case  not_finished:
            printf("not_finished\n");
            break;
        case finished_bad_pack:
            printf("finished_bad_pack\n");
            printf("ERROR F:%s L:%d\n",__FILE__,__LINE__);
            return 1;
            break;
        case finished_good_pack:
            printf("finished_good_pack\n");
            break;
        case error:
        default:
            printf("ERROR F:%s L:%d\n",__FILE__,__LINE__); exit(1);
       } // switch
    
    } // i
printf("\n");
print_parser_status(stdout, p_parser);
    
printf("OK.\n");
return 0;    
}

