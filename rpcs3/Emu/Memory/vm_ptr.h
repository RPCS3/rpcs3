#pragma once

class CPUThread;

namespace vm
{
	template<typename T, int lvl = 1, typename AT = u32>
	class _ptr_base
	{
		AT m_addr;

	public:
		typedef typename std::remove_cv<T>::type type;
		static const u32 address_size = (u32)sizeof(AT);

		_ptr_base operator++ (int)
		{
			AT result = m_addr;
			m_addr += address_size;
			return make(result);
		}

		_ptr_base& operator++ ()
		{
			m_addr += address_size;
			return *this;
		}

		_ptr_base operator-- (int)
		{
			AT result = m_addr;
			m_addr -= address_size;
			return make(result);
		}

		_ptr_base& operator-- ()
		{
			m_addr -= address_size;
			return *this;
		}

		_ptr_base& operator += (AT count)
		{
			m_addr += count * address_size;
			return *this;
		}

		_ptr_base& operator -= (AT count)
		{
			m_addr -= count * address_size;
			return *this;
		}

		_ptr_base operator + (typename	remove_be_t<AT>::type count) const { return make(m_addr + count * address_size); }
		_ptr_base operator + (typename		to_be_t<AT>::type count) const { return make(m_addr + count * address_size); }
		_ptr_base operator - (typename	remove_be_t<AT>::type count) const { return make(m_addr - count * address_size); }
		_ptr_base operator - (typename		to_be_t<AT>::type count) const { return make(m_addr - count * address_size); }

		__forceinline bool operator <(const _ptr_base& right) const { return m_addr < right.m_addr; }
		__forceinline bool operator <=(const _ptr_base& right) const { return m_addr <= right.m_addr; }
		__forceinline bool operator >(const _ptr_base& right) const { return m_addr > right.m_addr; }
		__forceinline bool operator >=(const _ptr_base& right) const { return m_addr >= right.m_addr; }
		__forceinline bool operator ==(const _ptr_base& right) const { return m_addr == right.m_addr; }
		__forceinline bool operator !=(const _ptr_base& right) const { return m_addr != right.m_addr; }
		__forceinline bool operator ==(const nullptr_t& right) const { return m_addr == 0; }
		__forceinline bool operator !=(const nullptr_t& right) const { return m_addr != 0; }
		explicit operator bool() const { return m_addr != 0; }

		__forceinline _ptr_base<T, lvl - 1, std::conditional<is_be_t<T>::value, typename to_be_t<AT>::type, AT>>& operator *() const
		{
			return vm::get_ref<_ptr_base<T, lvl - 1, std::conditional<is_be_t<T>::value, typename to_be_t<AT>::type, AT>>>(vm::cast(m_addr));
		}

		__forceinline _ptr_base<T, lvl - 1, std::conditional<is_be_t<T>::value, typename to_be_t<AT>::type, AT>>& operator [](AT index) const
		{
			return vm::get_ref<_ptr_base<T, lvl - 1, std::conditional<is_be_t<T>::value, typename to_be_t<AT>::type, AT>>>(vm::cast(m_addr + sizeof(AT)* index));
		}

		//typedef typename invert_be_t<AT>::type AT2;

		template<typename AT2>
		operator const _ptr_base<T, lvl, AT2>() const
		{
			typename std::remove_const<AT2>::type addr = convert_le_be<AT2>(m_addr);
			return (_ptr_base<T, lvl, AT2>&)addr;
		}
		
		AT addr() const
		{
			return m_addr;
		}

		void set(const AT value)
		{
			m_addr = value;
		}

		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
	};
	
	template<typename T, typename AT>
	class _ptr_base<T, 1, AT>
	{
		AT m_addr;
		
	public:
		typedef typename std::remove_cv<T>::type type;
		static const u32 data_size = (u32)sizeof(T);

		__forceinline T* const operator -> () const
		{
			return vm::get_ptr<T>(vm::cast(m_addr));
		}

		_ptr_base operator++ (int)
		{
			AT result = m_addr;
			m_addr += data_size;
			return make(result);
		}

		_ptr_base& operator++ ()
		{
			m_addr += data_size;
			return *this;
		}

		_ptr_base operator-- (int)
		{
			AT result = m_addr;
			m_addr -= data_size;
			return make(result);
		}

		_ptr_base& operator-- ()
		{
			m_addr -= data_size;
			return *this;
		}

		_ptr_base& operator += (AT count)
		{
			m_addr += count * data_size;
			return *this;
		}

		_ptr_base& operator -= (AT count)
		{
			m_addr -= count * data_size;
			return *this;
		}

		_ptr_base operator + (typename	remove_be_t<AT>::type count) const { return make(m_addr + count * data_size); }
		_ptr_base operator + (typename		to_be_t<AT>::type count) const { return make(m_addr + count * data_size); }
		_ptr_base operator - (typename	remove_be_t<AT>::type count) const { return make(m_addr - count * data_size); }
		_ptr_base operator - (typename		to_be_t<AT>::type count) const { return make(m_addr - count * data_size); }

		__forceinline T& operator *() const
		{
			return vm::get_ref<T>(vm::cast(m_addr));
		}

		__forceinline T& operator [](typename remove_be_t<AT>::type index) const
		{
			return vm::get_ref<T>(vm::cast(m_addr + data_size * index));
		}

		__forceinline T& operator [](typename to_be_t<AT>::forced_type index) const
		{
			return vm::get_ref<T>(vm::cast(m_addr + data_size * index));
		}

		__forceinline bool operator <(const _ptr_base& right) const { return m_addr < right.m_addr; }
		__forceinline bool operator <=(const _ptr_base& right) const { return m_addr <= right.m_addr; }
		__forceinline bool operator >(const _ptr_base& right) const { return m_addr > right.m_addr; }
		__forceinline bool operator >=(const _ptr_base& right) const { return m_addr >= right.m_addr; }
		__forceinline bool operator ==(const _ptr_base& right) const { return m_addr == right.m_addr; }
		__forceinline bool operator !=(const _ptr_base& right) const { return m_addr != right.m_addr; }
		__forceinline bool operator ==(const nullptr_t& right) const { return m_addr == 0; }
		__forceinline bool operator !=(const nullptr_t& right) const { return m_addr != 0; }
		explicit operator bool() const { return m_addr != 0; }
		explicit operator T*() const { return get_ptr(); }

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

		template<typename U>
		void set(U&& value)
		{
			m_addr = convert_le_be<AT>(value);
		}

		/*
		operator T*() const
		{
			return get_ptr();
		}
		*/
		//typedef typename invert_be_t<AT>::type AT2;

		template<typename AT2>
		operator const _ptr_base<T, 1, AT2>() const
		{
			typename std::remove_const<AT2>::type addr = convert_le_be<AT2>(m_addr);
			return (_ptr_base<T, 1, AT2>&)addr;
		}

		T* get_ptr() const
		{
			return vm::get_ptr<T>(vm::cast(m_addr));
		}
		
		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
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

		void set(const AT value)
		{
			m_addr = value;
		}

		void* get_ptr() const
		{
			return vm::get_ptr<void>(vm::cast(m_addr));
		}

		explicit operator void*() const
		{
			return get_ptr();
		}

		__forceinline bool operator <(const _ptr_base& right) const { return m_addr < right.m_addr; }
		__forceinline bool operator <=(const _ptr_base& right) const { return m_addr <= right.m_addr; }
		__forceinline bool operator >(const _ptr_base& right) const { return m_addr > right.m_addr; }
		__forceinline bool operator >=(const _ptr_base& right) const { return m_addr >= right.m_addr; }
		__forceinline bool operator ==(const _ptr_base& right) const { return m_addr == right.m_addr; }
		__forceinline bool operator !=(const _ptr_base& right) const { return m_addr != right.m_addr; }
		__forceinline bool operator ==(const nullptr_t& right) const { return m_addr == 0; }
		__forceinline bool operator !=(const nullptr_t& right) const { return m_addr != 0; }
		explicit operator bool() const { return m_addr != 0; }

		//typedef typename invert_be_t<AT>::type AT2;

		template<typename AT2>
		operator const _ptr_base<void, 1, AT2>() const
		{
			typename std::remove_const<AT2>::type addr = convert_le_be<AT2>(m_addr);
			return (_ptr_base<void, 1, AT2>&)addr;
		}

		template<typename AT2>
		operator const _ptr_base<const void, 1, AT2>() const
		{
			typename std::remove_const<AT2>::type addr = convert_le_be<AT2>(m_addr);
			return (_ptr_base<const void, 1, AT2>&)addr;
		}

		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
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

		void set(const AT value)
		{
			m_addr = value;
		}

		const void* get_ptr() const
		{
			return vm::get_ptr<const void>(vm::cast(m_addr));
		}

		explicit operator const void*() const
		{
			return get_ptr();
		}

		__forceinline bool operator <(const _ptr_base& right) const { return m_addr < right.m_addr; }
		__forceinline bool operator <=(const _ptr_base& right) const { return m_addr <= right.m_addr; }
		__forceinline bool operator >(const _ptr_base& right) const { return m_addr > right.m_addr; }
		__forceinline bool operator >=(const _ptr_base& right) const { return m_addr >= right.m_addr; }
		__forceinline bool operator ==(const _ptr_base& right) const { return m_addr == right.m_addr; }
		__forceinline bool operator !=(const _ptr_base& right) const { return m_addr != right.m_addr; }
		__forceinline bool operator ==(const nullptr_t& right) const { return m_addr == 0; }
		__forceinline bool operator !=(const nullptr_t& right) const { return m_addr != 0; }
		explicit operator bool() const { return m_addr != 0; }

		//typedef typename invert_be_t<AT>::type AT2;

		template<typename AT2>
		operator const _ptr_base<const void, 1, AT2>() const
		{
			typename std::remove_const<AT2>::type addr = convert_le_be<AT2>(m_addr);
			return (_ptr_base<const void, 1, AT2>&)addr;
		}

		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
	};

	template<typename AT, typename RT, typename ...T>
	class _ptr_base<RT(*)(T...), 1, AT>
	{
		AT m_addr;

	public:
		typedef RT(*type)(T...);

		RT operator()(CPUThread& CPU, T... args) const; // defined in CB_FUNC.h, call using specified CPU thread context

		RT operator()(T... args) const; // defined in CB_FUNC.h, call using current CPU thread context

		AT addr() const
		{
			return m_addr;
		}

		void set(const AT value)
		{
			m_addr = value;
		}

		__forceinline bool operator <(const _ptr_base& right) const { return m_addr < right.m_addr; }
		__forceinline bool operator <=(const _ptr_base& right) const { return m_addr <= right.m_addr; }
		__forceinline bool operator >(const _ptr_base& right) const { return m_addr > right.m_addr; }
		__forceinline bool operator >=(const _ptr_base& right) const { return m_addr >= right.m_addr; }
		__forceinline bool operator ==(const _ptr_base& right) const { return m_addr == right.m_addr; }
		__forceinline bool operator !=(const _ptr_base& right) const { return m_addr != right.m_addr; }
		__forceinline bool operator ==(const nullptr_t& right) const { return m_addr == 0; }
		__forceinline bool operator !=(const nullptr_t& right) const { return m_addr != 0; }
		explicit operator bool() const { return m_addr != 0; }

		//typedef typename invert_be_t<AT>::type AT2;

		template<typename AT2>
		operator const _ptr_base<RT(*)(T...), 1, AT2>() const
		{
			typename std::remove_const<AT2>::type addr = convert_le_be<AT2>(m_addr);
			return (_ptr_base<RT(*)(T...), 1, AT2>&)addr;
		}

		static _ptr_base make(AT addr)
		{
			return (_ptr_base&)addr;
		}

		operator const std::function<RT(T...)>() const
		{
			typename std::remove_const<AT>::type addr = convert_le_be<AT>(m_addr);
			return [addr](T... args) -> RT { return make(addr)(args...); };
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
	};

	//BE pointer to LE data
	template<typename T, int lvl = 1, typename AT = u32> struct bptrl : public _ptr_base<T, lvl, typename to_be_t<AT>::type>
	{
		static bptrl make(AT addr)
		{
			return (bptrl&)addr;
		}

		using _ptr_base<T, lvl, typename to_be_t<AT>::type>::operator=;
		//using _ptr_base<T, lvl, typename to_be_t<AT>::type>::operator const _ptr_base<T, lvl, AT>;
	};

	//BE pointer to BE data
	template<typename T, int lvl = 1, typename AT = u32> struct bptrb : public _ptr_base<typename to_be_t<T>::type, lvl, typename to_be_t<AT>::type>
	{
		static bptrb make(AT addr)
		{
			return (bptrb&)addr;
		}

		using _ptr_base<typename to_be_t<T>::type, lvl, typename to_be_t<AT>::type>::operator=;
		//using _ptr_base<typename to_be_t<T>::type, lvl, typename to_be_t<AT>::type>::operator const _ptr_base<typename to_be_t<T>::type, lvl, AT>;
	};

	//LE pointer to BE data
	template<typename T, int lvl = 1, typename AT = u32> struct lptrb : public _ptr_base<typename to_be_t<T>::type, lvl, AT>
	{
		static lptrb make(AT addr)
		{
			return (lptrb&)addr;
		}

		using _ptr_base<typename to_be_t<T>::type, lvl, AT>::operator=;
		//using _ptr_base<typename to_be_t<T>::type, lvl, AT>::operator const _ptr_base<typename to_be_t<T>::type, lvl, typename to_be_t<AT>::type>;
	};

	//LE pointer to LE data
	template<typename T, int lvl = 1, typename AT = u32> struct lptrl : public _ptr_base<T, lvl, AT>
	{
		static lptrl make(AT addr)
		{
			return (lptrl&)addr;
		}

		using _ptr_base<T, lvl, AT>::operator=;
		//using _ptr_base<T, lvl, AT>::operator const _ptr_base<T, lvl, typename to_be_t<AT>::type>;
	};

	namespace ps3
	{
		//default pointer for HLE functions (LE pointer to BE data)
		template<typename T, int lvl = 1, typename AT = u32> struct ptr : public lptrb<T, lvl, AT>
		{
			static ptr make(AT addr)
			{
				return (ptr&)addr;
			}

			using lptrb<T, lvl, AT>::operator=;
			//using lptrb<T, lvl, AT>::operator const _ptr_base<typename to_be_t<T>::type, lvl, AT>;
		};

		//default pointer for HLE structures (BE pointer to BE data)
		template<typename T, int lvl = 1, typename AT = u32> struct bptr : public bptrb<T, lvl, AT>
		{
			static bptr make(AT addr)
			{
				return (bptr&)addr;
			}

			using bptrb<T, lvl, AT>::operator=;
			//using bptrb<T, lvl, AT>::operator const _ptr_base<typename to_be_t<T>::type, lvl, AT>;
		};
	}

	namespace psv
	{
		//default pointer for HLE functions & structures (LE pointer to LE data)
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

namespace fmt
{
	// external specializations for fmt::format function
	namespace detail
	{
		template<typename T, int lvl, typename AT>
		struct get_fmt<vm::ps3::ptr<T, lvl, AT>, false>
		{
			__forceinline static std::string text(const char* fmt, size_t len, const vm::ps3::ptr<T, lvl, AT>& arg)
			{
				return get_fmt<AT>::text(fmt, len, arg.addr());
			}
		};

		template<typename T, int lvl, typename AT>
		struct get_fmt<vm::ps3::bptr<T, lvl, AT>, false>
		{
			__forceinline static std::string text(const char* fmt, size_t len, const vm::ps3::bptr<T, lvl, AT>& arg)
			{
				return get_fmt<AT>::text(fmt, len, arg.addr());
			}
		};

		template<typename T, int lvl, typename AT>
		struct get_fmt<vm::psv::ptr<T, lvl, AT>, false>
		{
			__forceinline static std::string text(const char* fmt, size_t len, const vm::psv::ptr<T, lvl, AT>& arg)
			{
				return get_fmt<AT>::text(fmt, len, arg.addr());
			}
		};
	}
}
