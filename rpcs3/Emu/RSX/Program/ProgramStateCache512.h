#pragma once

#include "ProgramStateCache.h"

#ifdef ARCH_X64
#include "immintrin.h"

#if defined(_MSC_VER) && !defined(__clang__)
#define AVX512_ICL_FUNC
#else
#define AVX512_ICL_FUNC __attribute__((__target__("avx512f,avx512bw,avx512dq,avx512cd,avx512vl,avx512bitalg,avx512ifma,avx512vbmi,avx512vbmi2,avx512vnni,avx512vpopcntdq")))
#endif

static AVX512_ICL_FUNC NEVER_INLINE usz get_vertex_program_ucode_hash_512(const RSXVertexProgram &program)
{
	// Load all elements of the instruction_mask bitset
	const __m512i* instMask512 = utils::bless<const __m512i>(&program.instruction_mask);
	const __m128i* instMask128 = utils::bless<const __m128i>(&program.instruction_mask);

	const __m512i lowerMask = _mm512_loadu_si512(instMask512);
	const __m128i upper128 = _mm_loadu_si128(instMask128 + 4);
	const __m512i upperMask = _mm512_zextsi128_si512(upper128);

	__m512i maskIndex = _mm512_setzero_si512();
	const __m512i negativeOnes = _mm512_set1_epi64(-1);

	// Special masks to test against bitset
	const __m512i testMask0 = _mm512_set_epi64(
	0x0808080808080808,
	0x0808080808080808,
	0x0404040404040404,
	0x0404040404040404,
	0x0202020202020202,
	0x0202020202020202,
	0x0101010101010101,
	0x0101010101010101);

	const __m512i testMask1 = _mm512_set_epi64(
	0x8080808080808080,
	0x8080808080808080,
	0x4040404040404040,
	0x4040404040404040,
	0x2020202020202020,
	0x2020202020202020,
	0x1010101010101010,
	0x1010101010101010);

	const __m512i* instBuffer = reinterpret_cast<const __m512i*>(program.data.data());
	__m512i acc0 = _mm512_setzero_si512();
	__m512i acc1 = _mm512_setzero_si512();

	__m512i rotMask0 = _mm512_set_epi64(7, 6, 5, 4, 3, 2, 1, 0);
	__m512i rotMask1 = _mm512_set_epi64(15, 14, 13, 12, 11, 10, 9, 8);
	const __m512i rotMaskAdd = _mm512_set_epi64(16, 16, 16, 16, 16, 16, 16, 16);

	u32 instIndex = 0;

	// If there is remainder, add an extra (masked) iteration
	const u32 extraIteration = (program.data.size() % 32 != 0) ? 1 : 0;
	const u32 length = static_cast<u32>(program.data.size() / 32) + extraIteration;

	// The instruction mask will prevent us from reading out of bounds, we do not need a seperate masked loop
	// for the remainder, or a scalar loop.
	while (instIndex < (length))
	{
		const __m512i masks = _mm512_permutex2var_epi8(lowerMask, maskIndex, upperMask);
		const __mmask8 result0 = _mm512_test_epi64_mask(masks, testMask0);
		const __mmask8 result1 = _mm512_test_epi64_mask(masks, testMask1);
		const __m512i load0 = _mm512_maskz_loadu_epi64(result0, (instBuffer + instIndex * 2));
		const __m512i load1 = _mm512_maskz_loadu_epi64(result1, (instBuffer + (instIndex * 2)+ 1));

		const __m512i rotated0 = _mm512_rorv_epi64(load0, rotMask0);
		const __m512i rotated1 = _mm512_rorv_epi64(load1, rotMask1);

		acc0 = _mm512_add_epi64(acc0, rotated0);
		acc1 = _mm512_add_epi64(acc1, rotated1);

		rotMask0 = _mm512_add_epi64(rotMask0, rotMaskAdd);
		rotMask1 = _mm512_add_epi64(rotMask1, rotMaskAdd);
		maskIndex = _mm512_sub_epi8(maskIndex, negativeOnes);

		instIndex++;
	}

	const __m512i result = _mm512_add_epi64(acc0, acc1);
	return _mm512_reduce_add_epi64(result);
}

static AVX512_ICL_FUNC NEVER_INLINE bool vertex_program_compare_512(const RSXVertexProgram &binary1, const RSXVertexProgram &binary2)
{
	// Load all elements of the instruction_mask bitset
	const __m512i* instMask512 = utils::bless<const __m512i>(&binary1.instruction_mask);
	const __m128i* instMask128 = utils::bless<const __m128i>(&binary1.instruction_mask);

	const __m512i lowerMask = _mm512_loadu_si512(instMask512);
	const __m128i upper128 = _mm_loadu_si128(instMask128 + 4);
	const __m512i upperMask = _mm512_zextsi128_si512(upper128);

	__m512i maskIndex = _mm512_setzero_si512();
	const __m512i negativeOnes = _mm512_set1_epi64(-1);

	// Special masks to test against bitset
	const __m512i testMask0 = _mm512_set_epi64(
	0x0808080808080808,
	0x0808080808080808,
	0x0404040404040404,
	0x0404040404040404,
	0x0202020202020202,
	0x0202020202020202,
	0x0101010101010101,
	0x0101010101010101);

	const __m512i testMask1 = _mm512_set_epi64(
	0x8080808080808080,
	0x8080808080808080,
	0x4040404040404040,
	0x4040404040404040,
	0x2020202020202020,
	0x2020202020202020,
	0x1010101010101010,
	0x1010101010101010);

	const __m512i* instBuffer1 = reinterpret_cast<const __m512i*>(binary1.data.data());
	const __m512i* instBuffer2 = reinterpret_cast<const __m512i*>(binary2.data.data());

	// If there is remainder, add an extra (masked) iteration
	const u32 extraIteration = (binary1.data.size() % 32 != 0) ? 1 : 0;
	const u32 length = static_cast<u32>(binary1.data.size() / 32) + extraIteration;

	u32 instIndex = 0;

	// The instruction mask will prevent us from reading out of bounds, we do not need a seperate masked loop
	// for the remainder, or a scalar loop.
	while (instIndex < (length))
	{
		const __m512i masks = _mm512_permutex2var_epi8(lowerMask, maskIndex, upperMask);

		const __mmask8 result0 = _mm512_test_epi64_mask(masks, testMask0);
		const __mmask8 result1 = _mm512_test_epi64_mask(masks, testMask1);

		const __m512i load0 = _mm512_maskz_loadu_epi64(result0, (instBuffer1 + (instIndex * 2)));
		const __m512i load1 = _mm512_maskz_loadu_epi64(result0, (instBuffer2 + (instIndex * 2)));
		const __m512i load2 = _mm512_maskz_loadu_epi64(result1, (instBuffer1 + (instIndex * 2) + 1));
		const __m512i load3 = _mm512_maskz_loadu_epi64(result1, (instBuffer2 + (instIndex * 2)+ 1));

		const __mmask8 res0 = _mm512_cmpneq_epi64_mask(load0, load1);
		const __mmask8 res1 = _mm512_cmpneq_epi64_mask(load2, load3);

		const u8 result = _kortestz_mask8_u8(res0, res1);

		//kortestz will set result to 1 if all bits are zero, so invert the check for result
		if (!result)
		{
			return false;
		}

		maskIndex = _mm512_sub_epi8(maskIndex, negativeOnes);

		instIndex++;
	}

	return true;
}
#undef AVX512_ICL_FUNC
#endif
