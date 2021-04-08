#pragma once

#include <memory>
#include "util/types.hpp"
#include "util/atomic.hpp"
#include "util/auto_typemap.hpp"
#include "Utilities/StrFmt.h"

#include "util/to_endian.hpp"

namespace utils
{
	class shm;
}

namespace vm
{
	extern u8* const g_base_addr;
	extern u8* const g_sudo_addr;
	extern u8* const g_exec_addr;
	extern u8* const g_stat_addr;
	extern u8* const g_free_addr;
	extern u8 g_reservations[];

	struct writer_lock;

	enum memory_location_t : uint
	{
		main,
		user64k,
		user1m,
		rsx_context,
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

	// Page information
	using memory_page = atomic_t<u8>;

	// Change memory protection of specified memory region
	bool page_protect(u32 addr, u32 size, u8 flags_test = 0, u8 flags_set = 0, u8 flags_clear = 0);

	// Check flags for specified memory range (unsafe)
	bool check_addr(u32 addr, u8 flags, u32 size);

	template <u32 Size = 1>
	bool check_addr(u32 addr, u8 flags = page_readable)
	{
		extern std::array<memory_page, 0x100000000 / 4096> g_pages;

		if (Size - 1 >= 4095u || Size & (Size - 1) || addr % Size)
		{
			// TODO
			return check_addr(addr, flags, Size);
		}

		return !(~g_pages[addr / 4096] & (flags | page_allocated));
	}

	// Search and map memory in specified memory location (min alignment is 0x10000)
	u32 alloc(u32 size, memory_location_t location, u32 align = 0x10000);

	// Map memory at specified address (in optionally specified memory location)
	u32 falloc(u32 addr, u32 size, memory_location_t location = any, const std::shared_ptr<utils::shm>* src = nullptr);

	// Unmap memory at specified address (in optionally specified memory location), return size
	u32 dealloc(u32 addr, memory_location_t location = any, const std::shared_ptr<utils::shm>* src = nullptr);

	// utils::memory_lock wrapper for locking sudo memory
	void lock_sudo(u32 addr, u32 size);

	enum block_flags_3
	{
		page_hidden    = 0x1000,

		page_size_4k   = 0x100, // SYS_MEMORY_PAGE_SIZE_4K
		page_size_64k  = 0x200, // SYS_MEMORY_PAGE_SIZE_64K
		page_size_1m   = 0x400, // SYS_MEMORY_PAGE_SIZE_1M
		page_size_mask = 0xF00, // SYS_MEMORY_PAGE_SIZE_MASK

		stack_guarded  = 0x10,
		preallocated   = 0x20, // nonshareable

		bf0_0x1 = 0x1, // TODO: document
		bf0_0x2 = 0x2, // TODO: document

		bf0_mask = bf0_0x1 | bf0_0x2,
	};

	// Object that handles memory allocations inside specific constant bounds ("location")
	class block_t final
	{
		auto_typemap<block_t> m;

		// Common mapped region for special cases
		std::shared_ptr<utils::shm> m_common;

		bool try_alloc(u32 addr, u8 flags, u32 size, std::shared_ptr<utils::shm>&&);

	public:
		block_t(u32 addr, u32 size, u64 flags);

		~block_t();

	public:
		const u32 addr; // Start address
		const u32 size; // Total size
		const u64 flags; // Byte 0xF000: block_flags_3
						 // Byte 0x0F00: block_flags_2_page_size (SYS_MEMORY_PAGE_SIZE_*)
						 // Byte 0x00F0: block_flags_1
						 // Byte 0x000F: block_flags_0

		// Search and map memory (min alignment is 0x10000)
		u32 alloc(u32 size, const std::shared_ptr<utils::shm>* = nullptr, u32 align = 0x10000, u64 flags = 0);

		// Try to map memory at fixed location
		u32 falloc(u32 addr, u32 size, const std::shared_ptr<utils::shm>* = nullptr, u64 flags = 0);

		// Unmap memory at specified location previously returned by alloc(), return size
		u32 dealloc(u32 addr, const std::shared_ptr<utils::shm>* = nullptr);

		// Get memory at specified address (if size = 0, addr assumed exact)
		std::pair<u32, std::shared_ptr<utils::shm>> peek(u32 addr, u32 size = 0);

		// Get allocated memory count
		u32 used();

		// Internal
		u32 imp_used(const vm::writer_lock&);
	};

	// Create new memory block with specified parameters and return it
	std::shared_ptr<block_t> map(u32 addr, u32 size, u64 flags = 0);

	// Create new memory block with at arbitrary position with specified alignment
	std::shared_ptr<block_t> find_map(u32 size, u32 align, u64 flags = 0);

	// Delete existing memory block with specified start address, return it
	std::shared_ptr<block_t> unmap(u32 addr, bool must_be_empty = false);

	// Get memory block associated with optionally specified memory location or optionally specified address
	std::shared_ptr<block_t> get(memory_location_t location, u32 addr = 0);

	// Allocate segment at specified location, does nothing if exists already
	std::shared_ptr<block_t> reserve_map(memory_location_t location, u32 addr, u32 area_size, u64 flags = page_size_64k);

	// Get PS3 virtual memory address from the provided pointer (nullptr or pointer from outside is always converted to 0)
	// Super memory is allowed as well
	inline std::pair<vm::addr_t, bool> try_get_addr(const void* real_ptr)
	{
		const std::make_unsigned_t<std::ptrdiff_t> diff = static_cast<const u8*>(real_ptr) - g_base_addr;

		if (diff <= u64{UINT32_MAX} * 2 + 1)
		{
			return {vm::addr_t{static_cast<u32>(diff)}, true};
		}

		return {};
	}

	inline vm::addr_t get_addr(const void* ptr)
	{
		const auto [addr, ok] = try_get_addr(ptr);

		if (!ok)
		{
			fmt::throw_exception("Not a virtual memory pointer (%p)", ptr);
		}

		return addr;
	}

	template<typename T>
	struct cast_impl
	{
		static_assert(std::is_same<T, u32>::value, "vm::cast() error: unsupported type");
	};

	template<>
	struct cast_impl<u32>
	{
		static vm::addr_t cast(u32 addr,
			u32,
			u32,
			const char*,
			const char*)
		{
			return static_cast<vm::addr_t>(addr);
		}
	};

	template<>
	struct cast_impl<u64>
	{
		static vm::addr_t cast(u64 addr,
			u32 line,
			u32 col,
			const char* file,
			const char* func)
		{
			return static_cast<vm::addr_t>(::narrow<u32>(addr, line, col, file, func));
		}
	};

	template<typename T, bool Se>
	struct cast_impl<se_t<T, Se>>
	{
		static vm::addr_t cast(const se_t<T, Se>& addr,
			u32 line,
			u32 col,
			const char* file,
			const char* func)
		{
			return cast_impl<T>::cast(addr, line, col, file, func);
		}
	};

	template<typename T>
	vm::addr_t cast(const T& addr,
		u32 line = __builtin_LINE(),
		u32 col = __builtin_COLUMN(),
		const char* file = __builtin_FILE(),
		const char* func = __builtin_FUNCTION())
	{
		return cast_impl<T>::cast(addr, line, col, file, func);
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

	// Read or write virtual memory in a safe manner, returns false on failure
	bool try_access(u32 addr, void* ptr, u32 size, bool is_write);

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

	template <typename T, typename AT>
	class _ptr_base;

	template <typename T, typename AT>
	class _ref_base;
}
