#pragma once

#include "basic_types.h"
#include "v128.h"
#include <stdexcept>

#define IS_LE_MACHINE // only draft

namespace common
{
	template<typename T, std::size_t Size = sizeof(T)> struct se_storage
	{
		static_assert(!Size, "Bad se_storage<> type");
	};

	template<typename T> struct se_storage<T, 2>
	{
		using type = u16;

		[[deprecated]] static constexpr u16 swap(u16 src) // for reference
		{
			return (src >> 8) | (src << 8);
		}

		static inline u16 to(const T& src)
		{
			return _byteswap_ushort(reinterpret_cast<const u16&>(src));
		}

		static inline T from(u16 src)
		{
			const u16 result = _byteswap_ushort(src);
			return reinterpret_cast<const T&>(result);
		}
	};

	template<typename T> struct se_storage<T, 4>
	{
		using type = u32;

		[[deprecated]] static constexpr u32 swap(u32 src) // for reference
		{
			return (src >> 24) | (src << 24) | ((src >> 8) & 0x0000ff00) | ((src << 8) & 0x00ff0000);
		}

		static inline u32 to(const T& src)
		{
			return _byteswap_ulong(reinterpret_cast<const u32&>(src));
		}

		static inline T from(u32 src)
		{
			const u32 result = _byteswap_ulong(src);
			return reinterpret_cast<const T&>(result);
		}
	};

	template<typename T> struct se_storage<T, 8>
	{
		using type = u64;

		[[deprecated]] static constexpr u64 swap(u64 src) // for reference
		{
			return (src >> 56) | (src << 56) |
				((src >> 40) & 0x000000000000ff00) |
				((src >> 24) & 0x0000000000ff0000) |
				((src >> 8) & 0x00000000ff000000) |
				((src << 8) & 0x000000ff00000000) |
				((src << 24) & 0x0000ff0000000000) |
				((src << 40) & 0x00ff000000000000);
		}

		static inline u64 to(const T& src)
		{
			return _byteswap_uint64(reinterpret_cast<const u64&>(src));
		}

		static inline T from(u64 src)
		{
			const u64 result = _byteswap_uint64(src);
			return reinterpret_cast<const T&>(result);
		}
	};

	template<typename T> struct se_storage<T, 16>
	{
		using type = v128;

		static inline v128 to(const T& src)
		{
			return v128::byteswap(reinterpret_cast<const v128&>(src));
		}

		static inline T from(const v128& src)
		{
			const v128 result = v128::byteswap(src);
			return reinterpret_cast<const T&>(result);
		}
	};

	template<typename T> using se_storage_t = typename se_storage<T>::type;

	template<typename T1, typename T2> struct se_convert
	{
		using type_from = std::remove_cv_t<T1>;
		using type_to = std::remove_cv_t<T2>;
		using stype_from = se_storage_t<std::remove_cv_t<T1>>;
		using stype_to = se_storage_t<std::remove_cv_t<T2>>;
		using storage_from = se_storage<std::remove_cv_t<T1>>;
		using storage_to = se_storage<std::remove_cv_t<T2>>;

		static inline std::enable_if_t<std::is_same<type_from, type_to>::value, stype_to> convert(const stype_from& data)
		{
			return data;
		}

		static inline stype_to convert(const stype_from& data, ...)
		{
			return storage_to::to(storage_from::from(data));
		}
	};

	static struct se_raw_tag_t {} constexpr se_raw{};

	template<typename T, bool Se = true> class se_t;

	// se_t with switched endianness
	template<typename T> class se_t<T, true>
	{
		using type = typename std::remove_cv<T>::type;
		using stype = se_storage_t<type>;
		using storage = se_storage<type>;

		stype m_data;

		static_assert(!std::is_union<type>::value && !std::is_class<type>::value || std::is_same<type, v128>::value || std::is_same<type, u128>::value, "se_t<> error: invalid type (struct or union)");
		static_assert(!std::is_pointer<type>::value, "se_t<> error: invalid type (pointer)");
		static_assert(!std::is_reference<type>::value, "se_t<> error: invalid type (reference)");
		static_assert(!std::is_array<type>::value, "se_t<> error: invalid type (array)");
		static_assert(!std::is_enum<type>::value, "se_t<> error: invalid type (enumeration), use integral type instead");
		static_assert(alignof(type) == alignof(stype), "se_t<> error: unexpected alignment");

		template<typename T2, typename = void> struct bool_converter
		{
			static inline bool to_bool(const se_t<T2>& value)
			{
				return static_cast<bool>(value.value());
			}
		};

		template<typename T2> struct bool_converter<T2, std::enable_if_t<std::is_integral<T2>::value>>
		{
			static inline bool to_bool(const se_t<T2>& value)
			{
				return value.m_data != 0;
			}
		};

	public:
		se_t() = default;

		se_t(const se_t& right) = default;

		se_t(type value)
			: m_data(storage::to(value))
		{
		}

		// construct directly from raw data (don't use)
		constexpr se_t(const stype& raw_value, const se_raw_tag_t&)
			: m_data(raw_value)
		{
		}

		type value() const
		{
			return storage::from(m_data);
		}

		// access underlying raw data (don't use)
		constexpr const stype& raw_data() const noexcept
		{
			return m_data;
		}

		se_t& operator =(const se_t&) = default;

		se_t& operator =(type value)
		{
			return m_data = storage::to(value), *this;
		}

		operator type() const
		{
			return storage::from(m_data);
		}

		// optimization
		explicit operator bool() const
		{
			return bool_converter<type>::to_bool(*this);
		}

		// optimization
		template<typename T2> std::enable_if_t<IS_BINARY_COMPARABLE(T, T2), se_t&> operator &=(const se_t<T2>& right)
		{
			return m_data &= right.raw_data(), *this;
		}

		// optimization
		template<typename CT> std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator &=(CT right)
		{
			return m_data &= storage::to(right), *this;
		}

		// optimization
		template<typename T2> std::enable_if_t<IS_BINARY_COMPARABLE(T, T2), se_t&> operator |=(const se_t<T2>& right)
		{
			return m_data |= right.raw_data(), *this;
		}

		// optimization
		template<typename CT> std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator |=(CT right)
		{
			return m_data |= storage::to(right), *this;
		}

		// optimization
		template<typename T2> std::enable_if_t<IS_BINARY_COMPARABLE(T, T2), se_t&> operator ^=(const se_t<T2>& right)
		{
			return m_data ^= right.raw_data(), *this;
		}

		// optimization
		template<typename CT> std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator ^=(CT right)
		{
			return m_data ^= storage::to(right), *this;
		}
	};

	// se_t with native endianness
	template<typename T> class se_t<T, false>
	{
		using type = typename std::remove_cv<T>::type;

		type m_data;

		static_assert(!std::is_union<type>::value && !std::is_class<type>::value || std::is_same<type, v128>::value || std::is_same<type, u128>::value, "se_t<> error: invalid type (struct or union)");
		static_assert(!std::is_pointer<type>::value, "se_t<> error: invalid type (pointer)");
		static_assert(!std::is_reference<type>::value, "se_t<> error: invalid type (reference)");
		static_assert(!std::is_array<type>::value, "se_t<> error: invalid type (array)");
		static_assert(!std::is_enum<type>::value, "se_t<> error: invalid type (enumeration), use integral type instead");

	public:
		se_t() = default;

		se_t(const se_t&) = default;

		constexpr se_t(type value)
			: m_data(value)
		{
		}

		type value() const
		{
			return m_data;
		}

		se_t& operator =(const se_t& value) = default;

		se_t& operator =(type value)
		{
			return m_data = value, *this;
		}

		operator type() const
		{
			return m_data;
		}

		template<typename CT> std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator &=(const CT& right)
		{
			return m_data &= right, *this;
		}

		template<typename CT> std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator |=(const CT& right)
		{
			return m_data |= right, *this;
		}

		template<typename CT> std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<CT, T>::value, se_t&> operator ^=(const CT& right)
		{
			return m_data ^= right, *this;
		}
	};

	// se_t with native endianness (alias)
	template<typename T> using nse_t = se_t<T, false>;

	template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator +=(se_t<T, Se>& left, const T1& right)
	{
		auto value = left.value();
		return left = (value += right);
	}

	template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator -=(se_t<T, Se>& left, const T1& right)
	{
		auto value = left.value();
		return left = (value -= right);
	}

	template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator *=(se_t<T, Se>& left, const T1& right)
	{
		auto value = left.value();
		return left = (value *= right);
	}

	template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator /=(se_t<T, Se>& left, const T1& right)
	{
		auto value = left.value();
		return left = (value /= right);
	}

	template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator %=(se_t<T, Se>& left, const T1& right)
	{
		auto value = left.value();
		return left = (value %= right);
	}

	template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator <<=(se_t<T, Se>& left, const T1& right)
	{
		auto value = left.value();
		return left = (value <<= right);
	}

	template<typename T, bool Se, typename T1> inline se_t<T, Se>& operator >>=(se_t<T, Se>& left, const T1& right)
	{
		auto value = left.value();
		return left = (value >>= right);
	}

	template<typename T, bool Se> inline se_t<T, Se> operator ++(se_t<T, Se>& left, int)
	{
		auto value = left.value();
		auto result = value++;
		left = value;
		return result;
	}

	template<typename T, bool Se> inline se_t<T, Se> operator --(se_t<T, Se>& left, int)
	{
		auto value = left.value();
		auto result = value--;
		left = value;
		return result;
	}

	template<typename T, bool Se> inline se_t<T, Se>& operator ++(se_t<T, Se>& right)
	{
		auto value = right.value();
		return right = ++value;
	}

	template<typename T, bool Se> inline se_t<T, Se>& operator --(se_t<T, Se>& right)
	{
		auto value = right.value();
		return right = --value;
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T1, T2), bool> operator ==(const se_t<T1>& left, const se_t<T2>& right)
	{
		return left.raw_data() == right.raw_data();
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGRAL(T1) && IS_INTEGER(T2) && sizeof(T1) >= sizeof(T2), bool> operator ==(const se_t<T1>& left, T2 right)
	{
		return left.raw_data() == se_storage<T1>::to(right);
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGER(T1) && IS_INTEGRAL(T2) && sizeof(T1) <= sizeof(T2), bool> operator ==(T1 left, const se_t<T2>& right)
	{
		return se_storage<T2>::to(left) == right.raw_data();
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T1, T2), bool> operator !=(const se_t<T1>& left, const se_t<T2>& right)
	{
		return left.raw_data() != right.raw_data();
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGRAL(T1) && IS_INTEGER(T2) && sizeof(T1) >= sizeof(T2), bool> operator !=(const se_t<T1>& left, T2 right)
	{
		return left.raw_data() != se_storage<T1>::to(right);
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGER(T1) && IS_INTEGRAL(T2) && sizeof(T1) <= sizeof(T2), bool> operator !=(T1 left, const se_t<T2>& right)
	{
		return se_storage<T2>::to(left) != right.raw_data();
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T1, T2) && sizeof(T1) >= 4, se_t<decltype(T1() & T2())>> operator &(const se_t<T1>& left, const se_t<T2>& right)
	{
		return{ left.raw_data() & right.raw_data(), se_raw };
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGRAL(T1) && IS_INTEGER(T2) && sizeof(T1) >= sizeof(T2) && sizeof(T1) >= 4, se_t<decltype(T1() & T2())>> operator &(const se_t<T1>& left, T2 right)
	{
		return{ left.raw_data() & se_storage<T1>::to(right), se_raw };
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGER(T1) && IS_INTEGRAL(T2) && sizeof(T1) <= sizeof(T2) && sizeof(T2) >= 4, se_t<decltype(T1() & T2())>> operator &(T1 left, const se_t<T2>& right)
	{
		return{ se_storage<T2>::to(left) & right.raw_data(), se_raw };
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T1, T2) && sizeof(T1) >= 4, se_t<decltype(T1() | T2())>> operator |(const se_t<T1>& left, const se_t<T2>& right)
	{
		return{ left.raw_data() | right.raw_data(), se_raw };
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGRAL(T1) && IS_INTEGER(T2) && sizeof(T1) >= sizeof(T2) && sizeof(T1) >= 4, se_t<decltype(T1() | T2())>> operator |(const se_t<T1>& left, T2 right)
	{
		return{ left.raw_data() | se_storage<T1>::to(right), se_raw };
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGER(T1) && IS_INTEGRAL(T2) && sizeof(T1) <= sizeof(T2) && sizeof(T2) >= 4, se_t<decltype(T1() | T2())>> operator |(T1 left, const se_t<T2>& right)
	{
		return{ se_storage<T2>::to(left) | right.raw_data(), se_raw };
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_BINARY_COMPARABLE(T1, T2) && sizeof(T1) >= 4, se_t<decltype(T1() ^ T2())>> operator ^(const se_t<T1>& left, const se_t<T2>& right)
	{
		return{ left.raw_data() ^ right.raw_data(), se_raw };
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGRAL(T1) && IS_INTEGER(T2) && sizeof(T1) >= sizeof(T2) && sizeof(T1) >= 4, se_t<decltype(T1() ^ T2())>> operator ^(const se_t<T1>& left, T2 right)
	{
		return{ left.raw_data() ^ se_storage<T1>::to(right), se_raw };
	}

	// optimization
	template<typename T1, typename T2> inline std::enable_if_t<IS_INTEGER(T1) && IS_INTEGRAL(T2) && sizeof(T1) <= sizeof(T2) && sizeof(T2) >= 4, se_t<decltype(T1() ^ T2())>> operator ^(T1 left, const se_t<T2>& right)
	{
		return{ se_storage<T2>::to(left) ^ right.raw_data(), se_raw };
	}

	// optimization
	template<typename T> inline std::enable_if_t<IS_INTEGRAL(T) && sizeof(T) >= 4, se_t<decltype(~T())>> operator ~(const se_t<T>& right)
	{
		return{ ~right.raw_data(), se_raw };
	}

#ifdef IS_LE_MACHINE
	template<typename T> using be_t = se_t<T, true>;
	template<typename T> using le_t = se_t<T, false>;
#else
	template<typename T> using be_t = se_t<T, false>;
	template<typename T> using le_t = se_t<T, true>;
#endif


	template<typename T, bool Se, typename = void> struct to_se
	{
		using type = typename std::conditional<std::is_arithmetic<T>::value || std::is_enum<T>::value, se_t<T, Se>, T>::type;
	};

	template<typename T, bool Se> struct to_se<const T, Se, std::enable_if_t<!std::is_array<T>::value>> // move const qualifier
	{
		using type = const typename to_se<T, Se>::type;
	};

	template<typename T, bool Se> struct to_se<volatile T, Se, std::enable_if_t<!std::is_array<T>::value && !std::is_const<T>::value>> // move volatile qualifier
	{
		using type = volatile typename to_se<T, Se>::type;
	};

	template<typename T, bool Se> struct to_se<T[], Se>
	{
		using type = typename to_se<T, Se>::type[];
	};

	template<typename T, bool Se, std::size_t N> struct to_se<T[N], Se>
	{
		using type = typename to_se<T, Se>::type[N];
	};

	template<bool Se> struct to_se<u128, Se> { using type = se_t<u128, Se>; };
	template<bool Se> struct to_se<v128, Se> { using type = se_t<v128, Se>; };
	template<bool Se> struct to_se<bool, Se> { using type = bool; };
	template<bool Se> struct to_se<char, Se> { using type = char; };
	template<bool Se> struct to_se<u8, Se> { using type = u8; };
	template<bool Se> struct to_se<s8, Se> { using type = s8; };

#ifdef IS_LE_MACHINE
	template<typename T> using to_be_t = typename to_se<T, true>::type;
	template<typename T> using to_le_t = typename to_se<T, false>::type;
#else
	template<typename T> using to_be_t = typename to_se<T, false>::type;
	template<typename T> using to_le_t = typename to_se<T, true>::type;
#endif


	template<typename T, typename = void> struct to_ne
	{
		using type = T;
	};

	template<typename T, bool Se> struct to_ne<se_t<T, Se>>
	{
		using type = typename std::remove_cv<T>::type;
	};

	template<typename T> struct to_ne<const T, std::enable_if_t<!std::is_array<T>::value>> // move const qualifier
	{
		using type = const typename to_ne<T>::type;
	};

	template<typename T> struct to_ne<volatile T, std::enable_if_t<!std::is_array<T>::value && !std::is_const<T>::value>> // move volatile qualifier
	{
		using type = volatile typename to_ne<T>::type;
	};

	template<typename T> struct to_ne<T[]>
	{
		using type = typename to_ne<T>::type[];
	};

	template<typename T, std::size_t N> struct to_ne<T[N]>
	{
		using type = typename to_ne<T>::type[N];
	};

	// restore native endianness for T: returns T for be_t<T> or le_t<T>, T otherwise
	template<typename T> using to_ne_t = typename to_ne<T>::type;
}