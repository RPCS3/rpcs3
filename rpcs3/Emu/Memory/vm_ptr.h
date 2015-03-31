#pragma once

class PPUThread;
struct ARMv7Context;

namespace vm
{
	template<typename T, int lvl = 1, typename AT = u32>
	struct _ptr_base
	{
		AT m_addr;

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

		force_inline bool operator <(const _ptr_base& right) const { return m_addr < right.m_addr; }
		force_inline bool operator <=(const _ptr_base& right) const { return m_addr <= right.m_addr; }
		force_inline bool operator >(const _ptr_base& right) const { return m_addr > right.m_addr; }
		force_inline bool operator >=(const _ptr_base& right) const { return m_addr >= right.m_addr; }
		force_inline bool operator ==(const _ptr_base& right) const { return m_addr == right.m_addr; }
		force_inline bool operator !=(const _ptr_base& right) const { return m_addr != right.m_addr; }
		force_inline bool operator ==(const nullptr_t& right) const { return m_addr == 0; }
		force_inline bool operator !=(const nullptr_t& right) const { return m_addr != 0; }
		explicit operator bool() const { return m_addr != 0; }

		force_inline _ptr_base<T, lvl - 1, typename std::conditional<is_be_t<T>::value, typename to_be_t<AT>::type, AT>::type> operator *() const
		{
			AT addr = convert_le_be<AT>(read64(convert_le_be<u32>(m_addr)));
			return (_ptr_base<T, lvl - 1, typename std::conditional<is_be_t<T>::value, typename to_be_t<AT>::type, AT>::type>&)addr;
		}

		force_inline _ptr_base<T, lvl - 1, typename std::conditional<is_be_t<T>::value, typename to_be_t<AT>::type, AT>::type> operator [](AT index) const
		{
			AT addr = convert_le_be<AT>(read64(convert_le_be<u32>(m_addr + 8 * index)));
			return (_ptr_base<T, lvl - 1, typename std::conditional<is_be_t<T>::value, typename to_be_t<AT>::type, AT>::type>&)addr;
		}

		template<typename AT2>
		operator _ptr_base<T, lvl, AT2>() const
		{
			AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<_ptr_base<T, lvl, AT2>&>(addr);
		}
		
		AT addr() const
		{
			return m_addr;
		}

		template<typename U>
		void set(U&& value)
		{
			m_addr = convert_le_be<AT>(value);
		}

		static _ptr_base make(const AT& addr)
		{
			return reinterpret_cast<const _ptr_base&>(addr);
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
	};
	
	template<typename T, typename AT>
	struct _ptr_base<T, 1, AT>
	{
		AT m_addr;
		
		static_assert(!std::is_pointer<T>::value, "vm::_ptr_base<> error: invalid type (pointer)");
		static_assert(!std::is_reference<T>::value, "vm::_ptr_base<> error: invalid type (reference)");
		typedef typename std::remove_cv<T>::type type;

		force_inline static const u32 data_size()
		{
			return convert_le_be<AT>(sizeof(T));
		}

		force_inline T* const operator -> () const
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

		_ptr_base operator + (typename	remove_be_t<AT>::type count) const { return make(convert_le_be<AT>(convert_le_be<decltype(count)>(m_addr) + count * convert_le_be<decltype(count)>(data_size()))); }
		_ptr_base operator + (typename		to_be_t<AT>::type count) const { return make(convert_le_be<AT>(convert_le_be<decltype(count)>(m_addr) + count * convert_le_be<decltype(count)>(data_size()))); }
		_ptr_base operator - (typename	remove_be_t<AT>::type count) const { return make(convert_le_be<AT>(convert_le_be<decltype(count)>(m_addr) - count * convert_le_be<decltype(count)>(data_size()))); }
		_ptr_base operator - (typename		to_be_t<AT>::type count) const { return make(convert_le_be<AT>(convert_le_be<decltype(count)>(m_addr) - count * convert_le_be<decltype(count)>(data_size()))); }

		force_inline T& operator *() const
		{
			return vm::get_ref<T>(vm::cast(m_addr));
		}

		force_inline T& operator [](typename remove_be_t<AT>::type index) const
		{
			return vm::get_ref<T>(vm::cast(m_addr + data_size() * index));
		}

		force_inline T& operator [](typename to_be_t<AT>::forced_type index) const
		{
			return vm::get_ref<T>(vm::cast(m_addr + data_size() * index));
		}

		force_inline bool operator <(const _ptr_base& right) const { return m_addr < right.m_addr; }
		force_inline bool operator <=(const _ptr_base& right) const { return m_addr <= right.m_addr; }
		force_inline bool operator >(const _ptr_base& right) const { return m_addr > right.m_addr; }
		force_inline bool operator >=(const _ptr_base& right) const { return m_addr >= right.m_addr; }
		force_inline bool operator ==(const _ptr_base& right) const { return m_addr == right.m_addr; }
		force_inline bool operator !=(const _ptr_base& right) const { return m_addr != right.m_addr; }
		force_inline bool operator ==(const nullptr_t& right) const { return m_addr == 0; }
		force_inline bool operator !=(const nullptr_t& right) const { return m_addr != 0; }
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
		operator _ptr_base<T, 1, AT2>() const
		{
			AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<_ptr_base<T, 1, AT2>&>(addr);
		}

		T* get_ptr() const
		{
			return vm::get_ptr<T>(vm::cast(m_addr));
		}

		T* priv_ptr() const
		{
			return vm::priv_ptr<T>(vm::cast(m_addr));
		}
		
		static const _ptr_base make(const AT& addr)
		{
			return reinterpret_cast<const _ptr_base&>(addr);
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
	};

	template<typename AT>
	struct _ptr_base<void, 1, AT>
	{
		AT m_addr;
		
		AT addr() const
		{
			return m_addr;
		}

		template<typename U>
		void set(U&& value)
		{
			m_addr = convert_le_be<AT>(value);
		}

		void* get_ptr() const
		{
			return vm::get_ptr<void>(vm::cast(m_addr));
		}

		void* priv_ptr() const
		{
			return vm::priv_ptr<void>(vm::cast(m_addr));
		}

		explicit operator void*() const
		{
			return get_ptr();
		}

		force_inline bool operator <(const _ptr_base& right) const { return m_addr < right.m_addr; }
		force_inline bool operator <=(const _ptr_base& right) const { return m_addr <= right.m_addr; }
		force_inline bool operator >(const _ptr_base& right) const { return m_addr > right.m_addr; }
		force_inline bool operator >=(const _ptr_base& right) const { return m_addr >= right.m_addr; }
		force_inline bool operator ==(const _ptr_base& right) const { return m_addr == right.m_addr; }
		force_inline bool operator !=(const _ptr_base& right) const { return m_addr != right.m_addr; }
		force_inline bool operator ==(const nullptr_t& right) const { return m_addr == 0; }
		force_inline bool operator !=(const nullptr_t& right) const { return m_addr != 0; }
		explicit operator bool() const { return m_addr != 0; }

		template<typename AT2>
		operator _ptr_base<void, 1, AT2>() const
		{
			AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<_ptr_base<void, 1, AT2>&>(addr);
		}

		template<typename AT2>
		operator _ptr_base<const void, 1, AT2>() const
		{
			AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<_ptr_base<const void, 1, AT2>&>(addr);
		}

		static const _ptr_base make(const AT& addr)
		{
			return reinterpret_cast<const _ptr_base&>(addr);
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
	};

	template<typename AT>
	struct _ptr_base<const void, 1, AT>
	{
		AT m_addr;

		AT addr() const
		{
			return m_addr;
		}

		template<typename U>
		void set(U&& value)
		{
			m_addr = convert_le_be<AT>(value);
		}

		const void* get_ptr() const
		{
			return vm::get_ptr<const void>(vm::cast(m_addr));
		}

		const void* priv_ptr() const
		{
			return vm::priv_ptr<const void>(vm::cast(m_addr));
		}

		explicit operator const void*() const
		{
			return get_ptr();
		}

		force_inline bool operator <(const _ptr_base& right) const { return m_addr < right.m_addr; }
		force_inline bool operator <=(const _ptr_base& right) const { return m_addr <= right.m_addr; }
		force_inline bool operator >(const _ptr_base& right) const { return m_addr > right.m_addr; }
		force_inline bool operator >=(const _ptr_base& right) const { return m_addr >= right.m_addr; }
		force_inline bool operator ==(const _ptr_base& right) const { return m_addr == right.m_addr; }
		force_inline bool operator !=(const _ptr_base& right) const { return m_addr != right.m_addr; }
		force_inline bool operator ==(const nullptr_t& right) const { return m_addr == 0; }
		force_inline bool operator !=(const nullptr_t& right) const { return m_addr != 0; }
		explicit operator bool() const { return m_addr != 0; }

		template<typename AT2>
		operator _ptr_base<const void, 1, AT2>() const
		{
			AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<_ptr_base<const void, 1, AT2>&>(addr);
		}

		static const _ptr_base make(const AT& addr)
		{
			return reinterpret_cast<const _ptr_base&>(addr);
		}

		_ptr_base& operator = (const _ptr_base& right) = default;
	};

	template<typename AT, typename RT, typename ...T>
	struct _ptr_base<RT(T...), 1, AT>
	{
		AT m_addr;

		typedef RT(type)(T...);

		// defined in CB_FUNC.h, call using specified PPU thread context
		RT operator()(PPUThread& CPU, T... args) const;

		// defined in ARMv7Callback.h, passing context is mandatory
		RT operator()(ARMv7Context& context, T... args) const;

		// defined in CB_FUNC.h, call using current PPU thread context
		RT operator()(T... args) const;

		AT addr() const
		{
			return m_addr;
		}

		template<typename U>
		void set(U&& value)
		{
			m_addr = convert_le_be<AT>(value);
		}

		force_inline bool operator <(const _ptr_base& right) const { return m_addr < right.m_addr; }
		force_inline bool operator <=(const _ptr_base& right) const { return m_addr <= right.m_addr; }
		force_inline bool operator >(const _ptr_base& right) const { return m_addr > right.m_addr; }
		force_inline bool operator >=(const _ptr_base& right) const { return m_addr >= right.m_addr; }
		force_inline bool operator ==(const _ptr_base& right) const { return m_addr == right.m_addr; }
		force_inline bool operator !=(const _ptr_base& right) const { return m_addr != right.m_addr; }
		force_inline bool operator ==(const nullptr_t& right) const { return m_addr == 0; }
		force_inline bool operator !=(const nullptr_t& right) const { return m_addr != 0; }
		explicit operator bool() const { return m_addr != 0; }

		template<typename AT2>
		operator _ptr_base<type, 1, AT2>() const
		{
			AT2 addr = convert_le_be<AT2>(m_addr);
			return reinterpret_cast<_ptr_base<type, 1, AT2>&>(addr);
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
	struct _ptr_base<RT(*)(T...), 1, AT>
	{
		AT m_addr;

		static_assert(!sizeof(AT), "vm::_ptr_base<> error: use RT(T...) format for functions instead of RT(*)(T...)");
	};

	// Native endianness pointer to LE data
	template<typename T, int lvl = 1, typename AT = u32> using ptrl = _ptr_base<typename to_le_t<T>::type, lvl, AT>;

	// Native endianness pointer to BE data
	template<typename T, int lvl = 1, typename AT = u32> using ptrb = _ptr_base<typename to_be_t<T>::type, lvl, AT>;

	// BE pointer to LE data
	template<typename T, int lvl = 1, typename AT = u32> using bptrl = _ptr_base<typename to_le_t<T>::type, lvl, typename to_be_t<AT>::type>;

	// BE pointer to BE data
	template<typename T, int lvl = 1, typename AT = u32> using bptrb = _ptr_base<typename to_be_t<T>::type, lvl, typename to_be_t<AT>::type>;

	// LE pointer to LE data
	template<typename T, int lvl = 1, typename AT = u32> using lptrl = _ptr_base<typename to_le_t<T>::type, lvl, typename to_le_t<AT>::type>;

	// LE pointer to BE data
	template<typename T, int lvl = 1, typename AT = u32> using lptrb = _ptr_base<typename to_be_t<T>::type, lvl, typename to_le_t<AT>::type>;

	namespace ps3
	{
		// default pointer for PS3 HLE functions (Native endianness pointer to BE data)
		template<typename T, int lvl = 1, typename AT = u32> using ptr = ptrb<T, lvl, AT>;

		// default pointer for PS3 HLE structures (BE pointer to BE data)
		template<typename T, int lvl = 1, typename AT = u32> using bptr = bptrb<T, lvl, AT>;
	}

	namespace psv
	{
		// default pointer for PSV HLE functions (Native endianness pointer to LE data)
		template<typename T, int lvl = 1, typename AT = u32> using ptr = ptrl<T, lvl, AT>;

		// default pointer for PSV HLE structures (LE pointer to LE data)
		template<typename T, int lvl = 1, typename AT = u32> using lptr = lptrl<T, lvl, AT>;
	}

	// PS3 emulation is main now, so lets it be as default
	using namespace ps3;

	struct null_t
	{
		template<typename T, int lvl, typename AT> operator _ptr_base<T, lvl, AT>() const
		{
			const std::array<AT, 1> value = {};
			return _ptr_base<T, lvl, AT>::make(value[0]);
		}
	};

	// vm::null is convertible to any vm::ptr type as null pointer in virtual memory
	static null_t null;
}

namespace fmt
{
	// external specialization for fmt::format function

	template<typename T, int lvl, typename AT>
	struct unveil<vm::_ptr_base<T, lvl, AT>, false>
	{
		typedef typename unveil<AT>::result_type result_type;

		force_inline static result_type get_value(const vm::_ptr_base<T, lvl, AT>& arg)
		{
			return unveil<AT>::get_value(arg.addr());
		}
	};
}

// external specializations for PPU GPR (SC_FUNC.h, CB_FUNC.h)

template<typename T, bool is_enum>
struct cast_ppu_gpr;

template<typename T, int lvl, typename AT>
struct cast_ppu_gpr<vm::_ptr_base<T, lvl, AT>, false>
{
	force_inline static u64 to_gpr(const vm::_ptr_base<T, lvl, AT>& value)
	{
		return cast_ppu_gpr<AT, std::is_enum<AT>::value>::to_gpr(value.addr());
	}

	force_inline static vm::_ptr_base<T, lvl, AT> from_gpr(const u64 reg)
	{
		return vm::_ptr_base<T, lvl, AT>::make(cast_ppu_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg));
	}
};

// external specializations for ARMv7 GPR

template<typename T, bool is_enum>
struct cast_armv7_gpr;

template<typename T, int lvl, typename AT>
struct cast_armv7_gpr<vm::_ptr_base<T, lvl, AT>, false>
{
	force_inline static u32 to_gpr(const vm::_ptr_base<T, lvl, AT>& value)
	{
		return cast_armv7_gpr<AT, std::is_enum<AT>::value>::to_gpr(value.addr());
	}

	force_inline static vm::_ptr_base<T, lvl, AT> from_gpr(const u32 reg)
	{
		return vm::_ptr_base<T, lvl, AT>::make(cast_armv7_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg));
	}
};
