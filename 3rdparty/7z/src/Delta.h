/* HuffEnc.c -- functions for Huffman encoding
2023-09-07 : Igor Pavlov : Public domain */

#include "Precomp.h"

#include "HuffEnc.h"
#include "Sort.h"

#define kMaxLen 16
#define NUM_BITS 10
#define MASK ((1u << NUM_BITS) - 1)

#define NUM_COUNTERS 64

#define HUFFMAN_SPEED_OPT

void Huffman_Generate(const UInt32 *freqs, UInt32 *p, Byte *lens, UInt32 numSymbols, UInt32 maxLen)
{
  UInt32 num = 0;
  /* if (maxLen > 10) maxLen = 10; */
  {
    UInt32 i;
    
    #ifdef HUFFMAN_SPEED_OPT
    
    UInt32 counters[NUM_COUNTERS];
    for (i = 0; i < NUM_COUNTERS; i++)
      counters[i] = 0;
    for (i = 0; i < numSymbols; i++)
    {
      UInt32 freq = freqs[i];
      counters[(freq < NUM_COUNTERS - 1) ? freq : NUM_COUNTERS - 1]++;
    }
 
    for (i = 1; i < NUM_COUNTERS; i++)
    {
      UInt32 temp = counters[i];
      counters[i] = num;
      num += temp;
    }

    for (i = 0; i < numSymbols; i++)
    {
      UInt32 freq = freqs[i];
      if (freq == 0)
        lens[i] = 0;
      else
        p[counters[((freq < NUM_COUNTERS - 1) ? freq : NUM_COUNTERS - 1)]++] = i | (freq << NUM_BITS);
    }
    counters[0] = 0;
    HeapSort(p + counters[NUM_COUNTERS - 2], counters[NUM_COUNTERS - 1] - counters[NUM_COUNTERS - 2]);
    
    #else

    for (i = 0; i < numSymbols; i++)
    {
      UInt32 freq = freqs[i];
      if (freq == 0)
        lens[i] = 0;
      else
        p[num++] = i | (freq << NUM_BITS);
    }
    HeapSort(p, num);

    #endif
  }

  if (num < 2)
  {
    unsigned minCode = 0;
    unsigned maxCode = 1;
    if (num == 1)
    {
      maxCode = (unsigned)p[0] & MASK;
      if (maxCode == 0)
        maxCode++;
    }
    p[minCode] = 0;
    p[maxCode] = 1;
    lens[minCode] = lens[maxCode] = 1;
    return;
  }
  
  {
    UInt32 b, e, i;
  
    i = b = e = 0;
    do
    {
      UInt32 n, m, freq;
      n = (i != num && (b == e || (p[i] >> NUM_BITS) <= (p[b] >> NUM_BITS))) ? i++ : b++;
      freq = (p[n] & ~MASK);
      p[n] = (p[n] & MASK) | (e << NUM_BITS);
      m = (i != num && (b == e || (p[i] >> NUM_BITS) <= (p[b] >> NUM_BITS))) ? i++ : b++;
      freq += (p[m] & ~MASK);
      p[m] = (p[m] & MASK) | (e << NUM_BITS);
      p[e] = (p[e] & MASK) | freq;
      e++;
    }
    while (num - e > 1);
    
    {
      UInt32 lenCounters[kMaxLen + 1];
      for (i = 0; i <= kMaxLen; i++)
        lenCounters[i] = 0;
      
      p[--e] &= MASK;
      lenCounters[1] = 2;
      while (e != 0)
      {
        UInt32 len = (p[p[--e] >> NUM_BITS] >> NUM_BITS) + 1;
        p[e] = (p[e] & MASK) | (len << NUM_BITS);
        if (len >= maxLen)
          for (len = maxLen - 1; lenCounters[len] == 0; len--);
        lenCounters[len]--;
        lenCounters[(size_t)len + 1] += 2;
      }
      
      {
        UInt32 len;
        i = 0;
        for (len = maxLen; len != 0; len--)
        {
          UInt32 k;
          for (k = lenCounters[len]; k != 0; k--)
            lens[p[i++] & MASK] = (Byte)len;
        }
      }
      
      {
        UInt32 nextCodes[kMaxLen + 1];
        {
          UInt32 code = 0;
          UInt32 len;
          for (len = 1; len <= kMaxLen; len++)
            nextCodes[len] = code = (code + lenCounters[(size_t)len - 1]) << 1;
        }
        /* if (code + lenCounters[kMaxLen] - 1 != (1 << kMaxLen) - 1) throw 1; */

        {
          UInt32 k;
          for (k = 0; k < numSymbols; k++)
            p[k] = nextCodes[lens[k]]++;
        }
      }
    }
  }
}

#undef kMaxLen
#undef NUM_BITS
#undef MASK
#undef NUM_COUNTERS
#undef HUFFMAN_SPEED_OPT
