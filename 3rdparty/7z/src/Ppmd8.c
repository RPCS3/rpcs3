/* Ppmd8.c -- PPMdI codec
2023-04-02 : Igor Pavlov : Public domain
This code is based on PPMd var.I (2002): Dmitry Shkarin : Public domain */

#include "Precomp.h"

#include <string.h>

#include "Ppmd8.h"




MY_ALIGN(16)
static const Byte PPMD8_kExpEscape[16] = { 25, 14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
MY_ALIGN(16)
static const UInt16 PPMD8_kInitBinEsc[] = { 0x3CDD, 0x1F3F, 0x59BF, 0x48F3, 0x64A1, 0x5ABC, 0x6632, 0x6051};

#define MAX_FREQ 124
#define UNIT_SIZE 12

#define U2B(nu) ((UInt32)(nu) * UNIT_SIZE)
#define U2I(nu) (p->Units2Indx[(size_t)(nu) - 1])
#define I2U(indx) ((unsigned)p->Indx2Units[indx])


#define REF(ptr) Ppmd_GetRef(p, ptr)

#define STATS_REF(ptr) ((CPpmd_State_Ref)REF(ptr))

#define CTX(ref) ((CPpmd8_Context *)Ppmd8_GetContext(p, ref))
#define STATS(ctx) Ppmd8_GetStats(p, ctx)
#define ONE_STATE(ctx) Ppmd8Context_OneState(ctx)
#define SUFFIX(ctx) CTX((ctx)->Suffix)

typedef CPpmd8_Context * PPMD8_CTX_PTR;

struct CPpmd8_Node_;

typedef Ppmd_Ref_Type(struct CPpmd8_Node_) CPpmd8_Node_Ref;

typedef struct CPpmd8_Node_
{
  UInt32 Stamp;
  
  CPpmd8_Node_Ref Next;
  UInt32 NU;
} CPpmd8_Node;

#define NODE(r)  Ppmd_GetPtr_Type(p, r, CPpmd8_Node)

void Ppmd8_Construct(CPpmd8 *p)
{
  unsigned i, k, m;

  p->Base = NULL;

  for (i = 0, k = 0; i < PPMD_NUM_INDEXES; i++)
  {
    unsigned step = (i >= 12 ? 4 : (i >> 2) + 1);
    do { p->Units2Indx[k++] = (Byte)i; } while (--step);
    p->Indx2Units[i] = (Byte)k;
  }

  p->NS2BSIndx[0] = (0 << 1);
  p->NS2BSIndx[1] = (1 << 1);
  memset(p->NS2BSIndx + 2, (2 << 1), 9);
  memset(p->NS2BSIndx + 11, (3 << 1), 256 - 11);

  for (i = 0; i < 5; i++)
    p->NS2Indx[i] = (Byte)i;
  
  for (m = i, k = 1; i < 260; i++)
  {
    p->NS2Indx[i] = (Byte)m;
    if (--k == 0)
      k = (++m) - 4;
  }

  memcpy(p->ExpEscape, PPMD8_kExpEscape, 16);
}


void Ppmd8_Free(CPpmd8 *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->Base);
  p->Size = 0;
  p->Base = NULL;
}


BoolInt Ppmd8_Alloc(CPpmd8 *p, UInt32 size, ISzAllocPtr alloc)
{
  if (!p->Base || p->Size != size)
  {
    Ppmd8_Free(p, alloc);
    p->AlignOffset = (4 - size) & 3;
    if ((p->Base = (Byte *)ISzAlloc_Alloc(alloc, p->AlignOffset + size)) == NULL)
      return False;
    p->Size = size;
  }
  return True;
}



// ---------- Internal Memory Allocator ----------






#define EMPTY_NODE 0xFFFFFFFF


static void Ppmd8_InsertNode(CPpmd8 *p, void *node, unsigned indx)
{
  ((CPpmd8_Node *)node)->Stamp = EMPTY_NODE;
  ((CPpmd8_Node *)node)->Next = (CPpmd8_Node_Ref)p->FreeList[indx];
  ((CPpmd8_Node *)node)->NU = I2U(indx);
  p->FreeList[indx] = REF(node);
  p->Stamps[indx]++;
}


static void *Ppmd8_RemoveNode(CPpmd8 *p, unsigned indx)
{
  CPpmd8_Node *node = NODE((CPpmd8_Node_Ref)p->FreeList[indx]);
  p->FreeList[indx] = node->Next;
  p->Stamps[indx]--;

  return node;
}


static void Ppmd8_SplitBlock(CPpmd8 *p, void *ptr, unsigned oldIndx, unsigned newIndx)
{
  unsigned i, nu = I2U(oldIndx) - I2U(newIndx);
  ptr = (Byte *)ptr + U2B(I2U(newIndx));
  if (I2U(i = U2I(nu)) != nu)
  {
    unsigned k = I2U(--i);
    Ppmd8_InsertNode(p, ((Byte *)ptr) + U2B(k), nu - k - 1);
  }
  Ppmd8_InsertNode(p, ptr, i);
}














static void Ppmd8_GlueFreeBlocks(CPpmd8 *p)
{
  /*
  we use first UInt32 field of 12-bytes UNITs as record type stamp
    CPpmd_State    { Byte Symbol; Byte Freq; : Freq != 0xFF
    CPpmd8_Context { Byte NumStats; Byte Flags; UInt16 SummFreq;  : Flags != 0xFF ???
    CPpmd8_Node    { UInt32 Stamp            : Stamp == 0xFFFFFFFF for free record
                                             : Stamp == 0 for guard
    Last 12-bytes UNIT in array is always contains 12-bytes order-0 CPpmd8_Context record
  */
  CPpmd8_Node_Ref n;

  p->GlueCount = 1 << 13;
  memset(p->Stamps, 0, sizeof(p->Stamps));
  
  /* we set guard NODE at LoUnit */
  if (p->LoUnit != p->HiUnit)
    ((CPpmd8_Node *)(void *)p->LoUnit)->Stamp = 0;

  {
    /* Glue free blocks */
    CPpmd8_Node_Ref *prev = &n;
    unsigned i;
    for (i = 0; i < PPMD_NUM_INDEXES; i++)
    {

      CPpmd8_Node_Ref next = (CPpmd8_Node_Ref)p->FreeList[i];
      p->FreeList[i] = 0;
      while (next != 0)
      {
        CPpmd8_Node *node = NODE(next);
        UInt32 nu = node->NU;
        *prev = next;
        next = node->Next;
        if (nu != 0)
        {
          CPpmd8_Node *node2;
          prev = &(node->Next);
          while ((node2 = node + nu)->Stamp == EMPTY_NODE)
          {
            nu += node2->NU;
            node2->NU = 0;
            node->NU = nu;
          }
        }
      }
    }

    *prev = 0;
  }



  






  

  
  
  
  
  
  
  
  
  /* Fill lists of free blocks */
  while (n != 0)
  {
    CPpmd8_Node *node = NODE(n);
    UInt32 nu = node->NU;
    unsigned i;
    n = node->Next;
    if (nu == 0)
      continue;
    for (; nu > 128; nu -= 128, node += 128)
      Ppmd8_InsertNode(p, node, PPMD_NUM_INDEXES - 1);
    if (I2U(i = U2I(nu)) != nu)
    {
      unsigned k = I2U(--i);
      Ppmd8_InsertNode(p, node + k, (unsigned)nu - k - 1);
    }
    Ppmd8_InsertNode(p, node, i);
  }
}


Z7_NO_INLINE
static void *Ppmd8_AllocUnitsRare(CPpmd8 *p, unsigned indx)
{
  unsigned i;
  
  if (p->GlueCount == 0)
  {
    Ppmd8_GlueFreeBlocks(p);
    if (p->FreeList[indx] != 0)
      return Ppmd8_RemoveNode(p, indx);
  }
  
  i = indx;
  
  do
  {
    if (++i == PPMD_NUM_INDEXES)
    {
      UInt32 numBytes = U2B(I2U(indx));
      Byte *us = p->UnitsStart;
      p->GlueCount--;
      return ((UInt32)(us - p->Text) > numBytes) ? (p->UnitsStart = us - numBytes) : (NULL);
    }
  }
  while (p->FreeList[i] == 0);
  
  {
    void *block = Ppmd8_RemoveNode(p, i);
    Ppmd8_SplitBlock(p, block, i, indx);
    return block;
  }
}


static void *Ppmd8_AllocUnits(CPpmd8 *p, unsigned indx)
{
  if (p->FreeList[indx] != 0)
    return Ppmd8_RemoveNode(p, indx);
  {
    UInt32 numBytes = U2B(I2U(indx));
    Byte *lo = p->LoUnit;
    if ((UInt32)(p->HiUnit - lo) >= numBytes)
    {
      p->LoUnit = lo + numBytes;
      return lo;
    }
  }
  return Ppmd8_AllocUnitsRare(p, indx);
}


#define MEM_12_CPY(dest, src, num) \
  { UInt32 *d = (UInt32 *)dest; const UInt32 *z = (const UInt32 *)src; UInt32 n = num; \
    do { d[0] = z[0]; d[1] = z[1]; d[2] = z[2]; z += 3; d += 3; } while (--n); }



static void *ShrinkUnits(CPpmd8 *p, void *oldPtr, unsigned oldNU, unsigned newNU)
{
  unsigned i0 = U2I(oldNU);
  unsigned i1 = U2I(newNU);
  if (i0 == i1)
    return oldPtr;
  if (p->FreeList[i1] != 0)
  {
    void *ptr = Ppmd8_RemoveNode(p, i1);
    MEM_12_CPY(ptr, oldPtr, newNU)
    Ppmd8_InsertNode(p, oldPtr, i0);
    return ptr;
  }
  Ppmd8_SplitBlock(p, oldPtr, i0, i1);
  return oldPtr;
}


static void FreeUnits(CPpmd8 *p, void *ptr, unsigned nu)
{
  Ppmd8_InsertNode(p, ptr, U2I(nu));
}


static void SpecialFreeUnit(CPpmd8 *p, void *ptr)
{
  if ((Byte *)ptr != p->UnitsStart)
    Ppmd8_InsertNode(p, ptr, 0);
  else
  {
    #ifdef PPMD8_FREEZE_SUPPORT
    *(UInt32 *)ptr = EMPTY_NODE; /* it's used for (Flags == 0xFF) check in RemoveBinContexts() */
    #endif
    p->UnitsStart += UNIT_SIZE;
  }
}


/*
static void *MoveUnitsUp(CPpmd8 *p, void *oldPtr, unsigned nu)
{
  unsigned indx = U2I(nu);
  void *ptr;
  if ((Byte *)oldPtr > p->UnitsStart + (1 << 14) || REF(oldPtr) > p->FreeList[indx])
    return oldPtr;
  ptr = Ppmd8_RemoveNode(p, indx);
  MEM_12_CPY(ptr, oldPtr, nu)
  if ((Byte *)oldPtr != p->UnitsStart)
    Ppmd8_InsertNode(p, oldPtr, indx);
  else
    p->UnitsStart += U2B(I2U(indx));
  return ptr;
}
*/

static void ExpandTextArea(CPpmd8 *p)
{
  UInt32 count[PPMD_NUM_INDEXES];
  unsigned i;
 
  memset(count, 0, sizeof(count));
  if (p->LoUnit != p->HiUnit)
    ((CPpmd8_Node *)(void *)p->LoUnit)->Stamp = 0;
  
  {
    CPpmd8_Node *node = (CPpmd8_Node *)(void *)p->UnitsStart;
    while (node->Stamp == EMPTY_NODE)
    {
      UInt32 nu = node->NU;
      node->Stamp = 0;
      count[U2I(nu)]++;
      node += nu;
    }
    p->UnitsStart = (Byte *)node;
  }
  
  for (i = 0; i < PPMD_NUM_INDEXES; i++)
  {
    UInt32 cnt = count[i];
    if (cnt == 0)
      continue;
    {
      CPpmd8_Node_Ref *prev = (CPpmd8_Node_Ref *)&p->FreeList[i];
      CPpmd8_Node_Ref n = *prev;
      p->Stamps[i] -= cnt;
      for (;;)
      {
        CPpmd8_Node *node = NODE(n);
        n = node->Next;
        if (node->Stamp != 0)
        {
          prev = &node->Next;
          continue;
        }
        *prev = n;
        if (--cnt == 0)
          break;
      }
    }
  }
}


#define SUCCESSOR(p) Ppmd_GET_SUCCESSOR(p)
static void Ppmd8State_SetSuccessor(CPpmd_State *p, CPpmd_Void_Ref v)
{
  Ppmd_SET_SUCCESSOR(p, v)
}

#define RESET_TEXT(offs) { p->Text = p->Base + p->AlignOffset + (offs); }

Z7_NO_INLINE
static
void Ppmd8_RestartModel(CPpmd8 *p)
{
  unsigned i, k, m;

  memset(p->FreeList, 0, sizeof(p->FreeList));
  memset(p->Stamps, 0, sizeof(p->Stamps));
  RESET_TEXT(0)
  p->HiUnit = p->Text + p->Size;
  p->LoUnit = p->UnitsStart = p->HiUnit - p->Size / 8 / UNIT_SIZE * 7 * UNIT_SIZE;
  p->GlueCount = 0;

  p->OrderFall = p->MaxOrder;
  p->RunLength = p->InitRL = -(Int32)((p->MaxOrder < 12) ? p->MaxOrder : 12) - 1;
  p->PrevSuccess = 0;

  {
    CPpmd8_Context *mc = (PPMD8_CTX_PTR)(void *)(p->HiUnit -= UNIT_SIZE); /* AllocContext(p); */
    CPpmd_State *s = (CPpmd_State *)p->LoUnit; /* Ppmd8_AllocUnits(p, PPMD_NUM_INDEXES - 1); */
    
    p->LoUnit += U2B(256 / 2);
    p->MaxContext = p->MinContext = mc;
    p->FoundState = s;
    mc->Flags = 0;
    mc->NumStats = 256 - 1;
    mc->Union2.SummFreq = 256 + 1;
    mc->Union4.Stats = REF(s);
    mc->Suffix = 0;

    for (i = 0; i < 256; i++, s++)
    {
      s->Symbol = (Byte)i;
      s->Freq = 1;
      Ppmd8State_SetSuccessor(s, 0);
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  for (i = m = 0; m < 25; m++)
  {
    while (p->NS2Indx[i] == m)
      i++;
    for (k = 0; k < 8; k++)
    {
      unsigned r;
      UInt16 *dest = p->BinSumm[m] + k;
      const UInt16 val = (UInt16)(PPMD_BIN_SCALE - PPMD8_kInitBinEsc[k] / (i + 1));
      for (r = 0; r < 64; r += 8)
        dest[r] = val;
    }
  }

  for (i = m = 0; m < 24; m++)
  {
    unsigned summ;
    CPpmd_See *s;
    while (p->NS2Indx[(size_t)i + 3] == m + 3)
      i++;
    s = p->See[m];
    summ = ((2 * i + 5) << (PPMD_PERIOD_BITS - 4));
    for (k = 0; k < 32; k++, s++)
    {
      s->Summ = (UInt16)summ;
      s->Shift = (PPMD_PERIOD_BITS - 4);
      s->Count = 7;
    }
  }

  p->DummySee.Summ = 0; /* unused */
  p->DummySee.Shift = PPMD_PERIOD_BITS;
  p->DummySee.Count = 64; /* unused */
}


void Ppmd8_Init(CPpmd8 *p, unsigned maxOrder, unsigned restoreMethod)
{
  p->MaxOrder = maxOrder;
  p->RestoreMethod = restoreMethod;
  Ppmd8_RestartModel(p);
}


#define FLAG_RESCALED  (1 << 2)
// #define FLAG_SYM_HIGH  (1 << 3)
#define FLAG_PREV_HIGH (1 << 4)

#define HiBits_Prepare(sym) ((unsigned)(sym) + 0xC0)

#define HiBits_Convert_3(flags) (((flags) >> (8 - 3)) & (1 << 3))
#define HiBits_Convert_4(flags) (((flags) >> (8 - 4)) & (1 << 4))

#define PPMD8_HiBitsFlag_3(sym) HiBits_Convert_3(HiBits_Prepare(sym))
#define PPMD8_HiBitsFlag_4(sym) HiBits_Convert_4(HiBits_Prepare(sym))

// #define PPMD8_HiBitsFlag_3(sym) (0x08 * ((sym) >= 0x40))
// #define PPMD8_HiBitsFlag_4(sym) (0x10 * ((sym) >= 0x40))

/*
Refresh() is called when we remove some symbols (successors) in context.
It increases Escape_Freq for sum of all removed symbols.
*/

static void Refresh(CPpmd8 *p, PPMD8_CTX_PTR ctx, unsigned oldNU, unsigned scale)
{
  unsigned i = ctx->NumStats, escFreq, sumFreq, flags;
  CPpmd_State *s = (CPpmd_State *)ShrinkUnits(p, STATS(ctx), oldNU, (i + 2) >> 1);
  ctx->Union4.Stats = REF(s);

  // #ifdef PPMD8_FREEZE_SUPPORT
  /*
    (ctx->Union2.SummFreq >= ((UInt32)1 << 15)) can be in FREEZE mode for some files.
    It's not good for range coder. So new versions of support fix:
       -   original PPMdI code rev.1
       +   original PPMdI code rev.2
       -   7-Zip default ((PPMD8_FREEZE_SUPPORT is not defined)
       +   7-Zip (p->RestoreMethod >= PPMD8_RESTORE_METHOD_FREEZE)
    if we       use that fixed line, we can lose compatibility with some files created before fix
    if we don't use that fixed line, the program can work incorrectly in FREEZE mode in rare case.
  */
  // if (p->RestoreMethod >= PPMD8_RESTORE_METHOD_FREEZE)
  {
    scale |= (ctx->Union2.SummFreq >= ((UInt32)1 << 15));
  }
  // #endif



  flags = HiBits_Prepare(s->Symbol);
  {
    unsigned freq = s->Freq;
    escFreq = ctx->Union2.SummFreq - freq;
    freq = (freq + scale) >> scale;
    sumFreq = freq;
    s->Freq = (Byte)freq;
  }
 
  do
  {
    unsigned freq = (++s)->Freq;
    escFreq -= freq;
    freq = (freq + scale) >> scale;
    sumFreq += freq;
    s->Freq = (Byte)freq;
    flags |= HiBits_Prepare(s->Symbol);
  }
  while (--i);
  
  ctx->Union2.SummFreq = (UInt16)(sumFreq + ((escFreq + scale) >> scale));
  ctx->Flags = (Byte)((ctx->Flags & (FLAG_PREV_HIGH + FLAG_RESCALED * scale)) + HiBits_Convert_3(flags));
}


static void SWAP_STATES(CPpmd_State *t1, CPpmd_State *t2)
{
  CPpmd_State tmp = *t1;
  *t1 = *t2;
  *t2 = tmp;
}


/*
CutOff() reduces contexts:
  It conversts Successors at MaxOrder to another Contexts to NULL-Successors
  It removes RAW-Successors and NULL-Successors that are not Order-0
      and it removes contexts when it has no Successors.
  if the (Union4.Stats) is close to (UnitsStart), it moves it up.
*/

static CPpmd_Void_Ref CutOff(CPpmd8 *p, PPMD8_CTX_PTR ctx, unsigned order)
{
  int ns = ctx->NumStats;
  unsigned nu;
  CPpmd_State *stats;
  
  if (ns == 0)
  {
    CPpmd_State *s = ONE_STATE(ctx);
    CPpmd_Void_Ref successor = SUCCESSOR(s);
    if ((Byte *)Ppmd8_GetPtr(p, successor) >= p->UnitsStart)
    {
      if (order < p->MaxOrder)
        successor = CutOff(p, CTX(successor), order + 1);
      else
        successor = 0;
      Ppmd8State_SetSuccessor(s, successor);
      if (successor || order <= 9) /* O_BOUND */
        return REF(ctx);
    }
    SpecialFreeUnit(p, ctx);
    return 0;
  }

  nu = ((unsigned)ns + 2) >> 1;
  // ctx->Union4.Stats = STATS_REF(MoveUnitsUp(p, STATS(ctx), nu));
  {
    unsigned indx = U2I(nu);
    stats = STATS(ctx);

    if ((UInt32)((Byte *)stats - p->UnitsStart) <= (1 << 14)
        && (CPpmd_Void_Ref)ctx->Union4.Stats <= p->FreeList[indx])
    {
      void *ptr = Ppmd8_RemoveNode(p, indx);
      ctx->Union4.Stats = STATS_REF(ptr);
      MEM_12_CPY(ptr, (const void *)stats, nu)
      if ((Byte *)stats != p->UnitsStart)
        Ppmd8_InsertNode(p, stats, indx);
      else
        p->UnitsStart += U2B(I2U(indx));
      stats = ptr;
    }
  }

  {
    CPpmd_State *s = stats + (unsigned)ns;
    do
    {
      CPpmd_Void_Ref successor = SUCCESSOR(s);
      if ((Byte *)Ppmd8_GetPtr(p, successor) < p->UnitsStart)
      {
        CPpmd_State *s2 = stats + (unsigned)(ns--);
        if (order)
        {
          if (s != s2)
            *s = *s2;
        }
        else
        {
          SWAP_STATES(s, s2);
          Ppmd8State_SetSuccessor(s2, 0);
        }
      }
      else
      {
        if (order < p->MaxOrder)
          Ppmd8State_SetSuccessor(s, CutOff(p, CTX(successor), order + 1));
        else
          Ppmd8State_SetSuccessor(s, 0);
      }
    }
    while (--s >= stats);
  }
  
  if (ns != ctx->NumStats && order)
  {
    if (ns < 0)
    {
      FreeUnits(p, stats, nu);
      SpecialFreeUnit(p, ctx);
      return 0;
    }
    ctx->NumStats = (Byte)ns;
    if (ns == 0)
    {
      const Byte sym = stats->Symbol;
      ctx->Flags = (Byte)((ctx->Flags & FLAG_PREV_HIGH) + PPMD8_HiBitsFlag_3(sym));
      // *ONE_STATE(ctx) = *stats;
      ctx->Union2.State2.Symbol = sym;
      ctx->Union2.State2.Freq = (Byte)(((unsigned)stats->Freq + 11) >> 3);
      ctx->Union4.State4.Successor_0 = stats->Successor_0;
      ctx->Union4.State4.Successor_1 = stats->Successor_1;
      FreeUnits(p, stats, nu);
    }
    else
    {
      Refresh(p, ctx, nu, ctx->Union2.SummFreq > 16 * (unsigned)ns);
    }
  }
  
  return REF(ctx);
}



#ifdef PPMD8_FREEZE_SUPPORT

/*
RemoveBinContexts()
  It conversts Successors at MaxOrder to another Contexts to NULL-Successors
  It changes RAW-Successors to NULL-Successors
  removes Bin Context without Successor, if suffix of that context is also binary.
*/

static CPpmd_Void_Ref RemoveBinContexts(CPpmd8 *p, PPMD8_CTX_PTR ctx, unsigned order)
{
  if (!ctx->NumStats)
  {
    CPpmd_State *s = ONE_STATE(ctx);
    CPpmd_Void_Ref successor = SUCCESSOR(s);
    if ((Byte *)Ppmd8_GetPtr(p, successor) >= p->UnitsStart && order < p->MaxOrder)
      successor = RemoveBinContexts(p, CTX(successor), order + 1);
    else
      successor = 0;
    Ppmd8State_SetSuccessor(s, successor);
    /* Suffix context can be removed already, since different (high-order)
       Successors may refer to same context. So we check Flags == 0xFF (Stamp == EMPTY_NODE) */
    if (!successor && (!SUFFIX(ctx)->NumStats || SUFFIX(ctx)->Flags == 0xFF))
    {
      FreeUnits(p, ctx, 1);
      return 0;
    }
  }
  else
  {
    CPpmd_State *s = STATS(ctx) + ctx->NumStats;
    do
    {
      CPpmd_Void_Ref successor = SUCCESSOR(s);
      if ((Byte *)Ppmd8_GetPtr(p, successor) >= p->UnitsStart && order < p->MaxOrder)
        Ppmd8State_SetSuccessor(s, RemoveBinContexts(p, CTX(successor), order + 1));
      else
        Ppmd8State_SetSuccessor(s, 0);
    }
    while (--s >= STATS(ctx));
  }
  
  return REF(ctx);
}

#endif



static UInt32 GetUsedMemory(const CPpmd8 *p)
{
  UInt32 v = 0;
  unsigned i;
  for (i = 0; i < PPMD_NUM_INDEXES; i++)
    v += p->Stamps[i] * I2U(i);
  return p->Size - (UInt32)(p->HiUnit - p->LoUnit) - (UInt32)(p->UnitsStart - p->Text) - U2B(v);
}

#ifdef PPMD8_FREEZE_SUPPORT
  #define RESTORE_MODEL(c1, fSuccessor) RestoreModel(p, c1, fSuccessor)
#else
  #define RESTORE_MODEL(c1, fSuccessor) RestoreModel(p, c1)
#endif


static void RestoreModel(CPpmd8 *p, PPMD8_CTX_PTR ctxError
    #ifdef PPMD8_FREEZE_SUPPORT
    , PPMD8_CTX_PTR fSuccessor
    #endif
    )
{
  PPMD8_CTX_PTR c;
  CPpmd_State *s;
  RESET_TEXT(0)

  // we go here in cases of error of allocation for context (c1)
  // Order(MinContext) < Order(ctxError) <= Order(MaxContext)
  
  // We remove last symbol from each of contexts [p->MaxContext ... ctxError) contexts
  // So we rollback all created (symbols) before error.
  for (c = p->MaxContext; c != ctxError; c = SUFFIX(c))
    if (--(c->NumStats) == 0)
    {
      s = STATS(c);
      c->Flags = (Byte)((c->Flags & FLAG_PREV_HIGH) + PPMD8_HiBitsFlag_3(s->Symbol));
      // *ONE_STATE(c) = *s;
      c->Union2.State2.Symbol = s->Symbol;
      c->Union2.State2.Freq = (Byte)(((unsigned)s->Freq + 11) >> 3);
      c->Union4.State4.Successor_0 = s->Successor_0;
      c->Union4.State4.Successor_1 = s->Successor_1;

      SpecialFreeUnit(p, s);
    }
    else
    {
      /* Refresh() can increase Escape_Freq on value of Freq of last symbol, that was added before error.
         so the largest possible increase for Escape_Freq is (8) from value before ModelUpoadet() */
      Refresh(p, c, ((unsigned)c->NumStats + 3) >> 1, 0);
    }
 
  // increase Escape Freq for context [ctxError ... p->MinContext)
  for (; c != p->MinContext; c = SUFFIX(c))
    if (c->NumStats == 0)
    {
      // ONE_STATE(c)
      c->Union2.State2.Freq = (Byte)(((unsigned)c->Union2.State2.Freq + 1) >> 1);
    }
    else if ((c->Union2.SummFreq = (UInt16)(c->Union2.SummFreq + 4)) > 128 + 4 * c->NumStats)
      Refresh(p, c, ((unsigned)c->NumStats + 2) >> 1, 1);

  #ifdef PPMD8_FREEZE_SUPPORT
  if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
  {
    p->MaxContext = fSuccessor;
    p->GlueCount += !(p->Stamps[1] & 1); // why?
  }
  else if (p->RestoreMethod == PPMD8_RESTORE_METHOD_FREEZE)
  {
    while (p->MaxContext->Suffix)
      p->MaxContext = SUFFIX(p->MaxContext);
    RemoveBinContexts(p, p->MaxContext, 0);
    // we change the current mode to (PPMD8_RESTORE_METHOD_FREEZE + 1)
    p->RestoreMethod = PPMD8_RESTORE_METHOD_FREEZE + 1;
    p->GlueCount = 0;
    p->OrderFall = p->MaxOrder;
  }
  else
  #endif
  if (p->RestoreMethod == PPMD8_RESTORE_METHOD_RESTART || GetUsedMemory(p) < (p->Size >> 1))
    Ppmd8_RestartModel(p);
  else
  {
    while (p->MaxContext->Suffix)
      p->MaxContext = SUFFIX(p->MaxContext);
    do
    {
      CutOff(p, p->MaxContext, 0);
      ExpandTextArea(p);
    }
    while (GetUsedMemory(p) > 3 * (p->Size >> 2));
    p->GlueCount = 0;
    p->OrderFall = p->MaxOrder;
  }
  p->MinContext = p->MaxContext;
}



Z7_NO_INLINE
static PPMD8_CTX_PTR Ppmd8_CreateSuccessors(CPpmd8 *p, BoolInt skip, CPpmd_State *s1, PPMD8_CTX_PTR c)
{

  CPpmd_Byte_Ref upBranch = (CPpmd_Byte_Ref)SUCCESSOR(p->FoundState);
  Byte newSym, newFreq, flags;
  unsigned numPs = 0;
  CPpmd_State *ps[PPMD8_MAX_ORDER + 1]; /* fixed over Shkarin's code. Maybe it could work without + 1 too. */
  
  if (!skip)
    ps[numPs++] = p->FoundState;
  
  while (c->Suffix)
  {
    CPpmd_Void_Ref successor;
    CPpmd_State *s;
    c = SUFFIX(c);
    
    if (s1) { s = s1; s1 = NULL; }
    else if (c->NumStats != 0)
    {
      Byte sym = p->FoundState->Symbol;
      for (s = STATS(c); s->Symbol != sym; s++);
      if (s->Freq < MAX_FREQ - 9) { s->Freq++; c->Union2.SummFreq++; }
    }
    else
    {
      s = ONE_STATE(c);
      s->Freq = (Byte)(s->Freq + (!SUFFIX(c)->NumStats & (s->Freq < 24)));
    }
    successor = SUCCESSOR(s);
    if (successor != upBranch)
    {

      c = CTX(successor);
      if (numPs == 0)
      {
        
       
        return c;
      }
      break;
    }
    ps[numPs++] = s;
  }
  
  
  
  
  
  newSym = *(const Byte *)Ppmd8_GetPtr(p, upBranch);
  upBranch++;
  flags = (Byte)(PPMD8_HiBitsFlag_4(p->FoundState->Symbol) + PPMD8_HiBitsFlag_3(newSym));
  
  if (c->NumStats == 0)
    newFreq = c->Union2.State2.Freq;
  else
  {
    UInt32 cf, s0;
    CPpmd_State *s;
    for (s = STATS(c); s->Symbol != newSym; s++);
    cf = (UInt32)s->Freq - 1;
    s0 = (UInt32)c->Union2.SummFreq - c->NumStats - cf;
    /*
    

      max(newFreq)= (s->Freq - 1), when (s0 == 1)


    */
    newFreq = (Byte)(1 + ((2 * cf <= s0) ? (5 * cf > s0) : ((cf + 2 * s0 - 3) / s0)));
  }



  do
  {
    PPMD8_CTX_PTR c1;
    /* = AllocContext(p); */
    if (p->HiUnit != p->LoUnit)
      c1 = (PPMD8_CTX_PTR)(void *)(p->HiUnit -= UNIT_SIZE);
    else if (p->FreeList[0] != 0)
      c1 = (PPMD8_CTX_PTR)Ppmd8_RemoveNode(p, 0);
    else
    {
      c1 = (PPMD8_CTX_PTR)Ppmd8_AllocUnitsRare(p, 0);
      if (!c1)
        return NULL;
    }
    c1->Flags = flags;
    c1->NumStats = 0;
    c1->Union2.State2.Symbol = newSym;
    c1->Union2.State2.Freq = newFreq;
    Ppmd8State_SetSuccessor(ONE_STATE(c1), upBranch);
    c1->Suffix = REF(c);
    Ppmd8State_SetSuccessor(ps[--numPs], REF(c1));
    c = c1;
  }
  while (numPs != 0);
  
  return c;
}


static PPMD8_CTX_PTR ReduceOrder(CPpmd8 *p, CPpmd_State *s1, PPMD8_CTX_PTR c)
{
  CPpmd_State *s = NULL;
  PPMD8_CTX_PTR c1 = c;
  CPpmd_Void_Ref upBranch = REF(p->Text);
  
  #ifdef PPMD8_FREEZE_SUPPORT
  /* The BUG in Shkarin's code was fixed: ps could overflow in CUT_OFF mode. */
  CPpmd_State *ps[PPMD8_MAX_ORDER + 1];
  unsigned numPs = 0;
  ps[numPs++] = p->FoundState;
  #endif

  Ppmd8State_SetSuccessor(p->FoundState, upBranch);
  p->OrderFall++;

  for (;;)
  {
    if (s1)
    {
      c = SUFFIX(c);
      s = s1;
      s1 = NULL;
    }
    else
    {
      if (!c->Suffix)
      {
        #ifdef PPMD8_FREEZE_SUPPORT
        if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
        {
          do { Ppmd8State_SetSuccessor(ps[--numPs], REF(c)); } while (numPs);
          RESET_TEXT(1)
          p->OrderFall = 1;
        }
        #endif
        return c;
      }
      c = SUFFIX(c);
      if (c->NumStats)
      {
        if ((s = STATS(c))->Symbol != p->FoundState->Symbol)
          do { s++; } while (s->Symbol != p->FoundState->Symbol);
        if (s->Freq < MAX_FREQ - 9)
        {
          s->Freq = (Byte)(s->Freq + 2);
          c->Union2.SummFreq = (UInt16)(c->Union2.SummFreq + 2);
        }
      }
      else
      {
        s = ONE_STATE(c);
        s->Freq = (Byte)(s->Freq + (s->Freq < 32));
      }
    }
    if (SUCCESSOR(s))
      break;
    #ifdef PPMD8_FREEZE_SUPPORT
    ps[numPs++] = s;
    #endif
    Ppmd8State_SetSuccessor(s, upBranch);
    p->OrderFall++;
  }
  
  #ifdef PPMD8_FREEZE_SUPPORT
  if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
  {
    c = CTX(SUCCESSOR(s));
    do { Ppmd8State_SetSuccessor(ps[--numPs], REF(c)); } while (numPs);
    RESET_TEXT(1)
    p->OrderFall = 1;
    return c;
  }
  else
  #endif
  if (SUCCESSOR(s) <= upBranch)
  {
    PPMD8_CTX_PTR successor;
    CPpmd_State *s2 = p->FoundState;
    p->FoundState = s;

    successor = Ppmd8_CreateSuccessors(p, False, NULL, c);
    if (!successor)
      Ppmd8State_SetSuccessor(s, 0);
    else
      Ppmd8State_SetSuccessor(s, REF(successor));
    p->FoundState = s2;
  }
  
  {
    CPpmd_Void_Ref successor = SUCCESSOR(s);
    if (p->OrderFall == 1 && c1 == p->MaxContext)
    {
      Ppmd8State_SetSuccessor(p->FoundState, successor);
      p->Text--;
    }
    if (successor == 0)
      return NULL;
    return CTX(successor);
  }
}



void Ppmd8_UpdateModel(CPpmd8 *p);
Z7_NO_INLINE
void Ppmd8_UpdateModel(CPpmd8 *p)
{
  CPpmd_Void_Ref maxSuccessor, minSuccessor = SUCCESSOR(p->FoundState);
  PPMD8_CTX_PTR c;
  unsigned s0, ns, fFreq = p->FoundState->Freq;
  Byte flag, fSymbol = p->FoundState->Symbol;
  {
  CPpmd_State *s = NULL;
  if (p->FoundState->Freq < MAX_FREQ / 4 && p->MinContext->Suffix != 0)
  {
    /* Update Freqs in Suffix Context */

    c = SUFFIX(p->MinContext);
    
    if (c->NumStats == 0)
    {
      s = ONE_STATE(c);
      if (s->Freq < 32)
        s->Freq++;
    }
    else
    {
      Byte sym = p->FoundState->Symbol;
      s = STATS(c);

      if (s->Symbol != sym)
      {
        do
        {
        
          s++;
        }
        while (s->Symbol != sym);
        
        if (s[0].Freq >= s[-1].Freq)
        {
          SWAP_STATES(&s[0], &s[-1]);
          s--;
        }
      }
      
      if (s->Freq < MAX_FREQ - 9)
      {
        s->Freq = (Byte)(s->Freq + 2);
        c->Union2.SummFreq = (UInt16)(c->Union2.SummFreq + 2);
      }
    }
  }
  
  c = p->MaxContext;
  if (p->OrderFall == 0 && minSuccessor)
  {
    PPMD8_CTX_PTR cs = Ppmd8_CreateSuccessors(p, True, s, p->MinContext);
    if (!cs)
    {
      Ppmd8State_SetSuccessor(p->FoundState, 0);
      RESTORE_MODEL(c, CTX(minSuccessor));
      return;
    }
    Ppmd8State_SetSuccessor(p->FoundState, REF(cs));
    p->MinContext = p->MaxContext = cs;
    return;
  }
  



  {
    Byte *text = p->Text;
    *text++ = p->FoundState->Symbol;
    p->Text = text;
    if (text >= p->UnitsStart)
    {
      RESTORE_MODEL(c, CTX(minSuccessor)); /* check it */
      return;
    }
    maxSuccessor = REF(text);
  }

  if (!minSuccessor)
  {
    PPMD8_CTX_PTR cs = ReduceOrder(p, s, p->MinContext);
    if (!cs)
    {
      RESTORE_MODEL(c, NULL);
      return;
    }
    minSuccessor = REF(cs);
  }
  else if ((Byte *)Ppmd8_GetPtr(p, minSuccessor) < p->UnitsStart)
  {
    PPMD8_CTX_PTR cs = Ppmd8_CreateSuccessors(p, False, s, p->MinContext);
    if (!cs)
    {
      RESTORE_MODEL(c, NULL);
      return;
    }
    minSuccessor = REF(cs);
  }
  
  if (--p->OrderFall == 0)
  {
    maxSuccessor = minSuccessor;
    p->Text -= (p->MaxContext != p->MinContext);
  }
  #ifdef PPMD8_FREEZE_SUPPORT
  else if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
  {
    maxSuccessor = minSuccessor;
    RESET_TEXT(0)
    p->OrderFall = 0;
  }
  #endif
  }
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  flag = (Byte)(PPMD8_HiBitsFlag_3(fSymbol));
  s0 = p->MinContext->Union2.SummFreq - (ns = p->MinContext->NumStats) - fFreq;
  
  for (; c != p->MinContext; c = SUFFIX(c))
  {
    unsigned ns1;
    UInt32 sum;
    
    if ((ns1 = c->NumStats) != 0)
    {
      if ((ns1 & 1) != 0)
      {
        /* Expand for one UNIT */
        unsigned oldNU = (ns1 + 1) >> 1;
        unsigned i = U2I(oldNU);
        if (i != U2I((size_t)oldNU + 1))
        {
          void *ptr = Ppmd8_AllocUnits(p, i + 1);
          void *oldPtr;
          if (!ptr)
          {
            RESTORE_MODEL(c, CTX(minSuccessor));
            return;
          }
          oldPtr = STATS(c);
          MEM_12_CPY(ptr, oldPtr, oldNU)
          Ppmd8_InsertNode(p, oldPtr, i);
          c->Union4.Stats = STATS_REF(ptr);
        }
      }
      sum = c->Union2.SummFreq;
      /* max increase of Escape_Freq is 1 here.
         an average increase is 1/3 per symbol */
      sum += (3 * ns1 + 1 < ns);
      /* original PPMdH uses 16-bit variable for (sum) here.
         But (sum < ???). Do we need to truncate (sum) to 16-bit */
      // sum = (UInt16)sum;
    }
    else
    {
      
      CPpmd_State *s = (CPpmd_State*)Ppmd8_AllocUnits(p, 0);
      if (!s)
      {
        RESTORE_MODEL(c, CTX(minSuccessor));
        return;
      }
      {
        unsigned freq = c->Union2.State2.Freq;
        // s = *ONE_STATE(c);
        s->Symbol = c->Union2.State2.Symbol;
        s->Successor_0 = c->Union4.State4.Successor_0;
        s->Successor_1 = c->Union4.State4.Successor_1;
        // Ppmd8State_SetSuccessor(s, c->Union4.Stats);  // call it only for debug purposes to check the order of
                                              // (Successor_0 and Successor_1) in LE/BE.
        c->Union4.Stats = REF(s);
        if (freq < MAX_FREQ / 4 - 1)
          freq <<= 1;
        else
          freq = MAX_FREQ - 4;

        s->Freq = (Byte)freq;

        sum = freq + p->InitEsc + (ns > 2);   // Ppmd8 (> 2)
      }
    }

    {
      CPpmd_State *s = STATS(c) + ns1 + 1;
      UInt32 cf = 2 * (sum + 6) * (UInt32)fFreq;
      UInt32 sf = (UInt32)s0 + sum;
      s->Symbol = fSymbol;
      c->NumStats = (Byte)(ns1 + 1);
      Ppmd8State_SetSuccessor(s, maxSuccessor);
      c->Flags |= flag;
      if (cf < 6 * sf)
      {
        cf = (unsigned)1 + (cf > sf) + (cf >= 4 * sf);
        sum += 4;
        /* It can add (1, 2, 3) to Escape_Freq */
      }
      else
      {
        cf = (unsigned)4 + (cf > 9 * sf) + (cf > 12 * sf) + (cf > 15 * sf);
        sum += cf;
      }

      c->Union2.SummFreq = (UInt16)sum;
      s->Freq = (Byte)cf;
    }

  }
  p->MaxContext = p->MinContext = CTX(minSuccessor);
}
  


Z7_NO_INLINE
static void Ppmd8_Rescale(CPpmd8 *p)
{
  unsigned i, adder, sumFreq, escFreq;
  CPpmd_State *stats = STATS(p->MinContext);
  CPpmd_State *s = p->FoundState;

  /* Sort the list by Freq */
  if (s != stats)
  {
    CPpmd_State tmp = *s;
    do
      s[0] = s[-1];
    while (--s != stats);
    *s = tmp;
  }

  sumFreq = s->Freq;
  escFreq = p->MinContext->Union2.SummFreq - sumFreq;


  
  
  
  
  adder = (p->OrderFall != 0);
  
  #ifdef PPMD8_FREEZE_SUPPORT
  adder |= (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE);
  #endif
  
  sumFreq = (sumFreq + 4 + adder) >> 1;
  i = p->MinContext->NumStats;
  s->Freq = (Byte)sumFreq;
  
  do
  {
    unsigned freq = (++s)->Freq;
    escFreq -= freq;
    freq = (freq + adder) >> 1;
    sumFreq += freq;
    s->Freq = (Byte)freq;
    if (freq > s[-1].Freq)
    {
      CPpmd_State tmp = *s;
      CPpmd_State *s1 = s;
      do
      {
        s1[0] = s1[-1];
      }
      while (--s1 != stats && freq > s1[-1].Freq);
      *s1 = tmp;
    }
  }
  while (--i);
  
  if (s->Freq == 0)
  {
    /* Remove all items with Freq == 0 */
    CPpmd8_Context *mc;
    unsigned numStats, numStatsNew, n0, n1;
    
    i = 0; do { i++; } while ((--s)->Freq == 0);
    
    

    
    escFreq += i;
    mc = p->MinContext;
    numStats = mc->NumStats;
    numStatsNew = numStats - i;
    mc->NumStats = (Byte)(numStatsNew);
    n0 = (numStats + 2) >> 1;
    
    if (numStatsNew == 0)
    {
    
      unsigned freq = (2 * (unsigned)stats->Freq + escFreq - 1) / escFreq;
      if (freq > MAX_FREQ / 3)
        freq = MAX_FREQ / 3;
      mc->Flags = (Byte)((mc->Flags & FLAG_PREV_HIGH) + PPMD8_HiBitsFlag_3(stats->Symbol));
      
      
      
      
      
      s = ONE_STATE(mc);
      *s = *stats;
      s->Freq = (Byte)freq;
      p->FoundState = s;
      Ppmd8_InsertNode(p, stats, U2I(n0));
      return;
    }

    n1 = (numStatsNew + 2) >> 1;
    if (n0 != n1)
      mc->Union4.Stats = STATS_REF(ShrinkUnits(p, stats, n0, n1));
    {
      // here we are for max order only. So Ppmd8_MakeEscFreq() doesn't use mc->Flags
      // but we still need current (Flags & FLAG_PREV_HIGH), if we will convert context to 1-symbol context later.
      /*
      unsigned flags = HiBits_Prepare((s = STATS(mc))->Symbol);
      i = mc->NumStats;
      do { flags |= HiBits_Prepare((++s)->Symbol); } while (--i);
      mc->Flags = (Byte)((mc->Flags & ~FLAG_SYM_HIGH) + HiBits_Convert_3(flags));
      */
    }
  }



  


  {
    CPpmd8_Context *mc = p->MinContext;
    mc->Union2.SummFreq = (UInt16)(sumFreq + escFreq - (escFreq >> 1));
    mc->Flags |= FLAG_RESCALED;
    p->FoundState = STATS(mc);
  }
}


CPpmd_See *Ppmd8_MakeEscFreq(CPpmd8 *p, unsigned numMasked1, UInt32 *escFreq)
{
  CPpmd_See *see;
  const CPpmd8_Context *mc = p->MinContext;
  unsigned numStats = mc->NumStats;
  if (numStats != 0xFF)
  {
    // (3 <= numStats + 2 <= 256)   (3 <= NS2Indx[3] and NS2Indx[256] === 26)
    see = p->See[(size_t)(unsigned)p->NS2Indx[(size_t)numStats + 2] - 3]
        + (mc->Union2.SummFreq > 11 * (numStats + 1))
        + 2 * (unsigned)(2 * numStats < ((unsigned)SUFFIX(mc)->NumStats + numMasked1))
        + mc->Flags;

    {
      // if (see->Summ) field is larger than 16-bit, we need only low 16 bits of Summ
      unsigned summ = (UInt16)see->Summ; // & 0xFFFF
      unsigned r = (summ >> see->Shift);
      see->Summ = (UInt16)(summ - r);
      *escFreq = r + (r == 0);
    }
  }
  else
  {
    see = &p->DummySee;
    *escFreq = 1;
  }
  return see;
}

 
static void Ppmd8_NextContext(CPpmd8 *p)
{
  PPMD8_CTX_PTR c = CTX(SUCCESSOR(p->FoundState));
  if (p->OrderFall == 0 && (const Byte *)c >= p->UnitsStart)
    p->MaxContext = p->MinContext = c;
  else
    Ppmd8_UpdateModel(p);
}
 

void Ppmd8_Update1(CPpmd8 *p)
{
  CPpmd_State *s = p->FoundState;
  unsigned freq = s->Freq;
  freq += 4;
  p->MinContext->Union2.SummFreq = (UInt16)(p->MinContext->Union2.SummFreq + 4);
  s->Freq = (Byte)freq;
  if (freq > s[-1].Freq)
  {
    SWAP_STATES(s, &s[-1]);
    p->FoundState = --s;
    if (freq > MAX_FREQ)
      Ppmd8_Rescale(p);
  }
  Ppmd8_NextContext(p);
}


void Ppmd8_Update1_0(CPpmd8 *p)
{
  CPpmd_State *s = p->FoundState;
  CPpmd8_Context *mc = p->MinContext;
  unsigned freq = s->Freq;
  unsigned summFreq = mc->Union2.SummFreq;
  p->PrevSuccess = (2 * freq >= summFreq); // Ppmd8 (>=)
  p->RunLength += (int)p->PrevSuccess;
  mc->Union2.SummFreq = (UInt16)(summFreq + 4);
  freq += 4;
  s->Freq = (Byte)freq;
  if (freq > MAX_FREQ)
    Ppmd8_Rescale(p);
  Ppmd8_NextContext(p);
}


/*
void Ppmd8_UpdateBin(CPpmd8 *p)
{
  unsigned freq = p->FoundState->Freq;
  p->FoundState->Freq = (Byte)(freq + (freq < 196)); // Ppmd8 (196)
  p->PrevSuccess = 1;
  p->RunLength++;
  Ppmd8_NextContext(p);
}
*/

void Ppmd8_Update2(CPpmd8 *p)
{
  CPpmd_State *s = p->FoundState;
  unsigned freq = s->Freq;
  freq += 4;
  p->RunLength = p->InitRL;
  p->MinContext->Union2.SummFreq = (UInt16)(p->MinContext->Union2.SummFreq + 4);
  s->Freq = (Byte)freq;
  if (freq > MAX_FREQ)
    Ppmd8_Rescale(p);
  Ppmd8_UpdateModel(p);
}

/* H->I changes:
  NS2Indx
  GlueCount, and Glue method
  BinSum
  See / EscFreq
  Ppmd8_CreateSuccessors updates more suffix contexts
  Ppmd8_UpdateModel consts.
  PrevSuccess Update

Flags:
  (1 << 2) - the Context was Rescaled
  (1 << 3) - there is symbol in Stats with (sym >= 0x40) in
  (1 << 4) - main symbol of context is (sym >= 0x40)
*/

#undef RESET_TEXT
#undef FLAG_RESCALED
#undef FLAG_PREV_HIGH
#undef HiBits_Prepare
#undef HiBits_Convert_3
#undef HiBits_Convert_4
#undef PPMD8_HiBitsFlag_3
#undef PPMD8_HiBitsFlag_4
#undef RESTORE_MODEL

#undef MAX_FREQ
#undef UNIT_SIZE
#undef U2B
#undef U2I
#undef I2U

#undef REF
#undef STATS_REF
#undef CTX
#undef STATS
#undef ONE_STATE
#undef SUFFIX
#undef NODE
#undef EMPTY_NODE
#undef MEM_12_CPY
#undef SUCCESSOR
#undef SWAP_STATES
