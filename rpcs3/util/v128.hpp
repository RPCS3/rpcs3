#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include "util/types.hpp"

template <typename T>
concept Vector128 = (sizeof(T) == 16) && (std::is_trivial_v<T>);

// 128-bit vector type
union alignas(16) v128
{
	uchar _bytes[16];
	char _chars[16];

	template <typename T, usz N, usz M>
	struct masked_array_t // array type accessed as (index ^ M)
	{
		T m_data[N];

	public:
		T& operator[](usz index)
		{
			return m_data[index ^ M];
		}

		const T& operator[](usz index) const
		{
			return m_data[index ^ M];
		}
	};

	template <typename T, usz N = 16 / sizeof(T)>
	using normal_array_t = masked_array_t<T, N, std::endian::little == std::endian::native ? 0 : N - 1>;
	template <typename T, usz N = 16 / sizeof(T)>
	using reversed_array_t = masked_array_t<T, N, std::endian::little == std::endian::native ? N - 1 : 0>;

	normal_array_t<u64> _u64;
	normal_array_t<s64> _s64;
	reversed_array_t<u64> u64r;
	reversed_array_t<s64> s64r;

	normal_array_t<u32> _u32;
	normal_array_t<s32> _s32;
	reversed_array_t<u32> u32r;
	reversed_array_t<s32> s32r;

	normal_array_t<u16> _u16;
	normal_array_t<s16> _s16;
	reversed_array_t<u16> u16r;
	reversed_array_t<s16> s16r;

	normal_array_t<u8> _u8;
	normal_array_t<s8> _s8;
	reversed_array_t<u8> u8r;
	reversed_array_t<s8> s8r;

	normal_array_t<f32> _f;
	normal_array_t<f64> _d;
	reversed_array_t<f32> fr;
	reversed_array_t<f64> dr;

	u128 _u;
	s128 _s;

	v128() = default;

	constexpr v128(const v128&) noexcept = default;

	template <Vector128 T>
	constexpr v128(const T& rhs) noexcept
		: v128(std::bit_cast<v128>(rhs))
	{
	}

	constexpr v128& operator=(const v128&) noexcept = default;

	template <Vector128 T>
	constexpr operator T() const noexcept
	{
		return std::bit_cast<T>(*this);
	}

	ENABLE_BITWISE_SERIALIZATION;

	static v128 from64(u64 _0, u64 _1 = 0)
	{
		v128 ret;
		ret._u64[0] = _0;
		ret._u64[1] = _1;
		return ret;
	}

	static v128 from64r(u64 _1, u64 _0 = 0)
	{
		return from64(_0, _1);
	}

	static v128 from64p(u64 value)
	{
		v128 ret;
		ret._u64[0] = value;
		ret._u64[1] = value;
		return ret;
	}

	static v128 from32(u32 _0, u32 _1 = 0, u32 _2 = 0, u32 _3 = 0)
	{
		v128 ret;
		ret._u32[0] = _0;
		ret._u32[1] = _1;
		ret._u32[2] = _2;
		ret._u32[3] = _3;
		return ret;
	}

	static v128 from32r(u32 _3, u32 _2 = 0, u32 _1 = 0, u32 _0 = 0)
	{
		return from32(_0, _1, _2, _3);
	}

	static v128 from32p(u32 value)
	{
		v128 ret;
		ret._u32[0] = value;
		ret._u32[1] = value;
		ret._u32[2] = value;
		ret._u32[3] = value;
		return ret;
	}

	static v128 fromf32p(f32 value)
	{
		v128 ret;
		ret._f[0] = value;
		ret._f[1] = value;
		ret._f[2] = value;
		ret._f[3] = value;
		return ret;
	}

	static v128 from16p(u16 value)
	{
		v128 ret;
		ret._u16[0] = value;
		ret._u16[1] = value;
		ret._u16[2] = value;
		ret._u16[3] = value;
		ret._u16[4] = value;
		ret._u16[5] = value;
		ret._u16[6] = value;
		ret._u16[7] = value;
		return ret;
	}

	static v128 from8p(u8 value)
	{
		v128 ret;
		std::memset(&ret, value, sizeof(ret));
		return ret;
	}

	static v128 undef()
	{
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#elif _MSC_VER
#pragma warning(push)
#pragma warning(disable : 6001)
#endif
		v128 ret;
		return ret;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif _MSC_VER
#pragma warning(pop)
#endif
	}

	// Unaligned load with optional index offset
	static v128 loadu(const void* ptr, usz index = 0)
	{
		v128 ret;
		std::memcpy(&ret, static_cast<const u8*>(ptr) + index * sizeof(v128), sizeof(v128));
		return ret;
	}

	// Unaligned store with optional index offset
	static void storeu(v128 value, void* ptr, usz index = 0)
	{
		std::memcpy(static_cast<u8*>(ptr) + index * sizeof(v128), &value, sizeof(v128));
	}

	v128 operator|(const v128&) const;
	v128 operator&(const v128&) const;
	v128 operator^(const v128&) const;
	v128 operator~() const;

	bool operator==(const v128& right) const;

	void clear()
	{
		*this = {};
	}
};

template <typename T, usz N, usz M>
struct offset32_array<v128::masked_array_t<T, N, M>>
{
	template <typename Arg>
	static inline u32 index32(const Arg& arg)
	{
		return u32{sizeof(T)} * (static_cast<u32>(arg) ^ static_cast<u32>(M));
	}
};

template <>
struct std::hash<v128>
{
	usz operator()(const v128& key) const
	{
		return key._u64[0] + key._u64[1];
	}
};
