/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.
 

  
  The purpose of this header is to customize the library to tailor the sizes of variables to specific hardware and needs.
  
  On some microcontrollers you may want to use uint8_t for the buffer indexes, whereas on x64 you may prefer size_t.
  
*/

#include <stdint.h>

// defines the types of variables that will hold indexes
// remember that this library only handles indexes, not bytes.
// should be unsigned
#define CCBFsizeMAX UINT32_MAX
typedef uint32_t CCBFsize_t;
typedef volatile CCBFsize_t CCBFvolsize_t; // access to the variables of this type must be atomic on the targetted hardware


// defines the types of variables used to perform computation on indexes. Must be able to store up to twice the number of items in the buffer (of type CCBFsize_t).
// example: if the number of items in the buffer never exceeds 128, even uint8_t can suffice for CCBFbigsize_t. But if you're planning to store 200 items in the buffer, you'll have to use uint16_t for CCBFbigsize_t (2 * 200 = 400 > 255).
#define CCBFbigsizeMAX UINT64_MAX
typedef uint64_t CCBFbigsize_t;

