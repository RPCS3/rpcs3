#pragma once

class CPUThread;

namespace vm
{
	template<typename T> class page_alloc_t
	{
		u32 m_addr;

		void dealloc()
		{
			if (m_addr && !vm::dealloc(m_addr))
			{
				if (!std::uncaught_exception()) // don't throw during stack unwinding
				{
					throw EXCEPTION("Deallocation failed (addr=0x%x)", m_addr);
				}
			}
		}

	public:
		page_alloc_t()
			: m_addr(0)
		{
		}

		page_alloc_t(vm::memory_location_t location, u32 count = 1)
			: m_addr(alloc(sizeof32(T) * count, location, std::max<u32>(alignof32(T), 4096)))
		{
		}

		page_alloc_t(const page_alloc_t&) = delete;

		page_alloc_t(page_alloc_t&& other)
			: m_addr(other.m_addr)
		{
			other.m_addr = 0;
		}

		~page_alloc_t() noexcept(false) // allow exceptions
		{
			this->dealloc();
		}

		page_alloc_t& operator =(const page_alloc_t&) = delete;

		page_alloc_t& operator =(page_alloc_t&& other)
		{
			std::swap(m_addr, other.m_addr);

			return *this;
		}

		u32 get_addr() const
		{
			return m_addr;
		}
	};

	template<typename T> class stack_alloc_t
	{
		u32 m_addr;
		u32 m_old_pos;

		CPUThread& m_thread;

	public:
		stack_alloc_t() = delete;

		stack_alloc_t(CPUThread& thread, u32 count = 1)
			: m_thread(thread)
		{
			m_addr = vm::stack_push(thread, sizeof32(T) * count, alignof32(T), m_old_pos);
		}

		~stack_alloc_t() noexcept(false) // allow exceptions
		{
			if (!std::uncaught_exception()) // don't call during stack unwinding (it's pointless anyway)
			{
				vm::stack_pop(m_thread, m_addr, m_old_pos);
			}
		}

		stack_alloc_t(const stack_alloc_t&) = delete;

		stack_alloc_t(stack_alloc_t&&) = delete;

		stack_alloc_t& operator =(const stack_alloc_t&) = delete;

		stack_alloc_t& operator =(stack_alloc_t&&) = delete;

		u32 get_addr() const
		{
			return m_addr;
		}
	};

	template<typename T, template<typename> class A> class _var_base final : public _ptr_base<T>, private A<T>
	{
		using _ptr_base<T>::m_addr;

		using allocator = A<T>;

	public:
		template<typename... Args, typename = std::enable_if_t<std::is_constructible<A<T>, Args...>::value>> _var_base(Args&&... args)
			: allocator(std::forward<Args>(args)...)
		{
			m_addr = allocator::get_addr();
		}
	};

	template<typename T, template<typename> class A = vm::stack_alloc_t> using varl = _var_base<to_le_t<T>, A>;

	template<typename T, template<typename> class A = vm::stack_alloc_t> using varb = _var_base<to_be_t<T>, A>;

	namespace ps3
	{
		template<typename T, template<typename> class A = vm::stack_alloc_t> using var = varb<T, A>;
	}

	namespace psv
	{
		template<typename T, template<typename> class A = vm::stack_alloc_t> using var = varl<T, A>;
	}
}
