#pragma once

#include <arm_sve.h>

#if defined(__clang__)
#define BUFFERUTILS_SVE __attribute__((target("sve")))
#else
#define BUFFERUTILS_SVE __attribute__((target("+sve")))
#endif

// Older compilers do not provide the svcompact_u16 intrinsic.
// Please remove this nonsense once we upgrade gcc in the CI - Whatcookie
#if (defined(__clang__) && __clang_major__ >= 22) || (!defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 17)
#define BUFFERUTILS_HAS_SVE2P2
#if defined(__clang__)
#define BUFFERUTILS_SVE2P2 __attribute__((target("sve2p2")))
#else
#define BUFFERUTILS_SVE2P2 __attribute__((target("+sve2p2")))
#endif
#endif

namespace
{
	BUFFERUTILS_SVE std::tuple<u32, u32, u32> upload_swapped_sve_skip_restart(std::span<to_be_t<const u32>> src, std::span<u32> dst, u32 restart_index)
	{
		const u32 count = ::size32(src);
		u32 written = 0;
		auto min = svdup_n_u32(u32{0xffffffff});
		auto max = svdup_n_u32(0);
		const auto all = svptrue_b32();

		for (u64 i = 0; i < count; i += svcntw())
		{
			const auto active = svwhilelt_b32(i, u64{count});
			auto value = svld1_u32(active, reinterpret_cast<const u32*>(src.data()) + i);
			value = svrevb_u32_x(active, value);
			const auto keep = svcmpne_n_u32(active, value, restart_index);
			min = svmin_u32_m(keep, min, value);
			max = svmax_u32_m(keep, max, value);
			const auto packed = svcompact_u32(keep, value);
			const u32 processed = svcntp_b32(all, keep);
			const auto output = svwhilelt_b32(u64{0}, u64{processed});
			svst1_u32(output, dst.data() + written, packed);
			written += processed;
		}

		return {svminv_u32(all, min), svmaxv_u32(all, max), written};
	}

#ifdef BUFFERUTILS_HAS_SVE2P2
	BUFFERUTILS_SVE2P2 std::tuple<u16, u16, u32> upload_swapped_sve2p2_skip_restart(std::span<to_be_t<const u16>> src, std::span<u16> dst, u16 restart_index)
	{
		const u32 count = ::size32(src);
		u32 written = 0;
		auto min = svdup_n_u16(u16{0xffff});
		auto max = svdup_n_u16(0);
		const auto all = svptrue_b16();

		for (u64 i = 0; i < count; i += svcnth())
		{
			const auto active = svwhilelt_b16(i, u64{count});
			auto value = svld1_u16(active, reinterpret_cast<const u16*>(src.data()) + i);
			value = svrevb_u16_x(active, value);
			const auto keep = svcmpne_n_u16(active, value, restart_index);
			min = svmin_u16_m(keep, min, value);
			max = svmax_u16_m(keep, max, value);
			const auto packed = svcompact_u16(keep, value);
			const u32 processed = svcntp_b16(all, keep);
			const auto output = svwhilelt_b16(u64{0}, u64{processed});
			svst1_u16(output, dst.data() + written, packed);
			written += processed;
		}

		return {svminv_u16(all, min), svmaxv_u16(all, max), written};
	}
#endif
} // namespace

#undef BUFFERUTILS_SVE
#ifdef BUFFERUTILS_HAS_SVE2P2
#undef BUFFERUTILS_SVE2P2
#endif
