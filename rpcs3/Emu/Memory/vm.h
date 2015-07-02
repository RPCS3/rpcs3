#pragma once
#include "Memory.h"

class CPUThread;

namespace vm
{
	extern void* g_base_addr; // base address of ps3/psv virtual memory for common access
	extern void* g_priv_addr; // base address of ps3/psv virtual memory for privileged access

	enum memory_location : uint
	{
		main,
		user_space,
		stack,

		memory_location_count
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

	static void set_stack_size(u32 size) {}
	static void initialize_stack() {}

	// break the reservation, return true if it was successfully broken
	bool reservation_break(u32 addr);
	// read memory and reserve it for further atomic update, return true if the previous reservation was broken
	bool reservation_acquire(void* data, u32 addr, u32 size, const std::function<void()>& callback = nullptr);
	// same as reservation_acquire but does not have the callback argument
	// used by the PPU LLVM JIT since creating a std::function object in LLVM IR is too complicated
	bool reservation_acquire_no_cb(void* data, u32 addr, u32 size);
	// attempt to atomically update reserved memory
	bool reservation_update(u32 addr, const void* data, u32 size);
	// for internal use
	bool reservation_query(u32 addr, u32 size, bool is_writing, std::function<bool()> callback);
	// for internal use
	void reservation_free();
	// perform complete operation
	void reservation_op(u32 addr, u32 size, std::function<void()> proc);

	// for internal use
	void page_map(u32 addr, u32 size, u8 flags);
	// for internal use
	bool page_protect(u32 addr, u32 size, u8 flags_test = 0, u8 flags_set = 0, u8 flags_clear = 0);
	// for internal use
	void page_unmap(u32 addr, u32 size);

	// unsafe address check
	bool check_addr(u32 addr, u32 size = 1);

	bool map(u32 addr, u32 size, u32 flags);
	bool unmap(u32 addr, u32 size = 0, u32 flags = 0);
	u32 alloc(u32 size, memory_location location = user_space);
	u32 alloc(u32 addr, u32 size, memory_location location = user_space);
	void dealloc(u32 addr, memory_location location = user_space);
	
	template<typename T = void> T* get_ptr(u32 addr)
	{
		return reinterpret_cast<T*>(static_cast<u8*>(g_base_addr) + addr);
	}

	template<typename T> T& get_ref(u32 addr)
	{
		return *get_ptr<T>(addr);
	}

	template<typename T = void> T* priv_ptr(u32 addr)
	{
		return reinterpret_cast<T*>(static_cast<u8*>(g_priv_addr) + addr);
	}

	template<typename T> T& priv_ref(u32 addr)
	{
		return *priv_ptr<T>(addr);
	}

	inline u32 get_addr(const void* real_pointer)
	{
		const uintptr_t diff = reinterpret_cast<uintptr_t>(real_pointer) - reinterpret_cast<uintptr_t>(g_base_addr);
		const u32 res = static_cast<u32>(diff);

		if (res == diff)
		{
			return res;
		}

		if (real_pointer)
		{
			throw EXCEPTION("Not a virtual memory pointer (%p)", real_pointer);
		}

		return 0;
	}

	template<typename T> struct cast_ptr
	{
		static_assert(std::is_same<T, u32>::value, "Unsupported vm::cast() type");

		force_inline static u32 cast(const T& addr, const char* func)
		{
			return 0;
		}
	};

	template<> struct cast_ptr<u32>
	{
		force_inline static u32 cast(const u32 addr, const char* func)
		{
			return addr;
		}
	};

	template<> struct cast_ptr<u64>
	{
		force_inline static u32 cast(const u64 addr, const char* func)
		{
			const u32 res = static_cast<u32>(addr);

			if (res != addr)
			{
				throw EXCEPTION("%s(): failed to cast 0x%llx (too big value)", func, addr);
			}

			return res;
		}
	};

	template<typename T> struct cast_ptr<be_t<T>>
	{
		force_inline static u32 cast(const be_t<T>& addr, const char* func)
		{
			return cast_ptr<T>::cast(addr.value(), func);
		}
	};

	template<typename T> struct cast_ptr<le_t<T>>
	{
		force_inline static u32 cast(const le_t<T>& addr, const char* func)
		{
			return cast_ptr<T>::cast(addr.value(), func);
		}
	};

	template<typename T> force_inline static u32 cast(const T& addr, const char* func = "vm::cast")
	{
		return cast_ptr<T>::cast(addr, func);
	}

	static u8 read8(u32 addr)
	{
		return get_ref<u8>(addr);
	}

	static void write8(u32 addr, u8 value)
	{
		get_ref<u8>(addr) = value;
	}

	namespace ps3
	{
		void init();

		inline const be_t<u16>& read16(u32 addr)
		{
			return get_ref<const be_t<u16>>(addr);
		}

		inline void write16(u32 addr, be_t<u16> value)
		{
			get_ref<be_t<u16>>(addr) = value;
		}

		inline void write16(u32 addr, u16 value)
		{
			get_ref<be_t<u16>>(addr) = value;
		}

		inline const be_t<u32>& read32(u32 addr)
		{
			return get_ref<const be_t<u32>>(addr);
		}

		inline void write32(u32 addr, be_t<u32> value)
		{
			get_ref<be_t<u32>>(addr) = value;
		}

		inline void write32(u32 addr, u32 value)
		{
			get_ref<be_t<u32>>(addr) = value;
		}

		inline const be_t<u64>& read64(u32 addr)
		{
			return get_ref<const be_t<u64>>(addr);
		}

		inline void write64(u32 addr, be_t<u64> value)
		{
			get_ref<be_t<u64>>(addr) = value;
		}

		inline void write64(u32 addr, u64 value)
		{
			get_ref<be_t<u64>>(addr) = value;
		}

		inline const be_t<u128>& read128(u32 addr)
		{
			return get_ref<const be_t<u128>>(addr);
		}

		inline void write128(u32 addr, be_t<u128> value)
		{
			get_ref<be_t<u128>>(addr) = value;
		}

		inline void write128(u32 addr, u128 value)
		{
			get_ref<be_t<u128>>(addr) = value;
		}
	}
	
	namespace psv
	{
		void init();

		inline const le_t<u16>& read16(u32 addr)
		{
			return get_ref<const le_t<u16>>(addr);
		}

		inline void write16(u32 addr, le_t<u16> value)
		{
			get_ref<le_t<u16>>(addr) = value;
		}

		inline void write16(u32 addr, u16 value)
		{
			get_ref<le_t<u16>>(addr) = value;
		}

		inline const le_t<u32>& read32(u32 addr)
		{
			return get_ref<const le_t<u32>>(addr);
		}

		inline void write32(u32 addr, le_t<u32> value)
		{
			get_ref<le_t<u32>>(addr) = value;
		}

		inline void write32(u32 addr, u32 value)
		{
			get_ref<le_t<u32>>(addr) = value;
		}

		inline const le_t<u64>& read64(u32 addr)
		{
			return get_ref<const le_t<u64>>(addr);
		}

		inline void write64(u32 addr, le_t<u64> value)
		{
			get_ref<le_t<u64>>(addr) = value;
		}

		inline void write64(u32 addr, u64 value)
		{
			get_ref<le_t<u64>>(addr) = value;
		}

		inline const le_t<u128>& read128(u32 addr)
		{
			return get_ref<const le_t<u128>>(addr);
		}

		inline void write128(u32 addr, le_t<u128> value)
		{
			get_ref<le_t<u128>>(addr) = value;
		}

		inline void write128(u32 addr, u128 value)
		{
			get_ref<le_t<u128>>(addr) = value;
		}
	}

	namespace psp
	{
		using namespace psv;

		void init();
	}

	void close();

	u32 stack_push(CPUThread& CPU, u32 size, u32 align, u32& old_pos);
	void stack_pop(CPUThread& CPU, u32 addr, u32 old_pos);
}

#include "vm_ref.h"
#include "vm_ptr.h"
#include "vm_var.h"

namespace vm
{
	struct location_info
	{
		u32 addr_offset;
		u32 size;

		u32(*allocator)(u32 size);
		u32(*fixed_allocator)(u32 addr, u32 size);
		void(*deallocator)(u32 addr);

		u32 alloc_offset;

		template<typename T = char>
		_ptr_base<T> alloc(u32 count = 1) const
		{
			return{ allocator(count * sizeof32(T)) };
		}

		template<typename T = char>
		_ptr_base<T> fixed_alloc(u32 addr, u32 count = 1) const
		{
			return{ fixed_allocator(addr, count * sizeof32(T)) };
		}
	};

	extern location_info g_locations[memory_location_count];

	template<memory_location location = main>
	location_info& get()
	{
		assert(location < memory_location_count);
		return g_locations[location];
	}

	class stack
	{
		u32 m_begin;
		u32 m_size;
		int m_page_size;
		int m_position;
		u8 m_align;

	public:
		void init(u32 begin, u32 size, u32 page_size = 180, u8 align = 0x10)
		{
			m_begin = begin;
			m_size = size;
			m_page_size = page_size;
			m_position = 0;
			m_align = align;
		}

		u32 alloc_new_page()
		{
			assert(m_position + m_page_size < (int)m_size);
			m_position += (int)m_page_size;
			return m_begin + m_position;
		}

		u32 dealloc_new_page()
		{
			assert(m_position - m_page_size > 0);
			m_position -= (int)m_page_size;
			return m_begin + m_position;
		}
	};
}
