/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.
 
 */

#define hdrMAGIC 0x5555
typedef uint16_t magic_t; // try different sizes!

typedef uint16_t PackSize_t;

typedef struct __attribute__((packed)) pack_hdr_str // header of a data packet
{
magic_t magic; // == hdrMAGIC
PackSize_t size; // total packet size, including the header
} pack_hdr_t;

