#ifndef __PS2ETYPES_H__
#define __PS2ETYPES_H__

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif

#if defined (__linux__)  // some distributions are lower case
#define __LINUX__
#endif

// Basic types
#if defined(_MSC_VER)
typedef __int8  s8;
typedef __int16 s16;
typedef __int32 s32;
typedef __int64 s64;
typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;
#if defined(__x86_64__)
typedef u64 uptr;
#else
typedef u32 uptr;
#endif
#define PCSX2_ALIGNED16(x) __declspec(align(16)) x
#else
typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
#if defined(__x86_64__)
typedef u64 uptr;
#else
typedef u32 uptr;
#endif
#ifdef __LINUX__
typedef union _LARGE_INTEGER
{
	long long QuadPart;
} LARGE_INTEGER;
#endif
#if defined(__MINGW32__)
#define PCSX2_ALIGNED16(x) __declspec(align(16)) x
#else
#define PCSX2_ALIGNED16(x) x __attribute((aligned(16)))
#endif
#ifndef __forceinline
#define __forceinline inline
#endif
#endif
typedef struct
{
	int size;
	s8 *data;
} freezeData;
#endif /* __PS2ETYPES_H__ */
