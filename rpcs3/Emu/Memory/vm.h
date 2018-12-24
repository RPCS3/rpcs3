#pragma once

#include <map>
#include <functional>
#include <memory>
#include "Utilities/VirtualMemory.h"

class shared_mutex;
class cpu_thread;
class cond_x16;

namespace vm
{
	extern u8* const g_base_addr;
	extern u8* const g_sudo_addr;
	extern u8* const g_exec_addr;
	extern u8* const g_stat_addr;
	extern u8* const g_reservations;
	extern u8* const g_reservations2;

	enum memory_location_t : uint
	{
		main,
		user64k,
		user1m,
		video,
		stack,
		spu,

		memory_location_max,
		any = 0xffffffff,
	};

	enum page_info_t : u8
	{
		page_readable           = (1 << 0),
		page_writable           = (1 << 1),
		page_executable         = (1 << 2),

		page_fault_notification = (1 << 3),
		page_no_reservations    = (1 << 4),
		page_64k_size           = (1 << 5),
		page_1m_size            = (1 << 6),

		page_allocated          = (1 << 7),
	};

	// Address type
	enum addr_t : u32 {};

	extern shared_mutex g_mutex;

	extern thread_local atomic_t<cpu_thread*>* g_tls_locked;

	// Register reader
	bool passive_lock(cpu_thread& cpu, bool wait = true);

	// Unregister reader
	void passive_unlock(cpu_thread& cpu);

	// Unregister reader (foreign thread)
	void cleanup_unlock(cpu_thread& cpu) noexcept;

	// Optimization (set cpu_flag::memory)
	void temporary_unlock(cpu_thread& cpu) noexcept;
	void temporary_unlock() noexcept;

	class reader_lock final
	{
		bool m_upgraded = false;

	public:
		reader_lock(const reader_lock&) = delete;
		reader_lock& operator=(const reader_lock&) = delete;
		reader_lock();
		~reader_lock();

		void upgrade();
	};

	struct writer_lock final
	{
		const bool locked;

		writer_lock(const writer_lock&) = delete;
		writer_lock& operator=(const writer_lock&) = delete;
		writer_lock(u32 addr = 0);
		~writer_lock();

		explicit operator bool() const { return locked; }
	};

	// Get reservation status for further atomic update: last update timestamp
	inline atomic_t<u64>& reservation_acquire(u32 addr, u32 size)
	{
		// Access reservation info: stamp and the lock bit
		return reinterpret_cast<atomic_t<u64>*>(g_reservations)[addr / 128];
	}

	// Update reservation status
	inline void reservation_update(u32 addr, u32 size, bool lsb = false)
	{
		// Update reservation info with new timestamp
		reservation_acquire(addr, size) += 2;
	}

	// Get reservation sync variable
	inline cond_x16& reservation_notifier(u32 addr, u32 size)
	{
		return *reinterpret_cast<cond_x16*>(g_reservations2 + addr / 128 * 8);
	}

	void reservation_lock_internal(atomic_t<u64>&);

	inline atomic_t<u64>& reservation_lock(u32 addr, u32 size)
	{
		auto& res = vm::reservation_acquire(addr, size);

		if (UNLIKELY(atomic_storage<u64>::bts(res.raw(), 0)))
		{
			reservation_lock_internal(res);
		}

		return res;
	}

	// Change memory protection of specified memory region
	bool page_protect(u32 addr, u32 size, u8 flags_test = 0, u8 flags_set = 0, u8 flags_clear = 0);

	// Check flags for specified memory range (unsafe)
	bool check_addr(u32 addr, u32 size = 1, u8 flags = page_allocated);

	// Search and map memory in specified memory location (min alignment is 0x10000)
	u32 alloc(u32 size, memory_location_t location, u32 align = 0x10000);

	// Map memory at specified address (in optionally specified memory location)
	u32 falloc(u32 addr, u32 size, memory_location_t location = any);

	// Unmap memory at specified address (in optionally specified memory location), return size
	u32 dealloc(u32 addr, memory_location_t location = any);

	// dealloc() with no return value and no exceptions
	void dealloc_verbose_nothrow(u32 addr, memory_location_t location = any) noexcept;

	// Object that handles memory allocations inside specific constant bounds ("location")
	class block_t final
	{
		// Mapped regions: addr -> shm handle
		std::map<u32, std::pair<u32, std::shared_ptr<utils::shm>>> m_map;

		// Common mapped region for special cases
		std::shared_ptr<utils::shm> m_common;

		bool try_alloc(u32 addr, u8 flags, u32 size, std::shared_ptr<utils::shm>&&);

	public:
		block_t(u32 addr, u32 size, u64 flags = 0);

		~block_t();

	public:
		const u32 addr; // Start address
		const u32 size; // Total size
		const u64 flags; // Currently unused

		// Search and map memory (min alignment is 0x10000)
		u32 alloc(u32 size, u32 align = 0x10000, const std::shared_ptr<utils::shm>* = nullptr);

		// Try to map memory at fixed location
		u32 falloc(u32 addr, u32 size, const std::shared_ptr<utils::shm>* = nullptr);

		// Unmap memory at specified location previously returned by alloc(), return size
		u32 dealloc(u32 addr, const std::shared_ptr<utils::shm>* = nullptr);

		// Get memory at specified address (if size = 0, addr assumed exact)
		std::pair<u32, std::shared_ptr<utils::shm>> get(u32 addr, u32 size = 0);

		// Internal
		u32 imp_used(const vm::writer_lock&);

		// Get allocated memory count
		u32 used();
	};

	// Create new memory block with specified parameters and return it
	std::shared_ptr<block_t> map(u32 addr, u32 size, u64 flags = 0);

	// Create new memory block with at arbitrary position with specified alignment
	std::shared_ptr<block_t> find_map(u32 size, u32 align, u64 flags = 0);

	// Delete existing memory block with specified start address, return it
	std::shared_ptr<block_t> unmap(u32 addr, bool must_be_empty = false);

	// Get memory block associated with optionally specified memory location or optionally specified address
	std::shared_ptr<block_t> get(memory_location_t location, u32 addr = 0);

	// Get PS3/PSV virtual memory address from the provided pointer (nullptr always converted to 0)
	inline vm::addr_t get_addr(const void* real_ptr)
	{
		if (!real_ptr)
		{
			return vm::addr_t{};
		}

		const std::ptrdiff_t diff = static_cast<const u8*>(real_ptr) - g_base_addr;
		const u32 res = static_cast<u32>(diff);

		if (res == diff)
		{
			return static_cast<vm::addr_t>(res);
		}

		fmt::throw_exception("Not a virtual memory pointer (%p)", real_ptr);
	}

	template<typename T>
	struct cast_impl
	{
		static_assert(std::is_same<T, u32>::value, "vm::cast() error: unsupported type");
	};

	template<>
	struct cast_impl<u32>
	{
		static vm::addr_t cast(u32 addr, const char* loc)
		{
			return static_cast<vm::addr_t>(addr);
		}

		static vm::addr_t cast(u32 addr)
		{
			return static_cast<vm::addr_t>(addr);
		}
	};

	template<>
	struct cast_impl<u64>
	{
		static vm::addr_t cast(u64 addr, const char* loc)
		{
			return static_cast<vm::addr_t>(static_cast<u32>(addr));
		}

		static vm::addr_t cast(u64 addr)
		{
			return static_cast<vm::addr_t>(static_cast<u32>(addr));
		}
	};

	template<typename T, bool Se>
	struct cast_impl<se_t<T, Se>>
	{
		static vm::addr_t cast(const se_t<T, Se>& addr, const char* loc)
		{
			return cast_impl<T>::cast(addr, loc);
		}

		static vm::addr_t cast(const se_t<T, Se>& addr)
		{
			return cast_impl<T>::cast(addr);
		}
	};

	template<typename T>
	vm::addr_t cast(const T& addr, const char* loc)
	{
		return cast_impl<T>::cast(addr, loc);
	}

	template<typename T>
	vm::addr_t cast(const T& addr)
	{
		return cast_impl<T>::cast(addr);
	}

	// Convert specified PS3/PSV virtual memory address to a pointer for common access
	inline void* base(u32 addr)
	{
		return g_base_addr + addr;
	}

	inline const u8& read8(u32 addr)
	{
		return g_base_addr[addr];
	}

	inline void write8(u32 addr, u8 value)
	{
		g_base_addr[addr] = value;
	}

	inline namespace ps3_
	{
		// Convert specified PS3 address to a pointer of specified (possibly converted to BE) type
		template<typename T> inline to_be_t<T>* _ptr(u32 addr)
		{
			return static_cast<to_be_t<T>*>(base(addr));
		}

		// Convert specified PS3 address to a reference of specified (possibly converted to BE) type
		template<typename T> inline to_be_t<T>& _ref(u32 addr)
		{
			return *_ptr<T>(addr);
		}

		// Access memory bypassing memory protection
		template <typename T = u8>
		inline to_be_t<T>* get_super_ptr(u32 addr)
		{
			return reinterpret_cast<to_be_t<T>*>(g_sudo_addr + addr);
		}

		inline const be_t<u16>& read16(u32 addr)
		{
			return _ref<u16>(addr);
		}

		inline void write16(u32 addr, be_t<u16> value)
		{
			_ref<u16>(addr) = value;
		}

		inline const be_t<u32>& read32(u32 addr)
		{
			return _ref<u32>(addr);
		}

		inline void write32(u32 addr, be_t<u32> value)
		{
			_ref<u32>(addr) = value;
		}

		inline const be_t<u64>& read64(u32 addr)
		{
			return _ref<u64>(addr);
		}

		inline void write64(u32 addr, be_t<u64> value)
		{
			_ref<u64>(addr) = value;
		}

		void init();
	}

	void close();
}

#include "vm_var.h"
