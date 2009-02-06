#ifndef __PS2ETYPES_H__
#define __PS2ETYPES_H__

// Basic types
#if defined(__MSCW32__)

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

#elif defined(__LINUX__) || defined(__MINGW32__)

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

#endif

#endif /* __PS2ETYPES_H__ */
