#pragma once

class CPUThread;
struct ARMv7Context;

namespace vm
{
	template<typename T, int lvl = 1, typename AT = u32>
	class _ptr_base
	{
		AT m_addr;

	public:
		typedef typename std::remove_cv<T>::type type;
		static const u32 address_size = sizeof(AT);

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

		template<typename AT2>
		operator const _ptr_base<T, lvl, AT2>() const
		{
			const AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<const _ptr_base<T, lvl, AT2>&>(addr);
		}
		
		AT addr() const
		{
			return m_addr;
		}

		void set(const AT value)
		{
			m_addr = value;
		}

		static const _ptr_base make(const AT& addr)
		{
			return reinterpret_cast<const _ptr_base&>(addr);
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
	};
	
	template<typename T, typename AT>
	class _ptr_base<T, 1, AT>
	{
		AT m_addr;
		
	public:
		typedef typename std::remove_cv<T>::type type;

		__forceinline static const u32 data_size()
		{
			return sizeof(T);
		}

		__forceinline T* const operator -> () const
		{
			return vm::get_ptr<T>(vm::cast(m_addr));
		}

		_ptr_base operator++ (int)
		{
			AT result = m_addr;
			m_addr += data_size();
			return make(result);
		}

		_ptr_base& operator++ ()
		{
			m_addr += data_size();
			return *this;
		}

		_ptr_base operator-- (int)
		{
			AT result = m_addr;
			m_addr -= data_size();
			return make(result);
		}

		_ptr_base& operator-- ()
		{
			m_addr -= data_size();
			return *this;
		}

		_ptr_base& operator += (AT count)
		{
			m_addr += count * data_size();
			return *this;
		}

		_ptr_base& operator -= (AT count)
		{
			m_addr -= count * data_size();
			return *this;
		}

		_ptr_base operator + (typename	remove_be_t<AT>::type count) const { return make(m_addr + count * data_size()); }
		_ptr_base operator + (typename		to_be_t<AT>::type count) const { return make(m_addr + count * data_size()); }
		_ptr_base operator - (typename	remove_be_t<AT>::type count) const { return make(m_addr - count * data_size()); }
		_ptr_base operator - (typename		to_be_t<AT>::type count) const { return make(m_addr - count * data_size()); }

		__forceinline T& operator *() const
		{
			return vm::get_ref<T>(vm::cast(m_addr));
		}

		__forceinline T& operator [](typename remove_be_t<AT>::type index) const
		{
			return vm::get_ref<T>(vm::cast(m_addr + data_size() * index));
		}

		__forceinline T& operator [](typename to_be_t<AT>::forced_type index) const
		{
			return vm::get_ref<T>(vm::cast(m_addr + data_size() * index));
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
		
		AT addr() const
		{
			return m_addr;
		}

		template<typename U>
		void set(U&& value)
		{
			m_addr = convert_le_be<AT>(value);
		}

		template<typename AT2>
		operator const _ptr_base<T, 1, AT2>() const
		{
			const AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<const _ptr_base<T, 1, AT2>&>(addr);
		}

		T* get_ptr() const
		{
			return vm::get_ptr<T>(vm::cast(m_addr));
		}
		
		static const _ptr_base make(const AT& addr)
		{
			return reinterpret_cast<const _ptr_base&>(addr);
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

		template<typename AT2>
		operator const _ptr_base<void, 1, AT2>() const
		{
			const AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<const _ptr_base<void, 1, AT2>&>(addr);
		}

		template<typename AT2>
		operator const _ptr_base<const void, 1, AT2>() const
		{
			const AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<const _ptr_base<const void, 1, AT2>&>(addr);
		}

		static const _ptr_base make(const AT& addr)
		{
			return reinterpret_cast<const _ptr_base&>(addr);
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

		template<typename AT2>
		operator const _ptr_base<const void, 1, AT2>() const
		{
			const AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<const _ptr_base<const void, 1, AT2>&>(addr);
		}

		static const _ptr_base make(const AT& addr)
		{
			return reinterpret_cast<const _ptr_base&>(addr);
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
	};

	template<typename AT, typename RT, typename ...T>
	class _ptr_base<RT(T...), 1, AT>
	{
		AT m_addr;

	public:
		typedef RT(type)(T...);

		RT operator()(CPUThread& CPU, T... args) const; // defined in CB_FUNC.h, call using specified PPU thread context

		RT operator()(ARMv7Context& context, T... args) const; // defined in ARMv7Callback.h, passing context is mandatory

		RT operator()(T... args) const; // defined in CB_FUNC.h, call using current PPU thread context

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

		template<typename AT2>
		operator const _ptr_base<type, 1, AT2>() const
		{
			const AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<const _ptr_base<type, 1, AT2>&>(addr);
		}

		static const _ptr_base make(const AT& addr)
		{
			return reinterpret_cast<const _ptr_base&>(addr);
		}

		operator const std::function<type>() const
		{
			const AT addr = convert_le_be<AT>(m_addr);
			return [addr](T... args) -> RT { return make(addr)(args...); };
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
	};

	template<typename AT, typename RT, typename ...T>
	class _ptr_base<RT(*)(T...), 1, AT>
	{
		AT m_addr;

	public:
		static_assert(!sizeof(AT), "vm::_ptr_base<> error: use RT(T...) format for functions instead of RT(*)(T...)");
	};

	//BE pointer to LE data
	template<typename T, int lvl = 1, typename AT = u32> struct bptrl : public _ptr_base<T, lvl, typename to_be_t<AT>::type>
	{
		static const bptrl make(AT addr)
		{
			auto res = _ptr_base<T, lvl, typename to_be_t<AT>::type>::make(convert_le_be<typename to_be_t<AT>::type>(addr));
			return static_cast<const bptrl&>(res);
		}

		using _ptr_base<T, lvl, typename to_be_t<AT>::type>::operator=;
	};

	//BE pointer to BE data
	template<typename T, int lvl = 1, typename AT = u32> struct bptrb : public _ptr_base<typename to_be_t<T>::type, lvl, typename to_be_t<AT>::type>
	{
		static const bptrb make(AT addr)
		{
			auto res = _ptr_base<typename to_be_t<T>::type, lvl, typename to_be_t<AT>::type>::make(convert_le_be<typename to_be_t<AT>::type>(addr));
			return static_cast<const bptrb&>(res);
		}

		using _ptr_base<typename to_be_t<T>::type, lvl, typename to_be_t<AT>::type>::operator=;
	};

	//LE pointer to BE data
	template<typename T, int lvl = 1, typename AT = u32> struct lptrb : public _ptr_base<typename to_be_t<T>::type, lvl, AT>
	{
		static const lptrb make(AT addr)
		{
			auto res = _ptr_base<typename to_be_t<T>::type, lvl, AT>::make(addr);
			return static_cast<const lptrb&>(res);
		}

		using _ptr_base<typename to_be_t<T>::type, lvl, AT>::operator=;
	};

	//LE pointer to LE data
	template<typename T, int lvl = 1, typename AT = u32> struct lptrl : public _ptr_base<T, lvl, AT>
	{
		static const lptrl make(AT addr)
		{
			auto res = _ptr_base<T, lvl, AT>::make(addr);
			return static_cast<const lptrl&>(res);
		}

		using _ptr_base<T, lvl, AT>::operator=;
	};

	namespace ps3
	{
		template<typename T, int lvl = 1, typename AT = u32> struct ptr;
		template<typename T, int lvl = 1, typename AT = u32> struct bptr;

		//default pointer for HLE functions (LE pointer to BE data)
		template<typename T, int lvl, typename AT> struct ptr : public lptrb<T, lvl, AT>
		{
			static const ptr make(AT addr)
			{
				auto res = lptrb<T, lvl, AT>::make(addr);
				return static_cast<const ptr&>(res);
			}

			vm::ps3::bptr<T, lvl, AT> to_be() const
			{
				return vm::ps3::bptr<T, lvl, AT>::make(this->addr());
			}

			using lptrb<T, lvl, AT>::operator=;
		};

		//default pointer for HLE structures (BE pointer to BE data)
		template<typename T, int lvl, typename AT> struct bptr : public bptrb<T, lvl, AT>
		{
			static const bptr make(AT addr)
			{
				auto res = bptrb<T, lvl, AT>::make(addr);
				return static_cast<const bptr&>(res);
			}

			vm::ps3::ptr<T, lvl, AT> to_le() const
			{
				return vm::ps3::ptr<T, lvl, AT>::make(this->addr());
			}

			using bptrb<T, lvl, AT>::operator=;
		};
	}

	namespace psv
	{
		//default pointer for HLE functions & structures (LE pointer to LE data)
		template<typename T, int lvl = 1, typename AT = u32> struct ptr : public lptrl<T, lvl, AT>
		{
			static const ptr make(AT addr)
			{
				auto res = lptrl<T, lvl, AT>::make(addr);
				return static_cast<const ptr&>(res);
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

	template<typename T, int lvl, typename AT>
	struct unveil<vm::ps3::ptr<T, lvl, AT>, false>
	{
		typedef typename unveil<AT>::result_type result_type;

		__forceinline static result_type get_value(const vm::ps3::ptr<T, lvl, AT>& arg)
		{
			return unveil<AT>::get_value(arg.addr());
		}
	};

	template<typename T, int lvl, typename AT>
	struct unveil<vm::ps3::bptr<T, lvl, AT>, false>
	{
		typedef typename unveil<AT>::result_type result_type;

		__forceinline static result_type get_value(const vm::ps3::bptr<T, lvl, AT>& arg)
		{
			return unveil<AT>::get_value(arg.addr());
		}
	};

	template<typename T, int lvl, typename AT>
	struct unveil<vm::psv::ptr<T, lvl, AT>, false>
	{
		typedef typename unveil<AT>::result_type result_type;

		__forceinline static result_type get_value(const vm::psv::ptr<T, lvl, AT>& arg)
		{
			return unveil<AT>::get_value(arg.addr());
		}
	};
}

// external specializations for PPU GPR (SC_FUNC.h, CB_FUNC.h)

template<typename T, bool is_enum>
struct cast_ppu_gpr;

template<typename T, int lvl, typename AT>
struct cast_ppu_gpr<vm::ps3::ptr<T, lvl, AT>, false>
{
	__forceinline static u64 to_gpr(const vm::ps3::ptr<T, lvl, AT>& value)
	{
		return value.addr();
	}

	__forceinline static vm::ps3::ptr<T, lvl, AT> from_gpr(const u64 reg)
	{
		return vm::ps3::ptr<T, lvl, AT>::make(cast_ppu_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg));
	}
};

// external specializations for ARMv7 GPR

template<typename T, bool is_enum>
struct cast_armv7_gpr;

template<typename T, int lvl, typename AT>
struct cast_armv7_gpr<vm::psv::ptr<T, lvl, AT>, false>
{
	__forceinline static u32 to_gpr(const vm::psv::ptr<T, lvl, AT>& value)
	{
		return value.addr();
	}

	__forceinline static vm::psv::ptr<T, lvl, AT> from_gpr(const u32 reg)
	{
		return vm::psv::ptr<T, lvl, AT>::make(cast_armv7_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg));
	}
};
