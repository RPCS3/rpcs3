#pragma once

#include <map>
#include <mutex>

class thread_ctrl;

namespace vm
{
	extern u8* const g_base_addr;
	extern u8* const g_priv_addr;

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

		page_allocated          = (1 << 7),
	};

	// Address type
	enum addr_t : u32 {};

	struct access_violation : std::runtime_error
	{
		access_violation(u64 addr, const char* cause);
	};

	[[noreturn]] void throw_access_violation(u64 addr, const char* cause);

	// This flag is changed by various reservation functions and may have different meaning.
	// reservation_break() - true if the reservation was successfully broken.
	// reservation_acquire() - true if another existing reservation was broken.
	// reservation_free() - true if this thread's reservation was successfully removed.
	// reservation_op() - false if reservation_update() would succeed if called instead.
	// Write access to reserved memory - only set to true if the reservation was broken.
	extern thread_local bool g_tls_did_break_reservation;

	// Unconditionally break the reservation at specified address
	void reservation_break(u32 addr);

	// Reserve memory at the specified address for further atomic update
	void reservation_acquire(void* data, u32 addr, u32 size);

	// Attempt to atomically update previously reserved memory
	bool reservation_update(u32 addr, const void* data, u32 size);

	// Process a memory access error if it's caused by the reservation
	bool reservation_query(u32 addr, u32 size, bool is_writing, std::function<bool()> callback);

	// Returns true if the current thread owns reservation
	bool reservation_test(thread_ctrl* current);

	// Break all reservations created by the current thread
	void reservation_free();

	// Perform atomic operation unconditionally
	void reservation_op(u32 addr, u32 size, std::function<void()> proc);

	// Change memory protection of specified memory region
	bool page_protect(u32 addr, u32 size, u8 flags_test = 0, u8 flags_set = 0, u8 flags_clear = 0);

	// Check if existing memory range is allocated. Checking address before using it is very unsafe.
	// Return value may be wrong. Even if it's true and correct, actual memory protection may be read-only and no-access.
	bool check_addr(u32 addr, u32 size = 1);

	// Search and map memory in specified memory location (don't pass alignment smaller than 4096)
	u32 alloc(u32 size, memory_location_t location, u32 align = 4096);

	// Map memory at specified address (in optionally specified memory location)
	u32 falloc(u32 addr, u32 size, memory_location_t location = any);

	// Unmap memory at specified address (in optionally specified memory location)
	bool dealloc(u32 addr, memory_location_t location = any);

	// dealloc() with no return value and no exceptions
	void dealloc_verbose_nothrow(u32 addr, memory_location_t location = any) noexcept;

	// Object that handles memory allocations inside specific constant bounds ("location")
	class block_t final
	{
		std::map<u32, u32> m_map; // addr -> size mapping of mapped locations
		std::mutex m_mutex;

		bool try_alloc(u32 addr, u32 size);

	public:
		block_t(u32 addr, u32 size, u64 flags = 0)
			: addr(addr)
			, size(size)
			, flags(flags)
			, used(0)
		{
		}

		~block_t();

	public:
		const u32 addr; // start address
		const u32 size; // total size
		const u64 flags; // currently unused

		atomic_t<u32> used; // amount of memory used, may be increased manually to prevent some memory from allocating

		// Search and map memory (don't pass alignment smaller than 4096)
		u32 alloc(u32 size, u32 align = 4096);

		// Try to map memory at fixed location
		u32 falloc(u32 addr, u32 size);

		// Unmap memory at specified location previously returned by alloc()
		bool dealloc(u32 addr);
	};

	// create new memory block with specified parameters and return it
	std::shared_ptr<block_t> map(u32 addr, u32 size, u64 flags = 0);

	// delete existing memory block with specified start address
	std::shared_ptr<block_t> unmap(u32 addr);

	// get memory block associated with optionally specified memory location or optionally specified address
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

		throw fmt::exception("Not a virtual memory pointer (%p)", real_ptr);
	}

	// Convert pointer-to-member to a vm address compatible offset
	template<typename MT, typename T> inline u32 get_offset(MT T::*const member_ptr)
	{
		return static_cast<u32>(reinterpret_cast<std::uintptr_t>(&reinterpret_cast<char const volatile&>(reinterpret_cast<T*>(0ull)->*member_ptr)));
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
			return static_cast<vm::addr_t>(fmt::narrow<u32>("Memory address out of range: 0x%llx%s", addr, loc));
		}

		static vm::addr_t cast(u64 addr)
		{
			return static_cast<vm::addr_t>(fmt::narrow<u32>("Memory address out of range: 0x%llx", addr));
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

	// Convert specified PS3/PSV virtual memory address to a pointer for privileged access (always readable/writable if allocated)
	inline void* base_priv(u32 addr)
	{
		return g_priv_addr + addr;
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

		template<typename T> inline to_be_t<T>* _ptr_priv(u32 addr)
		{
			return static_cast<to_be_t<T>*>(base_priv(addr));
		}

		// Convert specified PS3 address to a reference of specified (possibly converted to BE) type
		template<typename T> inline to_be_t<T>& _ref(u32 addr)
		{
			return *_ptr<T>(addr);
		}

		template<typename T> inline to_be_t<T>& _ref_priv(u32 addr)
		{
			return *_ptr_priv<T>(addr);
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

		template<typename T> inline to_le_t<T>* _ptr_priv(u32 addr)
		{
			return static_cast<to_le_t<T>*>(base_priv(addr));
		}

		template<typename T> inline to_le_t<T>& _ref(u32 addr)
		{
			return *_ptr<T>(addr);
		}

		template<typename T> inline to_le_t<T>& _ref_priv(u32 addr)
		{
			return *_ptr_priv<T>(addr);
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

	u32 stack_push(u32 size, u32 align_v);
	void stack_pop_verbose(u32 addr, u32 size) noexcept;

	extern thread_local u64 g_tls_fault_count;
}

#include "vm_var.h"
