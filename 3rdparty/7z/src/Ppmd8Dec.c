/* Ppmd8Dec.c -- Ppmd8 (PPMdI) Decoder
2023-04-02 : Igor Pavlov : Public domain
This code is based on:
  PPMd var.I (2002): Dmitry Shkarin : Public domain
  Carryless rangecoder (1999): Dmitry Subbotin : Public domain */

#include "Precomp.h"

#include "Ppmd8.h"

#define kTop ((UInt32)1 << 24)
#define kBot ((UInt32)1 << 15)

#define READ_BYTE(p) IByteIn_Read((p)->Stream.In)

BoolInt Ppmd8_Init_RangeDec(CPpmd8 *p)
{
  unsigned i;
  p->Code = 0;
  p->Range = 0xFFFFFFFF;
  p->Low = 0;
  
  for (i = 0; i < 4; i++)
    p->Code = (p->Code << 8) | READ_BYTE(p);
  return (p->Code < 0xFFFFFFFF);
}

#define RC_NORM(p) \
  while ((p->Low ^ (p->Low + p->Range)) < kTop \
    || (p->Range < kBot && ((p->Range = (0 - p->Low) & (kBot - 1)), 1))) { \
      p->Code = (p->Code << 8) | READ_BYTE(p); \
      p->Range <<= 8; p->Low <<= 8; }

// we must use only one type of Normalization from two: LOCAL or REMOTE
#define RC_NORM_LOCAL(p)    // RC_NORM(p)
#define RC_NORM_REMOTE(p)   RC_NORM(p)

#define R p

Z7_FORCE_INLINE
// Z7_NO_INLINE
static void Ppmd8_RD_Decode(CPpmd8 *p, UInt32 start, UInt32 size)
{
  start *= R->Range;
  R->Low += start;
  R->Code -= start;
  R->Range *= size;
  RC_NORM_LOCAL(R)
}

#define RC_Decode(start, size)  Ppmd8_RD_Decode(p, start, size);
#define RC_DecodeFinal(start, size)  RC_Decode(start, size)  RC_NORM_REMOTE(R)
#define RC_GetThreshold(total)  (R->Code / (R->Range /= (total)))


#define CTX(ref) ((CPpmd8_Context *)Ppmd8_GetContext(p, ref))
// typedef CPpmd8_Context * CTX_PTR;
#define SUCCESSOR(p) Ppmd_GET_SUCCESSOR(p)
void Ppmd8_UpdateModel(CPpmd8 *p);

#define MASK(sym) ((unsigned char *)charMask)[sym]


int Ppmd8_DecodeSymbol(CPpmd8 *p)
{
  size_t charMask[256 / sizeof(size_t)];

  if (p->MinContext->NumStats != 0)
  {
    CPpmd_State *s = Ppmd8_GetStats(p, p->MinContext);
    unsigned i;
    UInt32 count, hiCnt;
    UInt32 summFreq = p->MinContext->Union2.SummFreq;

    PPMD8_CORRECT_SUM_RANGE(p, summFreq)


    count = RC_GetThreshold(summFreq);
    hiCnt = count;
    
    if ((Int32)(count -= s->Freq) < 0)
    {
      Byte sym;
      RC_DecodeFinal(0, s->Freq)
      p->FoundState = s;
      sym = s->Symbol;
      Ppmd8_Update1_0(p);
      return sym;
    }
    
    p->PrevSuccess = 0;
    i = p->MinContext->NumStats;
    
    do
    {
      if ((Int32)(count -= (++s)->Freq) < 0)
      {
        Byte sym;
        RC_DecodeFinal((hiCnt - count) - s->Freq, s->Freq)
        p->FoundState = s;
        sym = s->Symbol;
        Ppmd8_Update1(p);
        return sym;
      }
    }
    while (--i);
    
    if (hiCnt >= summFreq)
      return PPMD8_SYM_ERROR;

    hiCnt -= count;
    RC_Decode(hiCnt, summFreq - hiCnt)
    
    
    PPMD_SetAllBitsIn256Bytes(charMask)
    // i = p->MinContext->NumStats - 1;
    // do { MASK((--s)->Symbol) = 0; } while (--i);
    {
      CPpmd_State *s2 = Ppmd8_GetStats(p, p->MinContext);
      MASK(s->Symbol) = 0;
      do
      {
        unsigned sym0 = s2[0].Symbol;
        unsigned sym1 = s2[1].Symbol;
        s2 += 2;
        MASK(sym0) = 0;
        MASK(sym1) = 0;
      }
      while (s2 < s);
    }
  }
  else
  {
    CPpmd_State *s = Ppmd8Context_OneState(p->MinContext);
    UInt16 *prob = Ppmd8_GetBinSumm(p);
    UInt32 pr = *prob;
    UInt32 size0 = (R->Range >> 14) * pr;
    pr = PPMD_UPDATE_PROB_1(pr);
    
    if (R->Code < size0)
    {
      Byte sym;
      *prob = (UInt16)(pr + (1 << PPMD_INT_BITS));
      
      // RangeDec_DecodeBit0(size0);
      R->Range = size0;
      RC_NORM(R)
      
      
        
      // sym = (p->FoundState = Ppmd8Context_OneState(p->MinContext))->Symbol;
      // Ppmd8_UpdateBin(p);
      {
        unsigned freq = s->Freq;
        CPpmd8_Context *c = CTX(SUCCESSOR(s));
        sym = s->Symbol;
        p->FoundState = s;
        p->PrevSuccess = 1;
        p->RunLength++;
        s->Freq = (Byte)(freq + (freq < 196));
        // NextContext(p);
        if (p->OrderFall == 0 && (const Byte *)c >= p->UnitsStart)
          p->MaxContext = p->MinContext = c;
        else
          Ppmd8_UpdateModel(p);
      }
      return sym;
    }
    
    *prob = (UInt16)pr;
    p->InitEsc = p->ExpEscape[pr >> 10];
    
    // RangeDec_DecodeBit1(rc2, size0);
    R->Low += size0;
    R->Code -= size0;
    R->Range = (R->Range & ~((UInt32)PPMD_BIN_SCALE - 1)) - size0;
    RC_NORM_LOCAL(R)
    
    PPMD_SetAllBitsIn256Bytes(charMask)
    MASK(Ppmd8Context_OneState(p->MinContext)->Symbol) = 0;
    p->PrevSuccess = 0;
  }
  
  for (;;)
  {
    CPpmd_State *s, *s2;
    UInt32 freqSum, count, hiCnt;
    UInt32 freqSum2;
    CPpmd_See *see;
    CPpmd8_Context *mc;
    unsigned numMasked;
    RC_NORM_REMOTE(R)
    mc = p->MinContext;
    numMasked = mc->NumStats;
    
    do
    {
      p->OrderFall++;
      if (!mc->Suffix)
        return PPMD8_SYM_END;
      mc = Ppmd8_GetContext(p, mc->Suffix);
    }
    while (mc->NumStats == numMasked);
    
    s = Ppmd8_GetStats(p, mc);

    {
      unsigned num = (unsigned)mc->NumStats + 1;
      unsigned num2 = num / 2;

      num &= 1;
      hiCnt = (s->Freq & (unsigned)(MASK(s->Symbol))) & (0 - (UInt32)num);
      s += num;
      p->MinContext = mc;

      do
      {
        unsigned sym0 = s[0].Symbol;
        unsigned sym1 = s[1].Symbol;
        s += 2;
        hiCnt += (s[-2].Freq & (unsigned)(MASK(sym0)));
        hiCnt += (s[-1].Freq & (unsigned)(MASK(sym1)));
      }
      while (--num2);
    }
    
    see = Ppmd8_MakeEscFreq(p, numMasked, &freqSum);
    freqSum += hiCnt;
    freqSum2 = freqSum;
    PPMD8_CORRECT_SUM_RANGE(R, freqSum2)


    count = RC_GetThreshold(freqSum2);
    
    if (count < hiCnt)
    {
      Byte sym;
      // Ppmd_See_UPDATE(see) // new (see->Summ) value can overflow over 16-bits in some rare cases
      s = Ppmd8_GetStats(p, p->MinContext);
      hiCnt = count;

      
      {
        for (;;)
        {
          count -= s->Freq & (unsigned)(MASK((s)->Symbol)); s++; if ((Int32)count < 0) break;
          // count -= s->Freq & (unsigned)(MASK((s)->Symbol)); s++; if ((Int32)count < 0) break;
        }
      }
      s--;
      RC_DecodeFinal((hiCnt - count) - s->Freq, s->Freq)

      // new (see->Summ) value can overflow over 16-bits in some rare cases
      Ppmd_See_UPDATE(see)
      p->FoundState = s;
      sym = s->Symbol;
      Ppmd8_Update2(p);
      return sym;
    }

    if (count >= freqSum2)
      return PPMD8_SYM_ERROR;
    
    RC_Decode(hiCnt, freqSum2 - hiCnt)
    
    // We increase (see->Summ) for sum of Freqs of all non_Masked symbols.
    // new (see->Summ) value can overflow over 16-bits in some rare cases
    see->Summ = (UInt16)(see->Summ + freqSum);
    
    s = Ppmd8_GetStats(p, p->MinContext);
    s2 = s + p->MinContext->NumStats + 1;
    do
    {
      MASK(s->Symbol) = 0;
      s++;
    }
    while (s != s2);
  }
}

#undef kTop
#undef kBot
#undef READ_BYTE
#undef RC_NORM_BASE
#undef RC_NORM_1
#undef RC_NORM
#undef RC_NORM_LOCAL
#undef RC_NORM_REMOTE
#undef R
#undef RC_Decode
#undef RC_DecodeFinal
#undef RC_GetThreshold
#undef CTX
#undef SUCCESSOR
#undef MASK
