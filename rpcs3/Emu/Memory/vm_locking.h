#pragma once

#include "vm.h"
#include "Utilities/bit_set.h"

class cpu_thread;
class shared_mutex;

namespace vm
{
	extern shared_mutex g_mutex;

	extern thread_local atomic_t<cpu_thread*>* g_tls_locked;

	// Register reader
	void passive_lock(cpu_thread& cpu);
	atomic_t<u64>* range_lock(u32 begin, u32 end);

	// Unregister reader
	void passive_unlock(cpu_thread& cpu);

	// Unregister reader (foreign thread)
	void cleanup_unlock(cpu_thread& cpu) noexcept;

	// Optimization (set cpu_flag::memory)
	void temporary_unlock(cpu_thread& cpu) noexcept;
	void temporary_unlock() noexcept;

	class reader_lock final
	{
		enum class flags_t
		{
			upgrade,
			mem,

			__bitset_enum_max
		};

		bs_t<flags_t> m_flags = {};

	public:
		reader_lock(const reader_lock&) = delete;
		reader_lock& operator=(const reader_lock&) = delete;
		reader_lock();
		~reader_lock();

		void upgrade();
	};

	struct writer_lock final
	{
		u32 addr;

		writer_lock(const writer_lock&) = delete;
		writer_lock& operator=(const writer_lock&) = delete;
		writer_lock(u32 addr = 0, u64 rtime = 0);
		~writer_lock();

		explicit operator bool() const
		{
			return !(addr & 1);
		}
	};
} // namespace vm
