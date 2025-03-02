#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include "util/types.hpp"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif

namespace stx
{
	template <typename T, usz Align = alignof(T), usz Size = sizeof(T)>
	struct se_storage
	{
		struct type8
		{
			alignas(Align > alignof(T) ? alignof(T) : Align) uchar data[sizeof(T)];
		};

		struct type64
		{
			alignas(8) u64 data[sizeof(T) < 8 ? 1 : sizeof(T) / 8];
		};

		using type = std::conditional_t<(Align >= 8 && sizeof(T) % 8 == 0), type64, type8>;

		// Possibly unoptimized generic byteswap for unaligned data
		static constexpr type swap(const type& src) noexcept;
	};

	template <typename T>
	struct se_storage<T, 2, 2>
	{
		using type = u16;

		static constexpr u16 swap(u16 src) noexcept
		{
#if __cpp_lib_byteswap >= 202110L
			return std::byteswap(src);
#elif defined(__GNUG__)
			return __builtin_bswap16(src);
#else
			if (std::is_constant_evaluated())
			{
				return (src >> 8) | (src << 8);
			}

			return _byteswap_ushort(src);
#endif
		}
	};

	template <typename T>
	struct se_storage<T, 4, 4>
	{
		using type = u32;

		static constexpr u32 swap(u32 src) noexcept
		{
#if __cpp_lib_byteswap >= 202110L
			return std::byteswap(src);
#elif defined(__GNUG__)
			return __builtin_bswap32(src);
#else
			if (std::is_constant_evaluated())
			{
				const u32 v0 = ((src << 8) & 0xff00ff00) | ((src >> 8) & 0x00ff00ff);
				return (v0 << 16) | (v0 >> 16);
			}

			return _byteswap_ulong(src);
#endif
		}
	};

	template <typename T>
	struct se_storage<T, 8, 8>
	{
		using type = u64;

		static constexpr u64 swap(u64 src) noexcept
		{
#if __cpp_lib_byteswap >= 202110L
			return std::byteswap(src);
#elif defined(__GNUG__)
			return __builtin_bswap64(src);
#else
			if (std::is_constant_evaluated())
			{
				const u64 v0 = ((src << 8) & 0xff00ff00ff00ff00) | ((src >> 8) & 0x00ff00ff00ff00ff);
				const u64 v1 = ((v0 << 16) & 0xffff0000ffff0000) | ((v0 >> 16) & 0x0000ffff0000ffff);
				return (v1 << 32) | (v1 >> 32);
			}

			return _byteswap_uint64(src);
#endif
		}
	};

	template <typename T, usz Align, usz Size>
	constexpr typename se_storage<T, Align, Size>::type se_storage<T, Align, Size>::swap(const type& src) noexcept
	{
		// Try to keep u16/u32/u64 optimizations at the cost of more bitcasts
		if constexpr (sizeof(T) == 1)
		{
			return src;
		}
		else if constexpr (sizeof(T) == 2)
		{
			return std::bit_cast<type>(se_storage<u16>::swap(std::bit_cast<u16>(src)));
		}
		else if constexpr (sizeof(T) == 4)
		{
			return std::bit_cast<type>(se_storage<u32>::swap(std::bit_cast<u32>(src)));
		}
		else if constexpr (sizeof(T) == 8)
		{
			return std::bit_cast<type>(se_storage<u64>::swap(std::bit_cast<u64>(src)));
		}
		else if constexpr (sizeof(T) % 8 == 0)
		{
			type64 tmp = std::bit_cast<type64>(src);
			type64 dst{};

			// Swap u64 blocks
			for (usz i = 0; i < sizeof(T) / 8; i++)
			{
				dst.data[i] = se_storage<u64>::swap(tmp.data[sizeof(T) / 8 - 1 - i]);
			}

			return std::bit_cast<type>(dst);
		}
		else
		{
			type dst{};

			// Swap by moving every byte
			for (usz i = 0; i < sizeof(T); i++)
			{
				dst.data[i] = src.data[sizeof(T) - 1 - i];
			}

			return dst;
		}
	}

	// Endianness support template
	template <typename T, bool Swap, usz Align = alignof(T)>
	class alignas(Align) se_t
	{
		using type = std::remove_cv_t<T>;
		using stype = typename se_storage<type, Align>::type;
		using storage = se_storage<type, Align>;

		stype m_data;

		static_assert(!std::is_pointer_v<type>, "se_t<> error: invalid type (pointer)");
		static_assert(!std::is_reference_v<type>, "se_t<> error: invalid type (reference)");
		static_assert(!std::is_array_v<type>, "se_t<> error: invalid type (array)");
		static_assert(sizeof(type) == alignof(type), "se_t<> error: unexpected alignment");

		static constexpr stype to_data(type value) noexcept
		{
			if constexpr (Swap)
			{
				return storage::swap(std::bit_cast<stype>(value));
			}
			else
			{
				return std::bit_cast<stype>(value);
			}
		}

		static constexpr auto int_or_enum()
		{
			if constexpr (std::is_enum_v<type>)
			{
				return std::underlying_type_t<type>{};
			}
			else
			{
				return type{};
			}
		}

		using under = decltype(int_or_enum());

	public:
		ENABLE_BITWISE_SERIALIZATION;

		se_t() noexcept = default;

		constexpr se_t(type value) noexcept
			: m_data(to_data(value))
		{
		}

		constexpr type value() const noexcept
		{
			if constexpr (Swap)
			{
				return std::bit_cast<type>(storage::swap(m_data));
			}
			else
			{
				return std::bit_cast<type>(m_data);
			}
		}

		constexpr type get() const noexcept
		{
			return value();
		}

		constexpr se_t& operator=(type value) noexcept
		{
			m_data = to_data(value);
			return *this;
		}

		constexpr operator type() const noexcept
		{
			return value();
		}

#ifdef _MSC_VER
		explicit constexpr operator bool() const noexcept
		{
			static_assert(!type{});
			static_assert(!std::is_floating_point_v<type>);
			return !!std::bit_cast<type>(m_data);
		}
#endif

		constexpr auto operator~() const noexcept
		{
			if constexpr ((std::is_integral_v<T> || std::is_enum_v<T>) && std::is_convertible_v<T, int>)
			{
				// Return se_t of integral type if possible. Promotion to int is omitted on purpose (a compromise).
				return std::bit_cast<se_t<under, Swap>>(static_cast<under>(~std::bit_cast<under>(m_data)));
			}
			else
			{
				return ~value();
			}
		}

private:
		// Compatible bit pattern cast
		template <typename To, typename Test = int, typename T2>
		static constexpr To right_arg_cast(const T2& rhs) noexcept
		{
			return std::bit_cast<To>(static_cast<se_t<To, Swap>>(rhs));
		}

		template <typename To, typename Test = int, typename R, usz Align2>
		static constexpr To right_arg_cast(const se_t<R, Swap, Align2>& rhs) noexcept
		{
			if constexpr ((std::is_integral_v<R> || std::is_enum_v<R>) && std::is_convertible_v<R, Test> && sizeof(R) == sizeof(T))
			{
				// Optimization: allow to reuse bit pattern of any se_t with bit-compatible type
				return std::bit_cast<To>(rhs);
			}
			else
			{
				return std::bit_cast<To>(static_cast<se_t<To, Swap>>(rhs.value()));
			}
		}

	public:
		template <typename T2>
			requires requires(const T2& t2) { +t2; }
		constexpr bool operator==(const T2& rhs) const noexcept
		{
			using R = std::common_type_t<T2>;

			if constexpr ((std::is_integral_v<T> || std::is_enum_v<T>) && (std::is_integral_v<R> || std::is_enum_v<R>))
			{
				if constexpr (sizeof(T) >= sizeof(R))
				{
					if constexpr (std::is_convertible_v<T, int> && std::is_convertible_v<R, int>)
					{
						return std::bit_cast<under>(m_data) == right_arg_cast<under>(rhs);
					}
					else
					{
						// Compare with strict type on the right side (possibly scoped enum)
						return std::bit_cast<type>(m_data) == right_arg_cast<type, type>(rhs);
					}
				}
			}

			// Keep outside of if constexpr to make sure it fails on invalid comparison
			return value() == rhs;
		}

private:
		template <typename T2>
		static constexpr bool check_args_for_bitwise_op()
		{
			using R = std::common_type_t<T2>;

			if constexpr ((std::is_integral_v<T> || std::is_enum_v<T>) && (std::is_integral_v<R> || std::is_enum_v<R>))
			{
				if constexpr (std::is_convertible_v<T, int> && std::is_convertible_v<R, int> && sizeof(T) >= sizeof(R))
				{
					return true;
				}
			}

			return false;
		}

public:
		template <typename T2>
		constexpr auto operator&(const T2& rhs) const noexcept
		{
			if constexpr (check_args_for_bitwise_op<T2>())
			{
				return std::bit_cast<se_t<under, Swap>>(static_cast<under>(std::bit_cast<under>(m_data) & right_arg_cast<under>(rhs)));
			}
			else
			{
				return value() & rhs;
			}
		}

		template <typename T2>
		constexpr auto operator|(const T2& rhs) const noexcept
		{
			if constexpr (check_args_for_bitwise_op<T2>())
			{
				return std::bit_cast<se_t<under, Swap>>(static_cast<under>(std::bit_cast<under>(m_data) | right_arg_cast<under>(rhs)));
			}
			else
			{
				return value() | rhs;
			}
		}

		template <typename T2>
		constexpr auto operator^(const T2& rhs) const noexcept
		{
			if constexpr (check_args_for_bitwise_op<T2>())
			{
				return std::bit_cast<se_t<under, Swap>>(static_cast<under>(std::bit_cast<under>(m_data) ^ right_arg_cast<under>(rhs)));
			}
			else
			{
				return value() ^ rhs;
			}
		}

		template <typename T1>
		constexpr se_t& operator+=(const T1& rhs)
		{
			*this = value() + rhs;
			return *this;
		}

		template <typename T1>
		constexpr se_t& operator-=(const T1& rhs)
		{
			*this = value() - rhs;
			return *this;
		}

		template <typename T1>
		constexpr se_t& operator*=(const T1& rhs)
		{
			*this = value() * rhs;
			return *this;
		}

		template <typename T1>
		constexpr se_t& operator/=(const T1& rhs)
		{
			*this = value() / rhs;
			return *this;
		}

		template <typename T1>
		constexpr se_t& operator%=(const T1& rhs)
		{
			*this = value() % rhs;
			return *this;
		}

		template <typename T1>
		constexpr se_t& operator&=(const T1& rhs)
		{
			if constexpr (std::is_integral_v<T>)
			{
				m_data = std::bit_cast<stype, type>(static_cast<type>(std::bit_cast<type>(m_data) & right_arg_cast<type>(rhs)));
				return *this;
			}

			*this = value() & rhs;
			return *this;
		}

		template <typename T1>
		constexpr se_t& operator|=(const T1& rhs)
		{
			if constexpr (std::is_integral_v<T>)
			{
				m_data = std::bit_cast<stype, type>(static_cast<type>(std::bit_cast<type>(m_data) | right_arg_cast<type>(rhs)));
				return *this;
			}

			*this = value() | rhs;
			return *this;
		}

		template <typename T1>
		constexpr se_t& operator^=(const T1& rhs)
		{
			if constexpr (std::is_integral_v<T>)
			{
				m_data = std::bit_cast<stype, type>(static_cast<type>(std::bit_cast<type>(m_data) ^ right_arg_cast<type>(rhs)));
				return *this;
			}

			*this = value() ^ rhs;
			return *this;
		}

		template <typename T1>
		constexpr se_t& operator<<=(const T1& rhs)
		{
			*this = value() << rhs;
			return *this;
		}

		template <typename T1>
		constexpr se_t& operator>>=(const T1& rhs)
		{
			*this = value() >> rhs;
			return *this;
		}

		constexpr se_t& operator++()
		{
			T value = *this;
			*this = ++value;
			return *this;
		}

		constexpr se_t& operator--()
		{
			T value = *this;
			*this = --value;
			return *this;
		}

		constexpr T operator++(int)
		{
			T value = *this;
			T result = value++;
			*this = value;
			return result;
		}

		constexpr T operator--(int)
		{
			T value = *this;
			T result = value--;
			*this = value;
			return result;
		}
	};
}

// Specializations

template <typename T, bool Swap, usz Align, typename T2, bool Swap2, usz Align2>
struct std::common_type<stx::se_t<T, Swap, Align>, stx::se_t<T2, Swap2, Align2>> : std::common_type<T, T2> {};

template <typename T, bool Swap, usz Align, typename T2>
struct std::common_type<stx::se_t<T, Swap, Align>, T2> : std::common_type<T, std::common_type_t<T2>> {};

template <typename T, typename T2, bool Swap2, usz Align2>
struct std::common_type<T, stx::se_t<T2, Swap2, Align2>> : std::common_type<std::common_type_t<T>, T2> {};

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
