/* Blake2s.c -- BLAKE2s and BLAKE2sp Hash
2023-03-04 : Igor Pavlov : Public domain
2015 : Samuel Neves : Public domain */

#include "Precomp.h"

#include <string.h>

#include "Blake2.h"
#include "CpuArch.h"
#include "RotateDefs.h"

#define rotr32 rotrFixed

#define BLAKE2S_NUM_ROUNDS 10
#define BLAKE2S_FINAL_FLAG (~(UInt32)0)

static const UInt32 k_Blake2s_IV[8] =
{
  0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
  0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL
};

static const Byte k_Blake2s_Sigma[BLAKE2S_NUM_ROUNDS][16] =
{
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
  { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
  {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
  {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
  {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
  { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
  { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
  {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
  { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 } ,
};


static void Blake2s_Init0(CBlake2s *p)
{
  unsigned i;
  for (i = 0; i < 8; i++)
    p->h[i] = k_Blake2s_IV[i];
  p->t[0] = 0;
  p->t[1] = 0;
  p->f[0] = 0;
  p->f[1] = 0;
  p->bufPos = 0;
  p->lastNode_f1 = 0;
}


static void Blake2s_Compress(CBlake2s *p)
{
  UInt32 m[16];
  UInt32 v[16];
  
  {
    unsigned i;
    
    for (i = 0; i < 16; i++)
      m[i] = GetUi32(p->buf + i * sizeof(m[i]));
    
    for (i = 0; i < 8; i++)
      v[i] = p->h[i];
  }

  v[ 8] = k_Blake2s_IV[0];
  v[ 9] = k_Blake2s_IV[1];
  v[10] = k_Blake2s_IV[2];
  v[11] = k_Blake2s_IV[3];
  
  v[12] = p->t[0] ^ k_Blake2s_IV[4];
  v[13] = p->t[1] ^ k_Blake2s_IV[5];
  v[14] = p->f[0] ^ k_Blake2s_IV[6];
  v[15] = p->f[1] ^ k_Blake2s_IV[7];

  #define G(r,i,a,b,c,d) \
    a += b + m[sigma[2*i+0]];  d ^= a; d = rotr32(d, 16);  c += d;  b ^= c; b = rotr32(b, 12); \
    a += b + m[sigma[2*i+1]];  d ^= a; d = rotr32(d,  8);  c += d;  b ^= c; b = rotr32(b,  7); \

  #define R(r) \
    G(r,0,v[ 0],v[ 4],v[ 8],v[12]) \
    G(r,1,v[ 1],v[ 5],v[ 9],v[13]) \
    G(r,2,v[ 2],v[ 6],v[10],v[14]) \
    G(r,3,v[ 3],v[ 7],v[11],v[15]) \
    G(r,4,v[ 0],v[ 5],v[10],v[15]) \
    G(r,5,v[ 1],v[ 6],v[11],v[12]) \
    G(r,6,v[ 2],v[ 7],v[ 8],v[13]) \
    G(r,7,v[ 3],v[ 4],v[ 9],v[14]) \

  {
    unsigned r;
    for (r = 0; r < BLAKE2S_NUM_ROUNDS; r++)
    {
      const Byte *sigma = k_Blake2s_Sigma[r];
      R(r)
    }
    /* R(0); R(1); R(2); R(3); R(4); R(5); R(6); R(7); R(8); R(9); */
  }

  #undef G
  #undef R

  {
    unsigned i;
    for (i = 0; i < 8; i++)
      p->h[i] ^= v[i] ^ v[i + 8];
  }
}


#define Blake2s_Increment_Counter(S, inc) \
  { p->t[0] += (inc); p->t[1] += (p->t[0] < (inc)); }

#define Blake2s_Set_LastBlock(p) \
  { p->f[0] = BLAKE2S_FINAL_FLAG; p->f[1] = p->lastNode_f1; }


static void Blake2s_Update(CBlake2s *p, const Byte *data, size_t size)
{
  while (size != 0)
  {
    unsigned pos = (unsigned)p->bufPos;
    unsigned rem = BLAKE2S_BLOCK_SIZE - pos;

    if (size <= rem)
    {
      memcpy(p->buf + pos, data, size);
      p->bufPos += (UInt32)size;
      return;
    }

    memcpy(p->buf + pos, data, rem);
    Blake2s_Increment_Counter(S, BLAKE2S_BLOCK_SIZE)
    Blake2s_Compress(p);
    p->bufPos = 0;
    data += rem;
    size -= rem;
  }
}


static void Blake2s_Final(CBlake2s *p, Byte *digest)
{
  unsigned i;

  Blake2s_Increment_Counter(S, (UInt32)p->bufPos)
  Blake2s_Set_LastBlock(p)
  memset(p->buf + p->bufPos, 0, BLAKE2S_BLOCK_SIZE - p->bufPos);
  Blake2s_Compress(p);

  for (i = 0; i < 8; i++)
  {
    SetUi32(digest + sizeof(p->h[i]) * i, p->h[i])
  }
}


/* ---------- BLAKE2s ---------- */

/* we need to xor CBlake2s::h[i] with input parameter block after Blake2s_Init0() */
/*
typedef struct
{
  Byte  digest_length;
  Byte  key_length;
  Byte  fanout;
  Byte  depth;
  UInt32 leaf_length;
  Byte  node_offset[6];
  Byte  node_depth;
  Byte  inner_length;
  Byte  salt[BLAKE2S_SALTBYTES];
  Byte  personal[BLAKE2S_PERSONALBYTES];
} CBlake2sParam;
*/


static void Blake2sp_Init_Spec(CBlake2s *p, unsigned node_offset, unsigned node_depth)
{
  Blake2s_Init0(p);
  
  p->h[0] ^= (BLAKE2S_DIGEST_SIZE | ((UInt32)BLAKE2SP_PARALLEL_DEGREE << 16) | ((UInt32)2 << 24));
  p->h[2] ^= ((UInt32)node_offset);
  p->h[3] ^= ((UInt32)node_depth << 16) | ((UInt32)BLAKE2S_DIGEST_SIZE << 24);
  /*
  P->digest_length = BLAKE2S_DIGEST_SIZE;
  P->key_length = 0;
  P->fanout = BLAKE2SP_PARALLEL_DEGREE;
  P->depth = 2;
  P->leaf_length = 0;
  store48(P->node_offset, node_offset);
  P->node_depth = node_depth;
  P->inner_length = BLAKE2S_DIGEST_SIZE;
  */
}


void Blake2sp_Init(CBlake2sp *p)
{
  unsigned i;
  
  p->bufPos = 0;

  for (i = 0; i < BLAKE2SP_PARALLEL_DEGREE; i++)
    Blake2sp_Init_Spec(&p->S[i], i, 0);

  p->S[BLAKE2SP_PARALLEL_DEGREE - 1].lastNode_f1 = BLAKE2S_FINAL_FLAG;
}


void Blake2sp_Update(CBlake2sp *p, const Byte *data, size_t size)
{
  unsigned pos = p->bufPos;
  while (size != 0)
  {
    unsigned index = pos / BLAKE2S_BLOCK_SIZE;
    unsigned rem = BLAKE2S_BLOCK_SIZE - (pos & (BLAKE2S_BLOCK_SIZE - 1));
    if (rem > size)
      rem = (unsigned)size;
    Blake2s_Update(&p->S[index], data, rem);
    size -= rem;
    data += rem;
    pos += rem;
    pos &= (BLAKE2S_BLOCK_SIZE * BLAKE2SP_PARALLEL_DEGREE - 1);
  }
  p->bufPos = pos;
}


void Blake2sp_Final(CBlake2sp *p, Byte *digest)
{
  CBlake2s R;
  unsigned i;

  Blake2sp_Init_Spec(&R, 0, 1);
  R.lastNode_f1 = BLAKE2S_FINAL_FLAG;

  for (i = 0; i < BLAKE2SP_PARALLEL_DEGREE; i++)
  {
    Byte hash[BLAKE2S_DIGEST_SIZE];
    Blake2s_Final(&p->S[i], hash);
    Blake2s_Update(&R, hash, BLAKE2S_DIGEST_SIZE);
  }

  Blake2s_Final(&R, digest);
}

#undef rotr32
