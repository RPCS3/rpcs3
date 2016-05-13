#pragma once

#include "../../../Utilities/BitField.h"

#include <set>

enum class arm_encoding
{
	T1, T2, T3, T4, A1, A2,
};

// Get Thumb instruction size
inline bool arm_op_thumb_is_32(u32 op16)
{
	return (op16 >> 11) >= 29;
}

namespace arm_code
{
	inline namespace arm_encoding_alias
	{
		constexpr auto T1 = arm_encoding ::T1;
		constexpr auto T2 = arm_encoding ::T2;
		constexpr auto T3 = arm_encoding ::T3;
		constexpr auto T4 = arm_encoding ::T4;
		constexpr auto A1 = arm_encoding ::A1;
		constexpr auto A2 = arm_encoding ::A2;
	}

	// Bitfield aliases (because only u32 base type is used)
	template<uint I, uint N> using bf = bf_t<u32, I, N>;
	template<uint I, uint N> using sf = bf_t<s32, I, N>;
	template<u32 V, uint N = 32> using ff = ff_t<u32, V, N>;

	template<typename... T> using cf = cf_t<T...>;

	// Nullary op
	template<typename F>
	struct nullary
	{
		static inline u32 extract()
		{
			return F::extract(0);
		}
	};

	// Pair op: first field selection
	template<typename F = struct nopf>
	struct from_first
	{
		static inline u32 extract(u32 first, u32)
		{
			return F::extract(first);
		}
	};

	// Pair op: second field selection
	template<typename F = struct nopf>
	struct from_second
	{
		static inline u32 extract(u32, u32 second)
		{
			return F::extract(second);
		}
	};

	// No operation
	struct nopf
	{
		static inline u32 extract(u32 value)
		{
			return value;
		}
	};

	// Negate unary op
	template<typename F, uint N = 32>
	struct negatef : bf_base<u32, N>
	{
		static inline u32 extract(u32 value)
		{
			return (0u - F::extract(value)) & (~0u >> (32 - N));
		}
	};

	// NOT unary op
	template<typename F, uint N = F::bitsize>
	struct notf : bf_base<u32, N>
	{
		static inline u32 extract(u32 value)
		{
			return F::extract(value) ^ (~0u >> (32 - N));
		}
	};

	// XOR binary op
	template<typename F1, typename F2, uint N = F1::bitsize>
	struct xorf : bf_base<u32, N>
	{
		static inline u32 extract(u32 value)
		{
			return F1::extract(value) ^ F2::extract(value);
		}
	};

	// AND binary op
	template<typename F1, typename F2, uint N = F1::bitsize>
	struct andf : bf_base<u32, N>
	{
		static inline u32 extract(u32 value)
		{
			return F1::extract(value) & F2::extract(value);
		}
	};

	// OR binary op
	template<typename F1, typename F2, uint N = F1::bitsize>
	struct orf : bf_base<u32, N>
	{
		static inline u32 extract(u32 value)
		{
			return F1::extract(value) | F2::extract(value);
		}
	};

	// SHL binary op
	template<typename F1, typename F2, uint N = F1::bitsize>
	struct shlf : bf_base<u32, N>
	{
		static inline u32 extract(u32 value)
		{
			return F1::extract(value) << F2::extract(value);
		}
	};

	// EQ with constant binary op
	template<typename F1, uint C = 0>
	struct eqcf : bf_base<u32, 32>
	{
		static inline u32 extract(u32 value)
		{
			return F1::extract(value) == C;
		}
	};

	using imm12i38 = cf<bf<26, 1>, bf<12, 3>, bf<0, 8>>;

	// ThumbExpandImm(...)
	struct thumb_expand_imm
	{
		static constexpr uint bitsize()
		{
			return 32;
		}

		static u32 extract(u32 value)
		{
			const u32 imm12 = imm12i38::extract(value);

			if ((imm12 & 0xc00) >> 10)
			{
				const u32 x = (imm12 & 0x7f) | 0x80;
				const u32 s = (imm12 & 0xf80) >> 7;
				const u32 r = x >> s | x << (32 - s); // Rotate
				return r;
			}
			else
			{
				const u32 imm8 = imm12 & 0xff;
				switch ((imm12 & 0x300) >> 8)
				{
				case 0: return imm8;
				case 1: return imm8 << 16 | imm8;
				case 2: return imm8 << 24 | imm8 << 8;
				default: return imm8 << 24 | imm8 << 16 | imm8 << 8 | imm8;
				}
			}
		}
	};

	// ThumbExpandImm_C(..., carry) -> carry
	struct thumb_expand_imm_c
	{
		static u32 extract(u32 value, u32 carry_in)
		{
			const u32 imm12 = imm12i38::extract(value);

			if ((imm12 & 0xc00) >> 10)
			{
				const u32 x = (imm12 & 0x7f) | 0x80;
				const u32 s = (imm12 & 0xf80) >> 7;
				const u32 r = x >> s | x << (32 - s); // Rotate
				return (s ? r >> 31 : carry_in) & 1;
			}
			else
			{
				return carry_in & 1;
			}
		}
	};

	// ARMExpandImm(...)
	struct arm_expand_imm
	{
		static constexpr uint bitsize()
		{
			return 32;
		}

		static u32 extract(u32 value)
		{
			const u32 imm12 = bf<0, 12>::extract(value);

			const u32 x = imm12 & 0xff;
			const u32 s = (imm12 & 0xf00) >> 7;
			const u32 r = x >> s | x << (32 - s); // Rotate

			return r;
		}
	};

	// ARMExpandImm_C(..., carry) -> carry
	struct arm_expand_imm_c
	{
		static u32 extract(u32 value, u32 carry_in)
		{
			const u32 imm12 = bf<0, 12>::extract(value);

			const u32 x = imm12 & 0xff;
			const u32 s = (imm12 & 0xf00) >> 7;
			const u32 r = x >> s | x << (32 - s); // Rotate

			return (s ? r >> 31 : carry_in) & 1;
		}
	};

	// !InITBlock()
	struct not_in_it_block
	{
		static u32 extract(u32 value, u32 cond)
		{
			return cond != 0xf;
		}
	};

	enum SRType : u32
	{
		SRType_LSL,
		SRType_LSR,
		SRType_ASR,
		SRType_ROR,
		SRType_RRX,
	};

	template<typename T, typename N>
	struct decode_imm_shift_type
	{
		static u32 extract(u32 value)
		{
			const u32 type = T::extract(value);
			const u32 imm5 = N::extract(value);

			return type < SRType_ROR ? type : (imm5 ? SRType_ROR : SRType_RRX);
		}
	};

	template<typename T, typename N>
	struct decode_imm_shift_num
	{
		static u32 extract(u32 value)
		{
			const u32 type = T::extract(value);
			const u32 imm5 = N::extract(value);

			return imm5 || type == SRType_LSL ? imm5 : (type < SRType_ROR ? 32 : 1);
		}
	};

	using decode_imm_shift_type_thumb = decode_imm_shift_type<bf<4, 2>, cf<bf<12, 3>, bf<6, 2>>>;
	using decode_imm_shift_num_thumb = decode_imm_shift_num<bf<4, 2>, cf<bf<12, 3>, bf<6, 2>>>;
	using decode_imm_shift_type_arm = decode_imm_shift_type<bf<5, 2>, bf<7, 5>>;
	using decode_imm_shift_num_arm = decode_imm_shift_num<bf<5, 2>, bf<7, 5>>;

	template<arm_encoding> struct it; //

	template<> struct it<T1>
	{
		using mask = bf<0, 4>;
		using first = bf<4, 4>;

		using skip_if = eqcf<mask>; // Skip if mask==0
	};

	template<arm_encoding> struct hack; //

	template<> struct hack<T1>
	{
		using index = bf<0, 16>;

		using skip_if = void;
	};

	template<> struct hack<A1>
	{
		using index = cf<bf<8, 12>, bf<0, 4>>;

		using skip_if = void;
	};

	template<arm_encoding> struct mrc; //

	template<> struct mrc<T1>
	{
		using t = bf<12, 4>;
		using cp = bf<8, 4>;
		using opc1 = bf<21, 3>;
		using opc2 = bf<5, 3>;
		using cn = bf<16, 4>;
		using cm = bf<0, 4>;

		using skip_if = eqcf<bf<9, 3>, 5>;
	};

	template<> struct mrc<T2> : mrc<T1>
	{
		using skip_if = void;
	};

	template<> struct mrc<A1> : mrc<T1>
	{
		using skip_if = eqcf<bf<9, 3>, 5>;
	};

	template<> struct mrc<A2> : mrc<T1>
	{
		using skip_if = void;
	};

	template<arm_encoding> struct add_imm; //

	template<> struct add_imm<T1>
	{
		using d = bf<0, 3>;
		using n = bf<3, 3>;
		using set_flags = not_in_it_block;
		using imm32 = bf<6, 3>;

		using skip_if = void;
	};

	template<> struct add_imm<T2>
	{
		using d = bf<8, 3>;
		using n = d;
		using set_flags = not_in_it_block;
		using imm32 = bf<0, 8>;

		using skip_if = void;
	};

	template<> struct add_imm<T3>
	{
		using d = bf<8, 4>;
		using n = bf<16, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using imm32 = thumb_expand_imm;

		using skip_if = orf<andf<eqcf<d, 15>, bf<20, 1>>, eqcf<n, 13>>;
	};

	template<> struct add_imm<T4> : add_imm<T3>
	{
		using add_imm<T3>::n;
		using set_flags = from_first<ff<false, 1>>;
		using imm32 = imm12i38;

		using skip_if = orf<eqcf<n, 13>, eqcf<n, 15>>;
	};

	template<> struct add_imm<A1>
	{
		using d = bf<12, 4>;
		using n = bf<16, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using imm32 = arm_expand_imm;

		using skip_if = orf<orf<andf<eqcf<n, 15>, notf<bf<20, 1>>>, eqcf<n, 13>>, andf<eqcf<d, 15>, bf<20, 1>>>;
	};

	template<arm_encoding> struct add_reg; //

	template<> struct add_reg<T1>
	{
		using d = bf<0, 3>;
		using n = bf<3, 3>;
		using m = bf<6, 3>;
		using set_flags = not_in_it_block;
		using shift_t = ff<SRType_LSL>;
		using shift_n = ff<0u>;

		using skip_if = void;
	};

	template<> struct add_reg<T2>
	{
		using d = cf<bf<7, 1>, bf<0, 3>>;
		using n = d;
		using m = bf<3, 4>;
		using set_flags = from_first<ff<false, 1>>;
		using shift_t = ff<SRType_LSL>;
		using shift_n = ff<0u>;

		using skip_if = orf<eqcf<d, 13>, eqcf<m, 13>>;
	};

	template<> struct add_reg<T3>
	{
		using d = bf<8, 4>;
		using n = bf<16, 4>;
		using m = bf<0, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using shift_t = decode_imm_shift_type_thumb;
		using shift_n = decode_imm_shift_num_thumb;

		using skip_if = orf<andf<eqcf<d, 15>, bf<20, 1>>, eqcf<n, 13>>;
	};

	template<> struct add_reg<A1> : add_reg<T3>
	{
		using d = bf<12, 4>;
		using n = bf<16, 4>;
		using shift_t = decode_imm_shift_type_arm;
		using shift_n = decode_imm_shift_num_arm;

		using skip_if = orf<andf<eqcf<d, 15>, bf<20, 1>>, eqcf<n, 13>>;
	};

	template<arm_encoding> struct add_spi; //

	template<> struct add_spi<T1>
	{
		using d = bf<8, 3>;
		using set_flags = from_first<ff<false, 1>>;
		using imm32 = cf<bf<0, 8>, ff<0u, 2>>;

		using skip_if = void;
	};

	template<> struct add_spi<T2>
	{
		using d = ff<13u>;
		using set_flags = from_first<ff<false, 1>>;
		using imm32 = cf<bf<0, 7>, ff<0u, 2>>;

		using skip_if = void;
	};

	template<> struct add_spi<T3>
	{
		using d = bf<8, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using imm32 = thumb_expand_imm;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<> struct add_spi<T4>
	{
		using d = bf<8, 4>;
		using set_flags = from_first<ff<false, 1>>;
		using imm32 = imm12i38;

		using skip_if = void;
	};

	template<> struct add_spi<A1>
	{
		using d = bf<12, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using imm32 = arm_expand_imm;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<arm_encoding> struct add_spr; //

	template<> struct add_spr<T1>
	{
		using d = cf<bf<7, 1>, bf<0, 3>>;
		using m = d;
		using set_flags = from_first<ff<false, 1>>;
		using shift_t = ff<SRType_LSL>;
		using shift_n = ff<0u>;

		using skip_if = void;
	};

	template<> struct add_spr<T2>
	{
		using d = ff<13u>;
		using m = bf<3, 4>;
		using set_flags = from_first<ff<false, 1>>;
		using shift_t = ff<SRType_LSL>;
		using shift_n = ff<0u>;

		using skip_if = eqcf<m, 13>;
	};

	template<> struct add_spr<T3>
	{
		using d = bf<8, 4>;
		using m = bf<0, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using shift_t = decode_imm_shift_type_thumb;
		using shift_n = decode_imm_shift_num_thumb;

		using skip_if = void;
	};

	template<> struct add_spr<A1>
	{
		using d = bf<12, 4>;
		using m = bf<0, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using shift_t = decode_imm_shift_type_arm;
		using shift_n = decode_imm_shift_num_arm;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<arm_encoding> struct adc_imm; //

	template<> struct adc_imm<T1> : add_imm<T3>
	{
		using skip_if = void;
	};

	template<> struct adc_imm<A1> : add_imm<A1>
	{
		using skip_if = void;
	};

	template<arm_encoding> struct adc_reg; //

	template<> struct adc_reg<T1>
	{
		using d = bf<0, 3>;
		using n = d;
		using m = bf<3, 3>;
		using set_flags = not_in_it_block;
		using shift_t = ff<SRType_LSL>;
		using shift_n = ff<0u>;

		using skip_if = void;
	};

	template<> struct adc_reg<T2> : add_reg<T3>
	{
		using skip_if = void;
	};

	template<> struct adc_reg<A1> : add_reg<A1>
	{
		using add_reg<A1>::d;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<arm_encoding> struct adr; //

	template<> struct adr<T1>
	{
		using d = bf<8, 3>;
		using i = cf<bf<0, 8>, ff<0u, 2>>;

		using skip_if = void;
	};

	template<> struct adr<T2>
	{
		using d = bf<8, 4>;
		using i = negatef<imm12i38>;

		using skip_if = void;
	};

	template<> struct adr<T3>
	{
		using d = bf<8, 4>;
		using i = imm12i38;

		using skip_if = void;
	};

	template<> struct adr<A1>
	{
		using d = bf<12, 4>;
		using i = arm_expand_imm;

		using skip_if = void;
	};

	template<> struct adr<A2>
	{
		using d = bf<12, 4>;
		using i = negatef<arm_expand_imm>;

		using skip_if = void;
	};

	template<arm_encoding> struct and_imm; //

	template<> struct and_imm<T1>
	{
		using d = bf<8, 4>;
		using n = bf<16, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using imm32 = thumb_expand_imm;
		using carry = thumb_expand_imm_c;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<> struct and_imm<A1> : and_imm<T1>
	{
		using d = bf<12, 4>;
		using imm32 = arm_expand_imm;
		using carry = arm_expand_imm_c;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<arm_encoding> struct and_reg; //

	template<> struct and_reg<T1> : adc_reg<T1>
	{
		using skip_if = void;
	};

	template<> struct and_reg<T2> : adc_reg<T2>
	{
		using adc_reg<T2>::d;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<> struct and_reg<A1> : adc_reg<A1>
	{
		using adc_reg<A1>::d;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<arm_encoding> struct b; //

	template<> struct b<T1>
	{
		using imm32 = cf<sf<0, 8>, ff<0u, 1>>;
		using cond = from_first<bf<8, 4>>;

		using skip_if = eqcf<cond, 15>;
	};

	template<> struct b<T2>
	{
		using imm32 = cf<sf<0, 11>, ff<0u, 1>>;
		using cond = from_second<>;

		using skip_if = void;
	};

	template<> struct b<T3>
	{
		using imm32 = cf<sf<26, 1>, bf<11, 1>, bf<13, 1>, bf<16, 6>, bf<0, 11>, ff<0u, 1>>;
		using cond = from_first<bf<22, 4>>;

		using skip_if = eqcf<bf<23, 3>, 7>;
	};

	template<> struct b<T4>
	{
		using imm32 = cf<sf<26, 1>, notf<xorf<bf<13, 1>, bf<26, 1>>>, notf<xorf<bf<11, 1>, bf<26, 1>>>, bf<16, 10>, bf<0, 11>, ff<0u, 1>>;
		using cond = from_second<>;

		using skip_if = void;
	};

	template<> struct b<A1>
	{
		using imm32 = cf<sf<0, 24>, ff<0u, 2>>;
		using cond = from_second<>;

		using skip_if = void;
	};

	template<arm_encoding> struct bic_imm; //

	template<> struct bic_imm<T1> : and_imm<T1>
	{
		using skip_if = void;
	};

	template<> struct bic_imm<A1> : and_imm<A1>
	{
		using and_imm<A1>::d;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<arm_encoding> struct bic_reg; //

	template<> struct bic_reg<T1> : adc_reg<T1>
	{
		using skip_if = void;
	};

	template<> struct bic_reg<T2> : adc_reg<T2>
	{
		using skip_if = void;
	};

	template<> struct bic_reg<A1> : adc_reg<A1>
	{
		using adc_reg<A1>::d;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<arm_encoding> struct bl; //

	template<> struct bl<T1>
	{
		using imm32 = b<T4>::imm32;
		using to_arm = nullary<ff<false, 1>>;

		using skip_if = void;
	};

	template<> struct bl<T2>
	{
		using imm32 = cf<sf<26, 1>, notf<xorf<bf<13, 1>, bf<26, 1>>>, notf<xorf<bf<11, 1>, bf<26, 1>>>, bf<16, 10>, bf<1, 10>, ff<0u, 2>>;
		using to_arm = nullary<ff<true, 1>>;

		using skip_if = void;
	};

	template<> struct bl<A1>
	{
		using imm32 = b<A1>::imm32;
		using to_arm = nullary<ff<true, 1>>;

		using skip_if = void;
	};

	template<> struct bl<A2>
	{
		using imm32 = cf<sf<0, 24>, bf<24, 1>, ff<0u, 1>>;
		using to_arm = nullary<ff<false, 1>>;

		using skip_if = void;
	};

	template<arm_encoding type> struct blx; //

	template<> struct blx<T1>
	{
		using m = bf<3, 4>;

		using skip_if = void;
	};

	template<> struct blx<A1>
	{
		using m = bf<0, 4>;

		using skip_if = void;
	};

	template<arm_encoding> struct bx; //

	template<> struct bx<T1> : blx<T1>
	{
		using skip_if = void;
	};

	template<> struct bx<A1> : blx<A1>
	{
		using skip_if = void;
	};

	template<arm_encoding> struct cb_z; //

	template<> struct cb_z<T1>
	{
		using n = bf<0, 3>;
		using imm32 = cf<bf<9, 1>, bf<3, 5>, ff<0u, 1>>;
		using nonzero = bf<11, 1>;

		using skip_if = void;
	};

	template<arm_encoding> struct clz; //

	template<> struct clz<T1>
	{
		using d = bf<8, 4>;
		using m = bf<0, 4>;

		using skip_if = void;
	};

	template<> struct clz<A1>
	{
		using d = bf<12, 4>;
		using m = bf<0, 4>;

		using skip_if = void;
	};

	template<arm_encoding> struct cmp_imm; //

	template<> struct cmp_imm<T1>
	{
		using n = bf<8, 3>;
		using imm32 = bf<0, 8>;

		using skip_if = void;
	};

	template<> struct cmp_imm<T2>
	{
		using n = bf<16, 4>;
		using imm32 = thumb_expand_imm;

		using skip_if = void;
	};

	template<> struct cmp_imm<A1>
	{
		using n = bf<16, 4>;
		using imm32 = arm_expand_imm;

		using skip_if = void;
	};

	template<arm_encoding> struct cmp_reg; //

	template<> struct cmp_reg<T1>
	{
		using n = bf<0, 3>;
		using m = bf<3, 3>;
		using shift_t = ff<SRType_LSL>;
		using shift_n = ff<0u>;

		using skip_if = void;
	};

	template<> struct cmp_reg<T2> : cmp_reg<T1>
	{
		using n = cf<bf<7, 1>, bf<0, 3>>;
		using m = bf<3, 4>;

		using skip_if = void;
	};

	template<> struct cmp_reg<T3>
	{
		using n = bf<16, 4>;
		using m = bf<0, 4>;
		using shift_t = decode_imm_shift_type_thumb;
		using shift_n = decode_imm_shift_num_thumb;

		using skip_if = void;
	};

	template<> struct cmp_reg<A1> : cmp_reg<T3>
	{
		using shift_t = decode_imm_shift_type_arm;
		using shift_n = decode_imm_shift_num_arm;

		using skip_if = void;
	};

	template<arm_encoding> struct eor_imm; //

	template<> struct eor_imm<T1> : and_imm<T1>
	{
		using and_imm<T1>::d;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<> struct eor_imm<A1> : and_imm<A1>
	{
		using and_imm<A1>::d;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<arm_encoding> struct eor_reg; //

	template<> struct eor_reg<T1> : adc_reg<T1>
	{
		using skip_if = void;
	};

	template<> struct eor_reg<T2> : adc_reg<T2>
	{
		using adc_reg<T2>::d;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<> struct eor_reg<A1> : adc_reg<A1>
	{
		using adc_reg<A1>::d;

		using skip_if = andf<eqcf<d, 15>, bf<20, 1>>;
	};

	template<arm_encoding> struct ldm; //

	template<> struct ldm<T1>
	{
		using n = bf<8, 3>;
		using registers = bf<0, 8>;
		using wback = ff<true, 1>; // TODO

		using skip_if = void;
	};

	template<> struct ldm<T2>
	{
		using n = bf<16, 4>;
		using registers = andf<bf<0, 16>, ff<0xdfffu, 16>>; // TODO
		using wback = bf<21, 1>;

		using skip_if = andf<wback, eqcf<n, 13>>;
	};

	template<> struct ldm<A1> : ldm<T2>
	{
		using registers = bf<0, 16>;

		using skip_if = class TODO;
	};

	template<arm_encoding> struct ldr_imm; //

	template<> struct ldr_imm<T1>
	{
		using t = bf<0, 3>;
		using n = bf<3, 3>;
		using imm32 = cf<bf<6, 5>, ff<0u, 2>>;
		using index = ff<true, 1>;
		using add = ff<true, 1>;
		using wback = ff<false, 1>;

		using skip_if = void;
	};

	template<> struct ldr_imm<T2> : ldr_imm<T1>
	{
		using t = bf<8, 3>;
		using n = ff<13u>;
		using imm32 = cf<bf<0, 8>, ff<0u, 2>>;

		using skip_if = void;
	};

	template<> struct ldr_imm<T3> : ldr_imm<T1>
	{
		using t = bf<12, 4>;
		using n = bf<16, 4>;
		using imm32 = bf<0, 12>;

		using skip_if = eqcf<n, 15>;
	};

	template<> struct ldr_imm<T4>
	{
		using t = bf<12, 4>;
		using n = bf<16, 4>;
		using imm32 = bf<0, 8>;
		using index = bf<10, 1>;
		using add = bf<9, 1>;
		using wback = bf<8, 1>;

		using skip_if = orf<eqcf<n, 15>, orf<eqcf<bf<8, 3>, 6>, andf<eqcf<n, 13>, eqcf<bf<0, 11>, 512 + 256 + 4>>>>;
	};

	template<> struct ldr_imm<A1> : ldr_imm<T4>
	{
		using imm32 = bf<0, 12>;
		using index = bf<24, 1>;
		using add = bf<23, 1>;
		using wback = orf<notf<bf<24, 1>>, bf<21, 1>>;

		using skip_if = class TODO;
	};

	template<arm_encoding> struct ldr_lit; //

	template<> struct ldr_lit<T1>
	{
		using t = bf<8, 3>;
		using imm32 = cf<bf<0, 8>, ff<0u, 2>>;
		using add = ff<true, 1>;

		using skip_if = void;
	};

	template<> struct ldr_lit<T2>
	{
		using t = bf<12, 4>;
		using imm32 = bf<0, 12>;
		using add = bf<23, 1>;

		using skip_if = void;
	};

	template<> struct ldr_lit<A1> : ldr_lit<T2>
	{
		using skip_if = void;
	};

	template<arm_encoding> struct ldr_reg; //

	template<> struct ldr_reg<T1>
	{
		using t = bf<0, 3>;
		using n = bf<3, 3>;
		using m = bf<6, 3>;
		using index = ff<true, 1>;
		using add = ff<true, 1>;
		using wback = ff<false, 1>;
		using shift_t = ff<SRType_LSL>;
		using shift_n = ff<0u>;

		using skip_if = void;
	};

	template<> struct ldr_reg<T2> : ldr_reg<T1>
	{
		using t = bf<12, 4>;
		using n = bf<16, 4>;
		using m = bf<0, 4>;
		using shift_n = bf<4, 2>;

		using skip_if = eqcf<n, 15>;
	};

	template<> struct ldr_reg<A1> : ldr_reg<T2>
	{
		using index = bf<24, 1>;
		using add = bf<23, 1>;
		using wback = orf<notf<bf<24, 1>>, bf<21, 1>>;
		using shift_t = decode_imm_shift_type_arm;
		using shift_n = decode_imm_shift_num_arm;

		using skip_if = class TODO;
	};

	template<arm_encoding> struct ldrb_imm; //

	template<> struct ldrb_imm<T1> : ldr_imm<T1>
	{
		using imm32 = bf<6, 5>;

		using skip_if = void;
	};

	template<> struct ldrb_imm<T2> : ldr_imm<T3>
	{
		using ldr_imm<T3>::t;
		using ldr_imm<T3>::n;

		using skip_if = orf<eqcf<t, 15>, eqcf<n, 15>>;
	};

	template<> struct ldrb_imm<T3> : ldr_imm<T4>
	{
		using ldr_imm<T4>::t;
		using ldr_imm<T4>::n;

		using skip_if = orf<andf<eqcf<t, 15>, eqcf<bf<8, 3>, 4>>, orf<eqcf<n, 15>, eqcf<bf<8, 3>, 6>>>;
	};

	template<> struct ldrb_imm<A1> : ldr_imm<A1>
	{
		using skip_if = class TODO;
	};

	template<arm_encoding> struct ldrb_reg; //

	template<> struct ldrb_reg<T1> : ldr_reg<T1>
	{
		using skip_if = void;
	};

	template<> struct ldrb_reg<T2> : ldr_reg<T2>
	{
		using ldr_reg<T2>::t;
		using ldr_reg<T2>::n;

		using skip_if = orf<eqcf<t, 15>, eqcf<n, 15>>;
	};

	template<> struct ldrb_reg<A1> : ldr_reg<A1>
	{
		using skip_if = class TODO;
	};

	template<arm_encoding> struct ldrd_imm; //

	template<> struct ldrd_imm<T1>
	{
		using t = bf<12, 4>;
		using t2 = bf<8, 4>;
		using n = bf<16, 4>;
		using imm32 = cf<bf<0, 8>, ff<0u, 2>>;
		using index = bf<24, 1>;
		using add = bf<23, 1>;
		using wback = bf<21, 1>;
	};

	template<> struct ldrd_imm<A1> : ldrd_imm<T1>
	{
		using t2 = orf<bf<12, 4>, ff<1u, 4>>; // t+1 for even t
		using imm32 = cf<bf<8, 4>, bf<0, 4>>;
		using wback = orf<notf<bf<24, 1>>, bf<21, 1>>;
	};

	template<arm_encoding> struct ldrd_lit; //

	template<> struct ldrd_lit<T1>
	{
		using t = bf<12, 4>;
		using t2 = bf<8, 4>;
		using imm32 = cf<bf<0, 8>, ff<0u, 2>>;
		using add = bf<23, 1>;
	};

	template<> struct ldrd_lit<A1> : ldrd_lit<T1>
	{
		using t2 = orf<bf<12, 4>, ff<1u, 4>>; // t+1 for even t
		using imm32 = cf<bf<8, 4>, bf<0, 4>>;
	};

	template<arm_encoding> struct ldrh_imm; //

	template<> struct ldrh_imm<T1> : ldr_imm<T1>
	{
		using imm32 = cf<bf<6, 5>, ff<0u, 1>>;
	};

	template<> struct ldrh_imm<T2> : ldr_imm<T3> {};
	template<> struct ldrh_imm<T3> : ldr_imm<T4> {};

	template<> struct ldrh_imm<A1> : ldr_imm<A1>
	{
		using imm32 = cf<bf<8, 4>, bf<0, 4>>;
	};

	template<arm_encoding> struct ldrh_reg; //

	template<> struct ldrh_reg<T1> : ldr_reg<T1> {};
	template<> struct ldrh_reg<T2> : ldr_reg<T2> {};

	template<> struct ldrh_reg<A1> : ldr_reg<A1>
	{
		using shift_t = ff<SRType_LSL>; // ???
		using shift_n = ff<0u>;
	};

	template<arm_encoding> struct ldrsb_imm; //

	template<> struct ldrsb_imm<T1> : ldr_imm<T3> {};
	template<> struct ldrsb_imm<T2> : ldr_imm<T4> {};
	template<> struct ldrsb_imm<A1> : ldrh_imm<A1> {};

	template<arm_encoding> struct ldrex; //

	template<> struct ldrex<T1>
	{
		using t = bf<12, 4>;
		using n = bf<16, 4>;
		using imm32 = cf<bf<0, 8>, ff<0u, 2>>;
	};

	template<> struct ldrex<A1> : ldrex<T1>
	{
		using imm32 = ff<0u>; // Zero offset
	};

	template<arm_encoding, SRType> struct _shift_imm; //

	template<SRType type> struct _shift_imm<T1, type>
	{
		using d = bf<0, 3>;
		using m = bf<3, 3>;
		using set_flags = not_in_it_block;
		using shift_n = decode_imm_shift_num<ff<type>, bf<6, 5>>;
	};

	template<SRType type> struct _shift_imm<T2, type>
	{
		using d = bf<8, 4>;
		using m = bf<0, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using shift_n = decode_imm_shift_num<ff<type>, cf<bf<12, 3>, bf<6, 2>>>;
	};

	template<SRType type> struct _shift_imm<A1, type> : _shift_imm<T2, type>
	{
		using d = bf<12, 4>;
		using shift_n = decode_imm_shift_num<ff<type>, bf<7, 5>>;
	};

	template<arm_encoding type> struct lsl_imm : _shift_imm<type, SRType_LSL> {}; //
	template<arm_encoding type> struct lsr_imm : _shift_imm<type, SRType_LSR> {}; //
	template<arm_encoding type> struct asr_imm : _shift_imm<type, SRType_ASR> {}; //

	template<arm_encoding> struct lsl_reg; //

	template<> struct lsl_reg<T1>
	{
		using d = bf<0, 3>;
		using n = d;
		using m = bf<3, 3>;
		using set_flags = not_in_it_block;
	};

	template<> struct lsl_reg<T2>
	{
		using d = bf<8, 4>;
		using n = bf<16, 4>;
		using m = bf<0, 4>;
		using set_flags = from_first<bf<20, 1>>;
	};

	template<> struct lsl_reg<A1> : lsl_reg<T2>
	{
		using d = bf<12, 4>;
		using n = bf<0, 4>;
		using m = bf<8, 4>;
	};

	template<arm_encoding> struct lsr_reg; //

	template<> struct lsr_reg<T1> : lsl_reg<T1> {};
	template<> struct lsr_reg<T2> : lsl_reg<T2> {};
	template<> struct lsr_reg<A1> : lsl_reg<A1> {};

	//template<arm_encoding> struct asr_reg; //

	//template<> struct asr_reg<T1> : lsl_reg<T1> {};
	//template<> struct asr_reg<T2> : lsl_reg<T2> {};
	//template<> struct asr_reg<A1> : lsl_reg<A1> {};

	template<arm_encoding> struct mov_imm; //

	template<> struct mov_imm<T1>
	{
		using d = bf<8, 3>;
		using set_flags = not_in_it_block;
		using imm32 = bf<0, 8>;
		using carry = from_second<>;
	};

	template<> struct mov_imm<T2>
	{
		using d = bf<8, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using imm32 = thumb_expand_imm;
		using carry = thumb_expand_imm_c;
	};

	template<> struct mov_imm<T3>
	{
		using d = bf<8, 4>;
		using set_flags = from_first<ff<false, 1>>;
		using imm32 = cf<bf<16, 4>, imm12i38>;
		using carry = from_second<>;
	};

	template<> struct mov_imm<A1>
	{
		using d = bf<12, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using imm32 = arm_expand_imm;
		using carry = arm_expand_imm_c;
	};

	template<> struct mov_imm<A2>
	{
		using d = bf<12, 4>;
		using set_flags = from_first<ff<false, 1>>;
		using imm32 = cf<bf<16, 4>, bf<0, 12>>;
		using carry = from_second<>;
	};

	template<arm_encoding> struct mov_reg; //

	template<> struct mov_reg<T1>
	{
		using d = cf<bf<7, 1>, bf<0, 3>>;
		using m = bf<3, 4>;
		using set_flags = from_first<ff<false, 1>>;
	};

	template<> struct mov_reg<T2>
	{
		using d = bf<0, 3>;
		using m = bf<3, 3>;
		using set_flags = from_first<ff<true, 1>>;
	};

	template<> struct mov_reg<T3>
	{
		using d = bf<8, 4>;
		using m = bf<0, 4>;
		using set_flags = from_first<bf<20, 1>>;
	};

	template<> struct mov_reg<A1> : mov_reg<T3>
	{
		using d = bf<12, 4>;
	};

	template<arm_encoding> struct movt; //

	template<> struct movt<T1>
	{
		using d = bf<8, 4>;
		using imm16 = cf<bf<16, 4>, imm12i38>;
	};

	template<> struct movt<A1>
	{
		using d = bf<12, 4>;
		using imm16 = cf<bf<16, 4>, bf<0, 12>>;
	};

	template<arm_encoding> struct mul; //

	template<> struct mul<T1>
	{
		using d = bf<0, 3>;
		using n = bf<3, 3>;
		using m = d;
		using set_flags = not_in_it_block;
	};

	template<> struct mul<T2>
	{
		using d = bf<8, 4>;
		using n = bf<16, 4>;
		using m = bf<0, 4>;
		using set_flags = from_first<ff<false, 1>>;
	};

	template<> struct mul<A1>
	{
		using d = bf<16, 4>;
		using n = bf<0, 4>;
		using m = bf<8, 4>;
		using set_flags = from_first<bf<20, 1>>;
	};

	template<arm_encoding> struct mvn_imm; //

	template<> struct mvn_imm<T1> : mov_imm<T2> {};
	template<> struct mvn_imm<A1> : mov_imm<A1> {};

	template<arm_encoding> struct mvn_reg; //

	template<> struct mvn_reg<T1>
	{
		using d = bf<0, 3>;
		using m = bf<3, 3>;
		using set_flags = not_in_it_block;
		using shift_t = ff<SRType_LSL>;
		using shift_n = ff<0u>;
	};

	template<> struct mvn_reg<T2>
	{
		using d = bf<8, 4>;
		using m = bf<0, 4>;
		using set_flags = from_first<bf<20, 1>>;
		using shift_t = decode_imm_shift_type_thumb;
		using shift_n = decode_imm_shift_num_thumb;
	};

	template<> struct mvn_reg<A1> : mvn_reg<T2>
	{
		using d = bf<12, 4>;
		using shift_t = decode_imm_shift_type_arm;
		using shift_n = decode_imm_shift_num_arm;
	};

	template<arm_encoding> struct orr_imm; //

	template<> struct orr_imm<T1> : and_imm<T1> {};
	template<> struct orr_imm<A1> : and_imm<A1> {};

	template<arm_encoding> struct orr_reg; //

	template<> struct orr_reg<T1> : adc_reg<T1> {};
	template<> struct orr_reg<T2> : adc_reg<T2> {};
	template<> struct orr_reg<A1> : adc_reg<A1> {};

	template<arm_encoding> struct pop; //

	template<> struct pop<T1>
	{
		using registers = cf<bf<8, 1>, ff<0u, 7>, bf<0, 8>>;
	};

	template<> struct pop<T2>
	{
		using registers = andf<bf<0, 16>, ff<0xdfffu, 16>>;
	};

	template<> struct pop<T3>
	{
		using registers = shlf<ff<1u>, bf<12, 4>>; // 1 << Rt
	};

	template<> struct pop<A1>
	{
		using registers = bf<0, 16>;
	};

	template<> struct pop<A2> : pop<T3>
	{
	};

	template<arm_encoding> struct push; //

	template<> struct push<T1>
	{
		using registers = cf<bf<8, 1>, ff<0u, 6>, bf<0, 8>>;
	};

	template<> struct push<T2>
	{
		using registers = andf<bf<0, 16>, ff<0x5fffu, 16>>;
	};

	template<> struct push<T3> : pop<T3> {};
	template<> struct push<A1> : pop<A1> {};
	template<> struct push<A2> : pop<A2> {};

	template<arm_encoding> struct rev; //

	template<> struct rev<T1>
	{
		using d = bf<0, 3>;
		using m = bf<3, 3>;
	};

	template<> struct rev<T2> : clz<T1> {};
	template<> struct rev<A1> : clz<A1> {};

	template<arm_encoding> struct ror_imm; //

	template<> struct ror_imm<T1> : _shift_imm<T2, SRType_ROR> {};
	template<> struct ror_imm<A1> : _shift_imm<A1, SRType_ROR> {};

	template<arm_encoding> struct ror_reg; //

	template<> struct ror_reg<T1> : lsl_reg<T1> {};
	template<> struct ror_reg<T2> : lsl_reg<T2> {};
	template<> struct ror_reg<A1> : lsl_reg<A1> {};

	template<arm_encoding> struct rsb_imm; //

	template<> struct rsb_imm<T1> : add_imm<T1>
	{
		using imm32 = ff<0u>;
	};

	template<> struct rsb_imm<T2> : add_imm<T3> {};
	template<> struct rsb_imm<A1> : add_imm<A1> {};

	template<arm_encoding> struct stm; //

	template<> struct stm<T1> : ldm<T1>
	{
		using wback = ff<true, 1>;
	};

	template<> struct stm<T2> : ldm<T2>
	{
		using registers = andf<bf<0, 16>, ff<0x5fffu, 16>>; // TODO
	};

	template<> struct stm<A1> : ldm<A1> {};

	template<arm_encoding> struct str_imm; //

	template<> struct str_imm<T1> : ldr_imm<T1> {};
	template<> struct str_imm<T2> : ldr_imm<T2> {};
	template<> struct str_imm<T3> : ldr_imm<T3> {};
	template<> struct str_imm<T4> : ldr_imm<T4> {};
	template<> struct str_imm<A1> : ldr_imm<A1> {};

	template<arm_encoding> struct str_reg; //

	template<> struct str_reg<T1> : ldr_reg<T1> {};
	template<> struct str_reg<T2> : ldr_reg<T2> {};
	template<> struct str_reg<A1> : ldr_reg<A1> {};

	template<arm_encoding> struct strb_imm; //

	template<> struct strb_imm<T1> : ldrb_imm<T1> {};
	template<> struct strb_imm<T2> : ldrb_imm<T2> {};
	template<> struct strb_imm<T3> : ldrb_imm<T3> {};
	template<> struct strb_imm<A1> : ldrb_imm<A1> {};

	template<arm_encoding> struct strb_reg; //

	template<> struct strb_reg<T1> : ldrb_reg<T1> {};
	template<> struct strb_reg<T2> : ldrb_reg<T2> {};
	template<> struct strb_reg<A1> : ldrb_reg<A1> {};

	template<arm_encoding> struct strd_imm; //

	template<> struct strd_imm<T1> : ldrd_imm<T1> {};
	template<> struct strd_imm<A1> : ldrd_imm<A1> {};

	template<arm_encoding> struct strh_imm; //

	template<> struct strh_imm<T1> : ldrh_imm<T1> {};
	template<> struct strh_imm<T2> : ldrh_imm<T2> {};
	template<> struct strh_imm<T3> : ldrh_imm<T3> {};
	template<> struct strh_imm<A1> : ldrh_imm<A1> {};

	template<arm_encoding> struct strh_reg; //

	template<> struct strh_reg<T1> : ldrh_reg<T1> {};
	template<> struct strh_reg<T2> : ldrh_reg<T2> {};
	template<> struct strh_reg<A1> : ldrh_reg<A1> {};

	template<arm_encoding> struct strex; //

	template<> struct strex<T1>
	{
		using d = bf<8, 4>;
		using t = bf<12, 4>;
		using n = bf<16, 4>;
		using imm32 = cf<bf<0, 8>, ff<0u, 2>>;
	};

	template<> struct strex<A1>
	{
		using d = bf<12, 4>;
		using t = bf<0, 4>;
		using n = bf<16, 4>;
		using imm32 = ff<0u>; // Zero offset
	};

	template<arm_encoding> struct sub_imm; //

	template<> struct sub_imm<T1> : add_imm<T1> {};
	template<> struct sub_imm<T2> : add_imm<T2> {};
	template<> struct sub_imm<T3> : add_imm<T3> {};
	template<> struct sub_imm<T4> : add_imm<T4> {};
	template<> struct sub_imm<A1> : add_imm<A1> {};

	template<arm_encoding> struct sub_reg; //

	template<> struct sub_reg<T1> : add_reg<T1> {};
	template<> struct sub_reg<T2> : add_reg<T3> {};
	template<> struct sub_reg<A1> : add_reg<A1> {};

	template<arm_encoding> struct sub_spi; //

	template<> struct sub_spi<T1> : add_spi<T2> {};
	template<> struct sub_spi<T2> : add_spi<T3> {};
	template<> struct sub_spi<T3> : add_spi<T4> {};
	template<> struct sub_spi<A1> : add_spi<A1> {};

	template<arm_encoding> struct tst_imm; //

	template<> struct tst_imm<T1>
	{
		using n = bf<16, 4>;
		using imm32 = thumb_expand_imm;
		using carry = thumb_expand_imm_c;
	};

	template<> struct tst_imm<A1>
	{
		using n = bf<16, 4>;
		using imm32 = arm_expand_imm;
		using carry = arm_expand_imm_c;
	};

	template<arm_encoding> struct umull; //

	template<> struct umull<T1>
	{
		using d0 = bf<12, 4>;
		using d1 = bf<8, 4>;
		using n = bf<16, 4>;
		using m = bf<0, 4>;
		using set_flags = from_first<ff<false, 1>>;
	};

	template<> struct umull<A1>
	{
		using d0 = bf<12, 4>;
		using d1 = bf<16, 4>;
		using n = bf<0, 4>;
		using m = bf<8, 4>;
		using set_flags = from_first<bf<20, 1>>;
	};

	template<arm_encoding> struct uxtb; //

	template<> struct uxtb<T1>
	{
		using d = bf<0, 3>;
		using m = bf<3, 3>;
		using rotation = ff<0u, 5>;
	};

	template<> struct uxtb<T2>
	{
		using d = bf<8, 4>;
		using m = bf<0, 4>;
		using rotation = cf<bf<4, 2>, ff<0u, 3>>;
	};

	template<> struct uxtb<A1>
	{
		using d = bf<12, 4>;
		using m = bf<0, 4>;
		using rotation = cf<bf<10, 2>, ff<0u, 3>>;
	};
}

// TODO
#define SKIP_IF(cond) [](u32 c) -> bool { return cond; }
#define BF(start, end) ((c << (31 - (end))) >> ((start) + 31 - (end)))
#define BT(pos) ((c >> (pos)) & 1)

// ARMv7 decoder object. D provides function templates. T is function pointer type returned.
template<typename D, typename T = decltype(&D::UNK)>
class arm_decoder
{
	// Fast lookup table for 2-byte Thumb instructions
	std::array<T, 0x10000> m_op16_table{};

	// Fix for std::initializer_list (???)
	static inline T fix(T ptr)
	{
		return ptr;
	}

	struct instruction_info
	{
		u32 mask;
		u32 code;
		T pointer;
		bool(*skip)(u32 code); // TODO (if not nullptr, must be specified yet)

		bool match(u32 op) const
		{
			return (op & mask) == code && (!skip || !skip(op));
		}
	};

	// Lookup table for 4-byte Thumb instructions (points to the best appropriate position in sorted vector)
	std::array<typename std::vector<instruction_info>::const_iterator, 0x10000> m_op32_table{};

	std::vector<instruction_info> m_op16_list;
	std::vector<instruction_info> m_op32_list;
	std::vector<instruction_info> m_arm_list;

public:
	arm_decoder()
	{
		using namespace arm_code::arm_encoding_alias;

		m_op16_list.assign(
		{
			{ 0xffc0, 0x4140, fix(&D:: template ADC_REG<T1>), nullptr },
			{ 0xfe00, 0x1c00, fix(&D:: template ADD_IMM<T1>), nullptr },
			{ 0xf800, 0x3000, fix(&D:: template ADD_IMM<T2>), nullptr },
			{ 0xfe00, 0x1800, fix(&D:: template ADD_REG<T1>), nullptr },
			{ 0xff00, 0x4400, fix(&D:: template ADD_REG<T2>), SKIP_IF((c & 0x87) == 0x85 || BF(3, 6) == 13) },
			{ 0xf800, 0xa800, fix(&D:: template ADD_SPI<T1>), nullptr },
			{ 0xff80, 0xb000, fix(&D:: template ADD_SPI<T2>), nullptr },
			{ 0xff78, 0x4468, fix(&D:: template ADD_SPR<T1>), nullptr },
			{ 0xff87, 0x4485, fix(&D:: template ADD_SPR<T2>), SKIP_IF(BF(3, 6) == 13) },
			{ 0xf800, 0xa000, fix(&D:: template ADR<T1>), nullptr },
			{ 0xffc0, 0x4000, fix(&D:: template AND_REG<T1>), nullptr },
			{ 0xf800, 0x1000, fix(&D:: template ASR_IMM<T1>), nullptr },
			{ 0xffc0, 0x4100, fix(&D:: template ASR_REG<T1>), nullptr },
			{ 0xf000, 0xd000, fix(&D:: template B<T1>), SKIP_IF(BF(9, 11) == 0x7) },
			{ 0xf800, 0xe000, fix(&D:: template B<T2>), nullptr },
			{ 0xffc0, 0x4380, fix(&D:: template BIC_REG<T1>), nullptr },
			{ 0xff00, 0xbe00, fix(&D:: template BKPT<T1>), nullptr },
			{ 0xff80, 0x4780, fix(&D:: template BLX<T1>), nullptr },
			{ 0xff87, 0x4700, fix(&D:: template BX<T1>), nullptr },
			{ 0xf500, 0xb100, fix(&D:: template CB_Z<T1>), nullptr },
			{ 0xffc0, 0x42c0, fix(&D:: template CMN_REG<T1>), nullptr },
			{ 0xf800, 0x2800, fix(&D:: template CMP_IMM<T1>), nullptr },
			{ 0xffc0, 0x4280, fix(&D:: template CMP_REG<T1>), nullptr },
			{ 0xff00, 0x4500, fix(&D:: template CMP_REG<T2>), nullptr },
			{ 0xffc0, 0x4040, fix(&D:: template EOR_REG<T1>), nullptr },
			{ 0xff00, 0xbf00, fix(&D:: template IT<T1>), SKIP_IF(BF(0, 3) == 0) },
			{ 0xf800, 0xc800, fix(&D:: template LDM<T1>), nullptr },
			{ 0xf800, 0x6800, fix(&D:: template LDR_IMM<T1>), nullptr },
			{ 0xf800, 0x9800, fix(&D:: template LDR_IMM<T2>), nullptr },
			{ 0xf800, 0x4800, fix(&D:: template LDR_LIT<T1>), nullptr },
			{ 0xfe00, 0x5800, fix(&D:: template LDR_REG<T1>), nullptr },
			{ 0xf800, 0x7800, fix(&D:: template LDRB_IMM<T1>) },
			{ 0xfe00, 0x5c00, fix(&D:: template LDRB_REG<T1>) },
			{ 0xf800, 0x8800, fix(&D:: template LDRH_IMM<T1>) },
			{ 0xfe00, 0x5a00, fix(&D:: template LDRH_REG<T1>) },
			{ 0xfe00, 0x5600, fix(&D:: template LDRSB_REG<T1>) },
			{ 0xfe00, 0x5e00, fix(&D:: template LDRSH_REG<T1>) },
			{ 0xf800, 0x0000, fix(&D:: template LSL_IMM<T1>) },
			{ 0xffc0, 0x4080, fix(&D:: template LSL_REG<T1>) },
			{ 0xf800, 0x0800, fix(&D:: template LSR_IMM<T1>) },
			{ 0xffc0, 0x40c0, fix(&D:: template LSR_REG<T1>) },
			{ 0xf800, 0x2000, fix(&D:: template MOV_IMM<T1>) },
			{ 0xff00, 0x4600, fix(&D:: template MOV_REG<T1>) },
			{ 0xffc0, 0x0000, fix(&D:: template MOV_REG<T2>) },
			{ 0xffc0, 0x4340, fix(&D:: template MUL<T1>) },
			{ 0xffc0, 0x43c0, fix(&D:: template MVN_REG<T1>) },
			{ 0xffff, 0xbf00, fix(&D:: template NOP<T1>) },
			{ 0xffc0, 0x4300, fix(&D:: template ORR_REG<T1>) },
			{ 0xfe00, 0xbc00, fix(&D:: template POP<T1>) },
			{ 0xfe00, 0xb400, fix(&D:: template PUSH<T1>) },
			{ 0xffc0, 0xba00, fix(&D:: template REV<T1>) },
			{ 0xffc0, 0xba40, fix(&D:: template REV16<T1>) },
			{ 0xffc0, 0xbac0, fix(&D:: template REVSH<T1>) },
			{ 0xffc0, 0x41c0, fix(&D:: template ROR_REG<T1>) },
			{ 0xffc0, 0x4240, fix(&D:: template RSB_IMM<T1>) },
			{ 0xffc0, 0x4180, fix(&D:: template SBC_REG<T1>) },
			{ 0xf800, 0xc000, fix(&D:: template STM<T1>) },
			{ 0xf800, 0x6000, fix(&D:: template STR_IMM<T1>) },
			{ 0xf800, 0x9000, fix(&D:: template STR_IMM<T2>) },
			{ 0xfe00, 0x5000, fix(&D:: template STR_REG<T1>) },
			{ 0xf800, 0x7000, fix(&D:: template STRB_IMM<T1>) },
			{ 0xfe00, 0x5400, fix(&D:: template STRB_REG<T1>) },
			{ 0xf800, 0x8000, fix(&D:: template STRH_IMM<T1>) },
			{ 0xfe00, 0x5200, fix(&D:: template STRH_REG<T1>) },
			{ 0xfe00, 0x1e00, fix(&D:: template SUB_IMM<T1>) },
			{ 0xf800, 0x3800, fix(&D:: template SUB_IMM<T2>) },
			{ 0xfe00, 0x1a00, fix(&D:: template SUB_REG<T1>) },
			{ 0xff80, 0xb080, fix(&D:: template SUB_SPI<T1>) },
			{ 0xff00, 0xdf00, fix(&D:: template SVC<T1>) },
			{ 0xffc0, 0xb240, fix(&D:: template SXTB<T1>) },
			{ 0xffc0, 0xb200, fix(&D:: template SXTH<T1>) },
			{ 0xffc0, 0x4200, fix(&D:: template TST_REG<T1>) },
			{ 0xffc0, 0xb2c0, fix(&D:: template UXTB<T1>) },
			{ 0xffc0, 0xb280, fix(&D:: template UXTH<T1>) },
			{ 0xffff, 0xbf20, fix(&D:: template WFE<T1>) },
			{ 0xffff, 0xbf30, fix(&D:: template WFI<T1>) },
			{ 0xffff, 0xbf10, fix(&D:: template YIELD<T1>) },
		});

		m_op32_list.assign(
		{
			{ 0xffff0000, 0xf8700000, fix(&D:: template HACK<T1>), nullptr }, // "Undefined" Thumb opcode used
			{ 0xfbe08000, 0xf1400000, fix(&D:: template ADC_IMM<T1>), nullptr },
			{ 0xffe08000, 0xeb400000, fix(&D:: template ADC_REG<T2>), nullptr },
			{ 0xfbe08000, 0xf1000000, fix(&D:: template ADD_IMM<T3>), SKIP_IF((BF(8, 11) == 15 && BT(20)) || BF(16, 19) == 13) },
			{ 0xfbf08000, 0xf2000000, fix(&D:: template ADD_IMM<T4>), SKIP_IF((BF(16, 19) & 13) == 13) },
			{ 0xffe08000, 0xeb000000, fix(&D:: template ADD_REG<T3>), SKIP_IF((BF(8, 11) == 15 && BT(20)) || BF(16, 19) == 13) },
			{ 0xfbef8000, 0xf10d0000, fix(&D:: template ADD_SPI<T3>), SKIP_IF(BF(8, 11) == 15 && BT(20)) },
			{ 0xfbff8000, 0xf20d0000, fix(&D:: template ADD_SPI<T4>), nullptr },
			{ 0xffef8000, 0xeb0d0000, fix(&D:: template ADD_SPR<T3>), nullptr },
			{ 0xfbff8000, 0xf2af0000, fix(&D:: template ADR<T2>), nullptr },
			{ 0xfbff8000, 0xf20f0000, fix(&D:: template ADR<T3>), nullptr },
			{ 0xfbe08000, 0xf0000000, fix(&D:: template AND_IMM<T1>), SKIP_IF(BF(8, 11) == 15 && BT(20)) },
			{ 0xffe08000, 0xea000000, fix(&D:: template AND_REG<T2>), SKIP_IF(BF(8, 11) == 15 && BT(20)) },
			{ 0xffef8030, 0xea4f0020, fix(&D:: template ASR_IMM<T2>), nullptr },
			{ 0xffe0f0f0, 0xfa40f000, fix(&D:: template ASR_REG<T2>), nullptr },
			{ 0xf800d000, 0xf0008000, fix(&D:: template B<T3>), SKIP_IF(BF(23, 25) == 0x7) },
			{ 0xf800d000, 0xf0009000, fix(&D:: template B<T4>), nullptr },
			{ 0xffff8020, 0xf36f0000, fix(&D:: template BFC<T1>), nullptr },
			{ 0xfff08020, 0xf3600000, fix(&D:: template BFI<T1>), SKIP_IF(BF(16, 19) == 15) },
			{ 0xfbe08000, 0xf0200000, fix(&D:: template BIC_IMM<T1>), nullptr },
			{ 0xffe08000, 0xea200000, fix(&D:: template BIC_REG<T2>), nullptr },
			{ 0xf800d000, 0xf000d000, fix(&D:: template BL<T1>), nullptr },
			{ 0xf800c001, 0xf000c000, fix(&D:: template BL<T2>), nullptr },
			{ 0xfff0f0f0, 0xfab0f080, fix(&D:: template CLZ<T1>), nullptr },
			{ 0xfbf08f00, 0xf1100f00, fix(&D:: template CMN_IMM<T1>), nullptr },
			{ 0xfff08f00, 0xeb100f00, fix(&D:: template CMN_REG<T2>), nullptr },
			{ 0xfbf08f00, 0xf1b00f00, fix(&D:: template CMP_IMM<T2>), nullptr },
			{ 0xfff08f00, 0xebb00f00, fix(&D:: template CMP_REG<T3>), nullptr },
			{ 0xfffffff0, 0xf3af80f0, fix(&D:: template DBG<T1>), nullptr },
			{ 0xfffffff0, 0xf3bf8f50, fix(&D:: template DMB<T1>), nullptr },
			{ 0xfffffff0, 0xf3bf8f40, fix(&D:: template DSB<T1>), nullptr },
			{ 0xfbe08000, 0xf0800000, fix(&D:: template EOR_IMM<T1>), SKIP_IF(BF(8, 11) == 15 && BT(20)) },
			{ 0xffe08000, 0xea800000, fix(&D:: template EOR_REG<T2>), SKIP_IF(BF(8, 11) == 15 && BT(20)) },
			{ 0xffd02000, 0xe8900000, fix(&D:: template LDM<T2>), SKIP_IF(BT(21) && BF(16, 19) == 13) },
			{ 0xffd02000, 0xe9100000, fix(&D:: template LDMDB<T1>), nullptr },
			{ 0xfff00000, 0xf8d00000, fix(&D:: template LDR_IMM<T3>), SKIP_IF(BF(16, 19) == 15) },
			{ 0xfff00800, 0xf8500800, fix(&D:: template LDR_IMM<T4>), SKIP_IF(BF(16, 19) == 15 || BF(8, 10) == 6 || (c & 0xf07ff) == 0xd0304 || (c & 0x500) == 0) },
			{ 0xff7f0000, 0xf85f0000, fix(&D:: template LDR_LIT<T2>), nullptr },
			{ 0xfff00fc0, 0xf8500000, fix(&D:: template LDR_REG<T2>), SKIP_IF(BF(16, 19) == 15) },
			{ 0xfff00000, 0xf8900000, fix(&D:: template LDRB_IMM<T2>) },
			{ 0xfff00800, 0xf8100800, fix(&D:: template LDRB_IMM<T3>) },
			{ 0xff7f0000, 0xf81f0000, fix(&D:: template LDRB_LIT<T1>) },
			{ 0xfff00fc0, 0xf8100000, fix(&D:: template LDRB_REG<T2>) },
			{ 0xfe500000, 0xe8500000, fix(&D:: template LDRD_IMM<T1>), SKIP_IF((!BT(21) && !BT(24)) || BF(16, 19) == 15) },
			{ 0xfe7f0000, 0xe85f0000, fix(&D:: template LDRD_LIT<T1>) },
			{ 0xfff00f00, 0xe8500f00, fix(&D:: template LDREX<T1>) },
			{ 0xfff00fff, 0xe8d00f4f, fix(&D:: template LDREXB<T1>) },
			{ 0xfff000ff, 0xe8d0007f, fix(&D:: template LDREXD<T1>) },
			{ 0xfff00fff, 0xe8d00f5f, fix(&D:: template LDREXH<T1>) },
			{ 0xfff00000, 0xf8b00000, fix(&D:: template LDRH_IMM<T2>) },
			{ 0xfff00800, 0xf8300800, fix(&D:: template LDRH_IMM<T3>) },
			{ 0xff7f0000, 0xf83f0000, fix(&D:: template LDRH_LIT<T1>) },
			{ 0xfff00fc0, 0xf8300000, fix(&D:: template LDRH_REG<T2>) },
			{ 0xfff00000, 0xf9900000, fix(&D:: template LDRSB_IMM<T1>) },
			{ 0xfff00800, 0xf9100800, fix(&D:: template LDRSB_IMM<T2>) },
			{ 0xff7f0000, 0xf91f0000, fix(&D:: template LDRSB_LIT<T1>) },
			{ 0xfff00fc0, 0xf9100000, fix(&D:: template LDRSB_REG<T2>) },
			{ 0xfff00000, 0xf9b00000, fix(&D:: template LDRSH_IMM<T1>) },
			{ 0xfff00800, 0xf9300800, fix(&D:: template LDRSH_IMM<T2>) },
			{ 0xff7f0000, 0xf93f0000, fix(&D:: template LDRSH_LIT<T1>) },
			{ 0xfff00fc0, 0xf9300000, fix(&D:: template LDRSH_REG<T2>) },
			{ 0xffef8030, 0xea4f0000, fix(&D:: template LSL_IMM<T2>) },
			{ 0xffe0f0f0, 0xfa00f000, fix(&D:: template LSL_REG<T2>) },
			{ 0xffef8030, 0xea4f0010, fix(&D:: template LSR_IMM<T2>) },
			{ 0xffe0f0f0, 0xfa20f000, fix(&D:: template LSR_REG<T2>) },
			{ 0xfff000f0, 0xfb000000, fix(&D:: template MLA<T1>), SKIP_IF(BF(12, 15) == 15) },
			{ 0xfff000f0, 0xfb000010, fix(&D:: template MLS<T1>) },
			{ 0xfbef8000, 0xf04f0000, fix(&D:: template MOV_IMM<T2>) },
			{ 0xfbf08000, 0xf2400000, fix(&D:: template MOV_IMM<T3>) },
			{ 0xffeff0f0, 0xea4f0000, fix(&D:: template MOV_REG<T3>) },
			{ 0xfbf08000, 0xf2c00000, fix(&D:: template MOVT<T1>) },
			{ 0xff100010, 0xee100010, fix(&D:: template MRC_<T1>) },
			{ 0xff100010, 0xfe100010, fix(&D:: template MRC_<T2>) },
			{ 0xfffff0ff, 0xf3ef8000, fix(&D:: template MRS<T1>) },
			{ 0xfff0f3ff, 0xf3808000, fix(&D:: template MSR_REG<T1>) },
			{ 0xfff0f0f0, 0xfb00f000, fix(&D:: template MUL<T2>) },
			{ 0xfbef8000, 0xf06f0000, fix(&D:: template MVN_IMM<T1>) },
			{ 0xffef8000, 0xea6f0000, fix(&D:: template MVN_REG<T2>) },
			{ 0xffffffff, 0xf3af8000, fix(&D:: template NOP<T2>) },
			{ 0xfbe08000, 0xf0600000, fix(&D:: template ORN_IMM<T1>) },
			{ 0xffe08000, 0xea600000, fix(&D:: template ORN_REG<T1>) },
			{ 0xfbe08000, 0xf0400000, fix(&D:: template ORR_IMM<T1>) },
			{ 0xffe08000, 0xea400000, fix(&D:: template ORR_REG<T2>), SKIP_IF(BF(16, 19) == 15) },
			{ 0xfff08010, 0xeac00000, fix(&D:: template PKH<T1>) },
			{ 0xffff0000, 0xe8bd0000, fix(&D:: template POP<T2>) },
			{ 0xffff0fff, 0xf85d0b04, fix(&D:: template POP<T3>) },
			{ 0xffff0000, 0xe92d0000, fix(&D:: template PUSH<T2>) }, // had an error in arch ref
			{ 0xffff0fff, 0xf84d0d04, fix(&D:: template PUSH<T3>) },
			{ 0xfff0f0f0, 0xfa80f080, fix(&D:: template QADD<T1>) },
			{ 0xfff0f0f0, 0xfa90f010, fix(&D:: template QADD16<T1>) },
			{ 0xfff0f0f0, 0xfa80f010, fix(&D:: template QADD8<T1>) },
			{ 0xfff0f0f0, 0xfaa0f010, fix(&D:: template QASX<T1>) },
			{ 0xfff0f0f0, 0xfa80f090, fix(&D:: template QDADD<T1>) },
			{ 0xfff0f0f0, 0xfa80f0b0, fix(&D:: template QDSUB<T1>) },
			{ 0xfff0f0f0, 0xfae0f010, fix(&D:: template QSAX<T1>) },
			{ 0xfff0f0f0, 0xfa80f0a0, fix(&D:: template QSUB<T1>) },
			{ 0xfff0f0f0, 0xfad0f010, fix(&D:: template QSUB16<T1>) },
			{ 0xfff0f0f0, 0xfac0f010, fix(&D:: template QSUB8<T1>) },
			{ 0xfff0f0f0, 0xfa90f0a0, fix(&D:: template RBIT<T1>) },
			{ 0xfff0f0f0, 0xfa90f080, fix(&D:: template REV<T2>) },
			{ 0xfff0f0f0, 0xfa90f090, fix(&D:: template REV16<T2>) },
			{ 0xfff0f0f0, 0xfa90f0b0, fix(&D:: template REVSH<T2>) },
			{ 0xffef8030, 0xea4f0030, fix(&D:: template ROR_IMM<T1>) },
			{ 0xffe0f0f0, 0xfa60f000, fix(&D:: template ROR_REG<T2>) },
			{ 0xffeff0f0, 0xea4f0030, fix(&D:: template RRX<T1>) },
			{ 0xfbe08000, 0xf1c00000, fix(&D:: template RSB_IMM<T2>) },
			{ 0xffe08000, 0xebc00000, fix(&D:: template RSB_REG<T1>) },
			{ 0xfff0f0f0, 0xfa90f000, fix(&D:: template SADD16<T1>) },
			{ 0xfff0f0f0, 0xfa80f000, fix(&D:: template SADD8<T1>) },
			{ 0xfff0f0f0, 0xfaa0f000, fix(&D:: template SASX<T1>) },
			{ 0xfbe08000, 0xf1600000, fix(&D:: template SBC_IMM<T1>) },
			{ 0xffe08000, 0xeb600000, fix(&D:: template SBC_REG<T2>) },
			{ 0xfff08020, 0xf3400000, fix(&D:: template SBFX<T1>) },
			{ 0xfff0f0f0, 0xfb90f0f0, fix(&D:: template SDIV<T1>) }, // ???
			{ 0xfff0f0f0, 0xfaa0f080, fix(&D:: template SEL<T1>) },
			{ 0xfff0f0f0, 0xfa90f020, fix(&D:: template SHADD16<T1>) },
			{ 0xfff0f0f0, 0xfa80f020, fix(&D:: template SHADD8<T1>) },
			{ 0xfff0f0f0, 0xfaa0f020, fix(&D:: template SHASX<T1>) },
			{ 0xfff0f0f0, 0xfae0f020, fix(&D:: template SHSAX<T1>) },
			{ 0xfff0f0f0, 0xfad0f020, fix(&D:: template SHSUB16<T1>) },
			{ 0xfff0f0f0, 0xfac0f020, fix(&D:: template SHSUB8<T1>) },
			{ 0xfff000c0, 0xfb100000, fix(&D:: template SMLA__<T1>) },
			{ 0xfff000e0, 0xfb200000, fix(&D:: template SMLAD<T1>) },
			{ 0xfff000f0, 0xfbc00000, fix(&D:: template SMLAL<T1>) },
			{ 0xfff000c0, 0xfbc00080, fix(&D:: template SMLAL__<T1>) },
			{ 0xfff000e0, 0xfbc000c0, fix(&D:: template SMLALD<T1>) },
			{ 0xfff000e0, 0xfb300000, fix(&D:: template SMLAW_<T1>) },
			{ 0xfff000e0, 0xfb400000, fix(&D:: template SMLSD<T1>) },
			{ 0xfff000e0, 0xfbd000c0, fix(&D:: template SMLSLD<T1>) },
			{ 0xfff000e0, 0xfb500000, fix(&D:: template SMMLA<T1>) },
			{ 0xfff000e0, 0xfb600000, fix(&D:: template SMMLS<T1>) },
			{ 0xfff0f0e0, 0xfb50f000, fix(&D:: template SMMUL<T1>) },
			{ 0xfff0f0e0, 0xfb20f000, fix(&D:: template SMUAD<T1>) },
			{ 0xfff0f0c0, 0xfb10f000, fix(&D:: template SMUL__<T1>) },
			{ 0xfff000f0, 0xfb800000, fix(&D:: template SMULL<T1>) },
			{ 0xfff0f0e0, 0xfb30f000, fix(&D:: template SMULW_<T1>) },
			{ 0xfff0f0e0, 0xfb40f000, fix(&D:: template SMUSD<T1>) },
			{ 0xffd08020, 0xf3000000, fix(&D:: template SSAT<T1>) },
			{ 0xfff0f0e0, 0xf3200000, fix(&D:: template SSAT16<T1>) },
			{ 0xfff0f0f0, 0xfae0f000, fix(&D:: template SSAX<T1>) },
			{ 0xfff0f0f0, 0xfad0f000, fix(&D:: template SSUB16<T1>) },
			{ 0xfff0f0f0, 0xfac0f000, fix(&D:: template SSUB8<T1>) },
			{ 0xffd0a000, 0xe8800000, fix(&D:: template STM<T2>) },
			{ 0xffd0a000, 0xe9000000, fix(&D:: template STMDB<T1>) },
			{ 0xfff00000, 0xf8c00000, fix(&D:: template STR_IMM<T3>) },
			{ 0xfff00800, 0xf8400800, fix(&D:: template STR_IMM<T4>) },
			{ 0xfff00fc0, 0xf8400000, fix(&D:: template STR_REG<T2>) },
			{ 0xfff00000, 0xf8800000, fix(&D:: template STRB_IMM<T2>) },
			{ 0xfff00800, 0xf8000800, fix(&D:: template STRB_IMM<T3>) },
			{ 0xfff00fc0, 0xf8000000, fix(&D:: template STRB_REG<T2>) },
			{ 0xfe500000, 0xe8400000, fix(&D:: template STRD_IMM<T1>), SKIP_IF(!BT(21) && !BT(24)) },
			{ 0xfff00000, 0xe8400000, fix(&D:: template STREX<T1>) },
			{ 0xfff00ff0, 0xe8c00f40, fix(&D:: template STREXB<T1>) },
			{ 0xfff000f0, 0xe8c00070, fix(&D:: template STREXD<T1>) },
			{ 0xfff00ff0, 0xe8c00f50, fix(&D:: template STREXH<T1>) },
			{ 0xfff00000, 0xf8a00000, fix(&D:: template STRH_IMM<T2>) },
			{ 0xfff00800, 0xf8200800, fix(&D:: template STRH_IMM<T3>) },
			{ 0xfff00fc0, 0xf8200000, fix(&D:: template STRH_REG<T2>) },
			{ 0xfbe08000, 0xf1a00000, fix(&D:: template SUB_IMM<T3>), SKIP_IF((BF(8, 11) == 15 && BT(20)) || BF(16, 19) == 13) },
			{ 0xfbf08000, 0xf2a00000, fix(&D:: template SUB_IMM<T4>) },
			{ 0xffe08000, 0xeba00000, fix(&D:: template SUB_REG<T2>), SKIP_IF((BF(8, 11) == 15 && BT(20)) || BF(16, 19) == 13) },
			{ 0xfbef8000, 0xf1ad0000, fix(&D:: template SUB_SPI<T2>) },
			{ 0xfbff8000, 0xf2ad0000, fix(&D:: template SUB_SPI<T3>) },
			{ 0xffef8000, 0xebad0000, fix(&D:: template SUB_SPR<T1>) },
			{ 0xfff0f0c0, 0xfa40f080, fix(&D:: template SXTAB<T1>) },
			{ 0xfff0f0c0, 0xfa20f080, fix(&D:: template SXTAB16<T1>) },
			{ 0xfff0f0c0, 0xfa00f080, fix(&D:: template SXTAH<T1>) },
			{ 0xfffff0c0, 0xfa4ff080, fix(&D:: template SXTB<T2>) },
			{ 0xfffff0c0, 0xfa2ff080, fix(&D:: template SXTB16<T1>) },
			{ 0xfffff0c0, 0xfa0ff080, fix(&D:: template SXTH<T2>) },
			{ 0xfff0ffe0, 0xe8d0f000, fix(&D:: template TB_<T1>) },
			{ 0xfbf08f00, 0xf0900f00, fix(&D:: template TEQ_IMM<T1>) },
			{ 0xfff08f00, 0xea900f00, fix(&D:: template TEQ_REG<T1>) },
			{ 0xfbf08f00, 0xf0100f00, fix(&D:: template TST_IMM<T1>) },
			{ 0xfff08f00, 0xea100f00, fix(&D:: template TST_REG<T2>) },
			{ 0xfff0f0f0, 0xfa90f040, fix(&D:: template UADD16<T1>) },
			{ 0xfff0f0f0, 0xfa80f040, fix(&D:: template UADD8<T1>) },
			{ 0xfff0f0f0, 0xfaa0f040, fix(&D:: template UASX<T1>) },
			{ 0xfff08020, 0xf3c00000, fix(&D:: template UBFX<T1>) },
			{ 0xfff0f0f0, 0xfbb0f0f0, fix(&D:: template UDIV<T1>) }, // ???
			{ 0xfff0f0f0, 0xfa90f060, fix(&D:: template UHADD16<T1>) },
			{ 0xfff0f0f0, 0xfa80f060, fix(&D:: template UHADD8<T1>) },
			{ 0xfff0f0f0, 0xfaa0f060, fix(&D:: template UHASX<T1>) },
			{ 0xfff0f0f0, 0xfae0f060, fix(&D:: template UHSAX<T1>) },
			{ 0xfff0f0f0, 0xfad0f060, fix(&D:: template UHSUB16<T1>) },
			{ 0xfff0f0f0, 0xfac0f060, fix(&D:: template UHSUB8<T1>) },
			{ 0xfff000f0, 0xfbe00060, fix(&D:: template UMAAL<T1>) },
			{ 0xfff000f0, 0xfbe00000, fix(&D:: template UMLAL<T1>) },
			{ 0xfff000f0, 0xfba00000, fix(&D:: template UMULL<T1>) },
			{ 0xfff0f0f0, 0xfa90f050, fix(&D:: template UQADD16<T1>) },
			{ 0xfff0f0f0, 0xfa80f050, fix(&D:: template UQADD8<T1>) },
			{ 0xfff0f0f0, 0xfaa0f050, fix(&D:: template UQASX<T1>) },
			{ 0xfff0f0f0, 0xfae0f050, fix(&D:: template UQSAX<T1>) },
			{ 0xfff0f0f0, 0xfad0f050, fix(&D:: template UQSUB16<T1>) },
			{ 0xfff0f0f0, 0xfac0f050, fix(&D:: template UQSUB8<T1>) },
			{ 0xfff0f0f0, 0xfb70f000, fix(&D:: template USAD8<T1>) },
			{ 0xfff000f0, 0xfb700000, fix(&D:: template USADA8<T1>) },
			{ 0xffd08020, 0xf3800000, fix(&D:: template USAT<T1>) },
			{ 0xfff0f0e0, 0xf3a00000, fix(&D:: template USAT16<T1>) },
			{ 0xfff0f0f0, 0xfae0f040, fix(&D:: template USAX<T1>) },
			{ 0xfff0f0f0, 0xfad0f040, fix(&D:: template USUB16<T1>) },
			{ 0xfff0f0f0, 0xfac0f040, fix(&D:: template USUB8<T1>) },
			{ 0xfff0f0c0, 0xfa50f080, fix(&D:: template UXTAB<T1>), SKIP_IF(BF(16, 19) == 15) },
			{ 0xfff0f0c0, 0xfa30f080, fix(&D:: template UXTAB16<T1>) },
			{ 0xfff0f0c0, 0xfa10f080, fix(&D:: template UXTAH<T1>) },
			{ 0xfffff0c0, 0xfa5ff080, fix(&D:: template UXTB<T2>) },
			{ 0xfffff0c0, 0xfa3ff080, fix(&D:: template UXTB16<T1>) },
			{ 0xfffff0c0, 0xfa1ff080, fix(&D:: template UXTH<T2>) },
			{ 0xef800f10, 0xef000710, fix(&D:: template VABA_<T1>) },
			{ 0xef800f50, 0xef800500, fix(&D:: template VABA_<T2>) },
			{ 0xef800f10, 0xef000700, fix(&D:: template VABD_<T1>) },
			{ 0xef800f50, 0xef800700, fix(&D:: template VABD_<T2>) },
			{ 0xffa00f10, 0xff200d00, fix(&D:: template VABD_FP<T1>) },
			{ 0xffb30b90, 0xffb10300, fix(&D:: template VABS<T1>) },
			{ 0xffbf0ed0, 0xeeb00ac0, fix(&D:: template VABS<T2>) },
			{ 0xff800f10, 0xff000e10, fix(&D:: template VAC__<T1>) },
			{ 0xff800f10, 0xef000800, fix(&D:: template VADD<T1>) },
			{ 0xffa00f10, 0xef000d00, fix(&D:: template VADD_FP<T1>) },
			{ 0xffb00e50, 0xee300a00, fix(&D:: template VADD_FP<T2>) },
			{ 0xff800f50, 0xef800400, fix(&D:: template VADDHN<T1>) },
			{ 0xef800e50, 0xef800000, fix(&D:: template VADD_<T1>) },
			{ 0xffb00f10, 0xef000110, fix(&D:: template VAND<T1>) },
			{ 0xefb800b0, 0xef800030, fix(&D:: template VBIC_IMM<T1>) },
			{ 0xffb00f10, 0xef100110, fix(&D:: template VBIC_REG<T1>) },
			{ 0xff800f10, 0xff000110, fix(&D:: template VB__<T1>) },
			{ 0xff800f10, 0xff000810, fix(&D:: template VCEQ_REG<T1>) },
			{ 0xffa00f10, 0xef000e00, fix(&D:: template VCEQ_REG<T2>) },
			{ 0xffb30b90, 0xffb10100, fix(&D:: template VCEQ_ZERO<T1>) },
			{ 0xef800f10, 0xef000310, fix(&D:: template VCGE_REG<T1>) },
			{ 0xffa00f10, 0xff000e00, fix(&D:: template VCGE_REG<T2>) },
			{ 0xffb30b90, 0xffb10080, fix(&D:: template VCGE_ZERO<T1>) },
			{ 0xef800f10, 0xef000300, fix(&D:: template VCGT_REG<T1>) },
			{ 0xffa00f10, 0xff200e00, fix(&D:: template VCGT_REG<T2>) },
			{ 0xffb30b90, 0xffb10000, fix(&D:: template VCGT_ZERO<T1>) },
			{ 0xffb30b90, 0xffb10180, fix(&D:: template VCLE_ZERO<T1>) },
			{ 0xffb30f90, 0xffb00400, fix(&D:: template VCLS<T1>) },
			{ 0xffb30b90, 0xffb10200, fix(&D:: template VCLT_ZERO<T1>) },
			{ 0xffb30f90, 0xffb00480, fix(&D:: template VCLZ<T1>) },
			{ 0xffbf0e50, 0xeeb40a40, fix(&D:: template VCMP_<T1>) },
			{ 0xffbf0e7f, 0xeeb50a40, fix(&D:: template VCMP_<T2>) },
			{ 0xffb30f90, 0xffb00500, fix(&D:: template VCNT<T1>) },
			{ 0xffb30e10, 0xffb30600, fix(&D:: template VCVT_FIA<T1>) },
			{ 0xffb80e50, 0xeeb80a40, fix(&D:: template VCVT_FIF<T1>) },
			{ 0xef800e90, 0xef800e10, fix(&D:: template VCVT_FFA<T1>) },
			{ 0xffba0e50, 0xeeba0a40, fix(&D:: template VCVT_FFF<T1>) },
			{ 0xffbf0ed0, 0xeeb70ac0, fix(&D:: template VCVT_DF<T1>) },
			{ 0xffb30ed0, 0xffb20600, fix(&D:: template VCVT_HFA<T1>) },
			{ 0xffbe0f50, 0xeeb20a40, fix(&D:: template VCVT_HFF<T1>) },
			{ 0xffb00e50, 0xee800a00, fix(&D:: template VDIV<T1>) },
			{ 0xffb00f90, 0xffb00c00, fix(&D:: template VDUP_S<T1>) },
			{ 0xff900f5f, 0xee800b10, fix(&D:: template VDUP_R<T1>) },
			{ 0xffb00f10, 0xff000110, fix(&D:: template VEOR<T1>) },
			{ 0xffb00010, 0xefb00000, fix(&D:: template VEXT<T1>) },
			{ 0xef800b10, 0xef000000, fix(&D:: template VHADDSUB<T1>) },
			{ 0xffb00000, 0xf9200000, fix(&D:: template VLD__MS<T1>) }, // VLD1, VLD2, VLD3, VLD4
			{ 0xffb00f00, 0xf9a00c00, fix(&D:: template VLD1_SAL<T1>) },
			{ 0xffb00300, 0xf9a00000, fix(&D:: template VLD1_SL<T1>) },
			{ 0xffb00f00, 0xf9a00d00, fix(&D:: template VLD2_SAL<T1>) },
			{ 0xffb00300, 0xf9a00100, fix(&D:: template VLD2_SL<T1>) },
			{ 0xffb00f00, 0xf9a00e00, fix(&D:: template VLD3_SAL<T1>) },
			{ 0xffb00300, 0xf9a00200, fix(&D:: template VLD3_SL<T1>) },
			{ 0xffb00f00, 0xf9a00f00, fix(&D:: template VLD4_SAL<T1>) },
			{ 0xffb00300, 0xf9a00300, fix(&D:: template VLD4_SL<T1>) },
			{ 0xfe100f00, 0xec100b00, fix(&D:: template VLDM<T1>) },
			{ 0xfe100f00, 0xec100a00, fix(&D:: template VLDM<T2>) },
			{ 0xff300f00, 0xed100b00, fix(&D:: template VLDR<T1>) },
			{ 0xff300f00, 0xed100a00, fix(&D:: template VLDR<T2>) },
			{ 0xef800f00, 0xef000600, fix(&D:: template VMAXMIN<T1>) },
			{ 0xff800f10, 0xef000f00, fix(&D:: template VMAXMIN_FP<T1>) },
			{ 0xef800f10, 0xef000900, fix(&D:: template VML__<T1>) },
			{ 0xef800d50, 0xef800800, fix(&D:: template VML__<T2>) },
			{ 0xff800f10, 0xef000d10, fix(&D:: template VML__FP<T1>) },
			{ 0xffb00e10, 0xee000a00, fix(&D:: template VML__FP<T2>) },
			{ 0xef800a50, 0xef800040, fix(&D:: template VML__S<T1>) },
			{ 0xef800b50, 0xef800240, fix(&D:: template VML__S<T2>) },
			{ 0xefb80090, 0xef800010, fix(&D:: template VMOV_IMM<T1>) },
			{ 0xffb00ef0, 0xeeb00a00, fix(&D:: template VMOV_IMM<T2>) },
			{ 0xffb00f10, 0xef200110, fix(&D:: template VMOV_REG<T1>) },
			{ 0xffbf0ed0, 0xeeb00a40, fix(&D:: template VMOV_REG<T2>) },
			{ 0xff900f1f, 0xee000b10, fix(&D:: template VMOV_RS<T1>) },
			{ 0xff100f1f, 0xee100b10, fix(&D:: template VMOV_SR<T1>) },
			{ 0xffe00f7f, 0xee000a10, fix(&D:: template VMOV_RF<T1>) },
			{ 0xffe00fd0, 0xec400a10, fix(&D:: template VMOV_2RF<T1>) },
			{ 0xffe00fd0, 0xec400b10, fix(&D:: template VMOV_2RD<T1>) },
			{ 0xef870fd0, 0xef800a10, fix(&D:: template VMOVL<T1>) },
			{ 0xffb30fd0, 0xffb20200, fix(&D:: template VMOVN<T1>) },
			{ 0xffff0fff, 0xeef10a10, fix(&D:: template VMRS<T1>) },
			{ 0xffff0fff, 0xeee10a10, fix(&D:: template VMSR<T1>) },
			{ 0xef800f10, 0xef000910, fix(&D:: template VMUL_<T1>) },
			{ 0xef800d50, 0xef800c00, fix(&D:: template VMUL_<T2>) },
			{ 0xffa00f10, 0xff000d10, fix(&D:: template VMUL_FP<T1>) },
			{ 0xffb00e50, 0xee200a00, fix(&D:: template VMUL_FP<T2>) },
			{ 0xef800e50, 0xef800840, fix(&D:: template VMUL_S<T1>) },
			{ 0xef800f50, 0xef800a40, fix(&D:: template VMUL_S<T2>) },
			{ 0xefb800b0, 0xef800030, fix(&D:: template VMVN_IMM<T1>) },
			{ 0xffb30f90, 0xffb00580, fix(&D:: template VMVN_REG<T1>) },
			{ 0xffb30b90, 0xffb10380, fix(&D:: template VNEG<T1>) },
			{ 0xffbf0ed0, 0xeeb10a40, fix(&D:: template VNEG<T2>) },
			{ 0xffb00e10, 0xee100a00, fix(&D:: template VNM__<T1>) },
			{ 0xffb00e50, 0xee200a40, fix(&D:: template VNM__<T2>) },
			{ 0xffb00f10, 0xef300110, fix(&D:: template VORN_REG<T1>) },
			{ 0xefb800b0, 0xef800010, fix(&D:: template VORR_IMM<T1>) },
			{ 0xffb00f10, 0xef200110, fix(&D:: template VORR_REG<T1>) },
			{ 0xffb30f10, 0xffb00600, fix(&D:: template VPADAL<T1>) },
			{ 0xff800f10, 0xef000b10, fix(&D:: template VPADD<T1>) },
			{ 0xffa00f10, 0xff000d00, fix(&D:: template VPADD_FP<T1>) },
			{ 0xffb30f10, 0xffb00200, fix(&D:: template VPADDL<T1>) },
			{ 0xef800f00, 0xef000a00, fix(&D:: template VPMAXMIN<T1>) },
			{ 0xff800f10, 0xff000f00, fix(&D:: template VPMAXMIN_FP<T1>) },
			{ 0xffbf0f00, 0xecbd0b00, fix(&D:: template VPOP<T1>) },
			{ 0xffbf0f00, 0xecbd0a00, fix(&D:: template VPOP<T2>) },
			{ 0xffbf0f00, 0xed2d0b00, fix(&D:: template VPUSH<T1>) },
			{ 0xffbf0f00, 0xed2d0a00, fix(&D:: template VPUSH<T2>) },
			{ 0xffb30f90, 0xffb00700, fix(&D:: template VQABS<T1>) },
			{ 0xef800f10, 0xef000010, fix(&D:: template VQADD<T1>) },
			{ 0xff800d50, 0xef800900, fix(&D:: template VQDML_L<T1>) },
			{ 0xff800b50, 0xef800340, fix(&D:: template VQDML_L<T2>) },
			{ 0xff800f10, 0xef000b00, fix(&D:: template VQDMULH<T1>) },
			{ 0xef800f50, 0xef800c40, fix(&D:: template VQDMULH<T2>) },
			{ 0xff800f50, 0xef800d00, fix(&D:: template VQDMULL<T1>) },
			{ 0xff800f50, 0xef800b40, fix(&D:: template VQDMULL<T2>) },
			{ 0xffb30f10, 0xffb20200, fix(&D:: template VQMOV_N<T1>) },
			{ 0xffb30f90, 0xffb00780, fix(&D:: template VQNEG<T1>) },
			{ 0xff800f10, 0xff000b00, fix(&D:: template VQRDMULH<T1>) },
			{ 0xef800f50, 0xef800d40, fix(&D:: template VQRDMULH<T2>) },
			{ 0xef800f10, 0xef000510, fix(&D:: template VQRSHL<T1>) },
			{ 0xef800ed0, 0xef800850, fix(&D:: template VQRSHR_N<T1>) },
			{ 0xef800f10, 0xef000410, fix(&D:: template VQSHL_REG<T1>) },
			{ 0xef800e10, 0xef800610, fix(&D:: template VQSHL_IMM<T1>) },
			{ 0xef800ed0, 0xef800810, fix(&D:: template VQSHR_N<T1>) },
			{ 0xef800f10, 0xef000210, fix(&D:: template VQSUB<T1>) },
			{ 0xff800f50, 0xff800400, fix(&D:: template VRADDHN<T1>) },
			{ 0xffb30e90, 0xffb30400, fix(&D:: template VRECPE<T1>) },
			{ 0xffa00f10, 0xef000f10, fix(&D:: template VRECPS<T1>) },
			{ 0xffb30e10, 0xffb00000, fix(&D:: template VREV__<T1>) },
			{ 0xef800f10, 0xef000100, fix(&D:: template VRHADD<T1>) },
			{ 0xef800f10, 0xef000500, fix(&D:: template VRSHL<T1>) },
			{ 0xef800f10, 0xef800210, fix(&D:: template VRSHR<T1>) },
			{ 0xff800fd0, 0xef800850, fix(&D:: template VRSHRN<T1>) },
			{ 0xffb30e90, 0xffb30480, fix(&D:: template VRSQRTE<T1>) },
			{ 0xffa00f10, 0xef200f10, fix(&D:: template VRSQRTS<T1>) },
			{ 0xef800f10, 0xef800310, fix(&D:: template VRSRA<T1>) },
			{ 0xff800f50, 0xff800600, fix(&D:: template VRSUBHN<T1>) },
			{ 0xff800f10, 0xef800510, fix(&D:: template VSHL_IMM<T1>) },
			{ 0xef800f10, 0xef000400, fix(&D:: template VSHL_REG<T1>) },
			{ 0xef800fd0, 0xef800a10, fix(&D:: template VSHLL<T1>) },
			{ 0xffb30fd0, 0xffb20300, fix(&D:: template VSHLL<T2>) },
			{ 0xef800f10, 0xef800010, fix(&D:: template VSHR<T1>) },
			{ 0xff800fd0, 0xef800810, fix(&D:: template VSHRN<T1>) },
			{ 0xff800f10, 0xff800510, fix(&D:: template VSLI<T1>) },
			{ 0xffbf0ed0, 0xeeb10ac0, fix(&D:: template VSQRT<T1>) },
			{ 0xef800f10, 0xef800110, fix(&D:: template VSRA<T1>) },
			{ 0xff800f10, 0xff800410, fix(&D:: template VSRI<T1>) },
			{ 0xffb00000, 0xf9000000, fix(&D:: template VST__MS<T1>) }, // VST1, VST2, VST3, VST4
			{ 0xffb00300, 0xf9800000, fix(&D:: template VST1_SL<T1>) },
			{ 0xffb00300, 0xf9800100, fix(&D:: template VST2_SL<T1>) },
			{ 0xffb00300, 0xf9800200, fix(&D:: template VST3_SL<T1>) },
			{ 0xffb00300, 0xf9800300, fix(&D:: template VST4_SL<T1>) },
			{ 0xfe100f00, 0xec000b00, fix(&D:: template VSTM<T1>) },
			{ 0xfe100f00, 0xec000a00, fix(&D:: template VSTM<T2>) },
			{ 0xff300f00, 0xed000b00, fix(&D:: template VSTR<T1>) },
			{ 0xff300f00, 0xed000a00, fix(&D:: template VSTR<T2>) },
			{ 0xff800f10, 0xff000800, fix(&D:: template VSUB<T1>) },
			{ 0xffa00f10, 0xef200d00, fix(&D:: template VSUB_FP<T1>) },
			{ 0xffb00e50, 0xee300a40, fix(&D:: template VSUB_FP<T2>) },
			{ 0xff800f50, 0xef800600, fix(&D:: template VSUBHN<T1>) },
			{ 0xef800e50, 0xef800200, fix(&D:: template VSUB_<T1>) },
			{ 0xffb30f90, 0xffb20000, fix(&D:: template VSWP<T1>) },
			{ 0xffb00c10, 0xffb00800, fix(&D:: template VTB_<T1>) },
			{ 0xffb30f90, 0xffb20080, fix(&D:: template VTRN<T1>) },
			{ 0xff800f10, 0xef000810, fix(&D:: template VTST<T1>) },
			{ 0xffb30f90, 0xffb20100, fix(&D:: template VUZP<T1>) },
			{ 0xffb30f90, 0xffb20180, fix(&D:: template VZIP<T1>) },
			{ 0xffffffff, 0xf3af8002, fix(&D:: template WFE<T2>) },
			{ 0xffffffff, 0xf3af8003, fix(&D:: template WFI<T2>) },
			{ 0xffffffff, 0xf3af8001, fix(&D:: template YIELD<T2>) },
		});

		m_arm_list.assign(
		{
			{ 0x0ff000f0, 0x00700090, fix(&D:: template HACK<A1>), nullptr }, // "Undefined" ARM opcode used
			{ 0x0ffffff0, 0x012fff10, fix(&D:: template BX<A1>), nullptr },

			{ 0x0fe00000, 0x02a00000, fix(&D:: template ADC_IMM<A1>) },
			{ 0x0fe00010, 0x00a00000, fix(&D:: template ADC_REG<A1>) },
			{ 0x0fe00090, 0x00a00010, fix(&D:: template ADC_RSR<A1>) },
			{ 0x0fe00000, 0x02800000, fix(&D:: template ADD_IMM<A1>) },
			{ 0x0fe00010, 0x00800000, fix(&D:: template ADD_REG<A1>) },
			{ 0x0fe00090, 0x00800010, fix(&D:: template ADD_RSR<A1>) },
			{ 0x0fef0000, 0x028d0000, fix(&D:: template ADD_SPI<A1>) },
			{ 0x0fef0010, 0x008d0000, fix(&D:: template ADD_SPR<A1>) },
			{ 0x0fff0000, 0x028f0000, fix(&D:: template ADR<A1>) },
			{ 0x0fff0000, 0x024f0000, fix(&D:: template ADR<A2>) },
			{ 0x0fe00000, 0x02000000, fix(&D:: template AND_IMM<A1>) },
			{ 0x0fe00010, 0x00000000, fix(&D:: template AND_REG<A1>) },
			{ 0x0fe00090, 0x00000010, fix(&D:: template AND_RSR<A1>) },
			{ 0x0fef0070, 0x01a00040, fix(&D:: template ASR_IMM<A1>) },
			{ 0x0fef00f0, 0x01a00050, fix(&D:: template ASR_REG<A1>) },
			{ 0x0f000000, 0x0a000000, fix(&D:: template B<A1>) },
			{ 0x0fe0007f, 0x07c0001f, fix(&D:: template BFC<A1>) },
			{ 0x0fe00070, 0x07c00010, fix(&D:: template BFI<A1>) },
			{ 0x0fe00000, 0x03c00000, fix(&D:: template BIC_IMM<A1>) },
			{ 0x0fe00010, 0x01c00000, fix(&D:: template BIC_REG<A1>) },
			{ 0x0fe00090, 0x01c00010, fix(&D:: template BIC_RSR<A1>) },
			{ 0x0ff000f0, 0x01200070, fix(&D:: template BKPT<A1>) },
			{ 0x0f000000, 0x0b000000, fix(&D:: template BL<A1>) },
			{ 0xfe000000, 0xfa000000, fix(&D:: template BL<A2>) },
			{ 0x0ffffff0, 0x012fff30, fix(&D:: template BLX<A1>) },
			{ 0x0fff0ff0, 0x016f0f10, fix(&D:: template CLZ<A1>) },
			{ 0x0ff0f000, 0x03700000, fix(&D:: template CMN_IMM<A1>) },
			{ 0x0ff0f010, 0x01700000, fix(&D:: template CMN_REG<A1>) },
			{ 0x0ff0f090, 0x01700010, fix(&D:: template CMN_RSR<A1>) },
			{ 0x0ff0f000, 0x03500000, fix(&D:: template CMP_IMM<A1>) },
			{ 0x0ff0f010, 0x01500000, fix(&D:: template CMP_REG<A1>) },
			{ 0x0ff0f090, 0x01500010, fix(&D:: template CMP_RSR<A1>) },
			{ 0x0ffffff0, 0x0320f0f0, fix(&D:: template DBG<A1>) },
			{ 0xfffffff0, 0xf57ff050, fix(&D:: template DMB<A1>) },
			{ 0xfffffff0, 0xf57ff040, fix(&D:: template DSB<A1>) },
			{ 0x0fe00000, 0x02200000, fix(&D:: template EOR_IMM<A1>) },
			{ 0x0fe00010, 0x00200000, fix(&D:: template EOR_REG<A1>) },
			{ 0x0fe00090, 0x00200010, fix(&D:: template EOR_RSR<A1>) },
			{ 0x0fd00000, 0x08900000, fix(&D:: template LDM<A1>) },
			{ 0x0fd00000, 0x08100000, fix(&D:: template LDMDA<A1>) },
			{ 0x0fd00000, 0x09100000, fix(&D:: template LDMDB<A1>) },
			{ 0x0fd00000, 0x09900000, fix(&D:: template LDMIB<A1>) },
			{ 0x0e500000, 0x04100000, fix(&D:: template LDR_IMM<A1>) },
			{ 0x0f7f0000, 0x051f0000, fix(&D:: template LDR_LIT<A1>) },
			{ 0x0e500010, 0x06100000, fix(&D:: template LDR_REG<A1>) },
			{ 0x0e500000, 0x04500000, fix(&D:: template LDRB_IMM<A1>) },
			{ 0x0f7f0000, 0x055f0000, fix(&D:: template LDRB_LIT<A1>) },
			{ 0x0e500010, 0x06500000, fix(&D:: template LDRB_REG<A1>) },
			{ 0x0e5000f0, 0x004000d0, fix(&D:: template LDRD_IMM<A1>) },
			{ 0x0f7f00f0, 0x014f00d0, fix(&D:: template LDRD_LIT<A1>) },
			{ 0x0e500ff0, 0x000000d0, fix(&D:: template LDRD_REG<A1>) },
			{ 0x0ff00fff, 0x01900f9f, fix(&D:: template LDREX<A1>) },
			{ 0x0ff00fff, 0x01d00f9f, fix(&D:: template LDREXB<A1>) },
			{ 0x0ff00fff, 0x01b00f9f, fix(&D:: template LDREXD<A1>) },
			{ 0x0ff00fff, 0x01f00f9f, fix(&D:: template LDREXH<A1>) },
			{ 0x0e5000f0, 0x005000b0, fix(&D:: template LDRH_IMM<A1>) },
			{ 0x0f7f00f0, 0x015f00b0, fix(&D:: template LDRH_LIT<A1>) },
			{ 0x0e500ff0, 0x001000b0, fix(&D:: template LDRH_REG<A1>) },
			{ 0x0e5000f0, 0x005000d0, fix(&D:: template LDRSB_IMM<A1>) },
			{ 0x0f7f00f0, 0x015f00d0, fix(&D:: template LDRSB_LIT<A1>) },
			{ 0x0e500ff0, 0x001000d0, fix(&D:: template LDRSB_REG<A1>) },
			{ 0x0e5000f0, 0x005000f0, fix(&D:: template LDRSH_IMM<A1>) },
			{ 0x0f7f00f0, 0x015f00f0, fix(&D:: template LDRSH_LIT<A1>) },
			{ 0x0e500ff0, 0x001000f0, fix(&D:: template LDRSH_REG<A1>) },
			{ 0x0fef0070, 0x01a00000, fix(&D:: template LSL_IMM<A1>) },
			{ 0x0fef00f0, 0x01a00010, fix(&D:: template LSL_REG<A1>) },
			{ 0x0fef0030, 0x01a00020, fix(&D:: template LSR_IMM<A1>) },
			{ 0x0fef00f0, 0x01a00030, fix(&D:: template LSR_REG<A1>) },
			{ 0x0fe000f0, 0x00200090, fix(&D:: template MLA<A1>) },
			{ 0x0ff000f0, 0x00600090, fix(&D:: template MLS<A1>) },
			{ 0x0fef0000, 0x03a00000, fix(&D:: template MOV_IMM<A1>) },
			{ 0x0ff00000, 0x03000000, fix(&D:: template MOV_IMM<A2>) },
			{ 0x0fef0ff0, 0x01a00000, fix(&D:: template MOV_REG<A1>) },
			{ 0x0ff00000, 0x03400000, fix(&D:: template MOVT<A1>) },
			{ 0x0f100010, 0x0e100010, fix(&D:: template MRC_<A1>) },
			{ 0xff100010, 0xfe100010, fix(&D:: template MRC_<A2>) },
			{ 0x0fff0fff, 0x010f0000, fix(&D:: template MRS<A1>) },
			{ 0x0ff3f000, 0x0320f000, fix(&D:: template MSR_IMM<A1>) },
			{ 0x0ff3fff0, 0x0120f000, fix(&D:: template MSR_REG<A1>) },
			{ 0x0fe0f0f0, 0x00000090, fix(&D:: template MUL<A1>) },
			{ 0x0fef0000, 0x03e00000, fix(&D:: template MVN_IMM<A1>) },
			{ 0xffef0010, 0x01e00000, fix(&D:: template MVN_REG<A1>) },
			{ 0x0fef0090, 0x01e00010, fix(&D:: template MVN_RSR<A1>) },
			{ 0x0fffffff, 0x0320f000, fix(&D:: template NOP<A1>) },
			{ 0x0fe00000, 0x03800000, fix(&D:: template ORR_IMM<A1>) },
			{ 0x0fe00010, 0x01800000, fix(&D:: template ORR_REG<A1>) },
			{ 0x0fe00090, 0x01800010, fix(&D:: template ORR_RSR<A1>) },
			{ 0x0ff00030, 0x06800010, fix(&D:: template PKH<A1>) },
			{ 0x0fff0000, 0x08bd0000, fix(&D:: template POP<A1>) },
			{ 0x0fff0fff, 0x049d0004, fix(&D:: template POP<A2>) },
			{ 0x0fff0000, 0x092d0000, fix(&D:: template PUSH<A1>) },
			{ 0x0fff0fff, 0x052d0004, fix(&D:: template PUSH<A2>) },
			{ 0x0ff00ff0, 0x01000050, fix(&D:: template QADD<A1>) },
			{ 0x0ff00ff0, 0x06200f10, fix(&D:: template QADD16<A1>) },
			{ 0x0ff00ff0, 0x06200f90, fix(&D:: template QADD8<A1>) },
			{ 0x0ff00ff0, 0x06200f30, fix(&D:: template QASX<A1>) },
			{ 0x0ff00ff0, 0x01400050, fix(&D:: template QDADD<A1>) },
			{ 0x0ff00ff0, 0x01600050, fix(&D:: template QDSUB<A1>) },
			{ 0x0ff00ff0, 0x06200f50, fix(&D:: template QSAX<A1>) },
			{ 0x0ff00ff0, 0x01200050, fix(&D:: template QSUB<A1>) },
			{ 0x0ff00ff0, 0x06200f70, fix(&D:: template QSUB16<A1>) },
			{ 0x0ff00ff0, 0x06200ff0, fix(&D:: template QSUB8<A1>) },
			{ 0x0fff0ff0, 0x06ff0f30, fix(&D:: template RBIT<A1>) },
			{ 0x0fff0ff0, 0x06bf0f30, fix(&D:: template REV<A1>) },
			{ 0x0fff0ff0, 0x06bf0fb0, fix(&D:: template REV16<A1>) },
			{ 0x0fff0ff0, 0x06ff0fb0, fix(&D:: template REVSH<A1>) },
			{ 0x0fef0070, 0x01a00060, fix(&D:: template ROR_IMM<A1>) },
			{ 0x0fef00f0, 0x01a00070, fix(&D:: template ROR_REG<A1>) },
			{ 0x0fef0ff0, 0x01a00060, fix(&D:: template RRX<A1>) },
			{ 0x0fe00000, 0x02600000, fix(&D:: template RSB_IMM<A1>) },
			{ 0x0fe00010, 0x00600000, fix(&D:: template RSB_REG<A1>) },
			{ 0x0fe00090, 0x00600010, fix(&D:: template RSB_RSR<A1>) },
			{ 0x0fe00000, 0x02e00000, fix(&D:: template RSC_IMM<A1>) },
			{ 0x0fe00010, 0x00e00000, fix(&D:: template RSC_REG<A1>) },
			{ 0x0fe00090, 0x00e00010, fix(&D:: template RSC_RSR<A1>) },
			{ 0x0ff00ff0, 0x06100f10, fix(&D:: template SADD16<A1>) },
			{ 0x0ff00ff0, 0x06100f90, fix(&D:: template SADD8<A1>) },
			{ 0x0ff00ff0, 0x06100f30, fix(&D:: template SASX<A1>) },
			{ 0x0fe00000, 0x02c00000, fix(&D:: template SBC_IMM<A1>) },
			{ 0x0fe00010, 0x00c00000, fix(&D:: template SBC_REG<A1>) },
			{ 0x0fe00090, 0x00c00010, fix(&D:: template SBC_RSR<A1>) },
			{ 0x0fe00070, 0x07a00050, fix(&D:: template SBFX<A1>) },
			{ 0x0ff00ff0, 0x06800fb0, fix(&D:: template SEL<A1>) },
			{ 0x0ff00ff0, 0x06300f10, fix(&D:: template SHADD16<A1>) },
			{ 0x0ff00ff0, 0x06300f90, fix(&D:: template SHADD8<A1>) },
			{ 0x0ff00ff0, 0x06300f30, fix(&D:: template SHASX<A1>) },
			{ 0x0ff00ff0, 0x06300f50, fix(&D:: template SHSAX<A1>) },
			{ 0x0ff00ff0, 0x06300f70, fix(&D:: template SHSUB16<A1>) },
			{ 0x0ff00ff0, 0x06300ff0, fix(&D:: template SHSUB8<A1>) },
			{ 0x0ff00090, 0x01000080, fix(&D:: template SMLA__<A1>) },
			{ 0x0ff000d0, 0x07000010, fix(&D:: template SMLAD<A1>) },
			{ 0x0fe000f0, 0x00e00090, fix(&D:: template SMLAL<A1>) },
			{ 0x0ff00090, 0x01400080, fix(&D:: template SMLAL__<A1>) },
			{ 0x0ff000d0, 0x07400010, fix(&D:: template SMLALD<A1>) },
			{ 0x0ff000b0, 0x01200080, fix(&D:: template SMLAW_<A1>) },
			{ 0x0ff000d0, 0x07000050, fix(&D:: template SMLSD<A1>) },
			{ 0x0ff000d0, 0x07400050, fix(&D:: template SMLSLD<A1>) },
			{ 0x0ff000d0, 0x07500010, fix(&D:: template SMMLA<A1>) },
			{ 0x0ff000d0, 0x075000d0, fix(&D:: template SMMLS<A1>) },
			{ 0x0ff0f0d0, 0x0750f010, fix(&D:: template SMMUL<A1>) },
			{ 0x0ff0f0d0, 0x0700f010, fix(&D:: template SMUAD<A1>) },
			{ 0x0ff0f090, 0x01600080, fix(&D:: template SMUL__<A1>) },
			{ 0x0fe000f0, 0x00c00090, fix(&D:: template SMULL<A1>) },
			{ 0x0ff0f0b0, 0x012000a0, fix(&D:: template SMULW_<A1>) },
			{ 0x0ff0f0d0, 0x0700f050, fix(&D:: template SMUSD<A1>) },
			{ 0x0fe00030, 0x06a00010, fix(&D:: template SSAT<A1>) },
			{ 0x0ff00ff0, 0x06a00f30, fix(&D:: template SSAT16<A1>) },
			{ 0x0ff00ff0, 0x06100f50, fix(&D:: template SSAX<A1>) },
			{ 0x0ff00ff0, 0x06100f70, fix(&D:: template SSUB16<A1>) },
			{ 0x0ff00ff0, 0x06100ff0, fix(&D:: template SSUB8<A1>) },
			{ 0x0fd00000, 0x08800000, fix(&D:: template STM<A1>) },
			{ 0x0fd00000, 0x08000000, fix(&D:: template STMDA<A1>) },
			{ 0x0fd00000, 0x09000000, fix(&D:: template STMDB<A1>) },
			{ 0x0fd00000, 0x09800000, fix(&D:: template STMIB<A1>) },
			{ 0x0e500000, 0x04000000, fix(&D:: template STR_IMM<A1>) },
			{ 0x0e500010, 0x06000000, fix(&D:: template STR_REG<A1>) },
			{ 0x0e500000, 0x04400000, fix(&D:: template STRB_IMM<A1>) },
			{ 0x0e500010, 0x06400000, fix(&D:: template STRB_REG<A1>) },
			{ 0x0e5000f0, 0x004000f0, fix(&D:: template STRD_IMM<A1>) },
			{ 0x0e500ff0, 0x000000f0, fix(&D:: template STRD_REG<A1>) },
			{ 0x0ff00ff0, 0x01800f90, fix(&D:: template STREX<A1>) },
			{ 0x0ff00ff0, 0x01c00f90, fix(&D:: template STREXB<A1>) },
			{ 0x0ff00ff0, 0x01a00f90, fix(&D:: template STREXD<A1>) },
			{ 0x0ff00ff0, 0x01e00f90, fix(&D:: template STREXH<A1>) },
			{ 0x0e5000f0, 0x004000b0, fix(&D:: template STRH_IMM<A1>) },
			{ 0x0e500ff0, 0x000000b0, fix(&D:: template STRH_REG<A1>) },
			{ 0x0fe00000, 0x02400000, fix(&D:: template SUB_IMM<A1>) },
			{ 0x0fe00010, 0x00400000, fix(&D:: template SUB_REG<A1>) },
			{ 0x0fe00090, 0x00400010, fix(&D:: template SUB_RSR<A1>) },
			{ 0x0fef0000, 0x024d0000, fix(&D:: template SUB_SPI<A1>) },
			{ 0x0fef0010, 0x004d0000, fix(&D:: template SUB_SPR<A1>) },
			{ 0x0f000000, 0x0f000000, fix(&D:: template SVC<A1>) },
			{ 0x0ff003f0, 0x06a00070, fix(&D:: template SXTAB<A1>) },
			{ 0x0ff003f0, 0x06800070, fix(&D:: template SXTAB16<A1>) },
			{ 0x0ff003f0, 0x06b00070, fix(&D:: template SXTAH<A1>) },
			{ 0x0fff03f0, 0x06af0070, fix(&D:: template SXTB<A1>) },
			{ 0x0fff03f0, 0x068f0070, fix(&D:: template SXTB16<A1>) },
			{ 0x0fff03f0, 0x06bf0070, fix(&D:: template SXTH<A1>) },
			{ 0x0ff0f000, 0x03300000, fix(&D:: template TEQ_IMM<A1>) },
			{ 0x0ff0f010, 0x01300000, fix(&D:: template TEQ_REG<A1>) },
			{ 0x0ff0f090, 0x01300010, fix(&D:: template TEQ_RSR<A1>) },
			{ 0x0ff0f000, 0x03100000, fix(&D:: template TST_IMM<A1>) },
			{ 0x0ff0f010, 0x01100000, fix(&D:: template TST_REG<A1>) },
			{ 0x0ff0f090, 0x01100010, fix(&D:: template TST_RSR<A1>) },
			{ 0x0ff00ff0, 0x06500f10, fix(&D:: template UADD16<A1>) },
			{ 0x0ff00ff0, 0x06500f90, fix(&D:: template UADD8<A1>) },
			{ 0x0ff00ff0, 0x06500f30, fix(&D:: template UASX<A1>) },
			{ 0x0fe00070, 0x07e00050, fix(&D:: template UBFX<A1>) },
			{ 0x0ff00ff0, 0x06700f10, fix(&D:: template UHADD16<A1>) },
			{ 0x0ff00ff0, 0x06700f90, fix(&D:: template UHADD8<A1>) },
			{ 0x0ff00ff0, 0x06700f30, fix(&D:: template UHASX<A1>) },
			{ 0x0ff00ff0, 0x06700f50, fix(&D:: template UHSAX<A1>) },
			{ 0x0ff00ff0, 0x06700f70, fix(&D:: template UHSUB16<A1>) },
			{ 0x0ff00ff0, 0x06700ff0, fix(&D:: template UHSUB8<A1>) },
			{ 0x0ff000f0, 0x00400090, fix(&D:: template UMAAL<A1>) },
			{ 0x0fe000f0, 0x00a00090, fix(&D:: template UMLAL<A1>) },
			{ 0x0fe000f0, 0x00800090, fix(&D:: template UMULL<A1>) },
			{ 0x0ff00ff0, 0x06600f10, fix(&D:: template UQADD16<A1>) },
			{ 0x0ff00ff0, 0x06600f90, fix(&D:: template UQADD8<A1>) },
			{ 0x0ff00ff0, 0x06600f30, fix(&D:: template UQASX<A1>) },
			{ 0x0ff00ff0, 0x06600f50, fix(&D:: template UQSAX<A1>) },
			{ 0x0ff00ff0, 0x06600f70, fix(&D:: template UQSUB16<A1>) },
			{ 0x0ff00ff0, 0x06600ff0, fix(&D:: template UQSUB8<A1>) },
			{ 0x0ff0f0f0, 0x0780f010, fix(&D:: template USAD8<A1>) },
			{ 0x0ff000f0, 0x07800010, fix(&D:: template USADA8<A1>) },
			{ 0x0fe00030, 0x06e00010, fix(&D:: template USAT<A1>) },
			{ 0x0ff00ff0, 0x06e00f30, fix(&D:: template USAT16<A1>) },
			{ 0x0ff00ff0, 0x06500f50, fix(&D:: template USAX<A1>) },
			{ 0x0ff00ff0, 0x06500f70, fix(&D:: template USUB16<A1>) },
			{ 0x0ff00ff0, 0x06500ff0, fix(&D:: template USUB8<A1>) },
			{ 0x0ff003f0, 0x06e00070, fix(&D:: template UXTAB<A1>), SKIP_IF(BF(16, 19) == 15) },
			{ 0x0ff003f0, 0x06c00070, fix(&D:: template UXTAB16<A1>) },
			{ 0x0ff003f0, 0x06f00070, fix(&D:: template UXTAH<A1>) },
			{ 0x0fff03f0, 0x06ef0070, fix(&D:: template UXTB<A1>) },
			{ 0x0fff03f0, 0x06cf0070, fix(&D:: template UXTB16<A1>) },
			{ 0x0fff03f0, 0x06ff0070, fix(&D:: template UXTH<A1>) },
			{ 0xfe800f10, 0xf2000710, fix(&D:: template VABA_<A1>) },
			{ 0xfe800f50, 0xf2800500, fix(&D:: template VABA_<A2>) },
			{ 0xfe800f10, 0xf2000700, fix(&D:: template VABD_<A1>) },
			{ 0xfe800f50, 0xf2800700, fix(&D:: template VABD_<A2>) },
			{ 0xffa00f10, 0xf3200d00, fix(&D:: template VABD_FP<A1>) },
			{ 0xffb30b90, 0xf3b10300, fix(&D:: template VABS<A1>) },
			{ 0x0fbf0ed0, 0x0eb00ac0, fix(&D:: template VABS<A2>) },
			{ 0xff800f10, 0xf3000e10, fix(&D:: template VAC__<A1>) },
			{ 0xff800f10, 0xf2000800, fix(&D:: template VADD<A1>) },
			{ 0xffa00f10, 0xf2000d00, fix(&D:: template VADD_FP<A1>) },
			{ 0x0fb00e50, 0x0e300a00, fix(&D:: template VADD_FP<A2>) },
			{ 0xff800f50, 0xf2800400, fix(&D:: template VADDHN<A1>) },
			{ 0xfe800e50, 0xf2800000, fix(&D:: template VADD_<A1>) },
			{ 0xffb00f10, 0xf2000110, fix(&D:: template VAND<A1>) },
			{ 0xfeb000b0, 0xf2800030, fix(&D:: template VBIC_IMM<A1>) },
			{ 0xffb00f10, 0xf2100110, fix(&D:: template VBIC_REG<A1>) },
			{ 0xff800f10, 0xf3000110, fix(&D:: template VB__<A1>) },
			{ 0xff800f10, 0xf3000810, fix(&D:: template VCEQ_REG<A1>) },
			{ 0xffa00f10, 0xf2000e00, fix(&D:: template VCEQ_REG<A2>) },
			{ 0xffb30b90, 0xf3b10100, fix(&D:: template VCEQ_ZERO<A1>) },
			{ 0xfe800f10, 0xf2000310, fix(&D:: template VCGE_REG<A1>) },
			{ 0xffa00f10, 0xf3000e00, fix(&D:: template VCGE_REG<A2>) },
			{ 0xffb30b90, 0xf3b10080, fix(&D:: template VCGE_ZERO<A1>) },
			{ 0xfe800f10, 0xf2000300, fix(&D:: template VCGT_REG<A1>) },
			{ 0xffa00f10, 0xf3200e00, fix(&D:: template VCGT_REG<A2>) },
			{ 0xffb30b90, 0xf3b10000, fix(&D:: template VCGT_ZERO<A1>) },
			{ 0xffb30b90, 0xf3b10180, fix(&D:: template VCLE_ZERO<A1>) },
			{ 0xffb30f90, 0xf3b00400, fix(&D:: template VCLS<A1>) },
			{ 0xffb30b90, 0xf3b10200, fix(&D:: template VCLT_ZERO<A1>) },
			{ 0xffb30f90, 0xf3b00480, fix(&D:: template VCLZ<A1>) },
			{ 0x0fbf0e50, 0x0eb40a40, fix(&D:: template VCMP_<A1>) },
			{ 0x0fbf0e7f, 0x0eb50a40, fix(&D:: template VCMP_<A2>) },
			{ 0xffb30f90, 0xf3b00500, fix(&D:: template VCNT<A1>) },
			{ 0xffb30e10, 0xf3b30600, fix(&D:: template VCVT_FIA<A1>) },
			{ 0x0fb80e50, 0x0eb80a40, fix(&D:: template VCVT_FIF<A1>) },
			{ 0xfe800e90, 0xf2800e10, fix(&D:: template VCVT_FFA<A1>) },
			{ 0x0fba0e50, 0x0eba0a40, fix(&D:: template VCVT_FFF<A1>) },
			{ 0x0fbf0ed0, 0x0eb70ac0, fix(&D:: template VCVT_DF<A1>) },
			{ 0xffb30ed0, 0xf3b20600, fix(&D:: template VCVT_HFA<A1>) },
			{ 0x0fbe0f50, 0x0eb20a40, fix(&D:: template VCVT_HFF<A1>) },
			{ 0x0fb00e50, 0x0e800a00, fix(&D:: template VDIV<A1>) },
			{ 0xffb00f90, 0xf3b00c00, fix(&D:: template VDUP_S<A1>) },
			{ 0x0f900f5f, 0x0e800b10, fix(&D:: template VDUP_R<A1>) },
			{ 0xffb00f10, 0xf3000110, fix(&D:: template VEOR<A1>) },
			{ 0xffb00010, 0xf2b00000, fix(&D:: template VEXT<A1>) },
			{ 0xfe800b10, 0xf2000000, fix(&D:: template VHADDSUB<A1>) },
			{ 0xffb00000, 0xf4200000, fix(&D:: template VLD__MS<A1>) },
			{ 0xffb00f00, 0xf4a00c00, fix(&D:: template VLD1_SAL<A1>) },
			{ 0xffb00300, 0xf4a00000, fix(&D:: template VLD1_SL<A1>) },
			{ 0xffb00f00, 0xf4a00d00, fix(&D:: template VLD2_SAL<A1>) },
			{ 0xffb00300, 0xf4a00100, fix(&D:: template VLD2_SL<A1>) },
			{ 0xffb00f00, 0xf4a00e00, fix(&D:: template VLD3_SAL<A1>) },
			{ 0xffb00300, 0xf4a00200, fix(&D:: template VLD3_SL<A1>) },
			{ 0xffb00f00, 0xf4a00f00, fix(&D:: template VLD4_SAL<A1>) },
			{ 0xffb00300, 0xf4a00300, fix(&D:: template VLD4_SL<A1>) },
			{ 0x0e100f00, 0x0c100b00, fix(&D:: template VLDM<A1>) },
			{ 0x0e100f00, 0x0c100a00, fix(&D:: template VLDM<A2>) },
			{ 0x0f300f00, 0x0d100b00, fix(&D:: template VLDR<A1>) },
			{ 0x0f300f00, 0x0d100a00, fix(&D:: template VLDR<A2>) },
			{ 0xfe800f00, 0xf2000600, fix(&D:: template VMAXMIN<A1>) },
			{ 0xff800f10, 0xf2000f00, fix(&D:: template VMAXMIN_FP<A1>) },
			{ 0xfe800f10, 0xf2000900, fix(&D:: template VML__<A1>) },
			{ 0xfe800d50, 0xf2800800, fix(&D:: template VML__<A2>) },
			{ 0xff800f10, 0xf2000d10, fix(&D:: template VML__FP<A1>) },
			{ 0x0fb00e10, 0x0e000a00, fix(&D:: template VML__FP<A2>) },
			{ 0xfe800a50, 0xf2800040, fix(&D:: template VML__S<A1>) },
			{ 0xfe800b50, 0xf2800240, fix(&D:: template VML__S<A2>) },
			{ 0xfeb80090, 0xf2800010, fix(&D:: template VMOV_IMM<A1>) },
			{ 0x0fb00ef0, 0x0eb00a00, fix(&D:: template VMOV_IMM<A2>) },
			{ 0xffb00f10, 0xf2200110, fix(&D:: template VMOV_REG<A1>) },
			{ 0x0fbf0ed0, 0x0eb00a40, fix(&D:: template VMOV_REG<A2>) },
			{ 0x0f900f1f, 0x0e000b10, fix(&D:: template VMOV_RS<A1>) },
			{ 0x0f100f1f, 0x0e100b10, fix(&D:: template VMOV_SR<A1>) },
			{ 0x0fe00f7f, 0x0e000a10, fix(&D:: template VMOV_RF<A1>) },
			{ 0x0fe00fd0, 0x0c400a10, fix(&D:: template VMOV_2RF<A1>) },
			{ 0x0fe00fd0, 0x0c400b10, fix(&D:: template VMOV_2RD<A1>) },
			{ 0xfe870fd0, 0xf2800a10, fix(&D:: template VMOVL<A1>) },
			{ 0xffb30fd0, 0xf3b20200, fix(&D:: template VMOVN<A1>) },
			{ 0x0fff0fff, 0x0ef10a10, fix(&D:: template VMRS<A1>) },
			{ 0x0fff0fff, 0x0ee10a10, fix(&D:: template VMSR<A1>) },
			{ 0xfe800f10, 0xf2000910, fix(&D:: template VMUL_<A1>) },
			{ 0xfe800d50, 0xf2800c00, fix(&D:: template VMUL_<A2>) },
			{ 0xffa00f10, 0xf3000d10, fix(&D:: template VMUL_FP<A1>) },
			{ 0x0fb00e50, 0x0e200a00, fix(&D:: template VMUL_FP<A2>) },
			{ 0xfe800e50, 0xf2800840, fix(&D:: template VMUL_S<A1>) },
			{ 0xfe800f50, 0xf2800a40, fix(&D:: template VMUL_S<A2>) },
			{ 0xfeb800b0, 0xf2800030, fix(&D:: template VMVN_IMM<A1>) },
			{ 0xffb30f90, 0xf3b00580, fix(&D:: template VMVN_REG<A1>) },
			{ 0xffb30b90, 0xf3b10380, fix(&D:: template VNEG<A1>) },
			{ 0x0fbf0ed0, 0x0eb10a40, fix(&D:: template VNEG<A2>) },
			{ 0x0fb00e10, 0x0e100a00, fix(&D:: template VNM__<A1>) },
			{ 0x0fb00e50, 0x0e200a40, fix(&D:: template VNM__<A2>) },
			{ 0xffb00f10, 0xf2300110, fix(&D:: template VORN_REG<A1>) },
			{ 0xfeb800b0, 0xf2800010, fix(&D:: template VORR_IMM<A1>) },
			{ 0xffb00f10, 0xf2200110, fix(&D:: template VORR_REG<A1>) },
			{ 0xffb30f10, 0xf3b00600, fix(&D:: template VPADAL<A1>) },
			{ 0xff800f10, 0xf2000b10, fix(&D:: template VPADD<A1>) },
			{ 0xffa00f10, 0xf3000d00, fix(&D:: template VPADD_FP<A1>) },
			{ 0xffb30f10, 0xf3b00200, fix(&D:: template VPADDL<A1>) },
			{ 0xfe800f00, 0xf2000a00, fix(&D:: template VPMAXMIN<A1>) },
			{ 0xff800f10, 0xf3000f00, fix(&D:: template VPMAXMIN_FP<A1>) },
			{ 0x0fbf0f00, 0x0cbd0b00, fix(&D:: template VPOP<A1>) },
			{ 0x0fbf0f00, 0x0cbd0a00, fix(&D:: template VPOP<A2>) },
			{ 0x0fbf0f00, 0x0d2d0b00, fix(&D:: template VPUSH<A1>) },
			{ 0x0fbf0f00, 0x0d2d0a00, fix(&D:: template VPUSH<A2>) },
			// TODO: VQ* instructions
			{ 0xff800f50, 0xf3800400, fix(&D:: template VRADDHN<A1>) },
			{ 0xffb30e90, 0xf3b30400, fix(&D:: template VRECPE<A1>) },
			{ 0xffa00f10, 0xf2000f10, fix(&D:: template VRECPS<A1>) },
			{ 0xffb30e10, 0xf3b00000, fix(&D:: template VREV__<A1>) },
			{ 0xfe800f10, 0xf2000100, fix(&D:: template VRHADD<A1>) },
			{ 0xfe800f10, 0xf2000500, fix(&D:: template VRSHL<A1>) },
			{ 0xfe800f10, 0xf2800210, fix(&D:: template VRSHR<A1>) },
			{ 0xff800fd0, 0xf2800850, fix(&D:: template VRSHRN<A1>) },
			{ 0xffb30e90, 0xf3b30480, fix(&D:: template VRSQRTE<A1>) },
			{ 0xffa00f10, 0xf2200f10, fix(&D:: template VRSQRTS<A1>) },
			{ 0xfe800f10, 0xf2800310, fix(&D:: template VRSRA<A1>) },
			{ 0xff800f50, 0xf3800600, fix(&D:: template VRSUBHN<A1>) },
			{ 0xff800f10, 0xf2800510, fix(&D:: template VSHL_IMM<A1>) },
			{ 0xfe800f10, 0xf2000400, fix(&D:: template VSHL_REG<A1>) },
			{ 0xfe800fd0, 0xf2800a10, fix(&D:: template VSHLL<A1>) },
			{ 0xffb30fd0, 0xf3b20300, fix(&D:: template VSHLL<A2>) },
			{ 0xfe800f10, 0xf2800010, fix(&D:: template VSHR<A1>) },
			{ 0xff800fd0, 0xf2800810, fix(&D:: template VSHRN<A1>) },
			{ 0xff800f10, 0xf3800510, fix(&D:: template VSLI<A1>) },
			{ 0x0fbf0ed0, 0x0eb10ac0, fix(&D:: template VSQRT<A1>) },
			{ 0xfe800f10, 0xf2800110, fix(&D:: template VSRA<A1>) },
			{ 0xff800f10, 0xf3800410, fix(&D:: template VSRI<A1>) },
			{ 0xffb00000, 0xf4000000, fix(&D:: template VST__MS<A1>) },
			{ 0xffb00300, 0xf4800000, fix(&D:: template VST1_SL<A1>) },
			{ 0xffb00300, 0xf4800100, fix(&D:: template VST2_SL<A1>) },
			{ 0xffb00300, 0xf4800200, fix(&D:: template VST3_SL<A1>) },
			{ 0xffb00300, 0xf4800300, fix(&D:: template VST4_SL<A1>) },
			{ 0x0e100f00, 0x0c000b00, fix(&D:: template VSTM<A1>) },
			{ 0x0e100f00, 0x0c000a00, fix(&D:: template VSTM<A2>) },
			{ 0x0f300f00, 0x0d000b00, fix(&D:: template VSTR<A1>) },
			{ 0x0f300f00, 0x0d000a00, fix(&D:: template VSTR<A2>) },
			{ 0xff800f10, 0xf3000800, fix(&D:: template VSUB<A1>) },
			{ 0xffa00f10, 0xf2200d00, fix(&D:: template VSUB_FP<A1>) },
			{ 0x0fb00e50, 0x0e300a40, fix(&D:: template VSUB_FP<A2>) },
			{ 0xff800f50, 0xf2800600, fix(&D:: template VSUBHN<A1>) },
			{ 0xfe800e50, 0xf2800200, fix(&D:: template VSUB_<A1>) },
			{ 0xffb30f90, 0xf3b20000, fix(&D:: template VSWP<A1>) },
			{ 0xffb00c10, 0xf3b00800, fix(&D:: template VTB_<A1>) },
			{ 0xffb30f90, 0xf3b20080, fix(&D:: template VTRN<A1>) },
			{ 0xff800f10, 0xf2000810, fix(&D:: template VTST<A1>) },
			{ 0xffb30f90, 0xf3b20100, fix(&D:: template VUZP<A1>) },
			{ 0xffb30f90, 0xf3b20180, fix(&D:: template VZIP<A1>) },
			{ 0x0fffffff, 0x0320f002, fix(&D:: template WFE<A1>) },
			{ 0x0fffffff, 0x0320f003, fix(&D:: template WFI<A1>) },
			{ 0x0fffffff, 0x0320f001, fix(&D:: template YIELD<A1>) },
		});

		m_op32_table.fill(m_op32_list.cbegin());

		for (u32 i = 0; i < 0x10000; i++)
		{
			for (auto& opcode : m_op16_list)
			{
				if (opcode.match(i))
				{
					m_op16_table[i] = opcode.pointer;
					break;
				}
			}

			if (!m_op16_table[i] && !arm_op_thumb_is_32(i))
			{
				m_op16_table[i] = &D::UNK;
			}
		}

		std::set<std::set<const instruction_info*>> result;
		
		for (u32 i = 0xe800; i < 0x10000; i++)
		{
			if (m_op16_table[i]) LOG_ERROR(ARMv7, "Invalid m_op16_table entry 0x%04x", i);

			//std::set<const instruction_info*> matches;

			//for (u32 j = 0; j < 0x10000; j++)
			//{
			//	for (auto& o : m_op32_list)
			//	{
			//		if (o.match(i << 16 | j))
			//		{
			//			matches.emplace(&o);
			//			break;
			//		}
			//	}
			//}

			//result.emplace(std::move(matches));
		}

		for (const auto& s : result)
		{
			LOG_NOTICE(ARMv7, "Set found (%u):", s.size());
			for (const auto& e : s)
			{
				LOG_NOTICE(ARMv7, "** 0x%08x, 0x%08x", e->mask, e->code);
			}
		}
	}

	// First chance
	T decode_thumb(u16 op16) const
	{
		return m_op16_table[op16];
	}

	// Second step
	T decode_thumb(u32 op32) const
	{
		for (auto i = m_op32_table[op32 >> 16], end = m_op32_list.end(); i != end; i++)
		{
			if (i->match(op32))
			{
				return i->pointer;
			}
		}

		return &D::UNK;
	}

	T decode_arm(u32 op) const
	{
		for (auto& i : m_arm_list)
		{
			if (i.match(op))
			{
				return i.pointer;
			}
		}

		return &D::UNK;
	}
};

#undef SKIP_IF
#undef BF
#undef BT
