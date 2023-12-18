/* Bra.c -- Branch converters for RISC code
2023-04-02 : Igor Pavlov : Public domain */

#include "Precomp.h"

#include "Bra.h"
#include "CpuArch.h"
#include "RotateDefs.h"

#if defined(MY_CPU_SIZEOF_POINTER) \
    && ( MY_CPU_SIZEOF_POINTER == 4 \
      || MY_CPU_SIZEOF_POINTER == 8)
  #define BR_CONV_USE_OPT_PC_PTR
#endif

#ifdef BR_CONV_USE_OPT_PC_PTR
#define BR_PC_INIT  pc -= (UInt32)(SizeT)p;
#define BR_PC_GET   (pc + (UInt32)(SizeT)p)
#else
#define BR_PC_INIT  pc += (UInt32)size;
#define BR_PC_GET   (pc - (UInt32)(SizeT)(lim - p))
// #define BR_PC_INIT
// #define BR_PC_GET   (pc + (UInt32)(SizeT)(p - data))
#endif

#define BR_CONVERT_VAL(v, c) if (encoding) v += c; else v -= c;
// #define BR_CONVERT_VAL(v, c) if (!encoding) c = (UInt32)0 - c; v += c;

#define Z7_BRANCH_CONV(name) z7_BranchConv_ ## name

#define Z7_BRANCH_FUNC_MAIN(name) \
static \
Z7_FORCE_INLINE \
Z7_ATTRIB_NO_VECTOR \
Byte *Z7_BRANCH_CONV(name)(Byte *p, SizeT size, UInt32 pc, int encoding)

#define Z7_BRANCH_FUNC_IMP(name, m, encoding) \
Z7_NO_INLINE \
Z7_ATTRIB_NO_VECTOR \
Byte *m(name)(Byte *data, SizeT size, UInt32 pc) \
  { return Z7_BRANCH_CONV(name)(data, size, pc, encoding); } \

#ifdef Z7_EXTRACT_ONLY
#define Z7_BRANCH_FUNCS_IMP(name) \
  Z7_BRANCH_FUNC_IMP(name, Z7_BRANCH_CONV_DEC, 0)
#else
#define Z7_BRANCH_FUNCS_IMP(name) \
  Z7_BRANCH_FUNC_IMP(name, Z7_BRANCH_CONV_DEC, 0) \
  Z7_BRANCH_FUNC_IMP(name, Z7_BRANCH_CONV_ENC, 1)
#endif

#if defined(__clang__)
#define BR_EXTERNAL_FOR
#define BR_NEXT_ITERATION  continue;
#else
#define BR_EXTERNAL_FOR    for (;;)
#define BR_NEXT_ITERATION  break;
#endif

#if defined(__clang__) && (__clang_major__ >= 8) \
  || defined(__GNUC__) && (__GNUC__ >= 1000) \
  // GCC is not good for __builtin_expect() here
  /* || defined(_MSC_VER) && (_MSC_VER >= 1920) */
  // #define Z7_unlikely [[unlikely]]
  // #define Z7_LIKELY(x)   (__builtin_expect((x), 1))
  #define Z7_UNLIKELY(x) (__builtin_expect((x), 0))
  // #define Z7_likely [[likely]]
#else
  // #define Z7_LIKELY(x)   (x)
  #define Z7_UNLIKELY(x) (x)
  // #define Z7_likely
#endif


Z7_BRANCH_FUNC_MAIN(ARM64)
{
  // Byte *p = data;
  const Byte *lim;
  const UInt32 flag = (UInt32)1 << (24 - 4);
  const UInt32 mask = ((UInt32)1 << 24) - (flag << 1);
  size &= ~(SizeT)3;
  // if (size == 0) return p;
  lim = p + size;
  BR_PC_INIT
  pc -= 4;  // because (p) will point to next instruction
  
  BR_EXTERNAL_FOR
  {
    // Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
    for (;;)
    {
      UInt32 v;
      if Z7_UNLIKELY(p == lim)
        return p;
      v = GetUi32a(p);
      p += 4;
      if Z7_UNLIKELY(((v - 0x94000000) & 0xfc000000) == 0)
      {
        UInt32 c = BR_PC_GET >> 2;
        BR_CONVERT_VAL(v, c)
        v &= 0x03ffffff;
        v |= 0x94000000;
        SetUi32a(p - 4, v)
        BR_NEXT_ITERATION
      }
      // v = rotlFixed(v, 8);  v += (flag << 8) - 0x90;  if Z7_UNLIKELY((v & ((mask << 8) + 0x9f)) == 0)
      v -= 0x90000000;  if Z7_UNLIKELY((v & 0x9f000000) == 0)
      {
        UInt32 z, c;
        // v = rotrFixed(v, 8);
        v += flag; if Z7_UNLIKELY(v & mask) continue;
        z = (v & 0xffffffe0) | (v >> 26);
        c = (BR_PC_GET >> (12 - 3)) & ~(UInt32)7;
        BR_CONVERT_VAL(z, c)
        v &= 0x1f;
        v |= 0x90000000;
        v |= z << 26;
        v |= 0x00ffffe0 & ((z & (((flag << 1) - 1))) - flag);
        SetUi32a(p - 4, v)
      }
    }
  }
}
Z7_BRANCH_FUNCS_IMP(ARM64)


Z7_BRANCH_FUNC_MAIN(ARM)
{
  // Byte *p = data;
  const Byte *lim;
  size &= ~(SizeT)3;
  lim = p + size;
  BR_PC_INIT
  /* in ARM: branch offset is relative to the +2 instructions from current instruction.
     (p) will point to next instruction */
  pc += 8 - 4;
  
  for (;;)
  {
    for (;;)
    {
      if Z7_UNLIKELY(p >= lim) { return p; }  p += 4;  if Z7_UNLIKELY(p[-1] == 0xeb) break;
      if Z7_UNLIKELY(p >= lim) { return p; }  p += 4;  if Z7_UNLIKELY(p[-1] == 0xeb) break;
    }
    {
      UInt32 v = GetUi32a(p - 4);
      UInt32 c = BR_PC_GET >> 2;
      BR_CONVERT_VAL(v, c)
      v &= 0x00ffffff;
      v |= 0xeb000000;
      SetUi32a(p - 4, v)
    }
  }
}
Z7_BRANCH_FUNCS_IMP(ARM)


Z7_BRANCH_FUNC_MAIN(PPC)
{
  // Byte *p = data;
  const Byte *lim;
  size &= ~(SizeT)3;
  lim = p + size;
  BR_PC_INIT
  pc -= 4;  // because (p) will point to next instruction
  
  for (;;)
  {
    UInt32 v;
    for (;;)
    {
      if Z7_UNLIKELY(p == lim)
        return p;
      // v = GetBe32a(p);
      v = *(UInt32 *)(void *)p;
      p += 4;
      // if ((v & 0xfc000003) == 0x48000001) break;
      // if ((p[-4] & 0xFC) == 0x48 && (p[-1] & 3) == 1) break;
      if Z7_UNLIKELY(
          ((v - Z7_CONV_BE_TO_NATIVE_CONST32(0x48000001))
              & Z7_CONV_BE_TO_NATIVE_CONST32(0xfc000003)) == 0) break;
    }
    {
      v = Z7_CONV_NATIVE_TO_BE_32(v);
      {
        UInt32 c = BR_PC_GET;
        BR_CONVERT_VAL(v, c)
      }
      v &= 0x03ffffff;
      v |= 0x48000000;
      SetBe32a(p - 4, v)
    }
  }
}
Z7_BRANCH_FUNCS_IMP(PPC)


#ifdef Z7_CPU_FAST_ROTATE_SUPPORTED
#define BR_SPARC_USE_ROTATE
#endif

Z7_BRANCH_FUNC_MAIN(SPARC)
{
  // Byte *p = data;
  const Byte *lim;
  const UInt32 flag = (UInt32)1 << 22;
  size &= ~(SizeT)3;
  lim = p + size;
  BR_PC_INIT
  pc -= 4;  // because (p) will point to next instruction
  for (;;)
  {
    UInt32 v;
    for (;;)
    {
      if Z7_UNLIKELY(p == lim)
        return p;
      /* // the code without GetBe32a():
      { const UInt32 v = GetUi16a(p) & 0xc0ff; p += 4; if (v == 0x40 || v == 0xc07f) break; }
      */
      v = GetBe32a(p);
      p += 4;
    #ifdef BR_SPARC_USE_ROTATE
      v = rotlFixed(v, 2);
      v += (flag << 2) - 1;
      if Z7_UNLIKELY((v & (3 - (flag << 3))) == 0)
    #else
      v += (UInt32)5 << 29;
      v ^= (UInt32)7 << 29;
      v += flag;
      if Z7_UNLIKELY((v & (0 - (flag << 1))) == 0)
    #endif
        break;
    }
    {
      // UInt32 v = GetBe32a(p - 4);
    #ifndef BR_SPARC_USE_ROTATE
      v <<= 2;
    #endif
      {
        UInt32 c = BR_PC_GET;
        BR_CONVERT_VAL(v, c)
      }
      v &= (flag << 3) - 1;
    #ifdef BR_SPARC_USE_ROTATE
      v -= (flag << 2) - 1;
      v = rotrFixed(v, 2);
    #else
      v -= (flag << 2);
      v >>= 2;
      v |= (UInt32)1 << 30;
    #endif
      SetBe32a(p - 4, v)
    }
  }
}
Z7_BRANCH_FUNCS_IMP(SPARC)


Z7_BRANCH_FUNC_MAIN(ARMT)
{
  // Byte *p = data;
  Byte *lim;
  size &= ~(SizeT)1;
  // if (size == 0) return p;
  if (size <= 2) return p;
  size -= 2;
  lim = p + size;
  BR_PC_INIT
  /* in ARM: branch offset is relative to the +2 instructions from current instruction.
     (p) will point to the +2 instructions from current instruction */
  // pc += 4 - 4;
  // if (encoding) pc -= 0xf800 << 1; else pc += 0xf800 << 1;
  // #define ARMT_TAIL_PROC { goto armt_tail; }
  #define ARMT_TAIL_PROC { return p; }
  
  do
  {
    /* in MSVC 32-bit x86 compilers:
       UInt32 version : it loads value from memory with movzx
       Byte   version : it loads value to 8-bit register (AL/CL)
       movzx version is slightly faster in some cpus
    */
    unsigned b1;
    // Byte / unsigned
    b1 = p[1];
    // optimized version to reduce one (p >= lim) check:
    // unsigned a1 = p[1];  b1 = p[3];  p += 2;  if Z7_LIKELY((b1 & (a1 ^ 8)) < 0xf8)
    for (;;)
    {
      unsigned b3; // Byte / UInt32
      /* (Byte)(b3) normalization can use low byte computations in MSVC.
         It gives smaller code, and no loss of speed in some compilers/cpus.
         But new MSVC 32-bit x86 compilers use more slow load
         from memory to low byte register in that case.
         So we try to use full 32-bit computations for faster code.
      */
      // if (p >= lim) { ARMT_TAIL_PROC }  b3 = b1 + 8;  b1 = p[3];  p += 2;  if ((b3 & b1) >= 0xf8) break;
      if Z7_UNLIKELY(p >= lim) { ARMT_TAIL_PROC }  b3 = p[3];  p += 2;  if Z7_UNLIKELY((b3 & (b1 ^ 8)) >= 0xf8) break;
      if Z7_UNLIKELY(p >= lim) { ARMT_TAIL_PROC }  b1 = p[3];  p += 2;  if Z7_UNLIKELY((b1 & (b3 ^ 8)) >= 0xf8) break;
    }
    {
      /* we can adjust pc for (0xf800) to rid of (& 0x7FF) operation.
         But gcc/clang for arm64 can use bfi instruction for full code here */
      UInt32 v =
          ((UInt32)GetUi16a(p - 2) << 11) |
          ((UInt32)GetUi16a(p) & 0x7FF);
      /*
      UInt32 v =
            ((UInt32)p[1 - 2] << 19)
          + (((UInt32)p[1] & 0x7) << 8)
          + (((UInt32)p[-2] << 11))
          + (p[0]);
      */
      p += 2;
      {
        UInt32 c = BR_PC_GET >> 1;
        BR_CONVERT_VAL(v, c)
      }
      SetUi16a(p - 4, (UInt16)(((v >> 11) & 0x7ff) | 0xf000))
      SetUi16a(p - 2, (UInt16)(v | 0xf800))
      /*
      p[-4] = (Byte)(v >> 11);
      p[-3] = (Byte)(0xf0 | ((v >> 19) & 0x7));
      p[-2] = (Byte)v;
      p[-1] = (Byte)(0xf8 | (v >> 8));
      */
    }
  }
  while (p < lim);
  return p;
  // armt_tail:
  // if ((Byte)((lim[1] & 0xf8)) != 0xf0) { lim += 2; }  return lim;
  // return (Byte *)(lim + ((Byte)((lim[1] ^ 0xf0) & 0xf8) == 0 ? 0 : 2));
  // return (Byte *)(lim + (((lim[1] ^ ~0xfu) & ~7u) == 0 ? 0 : 2));
  // return (Byte *)(lim + 2 - (((((unsigned)lim[1] ^ 8) + 8) >> 7) & 2));
}
Z7_BRANCH_FUNCS_IMP(ARMT)


// #define BR_IA64_NO_INLINE

Z7_BRANCH_FUNC_MAIN(IA64)
{
  // Byte *p = data;
  const Byte *lim;
  size &= ~(SizeT)15;
  lim = p + size;
  pc -= 1 << 4;
  pc >>= 4 - 1;
  // pc -= 1 << 1;
  
  for (;;)
  {
    unsigned m;
    for (;;)
    {
      if Z7_UNLIKELY(p == lim)
        return p;
      m = (unsigned)((UInt32)0x334b0000 >> (*p & 0x1e));
      p += 16;
      pc += 1 << 1;
      if (m &= 3)
        break;
    }
    {
      p += (ptrdiff_t)m * 5 - 20; // negative value is expected here.
      do
      {
        const UInt32 t =
          #if defined(MY_CPU_X86_OR_AMD64)
            // we use 32-bit load here to reduce code size on x86:
            GetUi32(p);
          #else
            GetUi16(p);
          #endif
        UInt32 z = GetUi32(p + 1) >> m;
        p += 5;
        if (((t >> m) & (0x70 << 1)) == 0
            && ((z - (0x5000000 << 1)) & (0xf000000 << 1)) == 0)
        {
          UInt32 v = (UInt32)((0x8fffff << 1) | 1) & z;
          z ^= v;
        #ifdef BR_IA64_NO_INLINE
          v |= (v & ((UInt32)1 << (23 + 1))) >> 3;
          {
            UInt32 c = pc;
            BR_CONVERT_VAL(v, c)
          }
          v &= (0x1fffff << 1) | 1;
        #else
          {
            if (encoding)
            {
              // pc &= ~(0xc00000 << 1); // we just need to clear at least 2 bits
              pc &= (0x1fffff << 1) | 1;
              v += pc;
            }
            else
            {
              // pc |= 0xc00000 << 1; // we need to set at least 2 bits
              pc |= ~(UInt32)((0x1fffff << 1) | 1);
              v -= pc;
            }
          }
          v &= ~(UInt32)(0x600000 << 1);
        #endif
          v += (0x700000 << 1);
          v &= (0x8fffff << 1) | 1;
          z |= v;
          z <<= m;
          SetUi32(p + 1 - 5, z)
        }
        m++;
      }
      while (m &= 3); // while (m < 4);
    }
  }
}
Z7_BRANCH_FUNCS_IMP(IA64)
