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
	// attempt to atomically update reserved memory
	bool reservation_update(u32 addr, const void* data, u32 size);
	// for internal use
	bool reservation_query(u32 addr, u32 size, bool is_writing);
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
	
	template<typename T = void>
	T* const get_ptr(u32 addr)
	{
		return reinterpret_cast<T*>(static_cast<u8*>(g_base_addr) + addr);
	}

	template<typename T>
	T& get_ref(u32 addr)
	{
		return *get_ptr<T>(addr);
	}

	template<typename T = void>
	T* const get_priv_ptr(u32 addr)
	{
		return reinterpret_cast<T*>(static_cast<u8*>(g_priv_addr) + addr);
	}

	template<typename T>
	T& get_priv_ref(u32 addr)
	{
		return *get_priv_ptr<T>(addr);
	}

	u32 get_addr(const void* real_pointer);

	__noinline void error(const u64 addr, const char* func);

	template<typename T>
	struct cast_ptr
	{
		static_assert(std::is_same<T, u32>::value, "Unsupported vm::cast() type");

		__forceinline static u32 cast(const T& addr, const char* func)
		{
			return 0;
		}
	};

#ifdef __APPLE__
	template<>
	struct cast_ptr<unsigned long>
	{
		__forceinline static u32 cast(const unsigned long addr, const char* func)
		{
			const u32 res = static_cast<u32>(addr);
			if (res != addr)
			{
				vm::error(addr, func);
			}

			return res;
		}
	};
#endif

	template<>
	struct cast_ptr<u32>
	{
		__forceinline static u32 cast(const u32 addr, const char* func)
		{
			return addr;
		}
	};

	template<>
	struct cast_ptr<u64>
	{
		__forceinline static u32 cast(const u64 addr, const char* func)
		{
			const u32 res = static_cast<u32>(addr);
			if (res != addr)
			{
				vm::error(addr, func);
			}

			return res;
		}
	};

	template<typename T, typename T2>
	struct cast_ptr<be_t<T, T2>>
	{
		__forceinline static u32 cast(const be_t<T, T2>& addr, const char* func)
		{
			return cast_ptr<T>::cast(addr.value(), func);
		}
	};

	template<typename T>
	__forceinline static u32 cast(const T& addr, const char* func = "vm::cast")
	{
		return cast_ptr<T>::cast(addr, func);
	}

	namespace ps3
	{
		void init();

		static u8 read8(u32 addr)
		{
			return get_ref<u8>(addr);
		}

		static void write8(u32 addr, u8 value)
		{
			get_ref<u8>(addr) = value;
		}

		static u16 read16(u32 addr)
		{
			return get_ref<be_t<u16>>(addr);
		}

		static void write16(u32 addr, be_t<u16> value)
		{
			get_ref<be_t<u16>>(addr) = value;
		}

		static u32 read32(u32 addr)
		{
			return get_ref<be_t<u32>>(addr);
		}

		static void write32(u32 addr, be_t<u32> value)
		{
			get_ref<be_t<u32>>(addr) = value;
		}

		static u64 read64(u32 addr)
		{
			return get_ref<be_t<u64>>(addr);
		}

		static void write64(u32 addr, be_t<u64> value)
		{
			get_ref<be_t<u64>>(addr) = value;
		}

		static void write16(u32 addr, u16 value)
		{
			write16(addr, be_t<u16>::make(value));
		}

		static void write32(u32 addr, u32 value)
		{
			write32(addr, be_t<u32>::make(value));
		}

		static void write64(u32 addr, u64 value)
		{
			write64(addr, be_t<u64>::make(value));
		}

		static u128 read128(u32 addr)
		{
			return get_ref<be_t<u128>>(addr);
		}

		static void write128(u32 addr, u128 value)
		{
			get_ref<be_t<u128>>(addr) = value;
		}
	}
	
	namespace psv
	{
		void init();

		static u8 read8(u32 addr)
		{
			return get_ref<u8>(addr);
		}

		static void write8(u32 addr, u8 value)
		{
			get_ref<u8>(addr) = value;
		}

		static u16 read16(u32 addr)
		{
			return get_ref<u16>(addr);
		}

		static void write16(u32 addr, u16 value)
		{
			get_ref<u16>(addr) = value;
		}

		static u32 read32(u32 addr)
		{
			return get_ref<u32>(addr);
		}

		static void write32(u32 addr, u32 value)
		{
			get_ref<u32>(addr) = value;
		}

		static u64 read64(u32 addr)
		{
			return get_ref<u64>(addr);
		}

		static void write64(u32 addr, u64 value)
		{
			get_ref<u64>(addr) = value;
		}

		static u128 read128(u32 addr)
		{
			return get_ref<u128>(addr);
		}

		static void write128(u32 addr, u128 value)
		{
			get_ref<u128>(addr) = value;
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
		ptr<T> alloc(u32 count) const
		{
			return ptr<T>::make(allocator(count * sizeof(T)));
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
