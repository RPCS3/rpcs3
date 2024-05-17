/* BwtSort.c -- BWT block sorting
2023-04-02 : Igor Pavlov : Public domain */

#include "Precomp.h"

#include "BwtSort.h"
#include "Sort.h"

/* #define BLOCK_SORT_USE_HEAP_SORT */

/* Don't change it !!! */
#define kNumHashBytes 2
#define kNumHashValues (1 << (kNumHashBytes * 8))

/* kNumRefBitsMax must be < (kNumHashBytes * 8) = 16 */
#define kNumRefBitsMax 12

#define BS_TEMP_SIZE kNumHashValues

#ifdef BLOCK_SORT_EXTERNAL_FLAGS

/* 32 Flags in UInt32 word */
#define kNumFlagsBits 5
#define kNumFlagsInWord (1 << kNumFlagsBits)
#define kFlagsMask (kNumFlagsInWord - 1)
#define kAllFlags 0xFFFFFFFF

#else

#define kNumBitsMax 20
#define kIndexMask ((1 << kNumBitsMax) - 1)
#define kNumExtraBits (32 - kNumBitsMax)
#define kNumExtra0Bits (kNumExtraBits - 2)
#define kNumExtra0Mask ((1 << kNumExtra0Bits) - 1)

#define SetFinishedGroupSize(p, size) \
  {  *(p) |= ((((size) - 1) & kNumExtra0Mask) << kNumBitsMax); \
    if ((size) > (1 << kNumExtra0Bits)) { \
    *(p) |= 0x40000000;  *((p) + 1) |= ((((size) - 1)>> kNumExtra0Bits) << kNumBitsMax); } } \

static void SetGroupSize(UInt32 *p, UInt32 size)
{
  if (--size == 0)
    return;
  *p |= 0x80000000 | ((size & kNumExtra0Mask) << kNumBitsMax);
  if (size >= (1 << kNumExtra0Bits))
  {
    *p |= 0x40000000;
    p[1] |= ((size >> kNumExtra0Bits) << kNumBitsMax);
  }
}

#endif

/*
SortGroup - is recursive Range-Sort function with HeapSort optimization for small blocks
  "range" is not real range. It's only for optimization.
returns: 1 - if there are groups, 0 - no more groups
*/

static
UInt32
Z7_FASTCALL
SortGroup(UInt32 BlockSize, UInt32 NumSortedBytes, UInt32 groupOffset, UInt32 groupSize, int NumRefBits, UInt32 *Indices
  #ifndef BLOCK_SORT_USE_HEAP_SORT
  , UInt32 left, UInt32 range
  #endif
  )
{
  UInt32 *ind2 = Indices + groupOffset;
  UInt32 *Groups;
  if (groupSize <= 1)
  {
    /*
    #ifndef BLOCK_SORT_EXTERNAL_FLAGS
    SetFinishedGroupSize(ind2, 1)
    #endif
    */
    return 0;
  }
  Groups = Indices + BlockSize + BS_TEMP_SIZE;
  if (groupSize <= ((UInt32)1 << NumRefBits)
      #ifndef BLOCK_SORT_USE_HEAP_SORT
      && groupSize <= range
      #endif
      )
  {
    UInt32 *temp = Indices + BlockSize;
    UInt32 j;
    UInt32 mask, thereAreGroups, group, cg;
    {
      UInt32 gPrev;
      UInt32 gRes = 0;
      {
        UInt32 sp = ind2[0] + NumSortedBytes;
        if (sp >= BlockSize) sp -= BlockSize;
        gPrev = Groups[sp];
        temp[0] = (gPrev << NumRefBits);
      }
      
      for (j = 1; j < groupSize; j++)
      {
        UInt32 sp = ind2[j] + NumSortedBytes;
        UInt32 g;
        if (sp >= BlockSize) sp -= BlockSize;
        g = Groups[sp];
        temp[j] = (g << NumRefBits) | j;
        gRes |= (gPrev ^ g);
      }
      if (gRes == 0)
      {
        #ifndef BLOCK_SORT_EXTERNAL_FLAGS
        SetGroupSize(ind2, groupSize);
        #endif
        return 1;
      }
    }
    
    HeapSort(temp, groupSize);
    mask = (((UInt32)1 << NumRefBits) - 1);
    thereAreGroups = 0;
    
    group = groupOffset;
    cg = (temp[0] >> NumRefBits);
    temp[0] = ind2[temp[0] & mask];

    {
    #ifdef BLOCK_SORT_EXTERNAL_FLAGS
    UInt32 *Flags = Groups + BlockSize;
    #else
    UInt32 prevGroupStart = 0;
    #endif
    
    for (j = 1; j < groupSize; j++)
    {
      UInt32 val = temp[j];
      UInt32 cgCur = (val >> NumRefBits);
      
      if (cgCur != cg)
      {
        cg = cgCur;
        group = groupOffset + j;

        #ifdef BLOCK_SORT_EXTERNAL_FLAGS
        {
        UInt32 t = group - 1;
        Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask));
        }
        #else
        SetGroupSize(temp + prevGroupStart, j - prevGroupStart);
        prevGroupStart = j;
        #endif
      }
      else
        thereAreGroups = 1;
      {
      UInt32 ind = ind2[val & mask];
      temp[j] = ind;
      Groups[ind] = group;
      }
    }

    #ifndef BLOCK_SORT_EXTERNAL_FLAGS
    SetGroupSize(temp + prevGroupStart, j - prevGroupStart);
    #endif
    }

    for (j = 0; j < groupSize; j++)
      ind2[j] = temp[j];
    return thereAreGroups;
  }

  /* Check that all strings are in one group (cannot sort) */
  {
    UInt32 group, j;
    UInt32 sp = ind2[0] + NumSortedBytes; if (sp >= BlockSize) sp -= BlockSize;
    group = Groups[sp];
    for (j = 1; j < groupSize; j++)
    {
      sp = ind2[j] + NumSortedBytes; if (sp >= BlockSize) sp -= BlockSize;
      if (Groups[sp] != group)
        break;
    }
    if (j == groupSize)
    {
      #ifndef BLOCK_SORT_EXTERNAL_FLAGS
      SetGroupSize(ind2, groupSize);
      #endif
      return 1;
    }
  }

  #ifndef BLOCK_SORT_USE_HEAP_SORT
  {
  /* ---------- Range Sort ---------- */
  UInt32 i;
  UInt32 mid;
  for (;;)
  {
    UInt32 j;
    if (range <= 1)
    {
      #ifndef BLOCK_SORT_EXTERNAL_FLAGS
      SetGroupSize(ind2, groupSize);
      #endif
      return 1;
    }
    mid = left + ((range + 1) >> 1);
    j = groupSize;
    i = 0;
    do
    {
      UInt32 sp = ind2[i] + NumSortedBytes; if (sp >= BlockSize) sp -= BlockSize;
      if (Groups[sp] >= mid)
      {
        for (j--; j > i; j--)
        {
          sp = ind2[j] + NumSortedBytes; if (sp >= BlockSize) sp -= BlockSize;
          if (Groups[sp] < mid)
          {
            UInt32 temp = ind2[i]; ind2[i] = ind2[j]; ind2[j] = temp;
            break;
          }
        }
        if (i >= j)
          break;
      }
    }
    while (++i < j);
    if (i == 0)
    {
      range = range - (mid - left);
      left = mid;
    }
    else if (i == groupSize)
      range = (mid - left);
    else
      break;
  }

  #ifdef BLOCK_SORT_EXTERNAL_FLAGS
  {
    UInt32 t = (groupOffset + i - 1);
    UInt32 *Flags = Groups + BlockSize;
    Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask));
  }
  #endif

  {
    UInt32 j;
    for (j = i; j < groupSize; j++)
      Groups[ind2[j]] = groupOffset + i;
  }

  {
  UInt32 res = SortGroup(BlockSize, NumSortedBytes, groupOffset, i, NumRefBits, Indices, left, mid - left);
  return res | SortGroup(BlockSize, NumSortedBytes, groupOffset + i, groupSize - i, NumRefBits, Indices, mid, range - (mid - left));
  }

  }

  #else

  /* ---------- Heap Sort ---------- */

  {
    UInt32 j;
    for (j = 0; j < groupSize; j++)
    {
      UInt32 sp = ind2[j] + NumSortedBytes; if (sp >= BlockSize) sp -= BlockSize;
      ind2[j] = sp;
    }

    HeapSortRef(ind2, Groups, groupSize);

    /* Write Flags */
    {
    UInt32 sp = ind2[0];
    UInt32 group = Groups[sp];

    #ifdef BLOCK_SORT_EXTERNAL_FLAGS
    UInt32 *Flags = Groups + BlockSize;
    #else
    UInt32 prevGroupStart = 0;
    #endif

    for (j = 1; j < groupSize; j++)
    {
      sp = ind2[j];
      if (Groups[sp] != group)
      {
        group = Groups[sp];
        #ifdef BLOCK_SORT_EXTERNAL_FLAGS
        {
        UInt32 t = groupOffset + j - 1;
        Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask));
        }
        #else
        SetGroupSize(ind2 + prevGroupStart, j - prevGroupStart);
        prevGroupStart = j;
        #endif
      }
    }

    #ifndef BLOCK_SORT_EXTERNAL_FLAGS
    SetGroupSize(ind2 + prevGroupStart, j - prevGroupStart);
    #endif
    }
    {
    /* Write new Groups values and Check that there are groups */
    UInt32 thereAreGroups = 0;
    for (j = 0; j < groupSize; j++)
    {
      UInt32 group = groupOffset + j;
      #ifndef BLOCK_SORT_EXTERNAL_FLAGS
      UInt32 subGroupSize = ((ind2[j] & ~0xC0000000) >> kNumBitsMax);
      if ((ind2[j] & 0x40000000) != 0)
        subGroupSize += ((ind2[(size_t)j + 1] >> kNumBitsMax) << kNumExtra0Bits);
      subGroupSize++;
      for (;;)
      {
        UInt32 original = ind2[j];
        UInt32 sp = original & kIndexMask;
        if (sp < NumSortedBytes) sp += BlockSize; sp -= NumSortedBytes;
        ind2[j] = sp | (original & ~kIndexMask);
        Groups[sp] = group;
        if (--subGroupSize == 0)
          break;
        j++;
        thereAreGroups = 1;
      }
      #else
      UInt32 *Flags = Groups + BlockSize;
      for (;;)
      {
        UInt32 sp = ind2[j]; if (sp < NumSortedBytes) sp += BlockSize; sp -= NumSortedBytes;
        ind2[j] = sp;
        Groups[sp] = group;
        if ((Flags[(groupOffset + j) >> kNumFlagsBits] & (1 << ((groupOffset + j) & kFlagsMask))) == 0)
          break;
        j++;
        thereAreGroups = 1;
      }
      #endif
    }
    return thereAreGroups;
    }
  }
  #endif
}

/* conditions: blockSize > 0 */
UInt32 BlockSort(UInt32 *Indices, const Byte *data, UInt32 blockSize)
{
  UInt32 *counters = Indices + blockSize;
  UInt32 i;
  UInt32 *Groups;
  #ifdef BLOCK_SORT_EXTERNAL_FLAGS
  UInt32 *Flags;
  #endif

  /* Radix-Sort for 2 bytes */
  for (i = 0; i < kNumHashValues; i++)
    counters[i] = 0;
  for (i = 0; i < blockSize - 1; i++)
    counters[((UInt32)data[i] << 8) | data[(size_t)i + 1]]++;
  counters[((UInt32)data[i] << 8) | data[0]]++;

  Groups = counters + BS_TEMP_SIZE;
  #ifdef BLOCK_SORT_EXTERNAL_FLAGS
  Flags = Groups + blockSize;
    {
      UInt32 numWords = (blockSize + kFlagsMask) >> kNumFlagsBits;
      for (i = 0; i < numWords; i++)
        Flags[i] = kAllFlags;
    }
  #endif

  {
    UInt32 sum = 0;
    for (i = 0; i < kNumHashValues; i++)
    {
      UInt32 groupSize = counters[i];
      if (groupSize > 0)
      {
        #ifdef BLOCK_SORT_EXTERNAL_FLAGS
        UInt32 t = sum + groupSize - 1;
        Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask));
        #endif
        sum += groupSize;
      }
      counters[i] = sum - groupSize;
    }

    for (i = 0; i < blockSize - 1; i++)
      Groups[i] = counters[((UInt32)data[i] << 8) | data[(size_t)i + 1]];
    Groups[i] = counters[((UInt32)data[i] << 8) | data[0]];

    for (i = 0; i < blockSize - 1; i++)
      Indices[counters[((UInt32)data[i] << 8) | data[(size_t)i + 1]]++] = i;
    Indices[counters[((UInt32)data[i] << 8) | data[0]]++] = i;
    
    #ifndef BLOCK_SORT_EXTERNAL_FLAGS
    {
    UInt32 prev = 0;
    for (i = 0; i < kNumHashValues; i++)
    {
      UInt32 prevGroupSize = counters[i] - prev;
      if (prevGroupSize == 0)
        continue;
      SetGroupSize(Indices + prev, prevGroupSize);
      prev = counters[i];
    }
    }
    #endif
  }

  {
  int NumRefBits;
  UInt32 NumSortedBytes;
  for (NumRefBits = 0; ((blockSize - 1) >> NumRefBits) != 0; NumRefBits++);
  NumRefBits = 32 - NumRefBits;
  if (NumRefBits > kNumRefBitsMax)
    NumRefBits = kNumRefBitsMax;

  for (NumSortedBytes = kNumHashBytes; ; NumSortedBytes <<= 1)
  {
    #ifndef BLOCK_SORT_EXTERNAL_FLAGS
    UInt32 finishedGroupSize = 0;
    #endif
    UInt32 newLimit = 0;
    for (i = 0; i < blockSize;)
    {
      UInt32 groupSize;
      #ifdef BLOCK_SORT_EXTERNAL_FLAGS

      if ((Flags[i >> kNumFlagsBits] & (1 << (i & kFlagsMask))) == 0)
      {
        i++;
        continue;
      }
      for (groupSize = 1;
        (Flags[(i + groupSize) >> kNumFlagsBits] & (1 << ((i + groupSize) & kFlagsMask))) != 0;
        groupSize++);
      
      groupSize++;

      #else

      groupSize = ((Indices[i] & ~0xC0000000) >> kNumBitsMax);
      {
      BoolInt finishedGroup = ((Indices[i] & 0x80000000) == 0);
      if ((Indices[i] & 0x40000000) != 0)
      {
        groupSize += ((Indices[(size_t)i + 1] >> kNumBitsMax) << kNumExtra0Bits);
        Indices[(size_t)i + 1] &= kIndexMask;
      }
      Indices[i] &= kIndexMask;
      groupSize++;
      if (finishedGroup || groupSize == 1)
      {
        Indices[i - finishedGroupSize] &= kIndexMask;
        if (finishedGroupSize > 1)
          Indices[(size_t)(i - finishedGroupSize) + 1] &= kIndexMask;
        {
        UInt32 newGroupSize = groupSize + finishedGroupSize;
        SetFinishedGroupSize(Indices + i - finishedGroupSize, newGroupSize)
        finishedGroupSize = newGroupSize;
        }
        i += groupSize;
        continue;
      }
      finishedGroupSize = 0;
      }

      #endif
      
      if (NumSortedBytes >= blockSize)
      {
        UInt32 j;
        for (j = 0; j < groupSize; j++)
        {
          UInt32 t = (i + j);
          /* Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask)); */
          Groups[Indices[t]] = t;
        }
      }
      else
        if (SortGroup(blockSize, NumSortedBytes, i, groupSize, NumRefBits, Indices
          #ifndef BLOCK_SORT_USE_HEAP_SORT
          , 0, blockSize
          #endif
          ) != 0)
          newLimit = i + groupSize;
      i += groupSize;
    }
    if (newLimit == 0)
      break;
  }
  }
  #ifndef BLOCK_SORT_EXTERNAL_FLAGS
  for (i = 0; i < blockSize;)
  {
    UInt32 groupSize = ((Indices[i] & ~0xC0000000) >> kNumBitsMax);
    if ((Indices[i] & 0x40000000) != 0)
    {
      groupSize += ((Indices[(size_t)i + 1] >> kNumBitsMax) << kNumExtra0Bits);
      Indices[(size_t)i + 1] &= kIndexMask;
    }
    Indices[i] &= kIndexMask;
    groupSize++;
    i += groupSize;
  }
  #endif
  return Groups[0];
}
