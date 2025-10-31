#pragma once

#include <memory>
#include <map>
#include "util/types.hpp"
#include "util/atomic.hpp"
#include "util/auto_typemap.hpp"

#include "util/to_endian.hpp"

class ppu_thread;

#ifdef RPCS3_HAS_MEMORY_BREAKPOINTS
#include "rpcs3qt/breakpoint_handler.h"
#include "util/logs.hpp"

LOG_CHANNEL(debugbp_log, "DebugBP");

void ppubreak(ppu_thread& ppu);
#endif

namespace utils
{
	class shm;

	template <typename T> class address_range;
	using address_range32 = address_range<u32>;
}

namespace vm
{
	extern u8* const g_base_addr;
	extern u8* const g_sudo_addr;
	extern u8* const g_exec_addr;
	extern u8* const g_stat_addr;
	extern u8* const g_free_addr;
	extern u8 g_reservations[65536 / 128 * 64];

	static constexpr u64 g_exec_addr_seg_offset = 0x2'0000'0000ULL;

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
	inline bool check_addr(u32 addr, u8 flags = page_readable)
	{
		extern std::array<memory_page, 0x100000000 / 4096> g_pages;

		if (Size - 1 >= 4095u || Size & (Size - 1) || addr % Size)
		{
			// TODO
			return check_addr(addr, flags, Size);
		}

		return !(~g_pages[addr / 4096] & (flags | page_allocated));
	}

	// Like check_addr but should only be used in lock-free context with care
	inline std::pair<bool, u8> get_addr_flags(u32 addr) noexcept
	{
		extern std::array<memory_page, 0x100000000 / 4096> g_pages;

		const u8 flags = g_pages[addr / 4096].load();

		return std::make_pair(!!(flags & page_allocated), flags);
	}

	// Read string in a safe manner (page aware) (bool true = if null-termination)
	bool read_string(u32 addr, u32 max_size, std::string& out_string, bool check_pages = true) noexcept;

	// Search and map memory in specified memory location (min alignment is 0x10000)
	u32 alloc(u32 size, memory_location_t location, u32 align = 0x10000);

	// Map memory at specified address (in optionally specified memory location)
	bool falloc(u32 addr, u32 size, memory_location_t location = any, const std::shared_ptr<utils::shm>* src = nullptr);

	// Unmap memory at specified address (in optionally specified memory location), return size
	u32 dealloc(u32 addr, memory_location_t location = any, const std::shared_ptr<utils::shm>* src = nullptr);

	// utils::memory_lock wrapper for locking sudo memory
	void lock_sudo(u32 addr, u32 size);

	enum block_flags_3
	{
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

	enum alloc_flags
	{
		alloc_hidden = 0x1000,
		alloc_unwritable = 0x2000,
		alloc_executable = 0x4000,

		alloc_prot_mask = alloc_hidden | alloc_unwritable | alloc_executable,
	};

	// Object that handles memory allocations inside specific constant bounds ("location")
	class block_t final
	{
		auto_typemap<block_t> m;

		// Common mapped region for special cases
		std::shared_ptr<utils::shm> m_common;

		atomic_t<u64> m_id = 0;

		bool try_alloc(u32 addr, u64 bflags, u32 size, std::shared_ptr<utils::shm>&&) const;

		// Unmap block
		bool unmap(std::vector<std::pair<u64, u64>>* unmapped = nullptr);
		friend bool _unmap_block(const std::shared_ptr<block_t>&, std::vector<std::pair<u64, u64>>* unmapped);

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
		bool falloc(u32 addr, u32 size, const std::shared_ptr<utils::shm>* = nullptr, u64 flags = 0);

		// Unmap memory at specified location previously returned by alloc(), return size
		u32 dealloc(u32 addr, const std::shared_ptr<utils::shm>* = nullptr) const;

		// Get memory at specified address (if size = 0, addr assumed exact)
		std::pair<u32, std::shared_ptr<utils::shm>> peek(u32 addr, u32 size = 0) const;

		// Get allocated memory count
		u32 used();

		// Internal
		u32 imp_used(const vm::writer_lock&) const;

		// Returns 0 if invalid, none-zero unique id if valid
		u64 is_valid() const
		{
			return m_id;
		}

		// Serialization helper for shared memory
		void get_shared_memory(std::vector<std::pair<utils::shm*, u32>>& shared);

		// Returns sample address for shared memory, 0 on failure
		u32 get_shm_addr(const std::shared_ptr<utils::shm>& shared);

		// Serialization
		void save(utils::serial& ar, std::map<utils::shm*, usz>& shared);
		block_t(utils::serial& ar, std::vector<std::shared_ptr<utils::shm>>& shared);
	};

	// Create new memory block with specified parameters and return it
	std::shared_ptr<block_t> map(u32 addr, u32 size, u64 flags = 0);

	// Create new memory block with at arbitrary position with specified alignment
	std::shared_ptr<block_t> find_map(u32 size, u32 align, u64 flags = 0);

	// Delete existing memory block with specified start address, .first=its ptr, .second=success
	std::pair<std::shared_ptr<block_t>, bool> unmap(u32 addr, bool must_be_empty = false, const std::shared_ptr<block_t>* ptr = nullptr);

	// Get memory block associated with optionally specified memory location or optionally specified address
	std::shared_ptr<block_t> get(memory_location_t location, u32 addr = 0);

	// Allocate segment at specified location, does nothing if exists already
	std::shared_ptr<block_t> reserve_map(memory_location_t location, u32 addr, u32 area_size, u64 flags = page_size_64k);

	// Get PS3 virtual memory address from the provided pointer (nullptr or pointer from outside is always converted to 0)
	// Super memory is allowed as well
	inline std::pair<vm::addr_t, bool> try_get_addr(const void* real_ptr)
	{
		const std::make_unsigned_t<std::ptrdiff_t> diff = static_cast<const u8*>(real_ptr) - g_base_addr;

		if (diff <= u64{u32{umax}} * 2 + 1)
		{
			return {vm::addr_t{static_cast<u32>(diff)}, true};
		}

		return {};
	}

	// Unsafe convert host ptr to PS3 VM address (clamp with 4GiB alignment assumption)
	inline vm::addr_t get_addr(const void* ptr)
	{
		return vm::addr_t{static_cast<u32>(uptr(ptr))};
	}

	template<typename T> requires (std::is_integral_v<decltype(+T{})> && (sizeof(+T{}) > 4 || std::is_signed_v<decltype(+T{})>))
	vm::addr_t cast(const T& addr, std::source_location src_loc = std::source_location::current())
	{
		return vm::addr_t{::narrow<u32>(+addr, src_loc)};
	}

	template<typename T> requires (std::is_integral_v<decltype(+T{})> && (sizeof(+T{}) <= 4 && !std::is_signed_v<decltype(+T{})>))
	vm::addr_t cast(const T& addr, u32 = 0, u32 = 0, const char* = nullptr, const char* = nullptr)
	{
		return vm::addr_t{static_cast<u32>(+addr)};
	}

	// Convert specified PS3/PSV virtual memory address to a pointer for common access
	template <typename T> requires (std::is_integral_v<decltype(+T{})>)
	inline void* base(T addr)
	{
		return g_base_addr + static_cast<u32>(vm::cast(addr));
	}

	inline const u8& read8(u32 addr)
	{
		return g_base_addr[addr];
	}

#ifdef RPCS3_HAS_MEMORY_BREAKPOINTS
	inline void write8(u32 addr, u8 value, ppu_thread* ppu = nullptr)
#else
	inline void write8(u32 addr, u8 value)
#endif
	{
		g_base_addr[addr] = value;

#ifdef RPCS3_HAS_MEMORY_BREAKPOINTS
		if (ppu && g_breakpoint_handler.HasBreakpoint(addr, breakpoint_types::bp_write))
		{
			debugbp_log.success("BPMW: breakpoint writing(8) 0x%x at 0x%x", value, addr);
			ppubreak(*ppu);
		}
#endif
	}

	// Read or write virtual memory in a safe manner, returns false on failure
	bool try_access(u32 addr, void* ptr, u32 size, bool is_write);

	inline namespace ps3_
	{
		// Convert specified PS3 address to a pointer of specified (possibly converted to BE) type
		template <typename T, typename U> inline to_be_t<T>* _ptr(const U& addr)
		{
			return static_cast<to_be_t<T>*>(base(addr));
		}

		// Convert specified PS3 address to a reference of specified (possibly converted to BE) type
		// Const lvalue: prevent abused writes
		template <typename T, typename U> inline const to_be_t<T>& _ref(const U& addr)
		{
			return *static_cast<const to_be_t<T>*>(base(addr));
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

#ifdef RPCS3_HAS_MEMORY_BREAKPOINTS
		template <typename T, typename U = T>
		inline void write(u32 addr, U value, ppu_thread* ppu = nullptr)
#else
		template <typename T, typename U = T>
		inline void write(u32 addr, U value, ppu_thread* = nullptr)
#endif
		{
			using dest_t = std::conditional_t<std::is_void_v<T>, U, T>;

			if constexpr (!std::is_void_v<T>)
			{
				*_ptr<dest_t>(addr) = value;
			}

#ifdef RPCS3_HAS_MEMORY_BREAKPOINTS
			if (ppu && g_breakpoint_handler.HasBreakpoint(addr, breakpoint_types::bp_write))
			{
				debugbp_log.success("BPMW: breakpoint writing(%d) 0x%x at 0x%x",
					sizeof(dest_t) * CHAR_BIT, value, addr);
				ppubreak(*ppu);
			}
#endif
		}

		inline void write16(u32 addr, be_t<u16> value, ppu_thread* ppu = nullptr)
		{
			write<be_t<u16>>(addr, value, ppu);
		}

		inline const be_t<u32>& read32(u32 addr)
		{
			return _ref<u32>(addr);
		}

		inline void write32(u32 addr, be_t<u32> value, ppu_thread* ppu = nullptr)
		{
			write<be_t<u32>>(addr, value, ppu);
		}

		inline const be_t<u64>& read64(u32 addr)
		{
			return _ref<u64>(addr);
		}

		inline void write64(u32 addr, be_t<u64> value, ppu_thread* ppu = nullptr)
		{
			write<be_t<u64>>(addr, value, ppu);
		}

		void init();
	}

	void close();

	void load(utils::serial& ar);
	void save(utils::serial& ar);

	// Returns sample address for shared memory, 0 on failure (wraps block_t::get_shm_addr)
	u32 get_shm_addr(const std::shared_ptr<utils::shm>& shared);

	template <typename T, typename AT>
	class _ptr_base;

	template <typename T, typename AT>
	class _ref_base;
}
