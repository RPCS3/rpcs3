#pragma once

#ifdef __GNUC__
#define likely(x) __builtin_expect(bool(x),1)
#define unlikely(x) __builtin_expect(bool(x),0)
#define force_inline inline __attribute__((always_inline))
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#define force_inline inline
#endif
