/* Blake2.h -- BLAKE2 Hash
2023-03-04 : Igor Pavlov : Public domain
2015 : Samuel Neves : Public domain */

#ifndef ZIP7_INC_BLAKE2_H
#define ZIP7_INC_BLAKE2_H

#include "7zTypes.h"

EXTERN_C_BEGIN

#define BLAKE2S_BLOCK_SIZE 64
#define BLAKE2S_DIGEST_SIZE 32
#define BLAKE2SP_PARALLEL_DEGREE 8

typedef struct
{
  UInt32 h[8];
  UInt32 t[2];
  UInt32 f[2];
  Byte buf[BLAKE2S_BLOCK_SIZE];
  UInt32 bufPos;
  UInt32 lastNode_f1;
  UInt32 dummy[2]; /* for sizeof(CBlake2s) alignment */
} CBlake2s;

/* You need to xor CBlake2s::h[i] with input parameter block after Blake2s_Init0() */
/*
void Blake2s_Init0(CBlake2s *p);
void Blake2s_Update(CBlake2s *p, const Byte *data, size_t size);
void Blake2s_Final(CBlake2s *p, Byte *digest);
*/


typedef struct
{
  CBlake2s S[BLAKE2SP_PARALLEL_DEGREE];
  unsigned bufPos;
} CBlake2sp;


void Blake2sp_Init(CBlake2sp *p);
void Blake2sp_Update(CBlake2sp *p, const Byte *data, size_t size);
void Blake2sp_Final(CBlake2sp *p, Byte *digest);

EXTERN_C_END

#endif
