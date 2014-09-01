#pragma once

namespace vm
{
	template<typename T, int lvl = 1, typename AT = u32>
	class _ptr_base
	{
		AT m_addr;

	public:
		typedef T type;

		_ptr_base operator++ (int)
		{
			AT result = m_addr;
			m_addr += sizeof(AT);
			return make(result);
		}

		_ptr_base& operator++ ()
		{
			m_addr += sizeof(AT);
			return *this;
		}

		_ptr_base operator-- (int)
		{
			AT result = m_addr;
			m_addr -= sizeof(AT);
			return make(result);
		}

		_ptr_base& operator-- ()
		{
			m_addr -= sizeof(AT);
			return *this;
		}

		_ptr_base& operator += (int count)
		{
			m_addr += count * sizeof(AT);
			return *this;
		}

		_ptr_base& operator -= (int count)
		{
			m_addr -= count * sizeof(AT);
			return *this;
		}

		_ptr_base operator + (int count) const
		{
			return make(m_addr + count * sizeof(AT));
		}

		_ptr_base operator - (int count) const
		{
			return make(m_addr - count * sizeof(AT));
		}

		__forceinline _ptr_base<T, lvl - 1, AT>& operator *()
		{
			return vm::get_ref<_ptr_base<T, lvl - 1, AT>>(m_addr);
		}

		__forceinline const _ptr_base<T, lvl - 1, AT>& operator *() const
		{
			return vm::get_ref<const _ptr_base<T, lvl - 1, AT>>(m_addr);
		}

		__forceinline _ptr_base<T, lvl - 1, AT>& operator [](int index)
		{
			return vm::get_ref<_ptr_base<T, lvl - 1, AT>>(m_addr + sizeof(AT) * index);
		}

		__forceinline const _ptr_base<T, lvl - 1, AT>& operator [](int index) const
		{
			return vm::get_ref<const _ptr_base<T, lvl - 1, AT>>(m_addr + sizeof(AT) * index);
		}

		operator bool() const
		{
			return m_addr != 0;
		}
		
		AT addr() const
		{
			return m_addr;
		}

		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}
	};
	
	template<typename T, typename AT>
	class _ptr_base<T, 1, AT>
	{
		AT m_addr;
		
	public:
		__forceinline T* const operator -> ()
		{
			return vm::get_ptr<T>(m_addr);
		}

		__forceinline const T* const operator -> () const
		{
			return vm::get_ptr<const T>(m_addr);
		}

		_ptr_base operator++ (int)
		{
			AT result = m_addr;
			m_addr += sizeof(T);
			return make(result);
		}

		_ptr_base& operator++ ()
		{
			m_addr += sizeof(T);
			return *this;
		}

		_ptr_base operator-- (int)
		{
			AT result = m_addr;
			m_addr -= sizeof(T);
			return make(result);
		}

		_ptr_base& operator-- ()
		{
			m_addr -= sizeof(T);
			return *this;
		}

		_ptr_base& operator += (int count)
		{
			m_addr += count * sizeof(T);
			return *this;
		}

		_ptr_base& operator -= (int count)
		{
			m_addr -= count * sizeof(T);
			return *this;
		}

		_ptr_base operator + (int count) const
		{
			return make(m_addr + count * sizeof(T));
		}

		_ptr_base operator - (int count) const
		{
			return make(m_addr - count * sizeof(T));
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
		operator _ref_base<T, AT>()
		{
			return _ref_base<T, AT>::make(m_addr);
		}
		
		operator const _ref_base<T, AT>() const
		{
			return _ref_base<T, AT>::make(m_addr);
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

		T* const get_ptr() const
		{
			return vm::get_ptr<T>(m_addr);
		}
		
		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}
	};

	template<typename AT>
	class _ptr_base<void, 1, AT>
	{
		AT m_addr;
		
	public:
		AT addr() const
		{
			return m_addr;
		}

		void* const get_ptr() const
		{
			return vm::get_ptr<void>(m_addr);
		}

		operator bool() const
		{
			return m_addr != 0;
		}

		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}
	};

	template<typename AT>
	class _ptr_base<const void, 1, AT>
	{
		AT m_addr;

	public:
		AT addr() const
		{
			return m_addr;
		}

		const void* const get_ptr() const
		{
			return vm::get_ptr<const void>(m_addr);
		}

		operator bool() const
		{
			return m_addr != 0;
		}

		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}
	};
	
	template<typename RT, typename AT>
	class _ptr_base<RT(*)(), 1, AT>
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

		operator bool() const
		{
			return m_addr != 0;
		}

		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}

		operator std::function<RT()>() const
		{
			const AT addr = m_addr;
			return [addr]() -> RT { return make(addr)(); };
		}
	};

	namespace ps3
	{
		template<typename T, int lvl, typename AT> struct ptr;
	}

	template<typename AT, typename RT, typename ...T>
	class _ptr_base<RT(*)(T...), 1, AT>
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
		struct _func_arg<_ptr_base<TT, 1, ATT>>
		{
			__forceinline static u64 get_value(const _ptr_base<TT, 1, ATT>& arg)
			{
				return arg.addr();
			}
		};

		template<typename TT, typename ATT>
		struct _func_arg<ps3::ptr<TT, 1, ATT>>
		{
			__forceinline static u64 get_value(const ps3::ptr<TT, 1, ATT>& arg)
			{
				return arg.addr();
			}
		};

		template<typename TT, typename ATT>
		struct _func_arg<_ref_base<TT, ATT>>
		{
			__forceinline static u64 get_value(const _ref_base<TT, ATT>& arg)
			{
				return arg.addr();
			}
		};

		template<typename TT, int i, typename ATT>
		struct _func_arg<mem_ptr_t<TT, i, ATT>>
		{
			static_assert(!i, "Invalid callback argument type (mem_ptr_t), use vm::ptr");
			
			__forceinline static u64 get_value(const mem_ptr_t<TT, i, ATT>& arg)
			{
				return 0;
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

		operator bool() const
		{
			return m_addr != 0;
		}

		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}

		operator std::function<RT(T...)>() const
		{
			const AT addr = m_addr;
			return [addr](T... args) -> RT { return make(addr)(args...); };
		}
	};

	//BE pointer to LE data
	template<typename T, int lvl = 1, typename AT = u32> struct bptrl : public _ptr_base<T, lvl, typename to_be_t<AT>::type>
	{
		static bptrl make(AT addr)
		{
			return (bptrl&)addr;
		}

		using _ptr_base<T, lvl, typename to_be_t<AT>::type>::operator=;
	};

	//BE pointer to BE data
	template<typename T, int lvl = 1, typename AT = u32> struct bptrb : public _ptr_base<typename to_be_t<T>::type, lvl, typename to_be_t<AT>::type>
	{
		static bptrb make(AT addr)
		{
			return (bptrb&)addr;
		}

		using _ptr_base<typename to_be_t<T>::type, lvl, typename to_be_t<AT>::type>::operator=;
	};

	//LE pointer to BE data
	template<typename T, int lvl = 1, typename AT = u32> struct lptrb : public _ptr_base<typename to_be_t<T>::type, lvl, AT>
	{
		static lptrb make(AT addr)
		{
			return (lptrb&)addr;
		}

		using _ptr_base<typename to_be_t<T>::type, lvl, AT>::operator=;
	};

	//LE pointer to LE data
	template<typename T, int lvl = 1, typename AT = u32> struct lptrl : public _ptr_base<T, lvl, AT>
	{
		static lptrl make(AT addr)
		{
			return (lptrl&)addr;
		}

		using _ptr_base<T, lvl, AT>::operator=;
	};

	namespace ps3
	{
		//default pointer for HLE functions (LE ptrerence to BE data)
		template<typename T, int lvl = 1, typename AT = u32> struct ptr : public lptrb<T, lvl, AT>
		{
			static ptr make(AT addr)
			{
				return (ptr&)addr;
			}

			using lptrb<T, lvl, AT>::operator=;
		};

		//default pointer for HLE structures (BE ptrerence to BE data)
		template<typename T, int lvl = 1, typename AT = u32> struct bptr : public bptrb<T, lvl, AT>
		{
			static bptr make(AT addr)
			{
				return (bptr&)addr;
			}

			using bptrb<T, lvl, AT>::operator=;
		};
	}

	namespace psv
	{
		//default pointer for HLE functions & structures (LE ptrerence to LE data)
		template<typename T, int lvl = 1, typename AT = u32> struct ptr : public lptrl<T, lvl, AT>
		{
			static ptr make(AT addr)
			{
				return (ptr&)addr;
			}

			using lptrl<T, lvl, AT>::operator=;
		};
	}

	//PS3 emulation is main now, so lets it be as default
	using namespace ps3;
}