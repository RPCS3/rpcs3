/* HuffEnc.h -- Huffman encoding
2023-03-05 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_HUFF_ENC_H
#define ZIP7_INC_HUFF_ENC_H

#include "7zTypes.h"

EXTERN_C_BEGIN

/*
Conditions:
  num <= 1024 = 2 ^ NUM_BITS
  Sum(freqs) < 4M = 2 ^ (32 - NUM_BITS)
  maxLen <= 16 = kMaxLen
  Num_Items(p) >= HUFFMAN_TEMP_SIZE(num)
*/
 
void Huffman_Generate(const UInt32 *freqs, UInt32 *p, Byte *lens, UInt32 num, UInt32 maxLen);

EXTERN_C_END

#endif
