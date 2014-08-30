#pragma once

namespace vm
{
	template<typename T, int lvl = 1, typename AT = u32>
	class ptr
	{
		AT m_addr;

	public:
		ptr operator++ (int)
		{
			AT result = m_addr;
			m_addr += sizeof(AT);
			return { result };
		}

		ptr& operator++ ()
		{
			m_addr += sizeof(AT);
			return *this;
		}

		ptr operator-- (int)
		{
			AT result = m_addr;
			m_addr -= sizeof(AT);
			return { result };
		}

		ptr& operator-- ()
		{
			m_addr -= sizeof(AT);
			return *this;
		}

		ptr& operator += (int count)
		{
			m_addr += count * sizeof(AT);
			return *this;
		}

		ptr& operator -= (int count)
		{
			m_addr -= count * sizeof(AT);
			return *this;
		}

		ptr operator + (int count) const
		{
			return { m_addr + count * sizeof(AT) };
		}

		ptr operator - (int count) const
		{
			return { m_addr - count * sizeof(AT) };
		}

		__forceinline ptr<T, lvl - 1, AT>& operator *()
		{
			return get_ref<ptr<T, lvl - 1, AT>>(m_addr);
		}

		__forceinline const ptr<T, lvl - 1, AT>& operator *() const
		{
			return get_ref<const ptr<T, lvl - 1, AT>>(m_addr);
		}

		__forceinline ptr<T, lvl - 1, AT>& operator [](int index)
		{
			return get_ref<ptr<T, lvl - 1, AT>>(m_addr + sizeof(AT) * index);
		}

		__forceinline const ptr<T, lvl - 1, AT>& operator [](int index) const
		{
			return get_ref<const ptr<T, lvl - 1, AT>>(m_addr + sizeof(AT) * index);
		}

		operator bool() const
		{
			return m_addr != 0;
		}
		
		AT addr() const
		{
			return m_addr;
		}

		static ptr make(u32 addr)
		{
			return (ptr&)addr;
		}
	};
	
	template<typename T, typename AT>
	class ptr<T, 1, AT>
	{
		AT m_addr;
		
	public:
		__forceinline T* operator -> ()
		{
			return vm::get_ptr<T>(m_addr);
		}

		__forceinline const T* operator -> () const
		{
			return vm::get_ptr<const T>(m_addr);
		}

		ptr operator++ (int)
		{
			AT result = m_addr;
			m_addr += sizeof(T);
			return { result };
		}

		ptr& operator++ ()
		{
			m_addr += sizeof(T);
			return *this;
		}

		ptr operator-- (int)
		{
			AT result = m_addr;
			m_addr -= sizeof(T);
			return { result };
		}

		ptr& operator-- ()
		{
			m_addr -= sizeof(T);
			return *this;
		}

		ptr& operator += (int count)
		{
			m_addr += count * sizeof(T);
			return *this;
		}

		ptr& operator -= (int count)
		{
			m_addr -= count * sizeof(T);
			return *this;
		}

		ptr operator + (int count) const
		{
			return { m_addr + count * sizeof(T) };
		}

		ptr operator - (int count) const
		{
			return { m_addr - count * sizeof(T) };
		}

		__forceinline T& operator *()
		{
			return get_ref<T>(m_addr);
		}

		__forceinline const T& operator *() const
		{
			return get_ref<T>(m_addr);
		}

		__forceinline T& operator [](int index)
		{
			return get_ref<T>(m_addr + sizeof(T) * index);
		}

		__forceinline const T& operator [](int index) const
		{
			return get_ref<const T>(m_addr + sizeof(T) * index);
		}
		
		/*
		operator ref<T>()
		{
			return { m_addr };
		}
		
		operator const ref<T>() const
		{
			return { m_addr };
		}
		*/
		
		AT addr() const
		{
			return m_addr;
		}

		operator bool() const
		{
			return m_addr != 0;
		}

		T* get_ptr() const
		{
			return vm::get_ptr<T>(m_addr);
		}
		
		static ptr make(u32 addr)
		{
			return (ptr&)addr;
		}
	};

	template<typename AT>
	class ptr<void, 1, AT>
	{
		AT m_addr;
		
	public:
		AT addr() const
		{
			return m_addr;
		}

		void* get_ptr() const
		{
			return vm::get_ptr<void>(m_addr);
		}

		operator bool() const
		{
			return m_addr != 0;
		}

		static ptr make(u32 addr)
		{
			return (ptr&)addr;
		}
	};
	
	template<typename RT, typename AT>
	class ptr<RT(*)(), 1, AT>
	{
		AT m_addr;

		__forceinline RT call_func(bool is_async) const
		{
			Callback cb;
			cb.SetAddr(m_addr);
			return (RT)cb.Branch(!is_async);
		}

	public:
		typedef RT(*type)();

		__forceinline RT operator()() const
		{
			return call_func(false);
		}

		__forceinline void async() const
		{
			call_func(true);
		}

		AT addr() const
		{
			return m_addr;
		}

		type get_ptr() const
		{
			return *((type*)vm::get_ptr<void*>(m_addr));
		}

		operator bool() const
		{
			return m_addr != 0;
		}

		static ptr make(u32 addr)
		{
			return (ptr&)addr;
		}
	};

	template<typename AT, typename RT, typename ...T>
	class ptr<RT(*)(T...), 1, AT>
	{
		AT m_addr;

		template<typename TT>
		struct _func_arg
		{
			__forceinline static u64 get_value(const TT& arg)
			{
				return arg;
			}
		};

		template<typename TT, typename ATT>
		struct _func_arg<ptr<TT, 1, ATT>>
		{
			__forceinline static u64 get_value(const ptr<TT, 1, ATT> arg)
			{
				return arg.addr();
			}
		};

		template<typename TT, typename ATT>
		struct _func_arg<_ref_base<TT, ATT>>
		{
			__forceinline static u64 get_value(const _ref_base<TT, ATT> arg)
			{
				return arg.addr();
			}
		};

		__forceinline RT call_func(bool is_async, T... args) const
		{
			Callback cb;
			cb.SetAddr(m_addr);
			cb.Handle(_func_arg<T>::get_value(args)...);
			return (RT)cb.Branch(!is_async);
		}

	public:
		typedef RT(*type)(T...);

		__forceinline RT operator()(T... args) const
		{
			return call_func(false, args...);
		}

		__forceinline void async(T... args) const
		{
			call_func(true, args...);
		}

		AT addr() const
		{
			return m_addr;
		}

		type get_ptr() const
		{
			return *((type*)vm::get_ptr<void*>(m_addr));
		}

		operator bool() const
		{
			return m_addr != 0;
		}

		static ptr make(u32 addr)
		{
			return (ptr&)addr;
		}
	};

	template<typename T, int lvl = 1, typename AT = u32>
	class beptr : public ptr<T, lvl, be_t<AT>> {};
}