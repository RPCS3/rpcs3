#pragma once

#include "ProgramStateCache.h"

#include <array>

#ifdef ARCH_X64
#include "immintrin.h"

#if defined(_MSC_VER) && !defined(__clang__)
#define AVX512_ICL_FUNC
#else
#define AVX512_ICL_FUNC __attribute__((__target__("bmi2,avx512f,avx512bw,avx512dq,avx512cd,avx512vl,avx512bitalg,avx512ifma,avx512vbmi,avx512vbmi2,avx512vnni,avx512vpopcntdq")))
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

// Gives a result from 8 instruction slots
// We need table to resolve conflicts where a "constant" may have been decoded as an instruction with a constant in the next slot
constexpr auto fragment_512_table = []
{
	std::array<u64, 256> result{};
	for (u32 flags = 0; flags < 256; flags++)
	{
		for (u32 incoming = 0; incoming < 2; incoming++)
		{
			u32 skip = incoming, lanes = 0;
			for (u32 slot = 0; slot < 8; slot++)
			{
				if (skip)
				{
					skip = 0;
				}
				else
				{
					lanes |= 3u << (slot * 2);
					skip = (flags >> slot) & 1;
				}
			}
			result[flags] |= u64(lanes | (skip << 16)) << (incoming * 32);
		}
	}
	return result;
}();
static_assert(sizeof(fragment_512_table) == 2048);

static AVX512_ICL_FUNC u32 fragment_constant_slots(__m512i lo, __m512i hi)
{
	// Candidate instruction slots
	const auto lo_types = _mm512_and_si512(lo, _mm512_set1_epi32(0x300));
	const auto hi_types = _mm512_and_si512(hi, _mm512_set1_epi32(0x300));
	const u32 lo_matches = _mm512_cmpeq_epi32_mask(lo_types, _mm512_set1_epi32(0x200));
	const u32 hi_matches = _mm512_cmpeq_epi32_mask(hi_types, _mm512_set1_epi32(0x200));
	// Combine high and low matches
	const u32 matches = lo_matches | (hi_matches << 16);
	// We want pext to gather from just bit 3, but we need any candidate slot to be valid
	// Adding 0b0110 carries bits 1/2 into bit 3
	// We then need to OR bit 3 back into the result
	const u32 candidateBits = ((matches & 0x66666666u) + 0x66666666u) | matches;

	// Returns a value we can use to lookup into the table
	return _pext_u32(candidateBits, 0x88888888u);
}

static AVX512_ICL_FUNC NEVER_INLINE usz get_fragment_program_ucode_hash_512(const RSXFragmentProgram& program)
{
	// Each instruction slot occupies two 64-bit lanes
	const usz slots = program.ucode_length / 16;

	const u8* instBuffer = static_cast<const u8*>(program.get_data());
	__m512i acc0 = _mm512_setzero_si512();
	__m512i acc1 = _mm512_setzero_si512();

	__m512i rotMask0 = _mm512_set_epi64(7, 6, 5, 4, 3, 2, 1, 0);
	__m512i rotMask1 = _mm512_set_epi64(15, 14, 13, 12, 11, 10, 9, 8);
	const __m512i rotMaskAdd = _mm512_set_epi64(16, 16, 16, 16, 16, 16, 16, 16);

	// Carry skips a constant slot belonging to the previous group
	u32 carry = 0;

	const usz groupCount = (slots + 7) / 8;
	const usz tailSlots = slots % 8;

	// The final group loads only lanes within the program
	const u32 tailMask = tailSlots ? (1u << (tailSlots * 2)) - 1 : 0xffff;

	for (usz groupIndex = 0, instIndex = 0; groupIndex < groupCount; groupIndex++, instIndex += 8)
	{
		const u32 mask = groupIndex + 1 == groupCount ? tailMask : 0xffff;

		// Keep the high load pointer in bounds when its mask is zero
		const usz highOffset = mask > 0xff ? 64 : 0;
		const u8* instPtr = instBuffer + instIndex * 16;

		const __m512i load0 = _mm512_maskz_loadu_epi64(static_cast<__mmask8>(mask), instPtr);
		const __m512i load1 = _mm512_maskz_loadu_epi64(static_cast<__mmask8>(mask >> 8), instPtr + highOffset);

		// For which slots do we decode with a constant reference?
		const u32 flags = fragment_constant_slots(load0, load1);

		// Resolve possible conflicts from constants decoded as valid instruction
		// Results with carry from previous iteration select the other half of the table
		const u32 state = static_cast<u32>(fragment_512_table[flags] >> (carry * 32));

		// Select instruction lanes after excluding inline constants
		const u32 instructionMask = state & mask;
		carry = state >> 16;

		// Rotate by physical slot position, including skipped constants
		acc0 = _mm512_mask_add_epi64(acc0, static_cast<__mmask8>(instructionMask), acc0, _mm512_rorv_epi64(load0, rotMask0));
		acc1 = _mm512_mask_add_epi64(acc1, static_cast<__mmask8>(instructionMask >> 8), acc1, _mm512_rorv_epi64(load1, rotMask1));

		rotMask0 = _mm512_add_epi64(rotMask0, rotMaskAdd);
		rotMask1 = _mm512_add_epi64(rotMask1, rotMaskAdd);
	}

	return static_cast<usz>(_mm512_reduce_add_epi64(_mm512_add_epi64(acc0, acc1)));
}

static AVX512_ICL_FUNC NEVER_INLINE bool compare_fragment_program_ucode_512(const void* lhs, const void* rhs, usz slots)
{
	// Each instruction slot occupies two 64-bit lanes
	const u8* instBuffer1 = static_cast<const u8*>(lhs);
	const u8* instBuffer2 = static_cast<const u8*>(rhs);

	// Carry skips a constant slot belonging to the previous group
	u32 carry = 0;
	usz instIndex = 0;

	const usz groupCount = (slots + 7) / 8;
	const usz tailSlots = slots % 8;

	// The final group loads only lanes within the program
	const u32 tailMask = tailSlots ? (1u << (tailSlots * 2)) - 1 : 0xffff;

	for (usz groupIndex = 0; groupIndex < groupCount; groupIndex++, instIndex += 8)
	{
		const u32 mask = groupIndex + 1 == groupCount ? tailMask : 0xffff;

		// Keep the high load pointer in bounds when its mask is zero
		const usz highOffset = mask > 0xff ? 64 : 0;

		const __m512i load0 = _mm512_maskz_loadu_epi64(static_cast<__mmask8>(mask), instBuffer1 + instIndex * 16);
		const __m512i load1 = _mm512_maskz_loadu_epi64(static_cast<__mmask8>(mask), instBuffer2 + instIndex * 16);
		const __m512i load2 = _mm512_maskz_loadu_epi64(static_cast<__mmask8>(mask >> 8), instBuffer1 + instIndex * 16 + highOffset);
		const __m512i load3 = _mm512_maskz_loadu_epi64(static_cast<__mmask8>(mask >> 8), instBuffer2 + instIndex * 16 + highOffset);

		// For which slots do we decode with a constant reference?
		const u32 flags = fragment_constant_slots(load0, load2);

		// Resolve possible conflicts from constants decoded as valid instruction
		// Results with carry from previous iteration select the other half of the table
		const u32 state = static_cast<u32>(fragment_512_table[flags] >> (carry * 32));

		// Select instruction lanes after excluding inline constants
		const u32 instructionMask = state & mask;

		if (_mm512_mask_cmpneq_epi64_mask(static_cast<__mmask8>(instructionMask), load0, load1) ||
			_mm512_mask_cmpneq_epi64_mask(static_cast<__mmask8>(instructionMask >> 8), load2, load3))
		{
			return false;
		}

		carry = state >> 16;
	}

	return true;
}
#undef AVX512_ICL_FUNC
#endif
