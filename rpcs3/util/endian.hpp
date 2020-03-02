#pragma once

#include <cstdint>
#include "Utilities/types.h"

#if __has_include(<bit>)
#include <bit>
#else
#include <type_traits>
#endif

namespace stx
{
	static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big);

	template <typename T, std::size_t Align = alignof(T), std::size_t Size = sizeof(T)>
	struct se_storage
	{
		struct type8
		{
			alignas(Align > alignof(T) ? alignof(T) : Align) unsigned char data[sizeof(T)];
		};

		struct type64
		{
			alignas(8) std::uint64_t data[sizeof(T) < 8 ? 1 : sizeof(T) / 8];
		};

		using type = std::conditional_t<(Align >= 8 && sizeof(T) % 8 == 0), type64, type8>;

		// Possibly unoptimized generic byteswap for unaligned data
		static constexpr type swap(const type& src) noexcept;
	};

	template <typename T>
	struct se_storage<T, alignof(std::uint16_t), 2>
	{
		using type = std::uint16_t;

		static constexpr std::uint16_t swap(std::uint16_t src) noexcept
		{
#if defined(__GNUG__)
			return __builtin_bswap16(src);
#else
			return _byteswap_ushort(src);
#endif
		}
	};

	template <typename T>
	struct se_storage<T, alignof(std::uint32_t), 4>
	{
		using type = std::uint32_t;

		static constexpr std::uint32_t swap(std::uint32_t src) noexcept
		{
#if defined(__GNUG__)
			return __builtin_bswap32(src);
#else
			return _byteswap_ulong(src);
#endif
		}
	};

	template <typename T>
	struct se_storage<T, alignof(std::uint64_t), 8>
	{
		using type = std::uint64_t;

		static constexpr std::uint64_t swap(std::uint64_t src) noexcept
		{
#if defined(__GNUG__)
			return __builtin_bswap64(src);
#else
			return _byteswap_uint64(src);
#endif
		}
	};

	template <typename T, std::size_t Align, std::size_t Size>
	constexpr typename se_storage<T, Align, Size>::type se_storage<T, Align, Size>::swap(const type& src) noexcept
	{
		// Try to keep u16/u32/u64 optimizations at the cost of more bitcasts
		if constexpr (sizeof(T) == 1)
		{
			return src;
		}
		else if constexpr (sizeof(T) == 2)
		{
			return std::bit_cast<type>(se_storage<std::uint16_t>::swap(std::bit_cast<std::uint16_t>(src)));
		}
		else if constexpr (sizeof(T) == 4)
		{
			return std::bit_cast<type>(se_storage<std::uint32_t>::swap(std::bit_cast<std::uint32_t>(src)));
		}
		else if constexpr (sizeof(T) == 8)
		{
			return std::bit_cast<type>(se_storage<std::uint64_t>::swap(std::bit_cast<std::uint64_t>(src)));
		}
		else if constexpr (sizeof(T) % 8 == 0)
		{
			type64 tmp = std::bit_cast<type64>(src);
			type64 dst{};

			// Swap u64 blocks
			for (std::size_t i = 0; i < sizeof(T) / 8; i++)
			{
				dst.data[i] = se_storage<std::uint64_t>::swap(tmp.data[sizeof(T) / 8 - 1 - i]);
			}

			return std::bit_cast<type>(dst);
		}
		else
		{
			type dst{};

			// Swap by moving every byte
			for (std::size_t i = 0; i < sizeof(T); i++)
			{
				dst.data[i] = src.data[sizeof(T) - 1 - i];
			}

			return dst;
		}
	}

	// Endianness support template
	template <typename T, bool Swap, std::size_t Align = alignof(T)>
	class alignas(Align) se_t
	{
		using type = typename std::remove_cv<T>::type;
		using stype = typename se_storage<type, Align>::type;
		using storage = se_storage<type, Align>;

		stype m_data;

		static_assert(!std::is_pointer<type>::value, "se_t<> error: invalid type (pointer)");
		static_assert(!std::is_reference<type>::value, "se_t<> error: invalid type (reference)");
		static_assert(!std::is_array<type>::value, "se_t<> error: invalid type (array)");
		static_assert(sizeof(type) == alignof(type), "se_t<> error: unexpected alignment");

		static stype to_data(type value) noexcept
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
			if constexpr (std::is_enum_v<simple_t<type>>)
			{
				return std::underlying_type_t<simple_t<type>>{};
			}
			else
			{
				return simple_t<type>{};
			}
		}

		using under = decltype(int_or_enum());

	public:
		se_t() = default;

		se_t(const se_t& right) = default;

		se_t(type value) noexcept
			: m_data(to_data(value))
		{
		}

		type value() const noexcept
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

		type get() const noexcept
		{
			return value();
		}

		se_t& operator=(const se_t&) = default;

		se_t& operator=(type value) noexcept
		{
			m_data = to_data(value);
			return *this;
		}

		using simple_type = simple_t<T>;

		operator type() const noexcept
		{
			return value();
		}

#ifdef _MSC_VER
		explicit operator bool() const noexcept
		{
			static_assert(!type{});
			static_assert(!std::is_floating_point_v<type>);
			return !!std::bit_cast<type>(m_data);
		}
#endif

		auto operator~() const noexcept
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
		static To right_arg_cast(const T2& rhs) noexcept
		{
			return std::bit_cast<To>(static_cast<se_t<To, Swap>>(rhs));
		}

		template <typename To, typename Test = int, typename R, std::size_t Align2>
		static To right_arg_cast(const se_t<R, Swap, Align2>& rhs) noexcept
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
		template <typename T2, typename = decltype(+std::declval<const T2&>())>
		bool operator==(const T2& rhs) const noexcept
		{
			using R = simple_t<T2>;

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

#if __cpp_impl_three_way_comparison >= 201711
#else
		template <typename T2, typename = decltype(+std::declval<const T2&>())>
		bool operator!=(const T2& rhs) const noexcept
		{
			return !operator==<T2>(rhs);
		}
#endif

private:
		template <typename T2>
		static constexpr bool check_args_for_bitwise_op()
		{
			using R = simple_t<T2>;

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
		auto operator&(const T2& rhs) const noexcept
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
		auto operator|(const T2& rhs) const noexcept
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
		auto operator^(const T2& rhs) const noexcept
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
		se_t& operator+=(const T1& rhs)
		{
			*this = value() + rhs;
			return *this;
		}

		template <typename T1>
		se_t& operator-=(const T1& rhs)
		{
			*this = value() - rhs;
			return *this;
		}

		template <typename T1>
		se_t& operator*=(const T1& rhs)
		{
			*this = value() * rhs;
			return *this;
		}

		template <typename T1>
		se_t& operator/=(const T1& rhs)
		{
			*this = value() / rhs;
			return *this;
		}

		template <typename T1>
		se_t& operator%=(const T1& rhs)
		{
			*this = value() % rhs;
			return *this;
		}

		template <typename T1>
		se_t& operator&=(const T1& rhs)
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
		se_t& operator|=(const T1& rhs)
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
		se_t& operator^=(const T1& rhs)
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
		se_t& operator<<=(const T1& rhs)
		{
			*this = value() << rhs;
			return *this;
		}

		template <typename T1>
		se_t& operator>>=(const T1& rhs)
		{
			*this = value() >> rhs;
			return *this;
		}

		se_t& operator++()
		{
			T value = *this;
			*this = ++value;
			return *this;
		}

		se_t& operator--()
		{
			T value = *this;
			*this = --value;
			return *this;
		}

		T operator++(int)
		{
			T value = *this;
			T result = value++;
			*this = value;
			return result;
		}

		T operator--(int)
		{
			T value = *this;
			T result = value--;
			*this = value;
			return result;
		}
	};
}
