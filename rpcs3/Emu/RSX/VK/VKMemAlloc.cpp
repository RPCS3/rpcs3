#define VMA_IMPLEMENTATION

#include "util/atomic.hpp"

#define VMA_ATOMIC_UINT32 atomic_t<u32>
#define VMA_ATOMIC_UINT64 atomic_t<u64>
#define compare_exchange_strong compare_exchange
#define compare_exchange_weak compare_exchange

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "3rdparty/GPUOpen/include/vk_mem_alloc.h"
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
