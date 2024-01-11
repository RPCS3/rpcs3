/* Alloc.c -- Memory allocation functions
2023-04-02 : Igor Pavlov : Public domain */

#include "Precomp.h"

#ifdef _WIN32
#include "7zWindows.h"
#endif
#include <stdlib.h>

#include "Alloc.h"

#ifdef _WIN32
#ifdef Z7_LARGE_PAGES
#if defined(__clang__) || defined(__GNUC__)
typedef void (*Z7_voidFunction)(void);
#define MY_CAST_FUNC (Z7_voidFunction)
#elif defined(_MSC_VER) && _MSC_VER > 1920
#define MY_CAST_FUNC  (void *)
// #pragma warning(disable : 4191) // 'type cast': unsafe conversion from 'FARPROC' to 'void (__cdecl *)()'
#else
#define MY_CAST_FUNC
#endif
#endif // Z7_LARGE_PAGES
#endif // _WIN32

// #define SZ_ALLOC_DEBUG
/* #define SZ_ALLOC_DEBUG */

/* use SZ_ALLOC_DEBUG to debug alloc/free operations */
#ifdef SZ_ALLOC_DEBUG

#include <string.h>
#include <stdio.h>
static int g_allocCount = 0;
#ifdef _WIN32
static int g_allocCountMid = 0;
static int g_allocCountBig = 0;
#endif


#define CONVERT_INT_TO_STR(charType, tempSize) \
  char temp[tempSize]; unsigned i = 0; \
  while (val >= 10) { temp[i++] = (char)('0' + (unsigned)(val % 10)); val /= 10; } \
  *s++ = (charType)('0' + (unsigned)val); \
  while (i != 0) { i--; *s++ = temp[i]; } \
  *s = 0;

static void ConvertUInt64ToString(UInt64 val, char *s)
{
  CONVERT_INT_TO_STR(char, 24)
}

#define GET_HEX_CHAR(t) ((char)(((t < 10) ? ('0' + t) : ('A' + (t - 10)))))

static void ConvertUInt64ToHex(UInt64 val, char *s)
{
  UInt64 v = val;
  unsigned i;
  for (i = 1;; i++)
  {
    v >>= 4;
    if (v == 0)
      break;
  }
  s[i] = 0;
  do
  {
    unsigned t = (unsigned)(val & 0xF);
    val >>= 4;
    s[--i] = GET_HEX_CHAR(t);
  }
  while (i);
}

#define DEBUG_OUT_STREAM stderr

static void Print(const char *s)
{
  fputs(s, DEBUG_OUT_STREAM);
}

static void PrintAligned(const char *s, size_t align)
{
  size_t len = strlen(s);
  for(;;)
  {
    fputc(' ', DEBUG_OUT_STREAM);
    if (len >= align)
      break;
    ++len;
  }
  Print(s);
}

static void PrintLn(void)
{
  Print("\n");
}

static void PrintHex(UInt64 v, size_t align)
{
  char s[32];
  ConvertUInt64ToHex(v, s);
  PrintAligned(s, align);
}

static void PrintDec(int v, size_t align)
{
  char s[32];
  ConvertUInt64ToString((unsigned)v, s);
  PrintAligned(s, align);
}

static void PrintAddr(void *p)
{
  PrintHex((UInt64)(size_t)(ptrdiff_t)p, 12);
}


#define PRINT_REALLOC(name, cnt, size, ptr) { \
    Print(name " "); \
    if (!ptr) PrintDec(cnt++, 10); \
    PrintHex(size, 10); \
    PrintAddr(ptr); \
    PrintLn(); }

#define PRINT_ALLOC(name, cnt, size, ptr) { \
    Print(name " "); \
    PrintDec(cnt++, 10); \
    PrintHex(size, 10); \
    PrintAddr(ptr); \
    PrintLn(); }
 
#define PRINT_FREE(name, cnt, ptr) if (ptr) { \
    Print(name " "); \
    PrintDec(--cnt, 10); \
    PrintAddr(ptr); \
    PrintLn(); }
 
#else

#ifdef _WIN32
#define PRINT_ALLOC(name, cnt, size, ptr)
#endif
#define PRINT_FREE(name, cnt, ptr)
#define Print(s)
#define PrintLn()
#define PrintHex(v, align)
#define PrintAddr(p)

#endif


/*
by specification:
  malloc(non_NULL, 0)   : returns NULL or a unique pointer value that can later be successfully passed to free()
  realloc(NULL, size)   : the call is equivalent to malloc(size)
  realloc(non_NULL, 0)  : the call is equivalent to free(ptr)

in main compilers:
  malloc(0)             : returns non_NULL
  realloc(NULL,     0)  : returns non_NULL
  realloc(non_NULL, 0)  : returns NULL
*/


void *MyAlloc(size_t size)
{
  if (size == 0)
    return NULL;
  // PRINT_ALLOC("Alloc    ", g_allocCount, size, NULL)
  #ifdef SZ_ALLOC_DEBUG
  {
    void *p = malloc(size);
    if (p)
    {
      PRINT_ALLOC("Alloc    ", g_allocCount, size, p)
    }
    return p;
  }
  #else
  return malloc(size);
  #endif
}

void MyFree(void *address)
{
  PRINT_FREE("Free    ", g_allocCount, address)
  
  free(address);
}

void *MyRealloc(void *address, size_t size)
{
  if (size == 0)
  {
    MyFree(address);
    return NULL;
  }
  // PRINT_REALLOC("Realloc  ", g_allocCount, size, address)
  #ifdef SZ_ALLOC_DEBUG
  {
    void *p = realloc(address, size);
    if (p)
    {
      PRINT_REALLOC("Realloc    ", g_allocCount, size, address)
    }
    return p;
  }
  #else
  return realloc(address, size);
  #endif
}


#ifdef _WIN32

void *MidAlloc(size_t size)
{
  if (size == 0)
    return NULL;
  #ifdef SZ_ALLOC_DEBUG
  {
    void *p = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if (p)
    {
      PRINT_ALLOC("Alloc-Mid", g_allocCountMid, size, p)
    }
    return p;
  }
  #else
  return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
  #endif
}

void MidFree(void *address)
{
  PRINT_FREE("Free-Mid", g_allocCountMid, address)

  if (!address)
    return;
  VirtualFree(address, 0, MEM_RELEASE);
}

#ifdef Z7_LARGE_PAGES

#ifdef MEM_LARGE_PAGES
  #define MY__MEM_LARGE_PAGES  MEM_LARGE_PAGES
#else
  #define MY__MEM_LARGE_PAGES  0x20000000
#endif

extern
SIZE_T g_LargePageSize;
SIZE_T g_LargePageSize = 0;
typedef SIZE_T (WINAPI *Func_GetLargePageMinimum)(VOID);

void SetLargePageSize(void)
{
  #ifdef Z7_LARGE_PAGES
  SIZE_T size;
  const
   Func_GetLargePageMinimum fn =
  (Func_GetLargePageMinimum) MY_CAST_FUNC GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),
       "GetLargePageMinimum");
  if (!fn)
    return;
  size = fn();
  if (size == 0 || (size & (size - 1)) != 0)
    return;
  g_LargePageSize = size;
  #endif
}

#endif // Z7_LARGE_PAGES

void *BigAlloc(size_t size)
{
  if (size == 0)
    return NULL;

  PRINT_ALLOC("Alloc-Big", g_allocCountBig, size, NULL)

  #ifdef Z7_LARGE_PAGES
  {
    SIZE_T ps = g_LargePageSize;
    if (ps != 0 && ps <= (1 << 30) && size > (ps / 2))
    {
      size_t size2;
      ps--;
      size2 = (size + ps) & ~ps;
      if (size2 >= size)
      {
        void *p = VirtualAlloc(NULL, size2, MEM_COMMIT | MY__MEM_LARGE_PAGES, PAGE_READWRITE);
        if (p)
        {
          PRINT_ALLOC("Alloc-BM ", g_allocCountMid, size2, p)
          return p;
        }
      }
    }
  }
  #endif

  return MidAlloc(size);
}

void BigFree(void *address)
{
  PRINT_FREE("Free-Big", g_allocCountBig, address)
  MidFree(address);
}

#endif // _WIN32


static void *SzAlloc(ISzAllocPtr p, size_t size) { UNUSED_VAR(p)  return MyAlloc(size); }
static void SzFree(ISzAllocPtr p, void *address) { UNUSED_VAR(p)  MyFree(address); }
const ISzAlloc g_Alloc = { SzAlloc, SzFree };

#ifdef _WIN32
static void *SzMidAlloc(ISzAllocPtr p, size_t size) { UNUSED_VAR(p)  return MidAlloc(size); }
static void SzMidFree(ISzAllocPtr p, void *address) { UNUSED_VAR(p)  MidFree(address); }
static void *SzBigAlloc(ISzAllocPtr p, size_t size) { UNUSED_VAR(p)  return BigAlloc(size); }
static void SzBigFree(ISzAllocPtr p, void *address) { UNUSED_VAR(p)  BigFree(address); }
const ISzAlloc g_MidAlloc = { SzMidAlloc, SzMidFree };
const ISzAlloc g_BigAlloc = { SzBigAlloc, SzBigFree };
#endif

/*
  uintptr_t : <stdint.h> C99 (optional)
            : unsupported in VS6
*/

#ifdef _WIN32
  typedef UINT_PTR UIntPtr;
#else
  /*
  typedef uintptr_t UIntPtr;
  */
  typedef ptrdiff_t UIntPtr;
#endif


#define ADJUST_ALLOC_SIZE 0
/*
#define ADJUST_ALLOC_SIZE (sizeof(void *) - 1)
*/
/*
  Use (ADJUST_ALLOC_SIZE = (sizeof(void *) - 1)), if
     MyAlloc() can return address that is NOT multiple of sizeof(void *).
*/


/*
#define MY_ALIGN_PTR_DOWN(p, align) ((void *)((char *)(p) - ((size_t)(UIntPtr)(p) & ((align) - 1))))
*/
#define MY_ALIGN_PTR_DOWN(p, align) ((void *)((((UIntPtr)(p)) & ~((UIntPtr)(align) - 1))))


#if !defined(_WIN32) && defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200112L)
  #define USE_posix_memalign
#endif

#ifndef USE_posix_memalign
#define MY_ALIGN_PTR_UP_PLUS(p, align) MY_ALIGN_PTR_DOWN(((char *)(p) + (align) + ADJUST_ALLOC_SIZE), align)
#endif

/*
  This posix_memalign() is for test purposes only.
  We also need special Free() function instead of free(),
  if this posix_memalign() is used.
*/

/*
static int posix_memalign(void **ptr, size_t align, size_t size)
{
  size_t newSize = size + align;
  void *p;
  void *pAligned;
  *ptr = NULL;
  if (newSize < size)
    return 12; // ENOMEM
  p = MyAlloc(newSize);
  if (!p)
    return 12; // ENOMEM
  pAligned = MY_ALIGN_PTR_UP_PLUS(p, align);
  ((void **)pAligned)[-1] = p;
  *ptr = pAligned;
  return 0;
}
*/

/*
  ALLOC_ALIGN_SIZE >= sizeof(void *)
  ALLOC_ALIGN_SIZE >= cache_line_size
*/

#define ALLOC_ALIGN_SIZE ((size_t)1 << 7)

static void *SzAlignedAlloc(ISzAllocPtr pp, size_t size)
{
  #ifndef USE_posix_memalign
  
  void *p;
  void *pAligned;
  size_t newSize;
  UNUSED_VAR(pp)

  /* also we can allocate additional dummy ALLOC_ALIGN_SIZE bytes after aligned
     block to prevent cache line sharing with another allocated blocks */

  newSize = size + ALLOC_ALIGN_SIZE * 1 + ADJUST_ALLOC_SIZE;
  if (newSize < size)
    return NULL;

  p = MyAlloc(newSize);
  
  if (!p)
    return NULL;
  pAligned = MY_ALIGN_PTR_UP_PLUS(p, ALLOC_ALIGN_SIZE);

  Print(" size="); PrintHex(size, 8);
  Print(" a_size="); PrintHex(newSize, 8);
  Print(" ptr="); PrintAddr(p);
  Print(" a_ptr="); PrintAddr(pAligned);
  PrintLn();

  ((void **)pAligned)[-1] = p;

  return pAligned;

  #else

  void *p;
  UNUSED_VAR(pp)
  if (posix_memalign(&p, ALLOC_ALIGN_SIZE, size))
    return NULL;

  Print(" posix_memalign="); PrintAddr(p);
  PrintLn();

  return p;

  #endif
}


static void SzAlignedFree(ISzAllocPtr pp, void *address)
{
  UNUSED_VAR(pp)
  #ifndef USE_posix_memalign
  if (address)
    MyFree(((void **)address)[-1]);
  #else
  free(address);
  #endif
}


const ISzAlloc g_AlignedAlloc = { SzAlignedAlloc, SzAlignedFree };



#define MY_ALIGN_PTR_DOWN_1(p) MY_ALIGN_PTR_DOWN(p, sizeof(void *))

/* we align ptr to support cases where CAlignOffsetAlloc::offset is not multiply of sizeof(void *) */
#define REAL_BLOCK_PTR_VAR(p) ((void **)MY_ALIGN_PTR_DOWN_1(p))[-1]
/*
#define REAL_BLOCK_PTR_VAR(p) ((void **)(p))[-1]
*/

static void *AlignOffsetAlloc_Alloc(ISzAllocPtr pp, size_t size)
{
  const CAlignOffsetAlloc *p = Z7_CONTAINER_FROM_VTBL_CONST(pp, CAlignOffsetAlloc, vt);
  void *adr;
  void *pAligned;
  size_t newSize;
  size_t extra;
  size_t alignSize = (size_t)1 << p->numAlignBits;

  if (alignSize < sizeof(void *))
    alignSize = sizeof(void *);
  
  if (p->offset >= alignSize)
    return NULL;

  /* also we can allocate additional dummy ALLOC_ALIGN_SIZE bytes after aligned
     block to prevent cache line sharing with another allocated blocks */
  extra = p->offset & (sizeof(void *) - 1);
  newSize = size + alignSize + extra + ADJUST_ALLOC_SIZE;
  if (newSize < size)
    return NULL;

  adr = ISzAlloc_Alloc(p->baseAlloc, newSize);
  
  if (!adr)
    return NULL;

  pAligned = (char *)MY_ALIGN_PTR_DOWN((char *)adr +
      alignSize - p->offset + extra + ADJUST_ALLOC_SIZE, alignSize) + p->offset;

  PrintLn();
  Print("- Aligned: ");
  Print(" size="); PrintHex(size, 8);
  Print(" a_size="); PrintHex(newSize, 8);
  Print(" ptr="); PrintAddr(adr);
  Print(" a_ptr="); PrintAddr(pAligned);
  PrintLn();

  REAL_BLOCK_PTR_VAR(pAligned) = adr;

  return pAligned;
}


static void AlignOffsetAlloc_Free(ISzAllocPtr pp, void *address)
{
  if (address)
  {
    const CAlignOffsetAlloc *p = Z7_CONTAINER_FROM_VTBL_CONST(pp, CAlignOffsetAlloc, vt);
    PrintLn();
    Print("- Aligned Free: ");
    PrintLn();
    ISzAlloc_Free(p->baseAlloc, REAL_BLOCK_PTR_VAR(address));
  }
}


void AlignOffsetAlloc_CreateVTable(CAlignOffsetAlloc *p)
{
  p->vt.Alloc = AlignOffsetAlloc_Alloc;
  p->vt.Free = AlignOffsetAlloc_Free;
}
