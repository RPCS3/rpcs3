#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include "util/types.hpp"

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

#ifdef _MSC_VER
	template <typename T>
	struct opaque_wrapper
	{
		u128 m_data;

		opaque_wrapper() = default;

		opaque_wrapper(const T& value)
			: m_data(std::bit_cast<u128>(value))
		{
		}

		opaque_wrapper& operator=(const T& value)
		{
			m_data = std::bit_cast<u128>(value);
			return *this;
		}

		operator T() const
		{
			return std::bit_cast<T>(m_data);
		}
	};

	opaque_wrapper<__m128> vf;
	opaque_wrapper<__m128i> vi;
	opaque_wrapper<__m128d> vd;
#else
	__m128 vf;
	__m128i vi;
	__m128d vd;
#endif

	using enable_bitcopy = std::true_type;

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

	static inline v128 fromV(const __m128i& value);

	static inline v128 fromF(const __m128& value);

	static inline v128 fromD(const __m128d& value);

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

	static inline v128 add8(const v128& left, const v128& right);

	static inline v128 add16(const v128& left, const v128& right);

	static inline v128 add32(const v128& left, const v128& right);

	static inline v128 addfs(const v128& left, const v128& right);

	static inline v128 addfd(const v128& left, const v128& right);

	static inline v128 sub8(const v128& left, const v128& right);

	static inline v128 sub16(const v128& left, const v128& right);

	static inline v128 sub32(const v128& left, const v128& right);

	static inline v128 subfs(const v128& left, const v128& right);

	static inline v128 subfd(const v128& left, const v128& right);

	static inline v128 maxu8(const v128& left, const v128& right);

	static inline v128 minu8(const v128& left, const v128& right);

	static inline v128 eq8(const v128& left, const v128& right);

	static inline v128 eq16(const v128& left, const v128& right);

	static inline v128 eq32(const v128& left, const v128& right);

	static inline v128 eq32f(const v128& left, const v128& right);

	static inline v128 fma32f(v128 a, const v128& b, const v128& c);

	bool operator==(const v128& right) const;

	// result = (~left) & (right)
	static inline v128 andnot(const v128& left, const v128& right);

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
