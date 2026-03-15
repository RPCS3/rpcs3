#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1002000

#include "util/atomic.hpp"
#include "Utilities/mutex.h"

// Protect some STL headers from macro (add more if it fails to compile)
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>

// Replace VMA atomics with atomic_t
#define VMA_ATOMIC_UINT32 atomic_t<u32>
#define VMA_ATOMIC_UINT64 atomic_t<u64>
#define compare_exchange_strong compare_exchange
#define compare_exchange_weak compare_exchange

// VMA 3.x added an internal transactional increment helper that assumes std::atomic.
// Keep using RPCS3's atomic_t to match the existing allocator integration.
#define _VMA_ATOMIC_TRANSACTIONAL_INCREMENT
template <typename T>
struct AtomicTransactionalIncrement
{
	using AtomicT = atomic_t<T>;

	~AtomicTransactionalIncrement()
	{
		if (m_atomic)
		{
			--(*m_atomic);
		}
	}

	void Commit()
	{
		m_atomic = nullptr;
	}

	T Increment(AtomicT* atomic)
	{
		m_atomic = atomic;
		return m_atomic->fetch_add(1);
	}

private:
	AtomicT* m_atomic = nullptr;
};

// Replace VMA mutex with shared_mutex
class VmaRWMutex
{
public:
	void LockRead() { m_mutex.lock_shared(); }
	void UnlockRead() { m_mutex.unlock_shared(); }
	bool TryLockRead() { return m_mutex.try_lock_shared(); }
	void LockWrite() { m_mutex.lock(); }
	void UnlockWrite() { m_mutex.unlock(); }
	bool TryLockWrite() { return m_mutex.try_lock(); }
	void Lock() { m_mutex.lock(); }
	void Unlock() { m_mutex.unlock(); }
	bool TryLock() { return m_mutex.try_lock(); }
private:
	shared_mutex m_mutex;
};

#define VMA_RW_MUTEX VmaRWMutex
#define VMA_MUTEX VmaRWMutex

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#ifdef __clang__
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#else
#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#endif
#endif
#include <vk_mem_alloc.h>
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
