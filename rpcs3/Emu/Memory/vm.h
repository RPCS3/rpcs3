#pragma once

#include <map>
#include <functional>
#include <memory>

class named_thread;
class cpu_thread;

namespace vm
{
	extern u8* const g_base_addr;
	extern u8* const g_exec_addr;

	enum memory_location_t : uint
	{
		main,
		user_space,
		video,
		stack,
		
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

	struct waiter
	{
		named_thread* owner;
		u32 addr;
		u32 size;
		u64 stamp;
		const void* data;

		waiter() = default;

		waiter(const waiter&) = delete;

		void init();
		void test() const;

		~waiter();
	};

	// Address type
	enum addr_t : u32 {};

	extern thread_local atomic_t<cpu_thread*>* g_tls_locked;

	// Register reader
	void passive_lock(cpu_thread& cpu);

	// Unregister reader
	void passive_unlock(cpu_thread& cpu);

	// Unregister reader (foreign thread)
	void cleanup_unlock(cpu_thread& cpu) noexcept;

	// Optimization (set cpu_flag::memory)
	void temporary_unlock(cpu_thread& cpu) noexcept;
	void temporary_unlock() noexcept;

	constexpr struct try_to_lock_t{} try_to_lock{};

	struct reader_lock final
	{
		const bool locked;

		reader_lock(const reader_lock&) = delete;
		reader_lock();
		reader_lock(const try_to_lock_t&);
		~reader_lock();

		explicit operator bool() const { return locked; }
	};

	struct writer_lock final
	{
		const bool locked;

		writer_lock(const writer_lock&) = delete;
		writer_lock(int full = 1);
		writer_lock(const try_to_lock_t&);
		~writer_lock();

		explicit operator bool() const { return locked; }
	};

	// Get reservation status for further atomic update: last update timestamp
	u64 reservation_acquire(u32 addr, u32 size);

	// End atomic update
	void reservation_update(u32 addr, u32 size);

	// Check and notify memory changes at address
	void notify(u32 addr, u32 size);

	// Check and notify memory changes
	void notify_all();

	// Change memory protection of specified memory region
	bool page_protect(u32 addr, u32 size, u8 flags_test = 0, u8 flags_set = 0, u8 flags_clear = 0);

	// Check flags for specified memory range (unsafe)
	bool check_addr(u32 addr, u32 size = 1, u8 flags = page_allocated);

	// Search and map memory in specified memory location (don't pass alignment smaller than 4096)
	u32 alloc(u32 size, memory_location_t location, u32 align = 4096, u32 sup = 0);

	// Map memory at specified address (in optionally specified memory location)
	u32 falloc(u32 addr, u32 size, memory_location_t location = any, u32 sup = 0);

	// Unmap memory at specified address (in optionally specified memory location), return size
	u32 dealloc(u32 addr, memory_location_t location = any, u32* sup_out = nullptr);

	// dealloc() with no return value and no exceptions
	void dealloc_verbose_nothrow(u32 addr, memory_location_t location = any) noexcept;

	// Object that handles memory allocations inside specific constant bounds ("location")
	class block_t final
	{
		std::map<u32, u32> m_map; // Mapped memory: addr -> size
		std::unordered_map<u32, u32> m_sup; // Supplementary info for allocations

		bool try_alloc(u32 addr, u32 size, u8 flags, u32 sup);

	public:
		block_t(u32 addr, u32 size, u64 flags = 0);

		~block_t();

	public:
		const u32 addr; // Start address
		const u32 size; // Total size
		const u64 flags; // Currently unused

		// Search and map memory (don't pass alignment smaller than 4096)
		u32 alloc(u32 size, u32 align = 4096, u32 sup = 0);

		// Try to map memory at fixed location
		u32 falloc(u32 addr, u32 size, u32 sup = 0);

		// Unmap memory at specified location previously returned by alloc(), return size
		u32 dealloc(u32 addr, u32* sup_out = nullptr);

		// Internal
		u32 imp_used(const vm::writer_lock&);

		// Get allocated memory count
		u32 used();
	};

	// Create new memory block with specified parameters and return it
	std::shared_ptr<block_t> map(u32 addr, u32 size, u64 flags = 0);

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

	namespace ps3
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
	
	namespace psv
	{
		template<typename T> inline to_le_t<T>* _ptr(u32 addr)
		{
			return static_cast<to_le_t<T>*>(base(addr));
		}

		template<typename T> inline to_le_t<T>& _ref(u32 addr)
		{
			return *_ptr<T>(addr);
		}

		inline const le_t<u16>& read16(u32 addr)
		{
			return _ref<u16>(addr);
		}

		inline void write16(u32 addr, le_t<u16> value)
		{
			_ref<u16>(addr) = value;
		}

		inline const le_t<u32>& read32(u32 addr)
		{
			return _ref<u32>(addr);
		}

		inline void write32(u32 addr, le_t<u32> value)
		{
			_ref<u32>(addr) = value;
		}

		inline const le_t<u64>& read64(u32 addr)
		{
			return _ref<u64>(addr);
		}

		inline void write64(u32 addr, le_t<u64> value)
		{
			_ref<u64>(addr) = value;
		}

		void init();
	}

	namespace psp
	{
		using namespace psv;

		void init();
	}

	void close();
}

#include "vm_var.h"
