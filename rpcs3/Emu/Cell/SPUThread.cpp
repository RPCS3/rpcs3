#include "stdafx.h"
#include "Utilities/JIT.h"
#include "Utilities/date_time.h"
#include "Emu/Memory/vm.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Memory/vm_reservation.h"

#include "Loader/ELF.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/perf_meter.hpp"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/Cell/lv2/sys_event_flag.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_interrupt.h"

#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/SPUAnalyser.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/SPURecompiler.h"
#include "Emu/Cell/timers.hpp"

#include "Emu/RSX/Core/RSXReservationLock.hpp"

#include <cmath>
#include <cfenv>
#include <thread>
#include <shared_mutex>
#include <span>
#include "util/vm.hpp"
#include "util/asm.hpp"
#include "util/v128.hpp"
#include "util/simd.hpp"
#include "util/sysinfo.hpp"
#include "util/serialization.hpp"

#if defined(ARCH_X64)
#ifdef _MSC_VER
#include <intrin.h>
#include <immintrin.h>
#else
#include <x86intrin.h>
#endif
#endif

// LUTs for SPU instructions

const u32 spu_frest_fraction_lut[32] =
{
	0x7FFBE0, 0x7F87A6, 0x70EF72, 0x708B40, 0x638B12, 0x633AEA, 0x5792C4, 0x574AA0,
	0x4CCA7E, 0x4C9262, 0x430A44, 0x42D62A, 0x3A2E12, 0x39FDFA, 0x3215E4, 0x31F1D2,
	0x2AA9BE, 0x2A85AC, 0x23D59A, 0x23BD8E, 0x1D8576, 0x1D8576, 0x17AD5A, 0x17AD5A,
	0x124543, 0x124543, 0x0D392D, 0x0D392D, 0x08851A, 0x08851A, 0x041D07, 0x041D07
};

const u32 spu_frest_exponent_lut[256] =
{
	0x7F800000, 0x7E000000, 0x7D800000, 0x7D000000, 0x7C800000, 0x7C000000, 0x7B800000, 0x7B000000, 0x7A800000, 0x7A000000, 0x79800000, 0x79000000, 0x78800000, 0x78000000, 0x77800000, 0x77000000,
	0x76800000, 0x76000000, 0x75800000, 0x75000000, 0x74800000, 0x74000000, 0x73800000, 0x73000000, 0x72800000, 0x72000000, 0x71800000, 0x71000000, 0x70800000, 0x70000000, 0x6F800000, 0x6F000000,
	0x6E800000, 0x6E000000, 0x6D800000, 0x6D000000, 0x6C800000, 0x6C000000, 0x6B800000, 0x6B000000, 0x6A800000, 0x6A000000, 0x69800000, 0x69000000, 0x68800000, 0x68000000, 0x67800000, 0x67000000,
	0x66800000, 0x66000000, 0x65800000, 0x65000000, 0x64800000, 0x64000000, 0x63800000, 0x63000000, 0x62800000, 0x62000000, 0x61800000, 0x61000000, 0x60800000, 0x60000000, 0x5F800000, 0x5F000000,
	0x5E800000, 0x5E000000, 0x5D800000, 0x5D000000, 0x5C800000, 0x5C000000, 0x5B800000, 0x5B000000, 0x5A800000, 0x5A000000, 0x59800000, 0x59000000, 0x58800000, 0x58000000, 0x57800000, 0x57000000,
	0x56800000, 0x56000000, 0x55800000, 0x55000000, 0x54800000, 0x54000000, 0x53800000, 0x53000000, 0x52800000, 0x52000000, 0x51800000, 0x51000000, 0x50800000, 0x50000000, 0x4F800000, 0x4F000000,
	0x4E800000, 0x4E000000, 0x4D800000, 0x4D000000, 0x4C800000, 0x4C000000, 0x4B800000, 0x4B000000, 0x4A800000, 0x4A000000, 0x49800000, 0x49000000, 0x48800000, 0x48000000, 0x47800000, 0x47000000,
	0x46800000, 0x46000000, 0x45800000, 0x45000000, 0x44800000, 0x44000000, 0x43800000, 0x43000000, 0x42800000, 0x42000000, 0x41800000, 0x41000000, 0x40800000, 0x40000000, 0x3F800000, 0x3F000000,
	0x3E800000, 0x3E000000, 0x3D800000, 0x3D000000, 0x3C800000, 0x3C000000, 0x3B800000, 0x3B000000, 0x3A800000, 0x3A000000, 0x39800000, 0x39000000, 0x38800000, 0x38000000, 0x37800000, 0x37000000,
	0x36800000, 0x36000000, 0x35800000, 0x35000000, 0x34800000, 0x34000000, 0x33800000, 0x33000000, 0x32800000, 0x32000000, 0x31800000, 0x31000000, 0x30800000, 0x30000000, 0x2F800000, 0x2F000000,
	0x2E800000, 0x2E000000, 0x2D800000, 0x2D000000, 0x2C800000, 0x2C000000, 0x2B800000, 0x2B000000, 0x2A800000, 0x2A000000, 0x29800000, 0x29000000, 0x28800000, 0x28000000, 0x27800000, 0x27000000,
	0x26800000, 0x26000000, 0x25800000, 0x25000000, 0x24800000, 0x24000000, 0x23800000, 0x23000000, 0x22800000, 0x22000000, 0x21800000, 0x21000000, 0x20800000, 0x20000000, 0x1F800000, 0x1F000000,
	0x1E800000, 0x1E000000, 0x1D800000, 0x1D000000, 0x1C800000, 0x1C000000, 0x1B800000, 0x1B000000, 0x1A800000, 0x1A000000, 0x19800000, 0x19000000, 0x18800000, 0x18000000, 0x17800000, 0x17000000,
	0x16800000, 0x16000000, 0x15800000, 0x15000000, 0x14800000, 0x14000000, 0x13800000, 0x13000000, 0x12800000, 0x12000000, 0x11800000, 0x11000000, 0x10800000, 0x10000000, 0x0F800000, 0x0F000000,
	0x0E800000, 0x0E000000, 0x0D800000, 0x0D000000, 0x0C800000, 0x0C000000, 0x0B800000, 0x0B000000, 0x0A800000, 0x0A000000, 0x09800000, 0x09000000, 0x08800000, 0x08000000, 0x07800000, 0x07000000,
	0x06800000, 0x06000000, 0x05800000, 0x05000000, 0x04800000, 0x04000000, 0x03800000, 0x03000000, 0x02800000, 0x02000000, 0x01800000, 0x01000000, 0x00800000, 0x00000000, 0x00000000, 0x00000000
};

const u32 spu_frsqest_fraction_lut[64] =
{
	0x350160, 0x34E954, 0x2F993D, 0x2F993D, 0x2AA523, 0x2AA523, 0x26190D, 0x26190D, 0x21E4F9, 0x21E4F9, 0x1E00E9, 0x1E00E9, 0x1A5CD9, 0x1A5CD9, 0x16F8CB, 0x16F8CB,
	0x13CCC0, 0x13CCC0, 0x10CCB3, 0x10CCB3, 0x0E00AA, 0x0E00AA, 0x0B58A1, 0x0B58A1, 0x08D498, 0x08D498, 0x067491, 0x067491, 0x043089, 0x043089, 0x020C83, 0x020C83,
	0x7FFDF4, 0x7FD1DE, 0x7859C8, 0x783DBA, 0x71559C, 0x71559C, 0x6AE57C, 0x6AE57C, 0x64F561, 0x64F561, 0x5F7149, 0x5F7149, 0x5A4D33, 0x5A4D33, 0x55811F, 0x55811F,
	0x51050F, 0x51050F, 0x4CC8FE, 0x4CC8FE, 0x48D0F0, 0x48D0F0, 0x4510E4, 0x4510E4, 0x4180D7, 0x4180D7, 0x3E24CC, 0x3E24CC, 0x3AF4C3, 0x3AF4C3, 0x37E8BA, 0x37E8BA
};

const u32 spu_frsqest_exponent_lut[256] =
{
	0x7F800000, 0x5E800000, 0x5E800000, 0x5E000000, 0x5E000000, 0x5D800000, 0x5D800000, 0x5D000000, 0x5D000000, 0x5C800000, 0x5C800000, 0x5C000000, 0x5C000000, 0x5B800000, 0x5B800000, 0x5B000000,
	0x5B000000, 0x5A800000, 0x5A800000, 0x5A000000, 0x5A000000, 0x59800000, 0x59800000, 0x59000000, 0x59000000, 0x58800000, 0x58800000, 0x58000000, 0x58000000, 0x57800000, 0x57800000, 0x57000000,
	0x57000000, 0x56800000, 0x56800000, 0x56000000, 0x56000000, 0x55800000, 0x55800000, 0x55000000, 0x55000000, 0x54800000, 0x54800000, 0x54000000, 0x54000000, 0x53800000, 0x53800000, 0x53000000,
	0x53000000, 0x52800000, 0x52800000, 0x52000000, 0x52000000, 0x51800000, 0x51800000, 0x51000000, 0x51000000, 0x50800000, 0x50800000, 0x50000000, 0x50000000, 0x4F800000, 0x4F800000, 0x4F000000,
	0x4F000000, 0x4E800000, 0x4E800000, 0x4E000000, 0x4E000000, 0x4D800000, 0x4D800000, 0x4D000000, 0x4D000000, 0x4C800000, 0x4C800000, 0x4C000000, 0x4C000000, 0x4B800000, 0x4B800000, 0x4B000000,
	0x4B000000, 0x4A800000, 0x4A800000, 0x4A000000, 0x4A000000, 0x49800000, 0x49800000, 0x49000000, 0x49000000, 0x48800000, 0x48800000, 0x48000000, 0x48000000, 0x47800000, 0x47800000, 0x47000000,
	0x47000000, 0x46800000, 0x46800000, 0x46000000, 0x46000000, 0x45800000, 0x45800000, 0x45000000, 0x45000000, 0x44800000, 0x44800000, 0x44000000, 0x44000000, 0x43800000, 0x43800000, 0x43000000,
	0x43000000, 0x42800000, 0x42800000, 0x42000000, 0x42000000, 0x41800000, 0x41800000, 0x41000000, 0x41000000, 0x40800000, 0x40800000, 0x40000000, 0x40000000, 0x3F800000, 0x3F800000, 0x3F000000,
	0x3F000000, 0x3E800000, 0x3E800000, 0x3E000000, 0x3E000000, 0x3D800000, 0x3D800000, 0x3D000000, 0x3D000000, 0x3C800000, 0x3C800000, 0x3C000000, 0x3C000000, 0x3B800000, 0x3B800000, 0x3B000000,
	0x3B000000, 0x3A800000, 0x3A800000, 0x3A000000, 0x3A000000, 0x39800000, 0x39800000, 0x39000000, 0x39000000, 0x38800000, 0x38800000, 0x38000000, 0x38000000, 0x37800000, 0x37800000, 0x37000000,
	0x37000000, 0x36800000, 0x36800000, 0x36000000, 0x36000000, 0x35800000, 0x35800000, 0x35000000, 0x35000000, 0x34800000, 0x34800000, 0x34000000, 0x34000000, 0x33800000, 0x33800000, 0x33000000,
	0x33000000, 0x32800000, 0x32800000, 0x32000000, 0x32000000, 0x31800000, 0x31800000, 0x31000000, 0x31000000, 0x30800000, 0x30800000, 0x30000000, 0x30000000, 0x2F800000, 0x2F800000, 0x2F000000,
	0x2F000000, 0x2E800000, 0x2E800000, 0x2E000000, 0x2E000000, 0x2D800000, 0x2D800000, 0x2D000000, 0x2D000000, 0x2C800000, 0x2C800000, 0x2C000000, 0x2C000000, 0x2B800000, 0x2B800000, 0x2B000000,
	0x2B000000, 0x2A800000, 0x2A800000, 0x2A000000, 0x2A000000, 0x29800000, 0x29800000, 0x29000000, 0x29000000, 0x28800000, 0x28800000, 0x28000000, 0x28000000, 0x27800000, 0x27800000, 0x27000000,
	0x27000000, 0x26800000, 0x26800000, 0x26000000, 0x26000000, 0x25800000, 0x25800000, 0x25000000, 0x25000000, 0x24800000, 0x24800000, 0x24000000, 0x24000000, 0x23800000, 0x23800000, 0x23000000,
	0x23000000, 0x22800000, 0x22800000, 0x22000000, 0x22000000, 0x21800000, 0x21800000, 0x21000000, 0x21000000, 0x20800000, 0x20800000, 0x20000000, 0x20000000, 0x1F800000, 0x1F800000, 0x1F000000
};

using spu_rdata_t = decltype(spu_thread::rdata);

template <>
void fmt_class_string<mfc_atomic_status>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](mfc_atomic_status arg)
	{
		switch (arg)
		{
		case MFC_PUTLLC_SUCCESS: return "PUTLLC";
		case MFC_PUTLLC_FAILURE: return "PUTLLC-FAIL";
		case MFC_PUTLLUC_SUCCESS: return "PUTLLUC";
		case MFC_GETLLAR_SUCCESS: return "GETLLAR";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<mfc_tag_update>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](mfc_tag_update arg)
	{
		switch (arg)
		{
		case MFC_TAG_UPDATE_IMMEDIATE: return "empty";
		case MFC_TAG_UPDATE_ANY: return "ANY";
		case MFC_TAG_UPDATE_ALL: return "ALL";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<spu_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](spu_type arg)
	{
		switch (arg)
		{
		case spu_type::threaded: return "Threaded";
		case spu_type::raw: return "Raw";
		case spu_type::isolated: return "Isolated";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<spu_block_hash>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%s", fmt::base57(be_t<u64>{arg}));

	// Print only 7 hash characters out of 11 (which covers roughly 48 bits)
	out.resize(out.size() - 4);

	// Print chunk address from lowest 16 bits
	fmt::append(out, "...chunk-0x%05x", (arg & 0xffff) * 4);
}

enum class spu_block_hash_short : u64{};

template <>
void fmt_class_string<spu_block_hash_short>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%s", fmt::base57(be_t<u64>{arg}));

	// Print only 7 hash characters out of 11 (which covers roughly 48 bits)
	out.resize(out.size() - 4);
}

// Verify AVX availability for TSX transactions
static const bool s_tsx_avx = utils::has_avx();

// Threshold for when rep mosvb is expected to outperform simd copies
// The threshold will be 0xFFFFFFFF when the performance of rep movsb is expected to be bad
static const u32 s_rep_movsb_threshold = utils::get_rep_movsb_threshold();

#if defined(_M_X64)
extern "C" void __movsb(uchar*, const uchar*, size_t);
#elif defined(ARCH_X64)
static FORCE_INLINE void __movsb(unsigned char * Dst, const unsigned char * Src, size_t Size)
{
	__asm__ __volatile__
	(
		"rep; movsb" :
		[Dst] "=D" (Dst), [Src] "=S" (Src), [Size] "=c" (Size) :
		"[Dst]" (Dst), "[Src]" (Src), "[Size]" (Size)
	);
}
#else
#define s_rep_movsb_threshold umax
#define __movsb std::memcpy
#endif

#if defined(ARCH_X64)
static FORCE_INLINE bool cmp_rdata_avx(const __m256i* lhs, const __m256i* rhs)
{
#if defined(_MSC_VER) || defined(__AVX__)
	// Interleave 2 cache line accesses (optimization)
	const __m256 x0 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_loadu_si256(lhs + 0)), _mm256_castsi256_ps(_mm256_loadu_si256(rhs + 0)));
	const __m256 x2 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_loadu_si256(lhs + 2)), _mm256_castsi256_ps(_mm256_loadu_si256(rhs + 2)));
	const __m256 x1 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_loadu_si256(lhs + 1)), _mm256_castsi256_ps(_mm256_loadu_si256(rhs + 1)));
	const __m256 x3 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_loadu_si256(lhs + 3)), _mm256_castsi256_ps(_mm256_loadu_si256(rhs + 3)));
	const __m256 c0 = _mm256_or_ps(x0, x1);
	const __m256 c1 = _mm256_or_ps(x2, x3);
	const __m256 c2 = _mm256_or_ps(c0, c1);
	return _mm256_testz_si256(_mm256_castps_si256(c2), _mm256_castps_si256(c2)) != 0;
#else
	bool result = 0;
	__asm__(
		"vmovups 0*32(%[lhs]), %%ymm0;" // load
		"vmovups 2*32(%[lhs]), %%ymm2;"
		"vmovups 1*32(%[lhs]), %%ymm1;"
		"vmovups 3*32(%[lhs]), %%ymm3;"
		"vxorps 0*32(%[rhs]), %%ymm0, %%ymm0;" // compare
		"vxorps 2*32(%[rhs]), %%ymm2, %%ymm2;"
		"vxorps 1*32(%[rhs]), %%ymm1, %%ymm1;"
		"vxorps 3*32(%[rhs]), %%ymm3, %%ymm3;"
		"vorps %%ymm0, %%ymm1, %%ymm0;" // merge
		"vorps %%ymm2, %%ymm3, %%ymm2;"
		"vorps %%ymm0, %%ymm2, %%ymm0;"
		"vptest %%ymm0, %%ymm0;" // test
		"vzeroupper"
		: "=@ccz" (result)
		: [lhs] "r" (lhs)
		, [rhs] "r" (rhs)
		: "cc" // Clobber flags
		, "xmm0" // Clobber registers ymm0-ymm3 (see mov_rdata_avx)
		, "xmm1"
		, "xmm2"
		, "xmm3"
	);
	return result;
#endif
}
#endif

#ifdef _MSC_VER
__forceinline
#endif
extern bool cmp_rdata(const spu_rdata_t& _lhs, const spu_rdata_t& _rhs)
{
#if defined(ARCH_X64)
#ifndef __AVX__
	if (s_tsx_avx) [[likely]]
#endif
	{
		return cmp_rdata_avx(reinterpret_cast<const __m256i*>(_lhs), reinterpret_cast<const __m256i*>(_rhs));
	}
#endif

	const auto lhs = reinterpret_cast<const v128*>(_lhs);
	const auto rhs = reinterpret_cast<const v128*>(_rhs);
	const v128 a = (lhs[0] ^ rhs[0]) | (lhs[1] ^ rhs[1]);
	const v128 c = (lhs[4] ^ rhs[4]) | (lhs[5] ^ rhs[5]);
	const v128 b = (lhs[2] ^ rhs[2]) | (lhs[3] ^ rhs[3]);
	const v128 d = (lhs[6] ^ rhs[6]) | (lhs[7] ^ rhs[7]);
	const v128 r = (a | b) | (c | d);
	return gv_testz(r);
}

#if defined(ARCH_X64)
static FORCE_INLINE void mov_rdata_avx(__m256i* dst, const __m256i* src)
{
#ifdef _MSC_VER
	_mm256_storeu_si256(dst + 0, _mm256_loadu_si256(src + 0));
	_mm256_storeu_si256(dst + 2, _mm256_loadu_si256(src + 2));
	_mm256_storeu_si256(dst + 1, _mm256_loadu_si256(src + 1));
	_mm256_storeu_si256(dst + 3, _mm256_loadu_si256(src + 3));
#else
	__asm__(
		"vmovdqu 0*32(%[src]), %%ymm0;" // load
		"vmovdqu %%ymm0, 0*32(%[dst]);" // store
		"vmovdqu 2*32(%[src]), %%ymm0;"
		"vmovdqu %%ymm0, 2*32(%[dst]);"
		"vmovdqu 1*32(%[src]), %%ymm0;"
		"vmovdqu %%ymm0, 1*32(%[dst]);"
		"vmovdqu 3*32(%[src]), %%ymm0;"
		"vmovdqu %%ymm0, 3*32(%[dst]);"
#ifndef __AVX__
		"vzeroupper" // Don't need in AVX mode (should be emitted automatically)
#endif
		:
		: [src] "r" (src)
		, [dst] "r" (dst)
#ifdef __AVX__
		: "ymm0" // Clobber ymm0 register (acknowledge its modification)
#else
		: "xmm0" // ymm0 is "unknown" if not compiled in AVX mode, so clobber xmm0 only
#endif
	);
#endif
}
#endif

#ifdef _MSC_VER
__forceinline
#endif
extern void mov_rdata(spu_rdata_t& _dst, const spu_rdata_t& _src)
{
#if defined(ARCH_X64)
#ifndef __AVX__
	if (s_tsx_avx) [[likely]]
#endif
	{
		mov_rdata_avx(reinterpret_cast<__m256i*>(_dst), reinterpret_cast<const __m256i*>(_src));
		return;
	}

	{
		const __m128i v0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 0));
		const __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 16));
		const __m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 32));
		const __m128i v3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 48));
		_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 0), v0);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 16), v1);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 32), v2);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 48), v3);
	}

	const __m128i v0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 64));
	const __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 80));
	const __m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 96));
	const __m128i v3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 112));
	_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 64), v0);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 80), v1);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 96), v2);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 112), v3);
#else
	std::memcpy(_dst, _src, 128);
#endif
}

#ifdef _MSC_VER
__forceinline
#endif
extern u32 compute_rdata_hash32(const spu_rdata_t& _src)
{
	const auto rhs = reinterpret_cast<const v128*>(_src);
	const v128 a = gv_add32(rhs[0], rhs[1]);
	const v128 c = gv_add32(rhs[4], rhs[5]);
	const v128 b = gv_add32(rhs[2], rhs[3]);
	const v128 d = gv_add32(rhs[6], rhs[7]);
	const v128 r = gv_add32(gv_add32(a, b), gv_add32(c, d));
	const v128 r1 = gv_add32(r, gv_shuffle32<1, 0, 3, 2>(r));
	return r1._u32[0] + r1._u32[2];
}

#if defined(ARCH_X64)
static FORCE_INLINE void mov_rdata_nt_avx(__m256i* dst, const __m256i* src)
{
#ifdef _MSC_VER
	_mm256_stream_si256(dst + 0, _mm256_load_si256(src + 0));
	_mm256_stream_si256(dst + 2, _mm256_load_si256(src + 2));
	_mm256_stream_si256(dst + 1, _mm256_load_si256(src + 1));
	_mm256_stream_si256(dst + 3, _mm256_load_si256(src + 3));
#else
	__asm__(
		"vmovdqa 0*32(%[src]), %%ymm0;" // load
		"vmovntdq %%ymm0, 0*32(%[dst]);" // store
		"vmovdqa 2*32(%[src]), %%ymm0;"
		"vmovntdq %%ymm0, 2*32(%[dst]);"
		"vmovdqa 1*32(%[src]), %%ymm0;"
		"vmovntdq %%ymm0, 1*32(%[dst]);"
		"vmovdqa 3*32(%[src]), %%ymm0;"
		"vmovntdq %%ymm0, 3*32(%[dst]);"
#ifndef __AVX__
		"vzeroupper" // Don't need in AVX mode (should be emitted automatically)
#endif
		:
		: [src] "r" (src)
		, [dst] "r" (dst)
#ifdef __AVX__
		: "ymm0" // Clobber ymm0 register (acknowledge its modification)
#else
		: "xmm0" // ymm0 is "unknown" if not compiled in AVX mode, so clobber xmm0 only
#endif
	);
#endif
}
#endif

extern void mov_rdata_nt(spu_rdata_t& _dst, const spu_rdata_t& _src)
{
#if defined(ARCH_X64)
#ifndef __AVX__
	if (s_tsx_avx) [[likely]]
#endif
	{
		mov_rdata_nt_avx(reinterpret_cast<__m256i*>(_dst), reinterpret_cast<const __m256i*>(_src));
		return;
	}

	{
		const __m128i v0 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 0));
		const __m128i v1 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 16));
		const __m128i v2 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 32));
		const __m128i v3 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 48));
		_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 0), v0);
		_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 16), v1);
		_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 32), v2);
		_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 48), v3);
	}

	const __m128i v0 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 64));
	const __m128i v1 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 80));
	const __m128i v2 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 96));
	const __m128i v3 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 112));
	_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 64), v0);
	_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 80), v1);
	_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 96), v2);
	_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 112), v3);
#else
	std::memcpy(_dst, _src, 128);
#endif
}

#if defined(_MSC_VER)
#define mwaitx_func
#define waitpkg_func
#else
#define mwaitx_func __attribute__((__target__("mwaitx")))
#define waitpkg_func __attribute__((__target__("waitpkg")))
#endif

#if defined(ARCH_X64)
// Waits for a number of TSC clock cycles in power optimized state
// Cstate is represented in bits [7:4]+1 cstate. So C0 requires bits [7:4] to be set to 0xf, C1 requires bits [7:4] to be set to 0.
template <typename T, typename... Args>
mwaitx_func static void __mwaitx(u32 cycles, u32 cstate, const void* cline, const Args&... args)
{
	constexpr u32 timer_enable = 0x2;

	// monitorx will wake if the cache line is written to, use it for reservations which fits it almost perfectly
	_mm_monitorx(const_cast<void*>(cline), 0, 0);

	// Use static function to force inline
	if (T::needs_wait(args...))
	{
		_mm_mwaitx(timer_enable, cstate, cycles);
	}
}

// First bit indicates cstate, 0x0 for C.02 state (lower power) or 0x1 for C.01 state (higher power)
waitpkg_func static void __tpause(u32 cycles, u32 cstate)
{
	const u64 tsc = utils::get_tsc() + cycles;
	_tpause(cstate, tsc);
}
#endif

namespace vm
{
	std::array<atomic_t<reservation_waiter_t>, 1024> g_resrv_waiters_count{};
}

void do_cell_atomic_128_store(u32 addr, const void* to_write);

extern thread_local u64 g_tls_fault_spu;

const spu_decoder<spu_itype> s_spu_itype;

namespace vm
{
	extern atomic_t<u64, 64> g_range_lock_set[64];

	// Defined here for performance reasons
	writer_lock::~writer_lock() noexcept
	{
		if (range_lock)
		{
			g_range_lock_bits[1] &= ~(1ull << (range_lock - g_range_lock_set));
			range_lock->release(0);
			return;
		}

		g_range_lock_bits[1].release(0);
	}
}

namespace spu
{
	namespace scheduler
	{
		std::array<atomic_t<u8>, 65536> atomic_instruction_table = {};
		constexpr u32 native_jiffy_duration_us = 1500; //About 1ms resolution with a half offset

		void acquire_pc_address(spu_thread& spu, u32 pc, u32 timeout_ms, u32 max_concurrent_instructions)
		{
			const u32 pc_offset = pc >> 2;

			if (atomic_instruction_table[pc_offset].observe() >= max_concurrent_instructions)
			{
				spu.state += cpu_flag::wait + cpu_flag::temp;

				if (timeout_ms > 0)
				{
					const u64 timeout = timeout_ms * 1000u; //convert to microseconds
					const u64 start = get_system_time();
					auto remaining = timeout;

					while (atomic_instruction_table[pc_offset].observe() >= max_concurrent_instructions)
					{
						if (remaining >= native_jiffy_duration_us)
							std::this_thread::sleep_for(1ms);
						else
							std::this_thread::yield();

						const auto now = get_system_time();
						const auto elapsed = now - start;

						if (elapsed > timeout) break;
						remaining = timeout - elapsed;
					}
				}
				else
				{
					//Slight pause if function is overburdened
					const auto count = atomic_instruction_table[pc_offset].observe() * 100ull;
					busy_wait(count);
				}

				ensure(!spu.check_state());
			}

			atomic_instruction_table[pc_offset]++;
		}

		void release_pc_address(u32 pc)
		{
			const u32 pc_offset = pc >> 2;

			atomic_instruction_table[pc_offset]--;
		}

		struct concurrent_execution_watchdog
		{
			u32 pc = 0;
			bool active = false;

			concurrent_execution_watchdog(spu_thread& spu)
				:pc(spu.pc)
			{
				if (const u32 max_concurrent_instructions = g_cfg.core.preferred_spu_threads)
				{
					acquire_pc_address(spu, pc, g_cfg.core.spu_delay_penalty, max_concurrent_instructions);
					active = true;
				}
			}

			~concurrent_execution_watchdog()
			{
				if (active)
					release_pc_address(pc);
			}
		};
	}
}

std::array<u32, 2> op_branch_targets(u32 pc, spu_opcode_t op)
{
	std::array<u32, 2> res{spu_branch_target(pc + 4), umax};

	switch (const auto type = s_spu_itype.decode(op.opcode))
	{
	case spu_itype::BR:
	case spu_itype::BRA:
	case spu_itype::BRNZ:
	case spu_itype::BRZ:
	case spu_itype::BRHNZ:
	case spu_itype::BRHZ:
	case spu_itype::BRSL:
	case spu_itype::BRASL:
	{
		const int index = (type == spu_itype::BR || type == spu_itype::BRA || type == spu_itype::BRSL || type == spu_itype::BRASL ? 0 : 1);
		res[index] = (spu_branch_target(type == spu_itype::BRASL || type == spu_itype::BRA ? 0 : pc, op.i16));

		if (res[0] == res[1])
		{
			res[1] = umax;
		}

		break;
	}
	case spu_itype::IRET:
	case spu_itype::BI:
	case spu_itype::BISLED:
	case spu_itype::BISL:
	case spu_itype::BIZ:
	case spu_itype::BINZ:
	case spu_itype::BIHZ:
	case spu_itype::BIHNZ: // TODO (detect constant address branches, such as for interrupts enable/disable pattern)

	case spu_itype::UNK:
	{
		res[0] = umax;
		break;
	}
	default: break;
	}

	return res;
}

const auto spu_putllc_tx = build_function_asm<u64(*)(u32 raddr, u64 rtime, void* _old, const void* _new)>("spu_putllc_tx", [](native_asm& c, auto& args)
{
	using namespace asmjit;

#if defined(ARCH_X64)
	Label fall = c.newLabel();
	Label fail = c.newLabel();
	Label _ret = c.newLabel();
	Label load = c.newLabel();

	//if (utils::has_avx() && !s_tsx_avx)
	//{
	//	c.vzeroupper();
	//}

	// Create stack frame if necessary (Windows ABI has only 6 volatile vector registers)
	c.push(x86::rbp);
	c.push(x86::rbx);
#ifdef _WIN32
	c.sub(x86::rsp, 168);
	if (s_tsx_avx)
	{
		c.vmovups(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
		c.vmovups(x86::oword_ptr(x86::rsp, 16), x86::xmm7);
	}
	else
	{
		c.movups(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
		c.movups(x86::oword_ptr(x86::rsp, 16), x86::xmm7);
		c.movups(x86::oword_ptr(x86::rsp, 32), x86::xmm8);
		c.movups(x86::oword_ptr(x86::rsp, 48), x86::xmm9);
		c.movups(x86::oword_ptr(x86::rsp, 64), x86::xmm10);
		c.movups(x86::oword_ptr(x86::rsp, 80), x86::xmm11);
		c.movups(x86::oword_ptr(x86::rsp, 96), x86::xmm12);
		c.movups(x86::oword_ptr(x86::rsp, 112), x86::xmm13);
		c.movups(x86::oword_ptr(x86::rsp, 128), x86::xmm14);
		c.movups(x86::oword_ptr(x86::rsp, 144), x86::xmm15);
	}
#else
	c.sub(x86::rsp, 40);
#endif

	// Prepare registers
	build_swap_rdx_with(c, args, x86::r10);
	c.movabs(args[1], reinterpret_cast<u64>(&vm::g_sudo_addr));
	c.mov(args[1], x86::qword_ptr(args[1]));
	c.lea(args[1], x86::qword_ptr(args[1], args[0]));
	c.prefetchw(x86::byte_ptr(args[1], 0));
	c.prefetchw(x86::byte_ptr(args[1], 64));
	c.and_(args[0].r32(), 0xff80);
	c.shr(args[0].r32(), 1);
	c.movabs(x86::r11, reinterpret_cast<u64>(+vm::g_reservations));
	c.lea(x86::r11, x86::qword_ptr(x86::r11, args[0]));

	// Prepare data
	if (s_tsx_avx)
	{
		c.vmovups(x86::ymm0, x86::ymmword_ptr(args[2], 0));
		c.vmovups(x86::ymm1, x86::ymmword_ptr(args[2], 32));
		c.vmovups(x86::ymm2, x86::ymmword_ptr(args[2], 64));
		c.vmovups(x86::ymm3, x86::ymmword_ptr(args[2], 96));
		c.vmovups(x86::ymm4, x86::ymmword_ptr(args[3], 0));
		c.vmovups(x86::ymm5, x86::ymmword_ptr(args[3], 32));
		c.vmovups(x86::ymm6, x86::ymmword_ptr(args[3], 64));
		c.vmovups(x86::ymm7, x86::ymmword_ptr(args[3], 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(args[2], 0));
		c.movaps(x86::xmm1, x86::oword_ptr(args[2], 16));
		c.movaps(x86::xmm2, x86::oword_ptr(args[2], 32));
		c.movaps(x86::xmm3, x86::oword_ptr(args[2], 48));
		c.movaps(x86::xmm4, x86::oword_ptr(args[2], 64));
		c.movaps(x86::xmm5, x86::oword_ptr(args[2], 80));
		c.movaps(x86::xmm6, x86::oword_ptr(args[2], 96));
		c.movaps(x86::xmm7, x86::oword_ptr(args[2], 112));
		c.movaps(x86::xmm8, x86::oword_ptr(args[3], 0));
		c.movaps(x86::xmm9, x86::oword_ptr(args[3], 16));
		c.movaps(x86::xmm10, x86::oword_ptr(args[3], 32));
		c.movaps(x86::xmm11, x86::oword_ptr(args[3], 48));
		c.movaps(x86::xmm12, x86::oword_ptr(args[3], 64));
		c.movaps(x86::xmm13, x86::oword_ptr(args[3], 80));
		c.movaps(x86::xmm14, x86::oword_ptr(args[3], 96));
		c.movaps(x86::xmm15, x86::oword_ptr(args[3], 112));
	}

	// Alloc args[0] to stamp0
	const auto stamp0 = args[0];
	build_get_tsc(c, stamp0);

	Label fail2 = c.newLabel();

	Label tx1 = build_transaction_enter(c, fall, [&]()
	{
		c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::ftx) - ::offset32(&spu_thread::rdata)), 1);
		build_get_tsc(c);
		c.sub(x86::rax, stamp0);
		c.movabs(x86::rbx, reinterpret_cast<u64>(&g_rtm_tx_limit2));
		c.cmp(x86::rax, x86::qword_ptr(x86::rbx));
		c.jae(fall);
	});

	// Check pause flag
	c.bt(x86::dword_ptr(args[2], ::offset32(&spu_thread::state) - ::offset32(&spu_thread::rdata)), static_cast<u32>(cpu_flag::pause));
	c.jc(fall);
	c.xbegin(tx1);

	if (s_tsx_avx)
	{
		c.vxorps(x86::ymm0, x86::ymm0, x86::ymmword_ptr(args[1], 0));
		c.vxorps(x86::ymm1, x86::ymm1, x86::ymmword_ptr(args[1], 32));
		c.vxorps(x86::ymm2, x86::ymm2, x86::ymmword_ptr(args[1], 64));
		c.vxorps(x86::ymm3, x86::ymm3, x86::ymmword_ptr(args[1], 96));
		c.vorps(x86::ymm0, x86::ymm0, x86::ymm1);
		c.vorps(x86::ymm1, x86::ymm2, x86::ymm3);
		c.vorps(x86::ymm0, x86::ymm1, x86::ymm0);
		c.vptest(x86::ymm0, x86::ymm0);
	}
	else
	{
		c.xorps(x86::xmm0, x86::oword_ptr(args[1], 0));
		c.xorps(x86::xmm1, x86::oword_ptr(args[1], 16));
		c.xorps(x86::xmm2, x86::oword_ptr(args[1], 32));
		c.xorps(x86::xmm3, x86::oword_ptr(args[1], 48));
		c.xorps(x86::xmm4, x86::oword_ptr(args[1], 64));
		c.xorps(x86::xmm5, x86::oword_ptr(args[1], 80));
		c.xorps(x86::xmm6, x86::oword_ptr(args[1], 96));
		c.xorps(x86::xmm7, x86::oword_ptr(args[1], 112));
		c.orps(x86::xmm0, x86::xmm1);
		c.orps(x86::xmm2, x86::xmm3);
		c.orps(x86::xmm4, x86::xmm5);
		c.orps(x86::xmm6, x86::xmm7);
		c.orps(x86::xmm0, x86::xmm2);
		c.orps(x86::xmm4, x86::xmm6);
		c.orps(x86::xmm0, x86::xmm4);
		c.ptest(x86::xmm0, x86::xmm0);
	}

	c.jnz(fail);

	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymmword_ptr(args[1], 0), x86::ymm4);
		c.vmovaps(x86::ymmword_ptr(args[1], 32), x86::ymm5);
		c.vmovaps(x86::ymmword_ptr(args[1], 64), x86::ymm6);
		c.vmovaps(x86::ymmword_ptr(args[1], 96), x86::ymm7);
	}
	else
	{
		c.movaps(x86::oword_ptr(args[1], 0), x86::xmm8);
		c.movaps(x86::oword_ptr(args[1], 16), x86::xmm9);
		c.movaps(x86::oword_ptr(args[1], 32), x86::xmm10);
		c.movaps(x86::oword_ptr(args[1], 48), x86::xmm11);
		c.movaps(x86::oword_ptr(args[1], 64), x86::xmm12);
		c.movaps(x86::oword_ptr(args[1], 80), x86::xmm13);
		c.movaps(x86::oword_ptr(args[1], 96), x86::xmm14);
		c.movaps(x86::oword_ptr(args[1], 112), x86::xmm15);
	}

	c.xend();
	c.lock().add(x86::qword_ptr(x86::r11), 64);
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx) - ::offset32(&spu_thread::rdata)), 1);
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);
	c.jmp(_ret);

	// XABORT is expensive so try to finish with xend instead
	c.bind(fail);

	// Load previous data to store back to rdata
	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymm0, x86::ymmword_ptr(args[1], 0));
		c.vmovaps(x86::ymm1, x86::ymmword_ptr(args[1], 32));
		c.vmovaps(x86::ymm2, x86::ymmword_ptr(args[1], 64));
		c.vmovaps(x86::ymm3, x86::ymmword_ptr(args[1], 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(args[1], 0));
		c.movaps(x86::xmm1, x86::oword_ptr(args[1], 16));
		c.movaps(x86::xmm2, x86::oword_ptr(args[1], 32));
		c.movaps(x86::xmm3, x86::oword_ptr(args[1], 48));
		c.movaps(x86::xmm4, x86::oword_ptr(args[1], 64));
		c.movaps(x86::xmm5, x86::oword_ptr(args[1], 80));
		c.movaps(x86::xmm6, x86::oword_ptr(args[1], 96));
		c.movaps(x86::xmm7, x86::oword_ptr(args[1], 112));
	}

	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx) - ::offset32(&spu_thread::rdata)), 1);
	c.jmp(fail2);

	c.bind(fall);
	c.mov(x86::rax, -1);
	c.jmp(_ret);

	c.bind(fail2);
	c.lock().sub(x86::qword_ptr(x86::r11), 64);
	c.bind(load);

	// Store previous data back to rdata
	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymmword_ptr(args[2], 0), x86::ymm0);
		c.vmovaps(x86::ymmword_ptr(args[2], 32), x86::ymm1);
		c.vmovaps(x86::ymmword_ptr(args[2], 64), x86::ymm2);
		c.vmovaps(x86::ymmword_ptr(args[2], 96), x86::ymm3);
	}
	else
	{
		c.movaps(x86::oword_ptr(args[2], 0), x86::xmm0);
		c.movaps(x86::oword_ptr(args[2], 16), x86::xmm1);
		c.movaps(x86::oword_ptr(args[2], 32), x86::xmm2);
		c.movaps(x86::oword_ptr(args[2], 48), x86::xmm3);
		c.movaps(x86::oword_ptr(args[2], 64), x86::xmm4);
		c.movaps(x86::oword_ptr(args[2], 80), x86::xmm5);
		c.movaps(x86::oword_ptr(args[2], 96), x86::xmm6);
		c.movaps(x86::oword_ptr(args[2], 112), x86::xmm7);
	}

	c.mov(x86::rax, -1);
	c.mov(x86::qword_ptr(args[2], ::offset32(&spu_thread::last_ftime) - ::offset32(&spu_thread::rdata)), x86::rax);
	c.xor_(x86::eax, x86::eax);
	//c.jmp(_ret);

	c.bind(_ret);

#ifdef _WIN32
	if (s_tsx_avx)
	{
		c.vmovups(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
		c.vmovups(x86::xmm7, x86::oword_ptr(x86::rsp, 16));
	}
	else
	{
		c.movups(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
		c.movups(x86::xmm7, x86::oword_ptr(x86::rsp, 16));
		c.movups(x86::xmm8, x86::oword_ptr(x86::rsp, 32));
		c.movups(x86::xmm9, x86::oword_ptr(x86::rsp, 48));
		c.movups(x86::xmm10, x86::oword_ptr(x86::rsp, 64));
		c.movups(x86::xmm11, x86::oword_ptr(x86::rsp, 80));
		c.movups(x86::xmm12, x86::oword_ptr(x86::rsp, 96));
		c.movups(x86::xmm13, x86::oword_ptr(x86::rsp, 112));
		c.movups(x86::xmm14, x86::oword_ptr(x86::rsp, 128));
		c.movups(x86::xmm15, x86::oword_ptr(x86::rsp, 144));
	}
	c.add(x86::rsp, 168);
#else
	c.add(x86::rsp, 40);
#endif

	c.pop(x86::rbx);
	c.pop(x86::rbp);

	if (s_tsx_avx)
	{
		c.vzeroupper();
	}

	maybe_flush_lbr(c);
	c.ret();
#else
	UNUSED(args);

	c.brk(Imm(0x42));
	c.ret(a64::x30);
#endif
});

const auto spu_putlluc_tx = build_function_asm<u64(*)(u32 raddr, const void* rdata, u64* _stx, u64* _ftx)>("spu_putlluc_tx", [](native_asm& c, auto& args)
{
	using namespace asmjit;

#if defined(ARCH_X64)
	Label fall = c.newLabel();
	Label _ret = c.newLabel();

	//if (utils::has_avx() && !s_tsx_avx)
	//{
	//	c.vzeroupper();
	//}

	// Create stack frame if necessary (Windows ABI has only 6 volatile vector registers)
	c.push(x86::rbp);
	c.push(x86::rbx);
	c.sub(x86::rsp, 40);
#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.movups(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
		c.movups(x86::oword_ptr(x86::rsp, 16), x86::xmm7);
	}
#endif
	// Prepare registers
	build_swap_rdx_with(c, args, x86::r10);
	c.movabs(x86::r11, reinterpret_cast<u64>(&vm::g_sudo_addr));
	c.mov(x86::r11, x86::qword_ptr(x86::r11));
	c.lea(x86::r11, x86::qword_ptr(x86::r11, args[0]));
	c.prefetchw(x86::byte_ptr(x86::r11, 0));
	c.prefetchw(x86::byte_ptr(x86::r11, 64));

	// Prepare data
	if (s_tsx_avx)
	{
		c.vmovups(x86::ymm0, x86::ymmword_ptr(args[1], 0));
		c.vmovups(x86::ymm1, x86::ymmword_ptr(args[1], 32));
		c.vmovups(x86::ymm2, x86::ymmword_ptr(args[1], 64));
		c.vmovups(x86::ymm3, x86::ymmword_ptr(args[1], 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(args[1], 0));
		c.movaps(x86::xmm1, x86::oword_ptr(args[1], 16));
		c.movaps(x86::xmm2, x86::oword_ptr(args[1], 32));
		c.movaps(x86::xmm3, x86::oword_ptr(args[1], 48));
		c.movaps(x86::xmm4, x86::oword_ptr(args[1], 64));
		c.movaps(x86::xmm5, x86::oword_ptr(args[1], 80));
		c.movaps(x86::xmm6, x86::oword_ptr(args[1], 96));
		c.movaps(x86::xmm7, x86::oword_ptr(args[1], 112));
	}

	c.and_(args[0].r32(), 0xff80);
	c.shr(args[0].r32(), 1);
	c.movabs(args[1], reinterpret_cast<u64>(+vm::g_reservations));
	c.lea(args[1], x86::qword_ptr(args[1], args[0]));

	// Alloc args[0] to stamp0
	const auto stamp0 = args[0];
	build_get_tsc(c, stamp0);

	Label tx1 = build_transaction_enter(c, fall, [&]()
	{
		// ftx++;
		c.add(x86::qword_ptr(args[3]), 1);
		build_get_tsc(c);
		c.sub(x86::rax, stamp0);
		c.movabs(x86::rbx, reinterpret_cast<u64>(&g_rtm_tx_limit2));
		c.cmp(x86::rax, x86::qword_ptr(x86::rbx));
		c.jae(fall);
	});

	c.xbegin(tx1);

	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymmword_ptr(x86::r11, 0), x86::ymm0);
		c.vmovaps(x86::ymmword_ptr(x86::r11, 32), x86::ymm1);
		c.vmovaps(x86::ymmword_ptr(x86::r11, 64), x86::ymm2);
		c.vmovaps(x86::ymmword_ptr(x86::r11, 96), x86::ymm3);
	}
	else
	{
		c.movaps(x86::oword_ptr(x86::r11, 0), x86::xmm0);
		c.movaps(x86::oword_ptr(x86::r11, 16), x86::xmm1);
		c.movaps(x86::oword_ptr(x86::r11, 32), x86::xmm2);
		c.movaps(x86::oword_ptr(x86::r11, 48), x86::xmm3);
		c.movaps(x86::oword_ptr(x86::r11, 64), x86::xmm4);
		c.movaps(x86::oword_ptr(x86::r11, 80), x86::xmm5);
		c.movaps(x86::oword_ptr(x86::r11, 96), x86::xmm6);
		c.movaps(x86::oword_ptr(x86::r11, 112), x86::xmm7);
	}

	c.xend();
	c.lock().add(x86::qword_ptr(args[1]), 32);
	// stx++
	c.add(x86::qword_ptr(args[2]), 1);
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);
	c.jmp(_ret);

	c.bind(fall);
	c.xor_(x86::eax, x86::eax);
	//c.jmp(_ret);

	c.bind(_ret);

#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.movups(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
		c.movups(x86::xmm7, x86::oword_ptr(x86::rsp, 16));
	}
	c.add(x86::rsp, 40);
#endif

	if (s_tsx_avx)
	{
		c.vzeroupper();
	}

	c.add(x86::rsp, 40);
	c.pop(x86::rbx);
	c.pop(x86::rbp);

	maybe_flush_lbr(c);
	c.ret();
#else
	UNUSED(args);

	c.brk(Imm(0x42));
	c.ret(a64::x30);
#endif
});

const auto spu_getllar_tx = build_function_asm<u64(*)(u32 raddr, void* rdata, cpu_thread* _cpu, u64 rtime)>("spu_getllar_tx", [](native_asm& c, auto& args)
{
	using namespace asmjit;

#if defined(ARCH_X64)
	Label fall = c.newLabel();
	Label _ret = c.newLabel();

	//if (utils::has_avx() && !s_tsx_avx)
	//{
	//	c.vzeroupper();
	//}

	// Create stack frame if necessary (Windows ABI has only 6 volatile vector registers)
	c.push(x86::rbp);
	c.push(x86::rbx);
	c.sub(x86::rsp, 40);
#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.movups(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
		c.movups(x86::oword_ptr(x86::rsp, 16), x86::xmm7);
	}
#endif

	// Prepare registers
	build_swap_rdx_with(c, args, x86::r10);
	c.movabs(x86::rbp, reinterpret_cast<u64>(&vm::g_sudo_addr));
	c.mov(x86::rbp, x86::qword_ptr(x86::rbp));
	c.lea(x86::rbp, x86::qword_ptr(x86::rbp, args[0]));
	c.and_(args[0].r32(), 0xff80);
	c.shr(args[0].r32(), 1);
	c.movabs(x86::r11, reinterpret_cast<u64>(+vm::g_reservations));
	c.lea(x86::r11, x86::qword_ptr(x86::r11, args[0]));

	// Alloc args[0] to stamp0
	const auto stamp0 = args[0];
	build_get_tsc(c, stamp0);

	// Begin transaction
	Label tx0 = build_transaction_enter(c, fall, [&]()
	{
		c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::ftx)), 1);
		build_get_tsc(c);
		c.sub(x86::rax, stamp0);
		c.movabs(x86::rbx, reinterpret_cast<u64>(&g_rtm_tx_limit1));
		c.cmp(x86::rax, x86::qword_ptr(x86::rbx));
		c.jae(fall);
	});

	// Check pause flag
	c.bt(x86::dword_ptr(args[2], ::offset32(&cpu_thread::state)), static_cast<u32>(cpu_flag::pause));
	c.jc(fall);
	c.mov(x86::rax, x86::qword_ptr(x86::r11));
	c.and_(x86::rax, -128);
	c.cmp(x86::rax, args[3]);
	c.jne(fall);
	c.xbegin(tx0);

	// Just read data to registers
	if (s_tsx_avx)
	{
		c.vmovups(x86::ymm0, x86::ymmword_ptr(x86::rbp, 0));
		c.vmovups(x86::ymm1, x86::ymmword_ptr(x86::rbp, 32));
		c.vmovups(x86::ymm2, x86::ymmword_ptr(x86::rbp, 64));
		c.vmovups(x86::ymm3, x86::ymmword_ptr(x86::rbp, 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(x86::rbp, 0));
		c.movaps(x86::xmm1, x86::oword_ptr(x86::rbp, 16));
		c.movaps(x86::xmm2, x86::oword_ptr(x86::rbp, 32));
		c.movaps(x86::xmm3, x86::oword_ptr(x86::rbp, 48));
		c.movaps(x86::xmm4, x86::oword_ptr(x86::rbp, 64));
		c.movaps(x86::xmm5, x86::oword_ptr(x86::rbp, 80));
		c.movaps(x86::xmm6, x86::oword_ptr(x86::rbp, 96));
		c.movaps(x86::xmm7, x86::oword_ptr(x86::rbp, 112));
	}

	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx)), 1);
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);

	// Store data
	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymmword_ptr(args[1], 0), x86::ymm0);
		c.vmovaps(x86::ymmword_ptr(args[1], 32), x86::ymm1);
		c.vmovaps(x86::ymmword_ptr(args[1], 64), x86::ymm2);
		c.vmovaps(x86::ymmword_ptr(args[1], 96), x86::ymm3);
	}
	else
	{
		c.movaps(x86::oword_ptr(args[1], 0), x86::xmm0);
		c.movaps(x86::oword_ptr(args[1], 16), x86::xmm1);
		c.movaps(x86::oword_ptr(args[1], 32), x86::xmm2);
		c.movaps(x86::oword_ptr(args[1], 48), x86::xmm3);
		c.movaps(x86::oword_ptr(args[1], 64), x86::xmm4);
		c.movaps(x86::oword_ptr(args[1], 80), x86::xmm5);
		c.movaps(x86::oword_ptr(args[1], 96), x86::xmm6);
		c.movaps(x86::oword_ptr(args[1], 112), x86::xmm7);
	}

	c.jmp(_ret);
	c.bind(fall);
	c.xor_(x86::eax, x86::eax);
	//c.jmp(_ret);

	c.bind(_ret);

#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.movups(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
		c.movups(x86::xmm7, x86::oword_ptr(x86::rsp, 16));
	}
#endif

	if (s_tsx_avx)
	{
		c.vzeroupper();
	}

	c.add(x86::rsp, 40);
	c.pop(x86::rbx);
	c.pop(x86::rbp);

	maybe_flush_lbr(c);
	c.ret();
#else
	UNUSED(args);

	c.brk(Imm(0x42));
	c.ret(a64::x30);
#endif
});

void spu_int_ctrl_t::set(u64 ints)
{
	// leave only enabled interrupts
	ints &= mask;

	// notify if at least 1 bit was set
	if (ints && ~stat.fetch_or(ints) & ints)
	{
		std::shared_lock rlock(id_manager::g_mutex);

		if (lv2_obj::check(tag))
		{
			if (auto handler = tag->handler; lv2_obj::check(handler))
			{
				rlock.unlock();
				thread_ctrl::notify(*handler->thread);
			}
		}
	}
}

const spu_imm_table_t g_spu_imm;

spu_imm_table_t::scale_table_t::scale_table_t()
{
	for (s32 i = -155; i < 174; i++)
	{
		m_data[i + 155] = v128::fromf32p(static_cast<float>(std::exp2(i)));
	}
}

spu_imm_table_t::spu_imm_table_t()
{
	for (u32 i = 0; i < std::size(sldq_pshufb); i++)
	{
		for (u32 j = 0; j < 16; j++)
		{
			sldq_pshufb[i]._u8[j] = static_cast<u8>(j - i);
		}
	}

	for (u32 i = 0; i < std::size(srdq_pshufb); i++)
	{
		const u32 im = (0u - i) & 0x1f;

		for (u32 j = 0; j < 16; j++)
		{
			srdq_pshufb[i]._u8[j] = (j + im > 15) ? 0xff : static_cast<u8>(j + im);
		}
	}

	for (u32 i = 0; i < std::size(rldq_pshufb); i++)
	{
		for (u32 j = 0; j < 16; j++)
		{
			rldq_pshufb[i]._u8[j] = static_cast<u8>((j - i) & 0xf);
		}
	}
}

void spu_thread::dump_regs(std::string& ret, std::any& /*custom_data*/) const
{
	const system_state emu_state = Emu.GetStatus(false);
	const bool is_stopped_or_frozen = state & cpu_flag::exit || emu_state == system_state::frozen || emu_state <= system_state::stopping;
	const spu_debugger_mode mode = debugger_mode.load();

	const bool floats_only = !is_stopped_or_frozen && mode == spu_debugger_mode::is_float;
	const bool is_decimal = !is_stopped_or_frozen && mode == spu_debugger_mode::is_decimal;

	SPUDisAsm dis_asm(cpu_disasm_mode::normal, ls);

	std::vector<v128> gpr_saved(128);
	be_t<u32> rdata_saved[32]{};
	be_t<u32> lsa_saved[32]{};
	spu_mfc_cmd mfc_regs_saved{};
	u32 saved_pc = umax;
	const u8* lsa_state_ptr = nullptr;

	// Load PC, GPRs and reservation data atomically
	// We may not load the entire context atomically, but there is importance their state being intact for debugging
	do
	{
		saved_pc = pc;

		// Account for list transfer: record EAL instead
		mfc_regs_saved = ch_mfc_cmd;
		lsa_state_ptr = _ptr<const u8>(mfc_regs_saved.eal < SPU_LS_SIZE && mfc_regs_saved.eal % 8 == 0 ? mfc_regs_saved.eal : mfc_regs_saved.lsa);

		std::memcpy(gpr_saved.data(), gpr.data(), sizeof(v128) * gpr.size());
		std::memcpy(rdata_saved, rdata, sizeof(rdata));
		std::memcpy(lsa_saved, lsa_state_ptr, std::min<usz>(128, SPU_LS_SIZE - (lsa_state_ptr - _ptr<u8>(0))));
		atomic_fence_acquire();
	}
	while (saved_pc != pc || std::memcmp(rdata_saved, rdata, sizeof(rdata)) != 0 || std::memcmp(gpr_saved.data(), gpr.data(), sizeof(v128) * gpr.size()) != 0
		|| std::memcmp(&mfc_regs_saved, &ch_mfc_cmd, sizeof(mfc_regs_saved)) != 0 || std::memcmp(lsa_saved, lsa_state_ptr, std::min<usz>(128, SPU_LS_SIZE - (lsa_state_ptr - _ptr<u8>(0)))) != 0);

	for (u32 i = 0; i < 128; i++, ret += '\n')
	{
		const auto r = gpr_saved[i];

		auto [is_const, const_value] = dis_asm.try_get_const_value(i, saved_pc & ~3);

		if (const_value != r)
		{
			// Expectation of predictable code path has not been met (such as a branch directly to the instruction)
			is_const = false;
		}

		fmt::append(ret, "%s%s ", spu_reg_name[i], is_const ? "©" : ":");

		if (auto [size, dst, src] = SPUDisAsm::try_get_insert_mask_info(r); size)
		{
			// Special: insertion masks

			const std::string_view type =
				size == 1 ? "byte" :
				size == 2 ? "half" :
				size == 4 ? "word" :
				size == 8 ? "dword" : "error";

			if ((size >= 4u && !src) || (size == 2u && src == 1u) || (size == 1u && src == 3u))
			{
				fmt::append(ret, "insert -> %s[%u]", type, dst);
				continue;
			}
		}

		auto to_f64 = [](u32 bits)
		{
			const u32 abs = bits & 0x7fff'ffff;
			constexpr u32 scale = (1 << 23);
			return std::copysign(abs < scale ? 0 : std::ldexp((scale + (abs % scale)) / f64{scale}, static_cast<int>(abs >> 23) - 127), bits >> 31 ? -1 : 1);
		};

		const double array[]{to_f64(r.u32r[0]), to_f64(r.u32r[1]), to_f64(r.u32r[2]), to_f64(r.u32r[3])};

		const u32 i3 = r._u32[3];
		const bool is_packed = v128::from32p(i3) == r;

		if (floats_only)
		{
			fmt::append(ret, "%g, %g, %g, %g", array[0], array[1], array[2], array[3]);
			continue;
		}

		if (is_packed)
		{
			bool printed_error = false;

			if (i3 >> 31)
			{
				const usz old_size = ret.size();

				fmt::append(ret, "%s (0x%x)", CellError{i3}, i3);

				// Test if failed to format (appended " 0x8".. in such case)
				if (ret[old_size] == '0')
				{
					// Failed
					ret.resize(old_size);
				}
				else
				{
					printed_error = true;
				}
			}

			if (!printed_error)
			{
				// Shortand formatting
				if (is_decimal)
				{
					fmt::append(ret, "%-11d", i3);
				}
				else
				{
					fmt::append(ret, "%08x", i3);
				}
			}
		}
		else
		{
			if (is_decimal)
			{
				fmt::append(ret, "%-11d %-11d %-11d %-11d", r.u32r[0], r.u32r[1], r.u32r[2], r.u32r[3]);
			}
			else
			{
				fmt::append(ret, "%08x %08x %08x %08x", r.u32r[0], r.u32r[1], r.u32r[2], r.u32r[3]);
			}
		}

		if (i3 >= 0x80 && is_exec_code(i3, { ls, SPU_LS_SIZE }))
		{
			dis_asm.disasm(i3);
			fmt::append(ret, " -> %s", dis_asm.last_opcode);
		}

		if (std::any_of(std::begin(array), std::end(array), [](f64 v){ return v != 0; }))
		{
			if (is_packed)
			{
				fmt::append(ret, " (%g)", array[0]);
			}
			else
			{
				fmt::append(ret, " (%g, %g, %g, %g)", array[0], array[1], array[2], array[3]);
			}
		}
	}

	const auto events = ch_events.load();

	fmt::append(ret, "\nEvent Stat: 0x%x\n", events.events);
	fmt::append(ret, "Event Mask: 0x%x\n", events.mask);
	fmt::append(ret, "Event Count: %u\n", events.count);
	fmt::append(ret, "SRR0: 0x%05x\n", srr0);
	fmt::append(ret, "Stall Stat: %s\n", ch_stall_stat);
	fmt::append(ret, "Stall Mask: 0x%x\n", ch_stall_mask);
	fmt::append(ret, "Tag Stat: %s\n", ch_tag_stat);
	fmt::append(ret, "Tag Update: %s\n", mfc_tag_update{ch_tag_upd});
	fmt::append(ret, "Atomic Stat: %s\n", ch_atomic_stat); // TODO: use mfc_atomic_status formatting
	fmt::append(ret, "Interrupts: %s\n", interrupts_enabled ? "Enabled" : "Disabled");
	fmt::append(ret, "Inbound Mailbox: %s\n", ch_in_mbox);
	fmt::append(ret, "Out Mailbox: %s\n", ch_out_mbox);
	fmt::append(ret, "Out Interrupts Mailbox: %s\n", ch_out_intr_mbox);
	fmt::append(ret, "SNR config: 0x%llx\n", snr_config);
	fmt::append(ret, "SNR1: %s\n", ch_snr1);
	fmt::append(ret, "SNR2: %s\n", ch_snr2);
	fmt::append(ret, "Last WrDec: %-9d (0x%08x) (%s)\n", ch_dec_value, ch_dec_value, is_dec_frozen ? "suspend" : "running");

	if (get_type() != spu_type::threaded)
	{
		for (usz i = 0; i < int_ctrl.size(); i++)
		{
			fmt::append(ret, "Int Ctrl[%u]: stat=0x%x, mask=0x%x\n", i, +int_ctrl[i].stat, +int_ctrl[i].mask);
		}

		fmt::append(ret, "MFC Prxy Cmd: [%s written: %x]\n", mfc_prxy_cmd, mfc_prxy_write_state.all);
		fmt::append(ret, "MFC Prxy Mask: 0x%x\n", +mfc_prxy_mask);
	}

	const u32 addr = raddr;
	const u64 rtime0 = rtime;

	if (vm::check_addr(addr))
	{
		fmt::append(ret, "Reservation Addr: 0x%x\n", addr);
		fmt::append(ret, "Reservation Time: 0x%x\n", rtime0 & 0xff'ff'ff);
	}
	else if (addr)
	{
		fmt::append(ret, "Reservation Addr: 0x%x (unmapped)\n", addr);
		fmt::append(ret, "Reservation Time: 0x%x\n", rtime0 & 0xff'ff'ff);
	}
	else
	{
		fmt::append(ret, "Reservation Addr: N/A\n");
		fmt::append(ret, "Reservation Time: N/A\n");
	}

	fmt::append(ret, "Reservation Data:\n");

	for (usz i = 0; i < std::size(rdata_saved); i += 4)
	{
		fmt::append(ret, "[0x%02x] %08x %08x %08x %08x\n", i * sizeof(rdata_saved[0])
			, rdata_saved[i + 0], rdata_saved[i + 1], rdata_saved[i + 2], rdata_saved[i + 3]);
	}

	fmt::append(ret, "\nLSA Data:\n");

	for (usz i = 0; i < std::size(lsa_saved); i += 4)
	{
		fmt::append(ret, "[0x%02x] %08x %08x %08x %08x\n", i * sizeof(lsa_saved[0])
			, lsa_saved[i + 0], lsa_saved[i + 1], lsa_saved[i + 2], lsa_saved[i + 3]);
	}
}

std::string spu_thread::dump_callstack() const
{
	std::string ret;

	fmt::append(ret, "Call stack:\n=========\n0x%08x (0x0) called\n", pc);

	for (const auto& sp : dump_callstack_list())
	{
		// TODO: function addresses too
		fmt::append(ret, "> from 0x%08x (sp=0x%08x)\n", sp.first, sp.second);
	}

	return ret;
}

std::vector<std::pair<u32, u32>> spu_thread::dump_callstack_list() const
{
	std::vector<std::pair<u32, u32>> call_stack_list;

	bool first = true;

	const v128 gpr0 = gpr[0];
	const u32 _pc = pc;

	// Declare first 128-bytes as invalid for stack (common values such as 0 do not make sense here)
	for (u32 sp = gpr[1]._u32[3]; (sp & 0xF) == 0u && sp >= 0x80u && sp <= 0x3FFE0u; first = false)
	{
		v128 lr = _ref<v128>(sp + 16);

		auto is_invalid = [this](v128 v)
		{
			const u32 addr = v._u32[3] & 0x3FFFC;

			if (v != v128::from32r(addr))
			{
				// 1) Non-zero lower words are invalid (because BRSL-like instructions generate only zeroes)
				// 2) Bits normally masked out by indirect branches are considered invalid
				return true;
			}

			return !addr || !is_exec_code(addr, { ls, SPU_LS_SIZE });
		};

		if (first && lr._u32[3] != gpr0._u32[3] && !is_invalid(gpr0))
		{
			// Detect functions with no stack or before LR has been stored
			std::vector<bool> passed(_pc / 4);

			// Start with PC
			std::vector<u32> start_points{_pc};

			bool is_ok = false;
			bool all_failed = false;

			for (usz start = 0; !all_failed && start < start_points.size(); start++)
			{
				for (u32 i = start_points[start]; i < SPU_LS_SIZE;)
				{
					if (i / 4 >= passed.size())
					{
						passed.resize(i / 4 + 1);
					}
					else if (passed[i / 4])
					{
						// Already passed
						break;
					}

					passed[i / 4] = true;

					const spu_opcode_t op{_ref<u32>(i)};
					const auto type = s_spu_itype.decode(op.opcode);

					if (start == 0 && type == spu_itype::STQD && op.ra == 1u && op.rt == 0u)
					{
						// Saving LR to stack: this is indeed a new function (ok because LR has not been saved yet)
						is_ok = true;
						break;
					}

					if (type == spu_itype::LQD && op.rt == 0u)
					{
						// Loading LR from stack: this is not a leaf function
						all_failed = true;
						break;
					}

					if (type == spu_itype::UNK)
					{
						// Ignore for now
						break;
					}

					if ((type == spu_itype::BRSL || type == spu_itype::BRASL || type == spu_itype::BISL) && op.rt == 0u)
					{
						// Gave up on link before saving
						all_failed = true;
						break;
					}

					if (type & spu_itype::branch && type >= spu_itype::BI && op.ra == 0u)
					{
						// Returned
						is_ok = true;
						break;
					}

					const auto results = op_branch_targets(i, op);

					bool proceeded = false;
					for (usz res_i = 0; res_i < results.size(); res_i++)
					{
						const u32 route_pc = results[res_i];

						if (route_pc >= SPU_LS_SIZE)
						{
							continue;
						}

						if (route_pc / 4 >= passed.size())
						{
							passed.resize(route_pc / 4 + 1);
						}

						if (!passed[route_pc / 4])
						{
							if (proceeded)
							{
								// Remember next route start point
								start_points.push_back(route_pc);
							}
							else
							{
								// Next PC
								i = route_pc;
								proceeded = true;
							}
						}
					}
				}
			}

			if (is_ok && !all_failed)
			{
				// Same stack as far as we know (for now)
				call_stack_list.emplace_back(gpr0._u32[3], sp);
			}
		}

		if (!is_invalid(lr))
		{
			// TODO: function addresses too
			call_stack_list.emplace_back(lr._u32[3], sp);
		}
		else if (!first)
		{
			break;
		}

		const u32 temp_sp = _ref<u32>(sp);

		if (temp_sp <= sp)
		{
			// Ensure ascending stack frame pointers
			break;
		}

		sp = temp_sp;
	}

	return call_stack_list;
}

std::string spu_thread::dump_misc() const
{
	std::string ret = cpu_thread::dump_misc();

	fmt::append(ret, "Block Weight: %u (Retreats: %u)", block_counter, block_failure);

	if (g_cfg.core.spu_prof)
	{
		// Get short function hash and position in chunk
		fmt::append(ret, "\nCurrent block: %s", spu_block_hash{atomic_storage<u64>::load(block_hash)});
	}

	const u32 offset = group ? SPU_FAKE_BASE_ADDR + (id & 0xffffff) * SPU_LS_SIZE : RAW_SPU_BASE_ADDR + index * RAW_SPU_OFFSET;

	fmt::append(ret, "\n[%s]", ch_mfc_cmd);
	fmt::append(ret, "\nLocal Storage: 0x%08x..0x%08x", offset, offset + 0x3ffff);

	if (const u64 _time = start_time)
	{
		if (const auto func = current_func)
		{
			ret += "\nIn function: ";
			ret += func;
		}
		else
		{
			ret += '\n';
		}


		fmt::append(ret, "\nWaiting: %fs", (get_system_time() - _time) / 1000000.);
	}
	else
	{
		ret += "\n\n";
	}

	fmt::append(ret, "\nTag Mask: 0x%08x", ch_tag_mask);
	fmt::append(ret, "\nMFC Queue Size: %u", mfc_size);

	for (u32 i = 0; i < 16; i++)
	{
		if (i < mfc_size)
		{
			fmt::append(ret, "\n%s", mfc_queue[i]);
		}
		else
		{
			break;
		}
	}

	return ret;
}

void spu_thread::cpu_on_stop()
{
	if (current_func && is_stopped(state - cpu_flag::stop))
	{
		if (start_time)
		{
			ppu_log.warning("'%s' aborted (%fs)", current_func, (get_system_time() - start_time) / 1000000.);
		}
		else
		{
			ppu_log.warning("'%s' aborted", current_func);
		}

		current_func = {};
	}

	// TODO: More conditions
	if (Emu.IsStopped() && g_cfg.core.spu_debug)
	{
		std::string ret;
		dump_all(ret);
		spu_log.notice("thread context: %s", ret);
	}

	if (is_stopped(state - cpu_flag::stop))
	{
		if (stx == 0 && ftx == 0 && last_succ == 0 && last_fail == 0)
		{
			perf_log.notice("SPU thread perf stats are not available.");
		}
		else
		{
			perf_log.notice("Perf stats for transactions: success %u, failure %u", stx, ftx);
			perf_log.notice("Perf stats for PUTLLC reload: success %u, failure %u", last_succ, last_fail);
		}
	}
}

void spu_thread::cpu_init()
{
	std::memset(gpr.data(), 0, gpr.size() * sizeof(gpr[0]));
	fpscr.Reset();

	ch_mfc_cmd = {};

	srr0 = 0;
	mfc_size = 0;
	mfc_barrier = 0;
	mfc_fence = 0;
	ch_tag_upd = 0;
	ch_tag_mask = 0;
	ch_tag_stat.data.raw() = {};
	ch_stall_mask = 0;
	ch_stall_stat.data.raw() = {};
	ch_atomic_stat.data.raw() = {};

	ch_out_mbox.data.raw() = {};
	ch_out_intr_mbox.data.raw() = {};

	ch_events.raw() = {};
	interrupts_enabled = false;
	raddr = 0;

	ch_dec_start_timestamp = get_timebased_time();
	ch_dec_value = option & SYS_SPU_THREAD_OPTION_DEC_SYNC_TB_ENABLE ? ~static_cast<u32>(ch_dec_start_timestamp) : 0;
	is_dec_frozen = false;

	if (get_type() >= spu_type::raw)
	{
		ch_in_mbox.clear();
		ch_snr1.data.raw() = {};
		ch_snr2.data.raw() = {};

		snr_config = 0;

		mfc_prxy_mask.raw() = 0;
		mfc_prxy_write_state = {};
	}

	status_npc.raw() = {get_type() == spu_type::isolated ? SPU_STATUS_IS_ISOLATED : 0, 0};
	run_ctrl.raw() = 0;

	spurs_last_task_timestamp = 0;
	spurs_wait_duration_last = 0;
	spurs_average_task_duration = 0;
	spurs_waited = false;
	spurs_entered_wait = false;

	int_ctrl[0].clear();
	int_ctrl[1].clear();
	int_ctrl[2].clear();

	gpr[1]._u32[3] = 0x3FFF0; // initial stack frame pointer
}

void spu_thread::cpu_return()
{
	if (get_type() >= spu_type::raw)
	{
		if (status_npc.fetch_op([this](status_npc_sync_var& state)
		{
			if (state.status & SPU_STATUS_RUNNING)
			{
				// Save next PC and current SPU Interrupt Status
				// Used only by RunCtrl stop requests
				state.status &= ~SPU_STATUS_RUNNING;
				state.npc = pc | +interrupts_enabled;
				return true;
			}

			return false;
		}).second)
		{
			status_npc.notify_one();
		}
	}
	else if (is_stopped())
	{
		ch_in_mbox.clear();

		if (ensure(group->running)-- == 1)
		{
			u32 last_stop = 0;
			{
				lv2_obj::notify_all_t notify;
				std::lock_guard lock(group->mutex);
				group->run_state = SPU_THREAD_GROUP_STATUS_INITIALIZED;

				if (!group->join_state)
				{
					group->join_state = SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT;
				}

				for (const auto& thread : group->threads)
				{
					if (thread && thread.get() != this && thread->status_npc.load().status >> 16 == SYS_SPU_THREAD_STOP_THREAD_EXIT)
					{
						// Wait for all threads to have error codes if exited by sys_spu_thread_exit
						for (u32 status; !thread->exit_status.try_read(status)
							|| status != thread->last_exit_status;)
						{
							utils::pause();
						}
					}
				}

				if (status_npc.load().status >> 16 == SYS_SPU_THREAD_STOP_THREAD_EXIT)
				{
					// Set exit status now, in conjunction with group state changes
					exit_status.set_value(last_exit_status);
				}

				last_stop = group->stop_count;

				if (last_stop == umax)
				{
					// Restart with some high count to preserve some meaning
					group->stop_count = 1000;
				}
				else
				{
					group->stop_count++;
				}

				if (const auto ppu = std::exchange(group->waiter, nullptr))
				{
					// Send exit status directly to the joining thread
					ppu->gpr[4] = group->join_state;
					ppu->gpr[5] = group->exit_status;
					group->join_state.release(0);
					lv2_obj::awake(ppu);
				}
			}

			// Notify on last thread stopped
			group->stop_count.notify_all();

			// Wait for terminators manually if needed (ensuring they quit before value-wrapping)
			while (last_stop == umax && group->wait_term_count)
			{
				std::this_thread::yield();
			}
		}
		else if (status_npc.load().status >> 16 == SYS_SPU_THREAD_STOP_THREAD_EXIT)
		{
			exit_status.set_value(last_exit_status);
		}
	}
}

extern thread_local std::string(*g_tls_log_prefix)();

void spu_thread::cpu_task()
{
#ifdef __APPLE__
	pthread_jit_write_protect_np(true);
#endif
	start_time = 0;

	// Get next PC and SPU Interrupt status
	pc = status_npc.load().npc;

	// Note: works both on RawSPU and threaded SPU!
	set_interrupt_status((pc & 1) != 0);

	pc &= 0x3fffc;

	std::fesetround(FE_TOWARDZERO);

	gv_set_zeroing_denormals();

	g_tls_log_prefix = []
	{
		const auto cpu = static_cast<spu_thread*>(get_current_cpu_thread());

		static thread_local shared_ptr<std::string> name_cache;

		if (!cpu->spu_tname.is_equal(name_cache)) [[unlikely]]
		{
			cpu->spu_tname.peek_op([&](const shared_ptr<std::string>& ptr)
			{
				if (ptr != name_cache)
				{
					name_cache = ptr;
				}
			});
		}

		const auto type = cpu->get_type();

		if (u64 hash = cpu->block_hash)
		{
			return fmt::format("%sSPU[0x%07x] Thread (%s) [0x%05x: %s]", type >= spu_type::raw ? type == spu_type::isolated ? "Iso" : "Raw" : "", cpu->lv2_id, *name_cache.get(), cpu->pc, spu_block_hash_short{atomic_storage<u64>::load(hash)});
		}

		return fmt::format("%sSPU[0x%07x] Thread (%s) [0x%05x]", type >= spu_type::raw ? type == spu_type::isolated ? "Iso" : "Raw" : "", cpu->lv2_id, *name_cache.get(), cpu->pc);
	};

	constexpr u32 invalid_spurs = 0u - 0x80;

	if (spurs_addr == 0)
	{
		// Evaluate it
		if (!group)
		{
			spurs_addr = invalid_spurs; // Some invalid non-0 address
		}
		else
		{
			const u32 arg = static_cast<u32>(group->args[index][1]);

			if (group->name.ends_with("CellSpursKernelGroup"sv) && vm::check_addr(arg))
			{
				spurs_addr = arg;
				group->spurs_running++;
			}
			else
			{
				spurs_addr = invalid_spurs;
			}
		}
	}

	if (jit)
	{
		while (true)
		{
			if (state) [[unlikely]]
			{
				if (check_state())
					break;
			}

			if (_ref<u32>(pc) == 0x0u)
			{
				if (spu_thread::stop_and_signal(0x0))
					pc += 4;
				continue;
			}

			spu_runtime::g_gateway(*this, _ptr<u8>(0), nullptr);
		}

		if (unsavable && is_stopped(state - cpu_flag::stop))
		{
			spu_log.warning("Aborting unsaveable state");
		}

		// Print some stats
		(!group || group->stop_count < 5 ? spu_log.notice : spu_log.trace)("Stats: Block Weight: %u (Retreats: %u);", block_counter, block_failure);
	}
	else
	{
		ensure(spu_runtime::g_interpreter);

		allow_interrupts_in_cpu_work = true;

		while (true)
		{
			if (state) [[unlikely]]
			{
				if (check_state())
					break;
			}

			spu_runtime::g_interpreter(*this, _ptr<u8>(0), nullptr);
		}

		allow_interrupts_in_cpu_work = false;
	}

	if (spurs_addr != invalid_spurs)
	{
		if (group->spurs_running.exchange(0))
		{
			group->spurs_running.notify_all();
		}
	}
}

void spu_thread::cpu_work()
{
	if (std::exchange(in_cpu_work, true))
	{
		return;
	}

	const u32 old_iter_count = cpu_work_iteration_count++;

	bool work_left = false;

	if (has_active_local_bps)
	{
		const u32 pos_at = pc / 4;
		const u32 pos_bit = 1u << (pos_at % 8);

		if (local_breakpoints[pos_at / 8] & pos_bit)
		{
			// Ignore repeatations until a different instruction is issued
			if (pc != current_bp_pc)
			{
				// Breakpoint hit
				state += cpu_flag::dbg_pause;
			}
		}

		current_bp_pc = pc;
		work_left = true;
	}
	else
	{
		current_bp_pc = umax;
	}

	const auto timeout = +g_cfg.core.mfc_transfers_timeout;

	if (u32 shuffle_count = g_cfg.core.mfc_transfers_shuffling)
	{
		// If either MFC size exceeds limit or timeout has been reached execute pending MFC commands
		if (mfc_size > shuffle_count || (timeout && get_system_time() - mfc_last_timestamp >= timeout))
		{
			work_left = do_mfc(false, false);
		}
		else
		{
			work_left = mfc_size != 0; // TODO: Optimize
		}
	}

	bool gen_interrupt = false;

	// Check interrupts every 16 iterations
	if (!(old_iter_count % 16) && allow_interrupts_in_cpu_work)
	{
		if (u32 mask = ch_events.load().mask & SPU_EVENT_INTR_BUSY_CHECK)
		{
			// LR check is expensive, do it once in a while
			if (old_iter_count /*% 256*/)
			{
				mask &= ~SPU_EVENT_LR;
			}

			get_events(mask);
		}

		gen_interrupt = check_mfc_interrupts(pc);
		work_left |= interrupts_enabled;
	}

	in_cpu_work = false;

	if (!work_left)
	{
		// No more pending work
		state.atomic_op([](bs_t<cpu_flag>& flags)
		{
			if (flags & cpu_flag::pending_recheck)
			{
				// Do not really remove ::pending because external thread may have pushed more pending work
				flags -= cpu_flag::pending_recheck;
			}
			else
			{
				flags -= cpu_flag::pending;
			}
		});
	}

	if (gen_interrupt)
	{
		// Interrupt! escape everything and restart execution
		spu_runtime::g_escape(this);
	}
}

struct raw_spu_cleanup
{
	raw_spu_cleanup() = default;

	raw_spu_cleanup(const raw_spu_cleanup&) = delete;

	raw_spu_cleanup& operator =(const raw_spu_cleanup&) = delete;

	~raw_spu_cleanup()
	{
		std::memset(spu_thread::g_raw_spu_id, 0, sizeof(spu_thread::g_raw_spu_id));
		spu_thread::g_raw_spu_ctr = 0;
		g_fxo->get<raw_spu_cleanup>(); // Register destructor
	}
};

void spu_thread::cleanup()
{
	// Deallocate local storage
	ensure(vm::dealloc(vm_offset(), vm::spu, &shm));

	// Deallocate RawSPU ID
	if (get_type() >= spu_type::raw)
	{
		g_raw_spu_id[index] = 0;
		g_raw_spu_ctr--;
	}

	// Free range lock (and signals cleanup was called to the destructor)
	vm::free_range_lock(range_lock);

	// Terminate and join thread
	static_cast<named_thread<spu_thread>&>(*this) = thread_state::finished;
}

spu_thread::~spu_thread()
{
	// Unmap LS and its mirrors
	shm->unmap(ls + SPU_LS_SIZE);
	shm->unmap(ls);
	shm->unmap(ls - SPU_LS_SIZE);
	utils::memory_release(ls - SPU_LS_SIZE * 2, SPU_LS_SIZE * 5);
}

u8* spu_thread::map_ls(utils::shm& shm, void* ptr)
{
	vm::writer_lock mlock;

	const auto ls = ptr ? static_cast<u8*>(ptr) : static_cast<u8*>(ensure(utils::memory_reserve(SPU_LS_SIZE * 5, nullptr, true))) + SPU_LS_SIZE * 2;
	ensure(shm.map_critical(ls - SPU_LS_SIZE).first && shm.map_critical(ls).first && shm.map_critical(ls + SPU_LS_SIZE).first);
	return ls;
}

void spu_thread::init_spu_decoder()
{
	ensure(!jit);

#if !defined(ARCH_X64) && !defined(ARCH_ARM64)
#error "Unimplemented"
#else
	const spu_decoder_type spu_decoder = g_cfg.core.spu_decoder;

#if defined(ARCH_X64)
	if (spu_decoder == spu_decoder_type::asmjit)
	{
		jit = spu_recompiler_base::make_asmjit_recompiler();
	}
	else
#endif
	if (spu_decoder == spu_decoder_type::llvm)
	{
#if defined(ARCH_X64)
		jit = spu_recompiler_base::make_fast_llvm_recompiler();
#elif defined(ARCH_ARM64)
		jit = spu_recompiler_base::make_llvm_recompiler();
#endif
	}
	else if (spu_decoder == spu_decoder_type::_static || spu_decoder == spu_decoder_type::dynamic)
	{
		//
	}
	else
	{
		fmt::throw_exception("Unsupported spu decoder '%s'", g_cfg.core.spu_decoder);
	}
#endif
}

spu_thread::spu_thread(lv2_spu_group* group, u32 index, std::string_view name, u32 lv2_id, bool is_isolated, u32 option)
	: cpu_thread(idm::last_id())
	, group(group)
	, index(index)
	, thread_type(group ? spu_type::threaded : is_isolated ? spu_type::isolated : spu_type::raw)
	, shm(std::make_shared<utils::shm>(SPU_LS_SIZE))
	, ls(static_cast<u8*>(utils::memory_reserve(SPU_LS_SIZE * 5, nullptr, true)) + SPU_LS_SIZE * 2)
	, option(option)
	, lv2_id(lv2_id)
	, spu_tname(make_single<std::string>(name))
{
	init_spu_decoder();

	if (g_cfg.core.mfc_debug)
	{
		utils::memory_commit(vm::g_stat_addr + vm_offset(), SPU_LS_SIZE);
		mfc_history.resize(max_mfc_dump_idx);
	}

	if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit || g_cfg.core.spu_decoder == spu_decoder_type::llvm)
	{
		if (g_cfg.core.spu_block_size != spu_block_size_type::safe)
		{
			// Initialize stack mirror
			std::memset(stack_mirror.data(), 0xff, sizeof(stack_mirror));
		}
	}

	if (get_type() >= spu_type::raw)
	{
		cpu_init();
	}

	range_lock = vm::alloc_range_lock();

	memset(&hv_ctx, 0, sizeof(hv_ctx));
}

void spu_thread::serialize_common(utils::serial& ar)
{
	ar(gpr, pc, ch_mfc_cmd, mfc_size, mfc_barrier, mfc_fence, mfc_prxy_cmd, mfc_prxy_mask, mfc_prxy_write_state.all
		, srr0, ch_tag_upd, ch_tag_mask, ch_tag_stat.data, ch_stall_mask, ch_stall_stat.data, ch_atomic_stat.data
		, ch_out_mbox.data, ch_out_intr_mbox.data, snr_config, ch_snr1.data, ch_snr2.data, ch_events.raw().all, interrupts_enabled
		, run_ctrl, exit_status.data, status_npc.raw().status, ch_dec_start_timestamp, ch_dec_value, is_dec_frozen);

	ar(std::span(mfc_queue, mfc_size));

	u32 vals[4]{};

	if (ar.is_writing())
	{
		const u8 count = ch_in_mbox.try_read(vals);
		ar(count, std::span(vals, count));
	}
	else
	{
		const u8 count = ar;
		ar(std::span(vals, count));
		ch_in_mbox.set_values(count, vals[0], vals[1], vals[2], vals[3]);
	}
}

spu_thread::spu_thread(utils::serial& ar, lv2_spu_group* group)
	: cpu_thread(idm::last_id())
	, group(group)
	, index(ar)
	, thread_type(group ? spu_type::threaded : ar.pop<u8>() ? spu_type::isolated : spu_type::raw)
	, shm(ensure(vm::get(vm::spu)->peek(vm_offset()).second))
	, ls(map_ls(*this->shm))
	, option(ar)
	, lv2_id(ar)
	, spu_tname(make_single<std::string>(ar.operator std::string()))
{
	init_spu_decoder();

	if (g_cfg.core.mfc_debug)
	{
		utils::memory_commit(vm::g_stat_addr + vm_offset(), SPU_LS_SIZE);
		mfc_history.resize(max_mfc_dump_idx);
	}

	if (g_cfg.core.spu_decoder != spu_decoder_type::_static && g_cfg.core.spu_decoder != spu_decoder_type::dynamic)
	{
		if (g_cfg.core.spu_block_size != spu_block_size_type::safe)
		{
			// Initialize stack mirror
			std::memset(stack_mirror.data(), 0xff, sizeof(stack_mirror));
		}
	}

	range_lock = vm::alloc_range_lock();

	serialize_common(ar);

	raddr = ::narrow<u32>(ar.pop<u64>());

	if (raddr)
	{
		// Acquire reservation
		if (!vm::check_addr(raddr))
		{
			fmt::throw_exception("SPU Serialization: Reservation address is not accessible! (addr=0x%x)", raddr);
		}

		rtime = vm::reservation_acquire(raddr);
		mov_rdata(rdata, *vm::get_super_ptr<spu_rdata_t>(raddr));
	}

	status_npc.raw().npc = pc | u8{interrupts_enabled};

	if (get_type() == spu_type::threaded)
	{
		for (auto& pair : spuq)
		{
			ar(pair.first);
			pair.second = idm::get_unlocked<lv2_obj, lv2_event_queue>(ar.pop<u32>());
		}

		for (auto& q : spup)
		{
			q = idm::get_unlocked<lv2_obj, lv2_event_queue>(ar.pop<u32>());
		}
	}
	else
	{
		for (spu_int_ctrl_t& ctrl : int_ctrl)
		{
			ar(ctrl.mask, ctrl.stat);
			ctrl.tag = idm::get_unlocked<lv2_obj, lv2_int_tag>(ar.pop<u32>());
		}

		g_raw_spu_ctr++;
		g_raw_spu_id[index] = id;
	}

	ar(stop_flag_removal_protection);
}

void spu_thread::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(spu);

	if (raddr)
	{
		// Last check for reservation-lost event
		get_events(SPU_EVENT_LR);
	}

	ar(index);

	if (get_type() != spu_type::threaded)
	{
		ar(u8{get_type() == spu_type::isolated});
	}

	ar(option, lv2_id, *spu_tname.load());

	serialize_common(ar);

	// Let's save it as u64 for future proofing
	ar(u64{raddr});

	if (get_type() == spu_type::threaded)
	{
		for (const auto& [key, q] : spuq)
		{
			ar(key);
			ar(lv2_obj::check(q) ? q->id : 0);
		}

		for (auto& p : spup)
		{
			ar(lv2_obj::check(p) ? p->id : 0);
		}
	}
	else
	{
		for (const spu_int_ctrl_t& ctrl : int_ctrl)
		{
			ar(ctrl.mask, ctrl.stat, lv2_obj::check(ctrl.tag) ? ctrl.tag->id : 0);
		}
	}

	ar(!!(state & cpu_flag::stop));
}

void spu_thread::push_snr(u32 number, u32 value)
{
	// Get channel
	const auto channel = number & 1 ? &ch_snr2 : &ch_snr1;

	// Prepare some data
	const u32 event_bit = SPU_EVENT_S1 >> (number & 1);
	const bool bitor_bit = !!((snr_config >> number) & 1);

	// Redundant, g_use_rtm is checked inside tx_start now.
	if (g_use_rtm && false)
	{
		bool channel_notify = false;
		bool thread_notify = false;

		const bool ok = utils::tx_start([&]
		{
			channel_notify = (channel->data.raw() == spu_channel::bit_wait);
			thread_notify = (channel->data.raw() & spu_channel::bit_count) == 0;

			if (channel_notify)
			{
				ensure(channel->jostling_value.raw() == spu_channel::bit_wait);
				channel->jostling_value.raw() = value;
				channel->data.raw() = 0;
			}
			else if (bitor_bit)
			{
				channel->data.raw() &= ~spu_channel::bit_wait;
				channel->data.raw() |= spu_channel::bit_count | value;
			}
			else
			{
				channel->data.raw() = spu_channel::bit_count | value;
			}

			if (thread_notify)
			{
				ch_events.raw().events |= event_bit;

				if (ch_events.raw().mask & event_bit)
				{
					ch_events.raw().count = 1;
					thread_notify = ch_events.raw().waiting != 0;
				}
				else
				{
					thread_notify = false;
				}
			}
		});

		if (ok)
		{
			if (channel_notify)
				channel->data.notify_one();
			if (thread_notify)
				this->notify();

			return;
		}
	}

	// Lock event channel in case it needs event notification
	ch_events.atomic_op([](ch_events_t& ev)
	{
		ev.locks++;
	});

	// Check corresponding SNR register settings
	auto push_state = channel->push(value, bitor_bit);

	if (push_state.old_count < push_state.count)
	{
		set_events(event_bit);
	}
	else if (!push_state.op_done)
	{
		ensure(is_stopped());

		if (auto cpu = cpu_thread::get_current())
		{
			cpu->state += cpu_flag::again;
		}
	}

	ch_events.atomic_op([](ch_events_t& ev)
	{
		ev.locks--;
	});
}

void spu_thread::do_dma_transfer(spu_thread* _this, const spu_mfc_cmd& args, u8* ls)
{
	perf_meter<"DMA"_u32> perf_;

	const bool is_get = (args.cmd & ~(MFC_BARRIER_MASK | MFC_FENCE_MASK | MFC_START_MASK)) == MFC_GET_CMD;

	u32 eal = args.eal;
	u32 lsa = args.lsa & 0x3ffff;

	// Keep src point to const
	u8* dst = nullptr;
	const u8* src = nullptr;

	std::tie(dst, src) = [&]() -> std::pair<u8*, const u8*>
	{
		u8* dst = vm::_ptr<u8>(eal);
		u8* src = ls + lsa;

		if (is_get)
		{
			std::swap(src, dst);
		}

		return {dst, src};
	}();

	// SPU Thread Group MMIO (LS and SNR) and RawSPU MMIO
	if (_this && eal >= RAW_SPU_BASE_ADDR)
	{
		if (g_cfg.core.mfc_debug && _this)
		{
			// TODO
		}

		const u32 index = (eal - SYS_SPU_THREAD_BASE_LOW) / SYS_SPU_THREAD_OFFSET; // thread number in group
		const u32 offset = (eal - SYS_SPU_THREAD_BASE_LOW) % SYS_SPU_THREAD_OFFSET; // LS offset or MMIO register

		if (eal < SYS_SPU_THREAD_BASE_LOW)
		{
			// RawSPU MMIO
			auto thread = idm::get_unlocked<named_thread<spu_thread>>(find_raw_spu((eal - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET));

			if (!thread)
			{
				// Access Violation
			}
			else if ((eal - RAW_SPU_BASE_ADDR) % RAW_SPU_OFFSET + args.size - 1 < SPU_LS_SIZE) // LS access
			{
			}
			else if (u32 value; args.size == 4 && is_get && thread->read_reg(eal, value))
			{
				_this->_ref<u32>(lsa) = value;
				return;
			}
			else if (args.size == 4 && !is_get && thread->write_reg(eal, args.cmd != MFC_SDCRZ_CMD ? + _this->_ref<u32>(lsa) : 0))
			{
				return;
			}
			else
			{
				fmt::throw_exception("Invalid RawSPU MMIO offset (cmd=[%s])", args);
			}
		}
		else if (_this->get_type() >= spu_type::raw)
		{
			// Access Violation
		}
		else if (_this->group && _this->group->threads_map[index] != -1)
		{
			auto& spu = static_cast<spu_thread&>(*_this->group->threads[_this->group->threads_map[index]]);

			if (offset + args.size <= SPU_LS_SIZE) // LS access
			{
				// redirect access
				if (auto ptr = spu.ls + offset; is_get)
					src = ptr;
				else
					dst = ptr;
			}
			else if (!is_get && args.size == 4 && (offset == SYS_SPU_THREAD_SNR1 || offset == SYS_SPU_THREAD_SNR2))
			{
				spu.push_snr(SYS_SPU_THREAD_SNR2 == offset, args.cmd != MFC_SDCRZ_CMD ? +_this->_ref<u32>(lsa) : 0);
				return;
			}
			else
			{
				fmt::throw_exception("Invalid MMIO offset (cmd=[%s])", args);
			}
		}
		else
		{
			// Access Violation
		}
	}

	// Cleanup: if PUT or GET happens after PUTLLC failure, it's too complicated and it's easier to just give up
	if (_this)
	{
		_this->last_faddr = 0;
	}

	// It is so rare that optimizations are not implemented (TODO)
	alignas(64) static constexpr u8 zero_buf[0x10000]{};

	if (args.cmd == MFC_SDCRZ_CMD)
	{
		src = zero_buf;
	}

	rsx::reservation_lock<false, 1> rsx_lock(eal, args.size, !is_get && (g_cfg.video.strict_rendering_mode || (g_cfg.core.rsx_fifo_accuracy && !g_cfg.core.spu_accurate_dma && eal < rsx::constants::local_mem_base)));

	if ((!g_use_rtm && !is_get) || g_cfg.core.spu_accurate_dma)  [[unlikely]]
	{
		perf_meter<"ADMA_GET"_u64> perf_get = perf_;
		perf_meter<"ADMA_PUT"_u64> perf_put = perf_;

		cpu_thread* _cpu = _this ? _this : get_current_cpu_thread();

		atomic_t<u64, 64>* range_lock = nullptr;

		if (!_this) [[unlikely]]
		{
			if (_cpu->get_class() == thread_class::spu)
			{
				// Use range_lock of current SPU thread for range locks
				range_lock = static_cast<spu_thread*>(_cpu)->range_lock;
			}
			else
			{
				goto plain_access;
			}
		}
		else
		{
			range_lock = _this->range_lock;
		}

		utils::prefetch_write(range_lock);

		for (u32 size = args.size, size0; is_get; size -= size0, dst += size0, src += size0, eal += size0)
		{
			size0 = std::min<u32>(128 - (eal & 127), std::min<u32>(size, 128));

			for (u64 i = 0;; [&]()
			{
				if (_cpu->state)
				{
					_cpu->check_state();
				}
				else if (++i < 25) [[likely]]
				{
					busy_wait(300);
				}
				else
				{
					_cpu->state += cpu_flag::wait + cpu_flag::temp;
					std::this_thread::yield();
					_cpu->check_state();
				}
			}())
			{
				const u64 time0 = vm::reservation_acquire(eal);

				if (time0 & 127)
				{
					continue;
				}

				const auto cpu = get_current_cpu_thread<spu_thread>();

				alignas(64) u8 temp[128];
				u8* dst0 = cpu && (eal & -128) == cpu->raddr ? temp : dst;

				if (dst0 == +temp && time0 != cpu->rtime)
				{
					// Validate rtime for read data
					cpu->set_events(SPU_EVENT_LR);
					cpu->raddr = 0;
				}

				switch (size0)
				{
				case 1:
				{
					*reinterpret_cast<u8*>(dst0) = *reinterpret_cast<const u8*>(src);
					break;
				}
				case 2:
				{
					*reinterpret_cast<u16*>(dst0) = *reinterpret_cast<const u16*>(src);
					break;
				}
				case 4:
				{
					*reinterpret_cast<u32*>(dst0) = *reinterpret_cast<const u32*>(src);
					break;
				}
				case 8:
				{
					*reinterpret_cast<u64*>(dst0) = *reinterpret_cast<const u64*>(src);
					break;
				}
				case 128:
				{
					mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst0), *reinterpret_cast<const spu_rdata_t*>(src));
					break;
				}
				default:
				{
					auto dst1 = dst0;
					auto src1 = src;
					auto size1 = size0;

					while (size1)
					{
						*reinterpret_cast<v128*>(dst1) = *reinterpret_cast<const v128*>(src1);

						dst1 += 16;
						src1 += 16;
						size1 -= 16;
					}

					break;
				}
				}

				if (time0 != vm::reservation_acquire(eal) || (size0 == 128 && !cmp_rdata(*reinterpret_cast<spu_rdata_t*>(dst0), *reinterpret_cast<const spu_rdata_t*>(src))))
				{
					continue;
				}

				if (dst0 == +temp)
				{
					// Write to LS
					std::memcpy(dst, dst0, size0);

					// Validate data
					if (std::memcmp(dst0, &cpu->rdata[eal & 127], size0) != 0)
					{
						cpu->set_events(SPU_EVENT_LR);
						cpu->raddr = 0;
					}
				}

				break;
			}

			if (size == size0)
			{
				if (g_cfg.core.mfc_debug && _this)
				{
					auto& dump = _this->mfc_history[_this->mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
					dump.cmd = args;
					dump.cmd.eah = _this->pc;
					dump.block_hash = _this->block_hash;
					std::memcpy(dump.data, is_get ? dst : src, std::min<u32>(args.size, 128));
				}

				return;
			}
		}

		if (g_cfg.core.spu_accurate_dma) [[unlikely]]
		{
			for (u32 size0, size = args.size;; size -= size0, dst += size0, src += size0, eal += size0)
			{
				size0 = std::min<u32>(128 - (eal & 127), std::min<u32>(size, 128));

				if (size0 == 128u && g_cfg.core.accurate_cache_line_stores)
				{
					// As atomic as PUTLLUC
					do_cell_atomic_128_store(eal, src);

					if (size == size0)
					{
						break;
					}

					continue;
				}

				// Lock each cache line
				auto& res = vm::reservation_acquire(eal);

				// Lock each bit corresponding to a byte being written, using some free space in reservation memory
				auto* bits = utils::bless<atomic_t<u128>>(vm::g_reservations + ((eal & 0xff80) / 2 + 16));

				// Get writing mask
				const u128 wmask = (~u128{} << (eal & 127)) & (~u128{} >> (127 - ((eal + size0 - 1) & 127)));
				//const u64 start = (eal & 127) / 2;
				//const u64 _end_ = ((eal + size0 - 1) & 127) / 2;
				//const u64 wmask = (UINT64_MAX << start) & (UINT64_MAX >> (63 - _end_));

				u128 old = 0;

				for (u64 i = 0; i != umax; [&]()
				{
					if (_cpu->state & cpu_flag::pause)
					{
						const bool ok = cpu_thread::if_suspended<0>(_cpu, {dst, dst + 64, &res}, [&]
						{
							std::memcpy(dst, src, size0);
							res += 128;
						});

						if (ok)
						{
							// Exit loop and function
							i = -1;
							bits = nullptr;
							return;
						}
					}

					if (true || ++i < 10)
					{
						busy_wait(500);
					}
					else
					{
						// Wait
						_cpu->state += cpu_flag::wait + cpu_flag::temp;
						// bits->wait(old, wmask);
						_cpu->check_state();
					}
				}())
				{
					// Completed in suspend_all()
					if (!bits)
					{
						break;
					}

					bool ok = false;

					std::tie(old, ok) = bits->fetch_op([&](u128& v)
					{
						if (v & wmask)
						{
							return false;
						}

						v |= wmask;
						return true;
					});

					if (ok) [[likely]]
					{
						break;
					}
				}

				if (!bits)
				{
					if (size == size0)
					{
						break;
					}

					continue;
				}

				// Lock reservation (shared)
				auto [_oldd, _ok] = res.fetch_op([&](u64& r)
				{
					if (r & vm::rsrv_unique_lock)
					{
						return false;
					}

					r += 1;
					return true;
				});

				if (!_ok)
				{
					vm::reservation_shared_lock_internal(res);
				}

				// Obtain range lock as normal store
				vm::range_lock(range_lock, eal, size0);

				switch (size0)
				{
				case 1:
				{
					*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
					break;
				}
				case 2:
				{
					*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
					break;
				}
				case 4:
				{
					*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
					break;
				}
				case 8:
				{
					*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
					break;
				}
				case 128:
				{
					mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));
					break;
				}
				default:
				{
					auto _dst = dst;
					auto _src = src;
					auto _size = size0;

					while (_size)
					{
						*reinterpret_cast<v128*>(_dst) = *reinterpret_cast<const v128*>(_src);

						_dst += 16;
						_src += 16;
						_size -= 16;
					}

					break;
				}
				}

				range_lock->release(0);

				res += 127;

				// Release bits and notify
				bits->atomic_op([&](u128& v)
				{
					v &= ~wmask;
				});

				// bits->notify_all(wmask);

				if (size == size0)
				{
					break;
				}
			}

			//atomic_fence_seq_cst();

			if (g_cfg.core.mfc_debug && _this)
			{
				auto& dump = _this->mfc_history[_this->mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
				dump.cmd = args;
				dump.cmd.eah = _this->pc;
				dump.block_hash = _this->block_hash;
				std::memcpy(dump.data, is_get ? dst : src, std::min<u32>(args.size, 128));
			}

			return;
		}
		else
		{
			perf_put.reset();
			perf_get.reset();
		}

		perf_meter<"DMA_PUT"_u64> perf2 = perf_;

		switch (u32 size = args.size)
		{
		case 1:
		{
			vm::range_lock<1>(range_lock, eal, 1);
			*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
			range_lock->release(0);
			break;
		}
		case 2:
		{
			vm::range_lock<2>(range_lock, eal, 2);
			*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
			range_lock->release(0);
			break;
		}
		case 4:
		{
			vm::range_lock<4>(range_lock, eal, 4);
			*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
			range_lock->release(0);
			break;
		}
		case 8:
		{
			vm::range_lock<8>(range_lock, eal, 8);
			*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
			range_lock->release(0);
			break;
		}
		default:
		{
			if (eal >> 28 == rsx::constants::local_mem_base >> 28)
			{
				if (size > s_rep_movsb_threshold)
				{
					__movsb(dst, src, size);
				}
				else
				{
					// Avoid unaligned stores in mov_rdata_avx
					if (reinterpret_cast<u64>(dst) & 0x10)
					{
						*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

						dst += 16;
						src += 16;
						size -= 16;
					}

					while (size >= 128)
					{
						mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));

						dst += 128;
						src += 128;
						size -= 128;
					}

					while (size)
					{
						*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

						dst += 16;
						src += 16;
						size -= 16;
					}
				}

				break;
			}

			if (((eal & 127) + size) <= 128)
			{
				vm::range_lock(range_lock, eal, size);

				while (size)
				{
					*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

					dst += 16;
					src += 16;
					size -= 16;
				}

				range_lock->release(0);
				break;
			}

			u32 range_addr = eal & -128;
			u32 range_end = utils::align(eal + size, 128);

			// Handle the case of crossing 64K page borders (TODO: maybe split in 4K fragments?)
			if (range_addr >> 16 != (range_end - 1) >> 16)
			{
				u32 nexta = range_end & -65536;
				u32 size0 = nexta - eal;
				size -= size0;

				// Split locking + transfer in two parts (before 64K border, and after it)
				vm::range_lock(range_lock, range_addr, size0);

				if (size > s_rep_movsb_threshold)
				{
					__movsb(dst, src, size0);
					dst += size0;
					src += size0;
				}
				else
				{
					// Avoid unaligned stores in mov_rdata_avx
					if (reinterpret_cast<u64>(dst) & 0x10)
					{
						*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

						dst += 16;
						src += 16;
						size0 -= 16;
					}

					while (size0 >= 128)
					{
						mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));

						dst += 128;
						src += 128;
						size0 -= 128;
					}

					while (size0)
					{
						*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

						dst += 16;
						src += 16;
						size0 -= 16;
					}
				}

				range_lock->release(0);
				range_addr = nexta;
			}

			vm::range_lock(range_lock, range_addr, range_end - range_addr);

			if (size > s_rep_movsb_threshold)
			{
				__movsb(dst, src, size);
			}
			else
			{
				// Avoid unaligned stores in mov_rdata_avx
				if (reinterpret_cast<u64>(dst) & 0x10)
				{
					*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

					dst += 16;
					src += 16;
					size -= 16;
				}

				while (size >= 128)
				{
					mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));

					dst += 128;
					src += 128;
					size -= 128;
				}

				while (size)
				{
					*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

					dst += 16;
					src += 16;
					size -= 16;
				}
			}

			range_lock->release(0);
			break;
		}
		}

		if (g_cfg.core.mfc_debug && _this)
		{
			auto& dump = _this->mfc_history[_this->mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
			dump.cmd = args;
			dump.cmd.eah = _this->pc;
			dump.block_hash = _this->block_hash;
			std::memcpy(dump.data, is_get ? dst : src, std::min<u32>(args.size, 128));
		}

		return;
	}

plain_access:

	switch (u32 size = args.size)
	{
	case 1:
	{
		*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
		break;
	}
	case 2:
	{
		*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
		break;
	}
	case 4:
	{
		*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
		break;
	}
	case 8:
	{
		*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
		break;
	}
	default:
	{
		if (size > s_rep_movsb_threshold)
		{
			__movsb(dst, src, size);
		}
		else
		{
			// Avoid unaligned stores in mov_rdata_avx
			if (reinterpret_cast<u64>(dst) & 0x10)
			{
				*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

				dst += 16;
				src += 16;
				size -= 16;
			}

			while (size >= 128)
			{
				mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));

				dst += 128;
				src += 128;
				size -= 128;
			}

			while (size)
			{
				*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

				dst += 16;
				src += 16;
				size -= 16;
			}
		}

		break;
	}
	}

	if (g_cfg.core.mfc_debug && _this)
	{
		auto& dump = _this->mfc_history[_this->mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
		dump.cmd = args;
		dump.cmd.eah = _this->pc;
		dump.block_hash = _this->block_hash;
		std::memcpy(dump.data, is_get ? dst : src, std::min<u32>(args.size, 128));
	}
}

bool spu_thread::do_dma_check(const spu_mfc_cmd& args)
{
	const u32 mask = utils::rol32(1, args.tag);

	if (mfc_barrier & mask || (args.cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK) && mfc_fence & mask)) [[unlikely]]
	{
		// Check for special value combination (normally impossible)
		if (false)
		{
			// Update barrier/fence masks if necessary
			mfc_barrier = 0;
			mfc_fence = 0;

			for (u32 i = 0; i < mfc_size; i++)
			{
				if ((mfc_queue[i].cmd & ~0xc) == MFC_BARRIER_CMD)
				{
					mfc_barrier |= -1;
					mfc_fence |= utils::rol32(1, mfc_queue[i].tag);
					continue;
				}

				if (true)
				{
					const u32 _mask = utils::rol32(1u, mfc_queue[i].tag);

					// A command with barrier hard blocks that tag until it's been dealt with
					if (mfc_queue[i].cmd & MFC_BARRIER_MASK)
					{
						mfc_barrier |= _mask;
					}

					// A new command that has a fence can't be executed until the stalled list has been dealt with
					mfc_fence |= _mask;
				}
			}

			if (mfc_barrier & mask || (args.cmd & MFC_FENCE_MASK && mfc_fence & mask))
			{
				return false;
			}

			return true;
		}

		return false;
	}

	return true;
}

bool spu_thread::do_list_transfer(spu_mfc_cmd& args)
{
	perf_meter<"MFC_LIST"_u64> perf0;

	// Amount of elements to fetch in one go
	constexpr u32 fetch_size = 6;

	struct alignas(8) list_element
	{
		u8 sb; // Stall-and-Notify bit (0x80)
		u8 pad;
		be_t<u16> ts; // List Transfer Size
		be_t<u32> ea; // External Address Low
	};

	alignas(16) list_element items[fetch_size];
	static_assert(sizeof(v128) % sizeof(list_element) == 0);

	spu_mfc_cmd transfer;
	transfer.eah  = 0;
	transfer.tag  = args.tag;
	transfer.cmd  = MFC{static_cast<u8>(args.cmd & ~0xf)};

	u32 index = fetch_size;

	auto item_ptr = _ptr<const list_element>(args.eal & 0x3fff8);
	u32 arg_lsa = args.lsa & 0x3fff0;
	u32 arg_size = args.size;

	u8 optimization_compatible = transfer.cmd & (MFC_GET_CMD | MFC_PUT_CMD);

	if (spu_log.trace || g_cfg.core.spu_accurate_dma || g_cfg.core.mfc_debug)
	{
		optimization_compatible = 0;
	}

	rsx::reservation_lock<false, 1> rsx_lock(0, 128, optimization_compatible == MFC_PUT_CMD && (g_cfg.video.strict_rendering_mode || (g_cfg.core.rsx_fifo_accuracy && !g_cfg.core.spu_accurate_dma)));

	constexpr u32 ts_mask = 0x7fff;

	// Assume called with size greater than 0
	while (true)
	{
		// Check if fetching is needed
		if (index == fetch_size)
		{
			const v128 data0 = v128::loadu(item_ptr, 0);
			const v128 data1 = v128::loadu(item_ptr, 1);
			const v128 data2 = v128::loadu(item_ptr, 2);

			// In a perfect world this would not be needed until after the if but relying on the compiler to keep the elements in SSE registers through it all is unrealistic
			std::memcpy(&items[sizeof(v128) / sizeof(list_element) * 0], &data0, sizeof(v128));
			std::memcpy(&items[sizeof(v128) / sizeof(list_element) * 1], &data1, sizeof(v128));
			std::memcpy(&items[sizeof(v128) / sizeof(list_element) * 2], &data2, sizeof(v128));

			u32 s_size = data0._u32[0];

			// We need to verify matching between odd and even elements (vector test is position independent)
			// 0-5 is the most unlikely couple match for many reasons so it skips the entire check very efficiently in most cases
			// Assumes padding bits should match
			if (optimization_compatible == MFC_GET_CMD && s_size == data0._u32[2] && arg_size >= fetch_size * 8)
			{
				const v128 ored = (data0 | data1 | data2) & v128::from64p(std::bit_cast<be_t<u64>>(1ull << 63 | (u64{ts_mask} << 32) | 0xe000'0000));
				const v128 anded = (data0 & data1 & data2) & v128::from64p(std::bit_cast<be_t<u64>>(0xe000'0000 | (u64{ts_mask} << 32)));

				// Tests:
				// 1. Unset stall-and-notify bit on all 6 elements
				// 2. Equality of transfer size across all 6 elements
				// 3. Be in the same 512mb region, this is because this case is not expected to be broken usually and we need to ensure MMIO is not involved in any of the transfers (assumes MMIO to be so rare that this is the last check)
				if (ored == anded && items[0].ea < RAW_SPU_BASE_ADDR && items[1].ea < RAW_SPU_BASE_ADDR)
				{
					// Execute the postponed byteswapping and masking
					s_size = std::bit_cast<be_t<u32>>(s_size) & ts_mask;

					u8* src = vm::_ptr<u8>(0);
					u8* dst = this->ls + arg_lsa;

					// Assume success, prepare the next elements
					arg_lsa += fetch_size * utils::align<u32>(s_size, 16);
					item_ptr += fetch_size;
					arg_size -= fetch_size * 8;

					// Type which is friendly for fused address calculations
					constexpr usz _128 = 128;

					// This whole function relies on many constraints to be met (crashes real MFC), we can a have minor optimization assuming EA alignment to be +16 with +16 byte transfers
#define MOV_T(type, index, _ea) { const usz ea = _ea; *reinterpret_cast<type*>(dst + index * utils::align<u32>(sizeof(type), 16) + ea % (sizeof(type) < 16 ? 16 : 1)) = *reinterpret_cast<const type*>(src + ea); } void()
#define MOV_128(index, ea) mov_rdata(*reinterpret_cast<decltype(rdata)*>(dst + index * _128), *reinterpret_cast<const decltype(rdata)*>(src + (ea)))

					switch (s_size)
					{
					case 0:
					{
						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 1:
					{
						MOV_T(u8, 0, items[0].ea);
						MOV_T(u8, 1, items[1].ea);
						MOV_T(u8, 2, items[2].ea);
						MOV_T(u8, 3, items[3].ea);
						MOV_T(u8, 4, items[4].ea);
						MOV_T(u8, 5, items[5].ea);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 2:
					{
						MOV_T(u16, 0, items[0].ea);
						MOV_T(u16, 1, items[1].ea);
						MOV_T(u16, 2, items[2].ea);
						MOV_T(u16, 3, items[3].ea);
						MOV_T(u16, 4, items[4].ea);
						MOV_T(u16, 5, items[5].ea);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 4:
					{
						MOV_T(u32, 0, items[0].ea);
						MOV_T(u32, 1, items[1].ea);
						MOV_T(u32, 2, items[2].ea);
						MOV_T(u32, 3, items[3].ea);
						MOV_T(u32, 4, items[4].ea);
						MOV_T(u32, 5, items[5].ea);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 8:
					{
						MOV_T(u64, 0, items[0].ea);
						MOV_T(u64, 1, items[1].ea);
						MOV_T(u64, 2, items[2].ea);
						MOV_T(u64, 3, items[3].ea);
						MOV_T(u64, 4, items[4].ea);
						MOV_T(u64, 5, items[5].ea);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 16:
					{
						MOV_T(v128, 0, items[0].ea);
						MOV_T(v128, 1, items[1].ea);
						MOV_T(v128, 2, items[2].ea);
						MOV_T(v128, 3, items[3].ea);
						MOV_T(v128, 4, items[4].ea);
						MOV_T(v128, 5, items[5].ea);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 32:
					{
						struct mem
						{
							v128 a[2];
						};

						MOV_T(mem, 0, items[0].ea);
						MOV_T(mem, 1, items[1].ea);
						MOV_T(mem, 2, items[2].ea);
						MOV_T(mem, 3, items[3].ea);
						MOV_T(mem, 4, items[4].ea);
						MOV_T(mem, 5, items[5].ea);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 48:
					{
						struct mem
						{
							v128 a[3];
						};

						MOV_T(mem, 0, items[0].ea);
						MOV_T(mem, 1, items[1].ea);
						MOV_T(mem, 2, items[2].ea);
						MOV_T(mem, 3, items[3].ea);
						MOV_T(mem, 4, items[4].ea);
						MOV_T(mem, 5, items[5].ea);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 64:
					{
						struct mem
						{
							v128 a[4];
						};

						// TODO: Optimize (4 16-bytes movings is bad)
						MOV_T(mem, 0, items[0].ea);
						MOV_T(mem, 1, items[1].ea);
						MOV_T(mem, 2, items[2].ea);
						MOV_T(mem, 3, items[3].ea);
						MOV_T(mem, 4, items[4].ea);
						MOV_T(mem, 5, items[5].ea);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 128:
					{
						MOV_128(0, items[0].ea);
						MOV_128(1, items[1].ea);
						MOV_128(2, items[2].ea);
						MOV_128(3, items[3].ea);
						MOV_128(4, items[4].ea);
						MOV_128(5, items[5].ea);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 256:
					{
						const usz ea0 = items[0].ea;
						MOV_128(0, ea0 + 0);
						MOV_128(1, ea0 + _128);
						const usz ea1 = items[1].ea;
						MOV_128(2, ea1 + 0);
						MOV_128(3, ea1 + _128);
						const usz ea2 = items[2].ea;
						MOV_128(4, ea2 + 0);
						MOV_128(5, ea2 + _128);
						const usz ea3 = items[3].ea;
						MOV_128(6, ea3 + 0);
						MOV_128(7, ea3 + _128);
						const usz ea4 = items[4].ea;
						MOV_128(8, ea4 + 0);
						MOV_128(9, ea4 + _128);
						const usz ea5 = items[5].ea;
						MOV_128(10, ea5 + 0);
						MOV_128(11, ea5 + _128);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					case 512:
					{
						const usz ea0 = items[0].ea;
						MOV_128(0 , ea0 + _128 * 0);
						MOV_128(1 , ea0 + _128 * 1);
						MOV_128(2 , ea0 + _128 * 2);
						MOV_128(3 , ea0 + _128 * 3);
						const usz ea1 = items[1].ea;
						MOV_128(4 , ea1 + _128 * 0);
						MOV_128(5 , ea1 + _128 * 1);
						MOV_128(6 , ea1 + _128 * 2);
						MOV_128(7 , ea1 + _128 * 3);
						const usz ea2 = items[2].ea;
						MOV_128(8 , ea2 + _128 * 0);
						MOV_128(9 , ea2 + _128 * 1);
						MOV_128(10, ea2 + _128 * 2);
						MOV_128(11, ea2 + _128 * 3);
						const usz ea3 = items[3].ea;
						MOV_128(12, ea3 + _128 * 0);
						MOV_128(13, ea3 + _128 * 1);
						MOV_128(14, ea3 + _128 * 2);
						MOV_128(15, ea3 + _128 * 3);
						const usz ea4 = items[4].ea;
						MOV_128(16, ea4 + _128 * 0);
						MOV_128(17, ea4 + _128 * 1);
						MOV_128(18, ea4 + _128 * 2);
						MOV_128(19, ea4 + _128 * 3);
						const usz ea5 = items[5].ea;
						MOV_128(20, ea5 + _128 * 0);
						MOV_128(21, ea5 + _128 * 1);
						MOV_128(22, ea5 + _128 * 2);
						MOV_128(23, ea5 + _128 * 3);

						if (!arg_size)
						{
							return true;
						}

						continue;
					}
					default:
					{
						// TODO: Are more cases common enough? (in the range of less than 512 bytes because for more than that the optimization is doubtful)
						break;
					}
					}
#undef MOV_T
#undef MOV_128
					// Optimization miss, revert changes
					arg_lsa -= fetch_size * utils::align<u32>(s_size, 16);
					item_ptr -= fetch_size;
					arg_size += fetch_size * 8;
				}
			}

			// Reset to elements array head
			index = 0;
		}

		const u32 size = items[index].ts & ts_mask;
		const u32 addr = items[index].ea;

		// Try to inline the transfer
		if (addr < RAW_SPU_BASE_ADDR && size && optimization_compatible == MFC_GET_CMD)
		{
			const u8* src = vm::_ptr<u8>(addr);
			u8* dst = this->ls + arg_lsa + (addr & 0xf);

			switch (u32 _size = size)
			{
			case 1:
			{
				*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
				break;
			}
			case 2:
			{
				*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
				break;
			}
			case 4:
			{
				*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
				break;
			}
			case 8:
			{
				*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
				break;
			}
			default:
			{
				if (_size > s_rep_movsb_threshold)
				{
					__movsb(dst, src, _size);
				}
				else
				{
					// Avoid unaligned stores in mov_rdata_avx
					if (reinterpret_cast<u64>(dst) & 0x10)
					{
						*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

						dst += 16;
						src += 16;
						_size -= 16;
					}

					while (_size >= 128)
					{
						mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));

						dst += 128;
						src += 128;
						_size -= 128;
					}

					while (_size)
					{
						*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

						dst += 16;
						src += 16;
						_size -= 16;
					}
				}

				break;
			}
			}

			arg_lsa += utils::align<u32>(size, 16);
		}
		// Avoid inlining huge transfers because it intentionally drops range lock unlock
		else if (optimization_compatible == MFC_PUT_CMD && ((addr >> 28 == rsx::constants::local_mem_base >> 28) || (addr < RAW_SPU_BASE_ADDR && size - 1 <= 0x400 - 1 && (addr % 0x10000 + (size - 1)) < 0x10000)))
		{
			if (addr >> 28 != rsx::constants::local_mem_base >> 28)
			{
				rsx_lock.update_if_enabled(addr, size, range_lock);

				if (!g_use_rtm)
				{
					vm::range_lock(range_lock, addr & -128, utils::align<u32>(addr + size, 128) - (addr & -128));
				}
			}
			else
			{
				range_lock->release(0);
				rsx_lock.unlock();
			}

			u8* dst = vm::_ptr<u8>(addr);
			const u8* src = this->ls + arg_lsa + (addr & 0xf);

			switch (u32 _size = size)
			{
			case 1:
			{
				*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
				break;
			}
			case 2:
			{
				*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
				break;
			}
			case 4:
			{
				*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
				break;
			}
			case 8:
			{
				*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
				break;
			}
			default:
			{
				if (_size > s_rep_movsb_threshold)
				{
					__movsb(dst, src, _size);
				}
				else
				{
					// Avoid unaligned stores in mov_rdata_avx
					if (reinterpret_cast<u64>(dst) & 0x10)
					{
						*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

						dst += 16;
						src += 16;
						_size -= 16;
					}

					while (_size >= 128)
					{
						mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));

						dst += 128;
						src += 128;
						_size -= 128;
					}

					while (_size)
					{
						*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

						dst += 16;
						src += 16;
						_size -= 16;
					}
				}

				break;
			}
			}

			arg_lsa += utils::align<u32>(size, 16);
		}
		else if (size)
		{
			range_lock->release(0);
			rsx_lock.unlock();

			spu_log.trace("LIST: item=0x%016x, lsa=0x%05x", std::bit_cast<be_t<u64>>(items[index]), arg_lsa | (addr & 0xf));

			transfer.eal  = addr;
			transfer.lsa  = arg_lsa | (addr & 0xf);
			transfer.size = size;

			arg_lsa += utils::align<u32>(size, 16);
			do_dma_transfer(this, transfer, ls);
		}

		arg_size -= 8;

		if (!arg_size)
		{
			// No more elements
			break;
		}

		item_ptr++;

		if (items[index].sb & 0x80) [[unlikely]]
		{
			range_lock->release(0);

			ch_stall_mask |= utils::rol32(1, args.tag);

			if (!ch_stall_stat.get_count())
			{
				set_events(SPU_EVENT_SN);
			}

			ch_stall_stat.set_value(utils::rol32(1, args.tag) | ch_stall_stat.get_value());

			args.tag |= 0x80; // Set stalled status
			args.eal = ::narrow<u32>(reinterpret_cast<const u8*>(item_ptr) - this->ls);
			args.lsa = arg_lsa;
			args.size = arg_size;
			return false;
		}

		index++;
	}

	range_lock->release(0);
	return true;
}

bool spu_thread::do_putllc(const spu_mfc_cmd& args)
{
	perf_meter<"PUTLLC-"_u64> perf0;
	perf_meter<"PUTLLC+"_u64> perf1 = perf0;

	// Store conditionally
	const u32 addr = args.eal & -128;

	if ([&]()
	{
		perf_meter<"PUTLLC."_u64> perf2 = perf0;

		if (raddr != addr)
		{
			return false;
		}

		const auto& to_write = _ref<spu_rdata_t>(args.lsa & 0x3ff80);
		auto& res = vm::reservation_acquire(addr);

		// TODO: Limit scope!!
		rsx::reservation_lock rsx_lock(addr, 128);

		if (rtime != res)
		{
			if (!g_cfg.core.spu_accurate_reservations && cmp_rdata(to_write, rdata))
			{
				raddr = 0;
				return true;
			}

			return false;
		}

		if (cmp_rdata(to_write, rdata))
		{
			if (!g_cfg.core.spu_accurate_reservations)
			{
				raddr = 0;
				return true;
			}

			// Writeback of unchanged data. Only check memory change
			if (cmp_rdata(rdata, vm::_ref<spu_rdata_t>(addr)) && res.compare_and_swap_test(rtime, rtime + 128))
			{
				raddr = 0; // Disable notification
				return true;
			}

			return false;
		}

		auto [_oldd, _ok] = res.fetch_op([&](u64& r)
		{
			if ((r & -128) != rtime || (r & 127))
			{
				return false;
			}

			r += vm::rsrv_unique_lock;
			return true;
		});

		if (!_ok)
		{
			// Already locked or updated: give up
			return false;
		}

		if (!g_cfg.core.spu_accurate_reservations)
		{
			if (addr - spurs_addr <= 0x80)
			{
				mov_rdata(*vm::_ptr<spu_rdata_t>(addr), to_write);
				res += 64;
				return true;
			}
		}
		else if (!g_use_rtm)
		{
			*vm::_ptr<atomic_t<u32>>(addr) += 0;
		}

		if (g_use_rtm) [[likely]]
		{
			switch (u64 count = spu_putllc_tx(addr, rtime, rdata, to_write))
			{
			case umax:
			{
				auto& data = *vm::get_super_ptr<spu_rdata_t>(addr);

				const bool ok = cpu_thread::suspend_all<+3>(this, {data, data + 64, &res}, [&]()
				{
					if ((res & -128) == rtime)
					{
						if (cmp_rdata(rdata, data))
						{
							mov_rdata(data, to_write);
							res += 64;
							return true;
						}
					}

					// Save previous data
					mov_rdata_nt(rdata, data);
					res -= 64;
					return false;
				});

				const u64 count2 = utils::get_tsc() - perf2.get();

				if (count2 > 20000 && g_cfg.core.perf_report) [[unlikely]]
				{
					perf_log.warning("PUTLLC: took too long: %.3fus (%u c) (addr=0x%x) (S)", count2 / (utils::get_tsc_freq() / 1000'000.), count2, addr);
				}

				if (ok)
				{
					break;
				}

				last_ftime = -1;
				[[fallthrough]];
			}
			case 0:
			{
				if (addr == last_faddr)
				{
					last_fail++;
				}

				if (last_ftime != umax)
				{
					last_faddr = 0;
					return false;
				}

				utils::prefetch_read(rdata);
				utils::prefetch_read(rdata + 64);
				last_faddr = addr;
				last_ftime = res.load() & -128;
				last_ftsc = utils::get_tsc();
				return false;
			}
			default:
			{
				if (count > 20000 && g_cfg.core.perf_report) [[unlikely]]
				{
					perf_log.warning("PUTLLC: took too long: %.3fus (%u c) (addr = 0x%x)", count / (utils::get_tsc_freq() / 1000'000.), count, addr);
				}

				break;
			}
			}

			if (addr == last_faddr)
			{
				last_succ++;
			}

			last_faddr = 0;
			return true;
		}

		auto& super_data = *vm::get_super_ptr<spu_rdata_t>(addr);
		const bool success = [&]()
		{
			// Full lock (heavyweight)
			// TODO: vm::check_addr
			vm::writer_lock lock(addr, range_lock);

			if (cmp_rdata(rdata, super_data))
			{
				mov_rdata(super_data, to_write);
				return true;
			}

			return false;
		}();

		res += success ? 64 : 0 - 64;
		return success;
	}())
	{
		if (raddr)
		{
			if (raddr != spurs_addr || pc != 0x11e4)
			{
				vm::reservation_notifier_notify(addr);
			}
			else
			{
				const u32 thread_bit_mask = (1u << index);
				constexpr usz SPU_IDLE = 0x73;

				const bool switched_from_running_to_idle = (static_cast<u8>(rdata[SPU_IDLE]) & thread_bit_mask) == 0 && (_ref<u8>(0x100 + SPU_IDLE) & thread_bit_mask) != 0;

				if (switched_from_running_to_idle)
				{
					vm::reservation_notifier_notify(addr);
				}
			}

			raddr = 0;
		}

		perf0.reset();
		return true;
	}
	else
	{
		if (raddr)
		{
			// Last check for event before we clear the reservation
			if (~ch_events.load().events & SPU_EVENT_LR)
			{
				if (raddr == addr)
				{
					set_events(SPU_EVENT_LR);
				}
				else
				{
					get_events(SPU_EVENT_LR);
				}
			}
		}

		if (!vm::check_addr(addr, vm::page_writable))
		{
			utils::trigger_write_page_fault(vm::base(addr));
		}

		raddr = 0;
		perf1.reset();
		return false;
	}
}

void do_cell_atomic_128_store(u32 addr, const void* to_write)
{
	perf_meter<"STORE128"_u64> perf0;

	const auto cpu = get_current_cpu_thread();
	rsx::reservation_lock rsx_lock(addr, 128);

	u64 shared_mem = vm::g_shmem[addr >> 16];

	if (!shared_mem)
	{
		shared_mem = addr;
	}

	{
		auto& sdata = *vm::get_super_ptr<spu_rdata_t>(addr);
		auto& res = *utils::bless<atomic_t<u128>>(vm::g_reservations + (addr & 0xff80) / 2);

		if (std::memcmp(static_cast<const u8*>(to_write), &sdata, 16) == 0 && std::memcmp(static_cast<const u8*>(to_write) + 64, &sdata[64], 16) == 0)
		{
			const auto& write_data = *static_cast<const spu_rdata_t*>(to_write);
			const u64 at_read_time = vm::reservation_acquire(addr);

			if (!(at_read_time & 127))
			{
				if (cmp_rdata(sdata, write_data) && at_read_time ==  vm::reservation_acquire(addr) && cmp_rdata(sdata, write_data))
				{
					// Write of the same data (verified atomically)
					vm::try_reservation_update(addr);
					return;
				}
			}
		}

		for (u64 j = 0;; j++)
		{
			auto [_oldd, _ok] = res.fetch_op([&](u128& r)
			{
				if (r & 127)
				{
					return false;
				}

				r &= static_cast<u64>(r);
				r |= u128{shared_mem} << 64;
				r |= u128{vm::rsrv_unique_lock | vm::rsrv_putunc_flag};
				return true;
			});

			if (_ok)
			{
				break;
			}

			if (static_cast<u64>(_oldd) & vm::rsrv_putunc_flag && static_cast<u64>(_oldd >> 64) == shared_mem)
			{
				// Abandon store
				for (u64 k = 0;; k++)
				{
					if (res ^ _oldd)
					{
						break;
					}

					if (auto cpu = get_current_cpu_thread(); cpu && cpu->state)
					{
						cpu->check_state();
					}
					else if (k < 15)
					{
						busy_wait(500);
					}
					else
					{
						std::this_thread::yield();
					}
				}

				return static_cast<void>(cpu->test_stopped());
			}

			if (auto cpu = get_current_cpu_thread(); cpu && cpu->state)
			{
				cpu->check_state();
			}
			else if (j < 15)
			{
				busy_wait(500);
			}
			else
			{
				std::this_thread::yield();
			}
		}

		u64 result = 1;

		if (!g_cfg.core.spu_accurate_reservations)
		{
			mov_rdata(sdata, *static_cast<const spu_rdata_t*>(to_write));
			vm::reservation_acquire(addr) += 32;
		}
		else if (cpu->state & cpu_flag::pause)
		{
			result = 0;
		}
		else if (!g_use_rtm)
		{
			// Provoke page fault
			utils::trigger_write_page_fault(vm::base(addr));

			// Hard lock
			auto spu = cpu ? cpu->try_get<spu_thread>() : nullptr;
			vm::writer_lock lock(addr, spu ? spu->range_lock : nullptr);
			mov_rdata(sdata, *static_cast<const spu_rdata_t*>(to_write));
			vm::reservation_acquire(addr) += 32;
		}
		else if (cpu->get_class() != thread_class::spu)
		{
			u64 stx, ftx;
			result = spu_putlluc_tx(addr, to_write, &stx, &ftx);
		}
		else
		{
			auto _spu = static_cast<spu_thread*>(cpu);
			result = spu_putlluc_tx(addr, to_write, &_spu->stx, &_spu->ftx);
		}

		if (result == 0)
		{
			cpu_thread::suspend_all<+2>(cpu, {}, [&]
			{
				mov_rdata(sdata, *static_cast<const spu_rdata_t*>(to_write));
			});

			vm::reservation_acquire(addr) += 32;
			result = utils::get_tsc() - perf0.get();
		}

		if (result > 20000 && g_cfg.core.perf_report) [[unlikely]]
		{
			perf_log.warning("STORE128: took too long: %.3fus (%u c) (addr=0x%x)", result / (utils::get_tsc_freq() / 1000'000.), result, addr);
		}

		static_cast<void>(cpu->test_stopped());
	}
}

void spu_thread::do_putlluc(const spu_mfc_cmd& args)
{
	perf_meter<"PUTLLUC"_u64> perf0;

	const u32 addr = args.eal & -128;

	if (raddr && addr == raddr && g_cfg.core.spu_accurate_reservations)
	{
		// Try to process PUTLLUC using PUTLLC when a reservation is active:
		// If it fails the reservation is cleared, LR event is set and we fallback to the main implementation
		// All of this is done atomically in PUTLLC
		if (!(ch_events.load().events & SPU_EVENT_LR) && do_putllc(args))
		{
			// Success, return as our job was done here
			return;
		}

		// Failure, fallback to the main implementation
		raddr = 0;
	}

	do_cell_atomic_128_store(addr, _ptr<spu_rdata_t>(args.lsa & 0x3ff80));
	vm::reservation_notifier_notify(addr);
}

bool spu_thread::do_mfc(bool can_escape, bool must_finish)
{
	u32 removed = 0;
	u32 barrier = 0;
	u32 fence = 0;
	u16 exec_mask = 0;
	bool pending = false;

	auto process_command = [&](spu_mfc_cmd& args)
	{
		// Select tag bit in the tag mask or the stall mask
		const u32 mask = utils::rol32(1, args.tag);

		if ((args.cmd & ~0xc) == MFC_BARRIER_CMD)
		{
			if (&args - mfc_queue <= removed)
			{
				// Remove barrier-class command if it's the first in the queue
				atomic_fence_seq_cst();
				removed++;
				return true;
			}

			// Block all tags
			barrier |= -1;
			fence |= mask;
			return false;
		}

		if (barrier & mask)
		{
			fence |= mask;
			return false;
		}

		if (args.cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK) && fence & mask)
		{
			if (args.cmd & MFC_BARRIER_MASK)
			{
				barrier |= mask;
			}

			return false;
		}

		// If command is not enabled in execution mask, execute it later
		if (!(exec_mask & (1u << (&args - mfc_queue))))
		{
			if (args.cmd & MFC_BARRIER_MASK)
			{
				barrier |= mask;
			}

			// Fence is set for any command
			fence |= mask;
			pending = true;
			return false;
		}

		if (args.cmd & MFC_LIST_MASK)
		{
			if (!(args.tag & 0x80))
			{
				if (do_list_transfer(args))
				{
					removed++;
					return true;
				}
			}

			if (args.cmd & MFC_BARRIER_MASK)
			{
				barrier |= mask;
			}

			fence |= mask;
			return false;
		}

		if (args.cmd == MFC_PUTQLLUC_CMD)
		{
			if (fence & mask)
			{
				return false;
			}

			do_putlluc(args);
		}
		else if (args.size)
		{
			do_dma_transfer(this, args, ls);
		}

		removed++;
		return true;
	};

	auto get_exec_mask = [&size = mfc_size]
	{
		// Get commands' execution mask
		// Mask bits are always set when mfc_transfers_shuffling is 0
		return static_cast<u16>((0 - (1u << std::min<u32>(g_cfg.core.mfc_transfers_shuffling, size))) | utils::get_tsc());
	};

	// Process enqueued commands
	while (true)
	{
		removed = 0;
		barrier = 0;
		fence = 0;

		// Shuffle commands execution (if enabled), explicit barriers are obeyed
		pending = false;
		exec_mask = get_exec_mask();

		static_cast<void>(std::remove_if(mfc_queue + 0, mfc_queue + mfc_size, process_command));

		mfc_size -= removed;
		mfc_barrier = barrier;
		mfc_fence = fence;

		if (removed && ch_tag_upd)
		{
			const u32 completed = get_mfc_completed();

			if (completed && ch_tag_upd == MFC_TAG_UPDATE_ANY)
			{
				ch_tag_stat.set_value(completed);
				ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			}
			else if (completed == ch_tag_mask && ch_tag_upd == MFC_TAG_UPDATE_ALL)
			{
				ch_tag_stat.set_value(completed);
				ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			}
		}

		if (can_escape && check_mfc_interrupts(pc + 4))
		{
			spu_runtime::g_escape(this);
		}

		if (!pending)
		{
			break;
		}

		if (!must_finish && g_cfg.core.mfc_shuffling_in_steps)
		{
			// Exit early, not all pending commands have to be executed at a single iteration
			// Update last timestamp so the next MFC timeout check will use the current time
			mfc_last_timestamp = get_system_time();
			return true;
		}
	}

	return false;
}

bool spu_thread::check_mfc_interrupts(u32 next_pc)
{
	if (ch_events.load().count && std::exchange(interrupts_enabled, false))
	{
		srr0 = next_pc;

		// Test for BR/BRA instructions (they are equivalent at zero pc)
		const u32 br = _ref<u32>(0);
		pc = (br & 0xfd80007f) == 0x30000000 ? (br >> 5) & 0x3fffc : 0;
		return true;
	}

	return false;
}

bool spu_thread::is_exec_code(u32 addr, std::span<const u8> ls_ptr, u32 base_addr, bool avoid_dead_code, bool is_range_limited)
{
	bool had_conditional = false;

	for (u32 i = 0; i < 40; i++)
	{
		if (addr & ~0x3FFFC)
		{
			return is_range_limited;
		}

		if (addr < base_addr || addr >= base_addr + ls_ptr.size())
		{
			return is_range_limited;
		}

		const u32 addr0 = spu_branch_target(addr);
		const spu_opcode_t op{read_from_ptr<be_t<u32>>(ls_ptr, addr0 - base_addr)};
		const auto type = s_spu_itype.decode(op.opcode);

		if (type == spu_itype::UNK || !op.opcode)
		{
			return false;
		}

		if (type == spu_itype::STOP)
		{
			if (op.rb)
			{
				return false;
			}

			if (avoid_dead_code)
			{
				switch (op.opcode)
				{
				case SYS_SPU_THREAD_STOP_YIELD:
				case SYS_SPU_THREAD_STOP_GROUP_EXIT:
				case SYS_SPU_THREAD_STOP_THREAD_EXIT:
				case SYS_SPU_THREAD_STOP_RECEIVE_EVENT:
				case SYS_SPU_THREAD_STOP_TRY_RECEIVE_EVENT:
				case SYS_SPU_THREAD_STOP_SWITCH_SYSTEM_MODULE:
				{
					break;
				}
				default:
				{
					return false;
				}
				}
			}
		}

		if (type == spu_itype::STOPD && !had_conditional)
		{
			return !avoid_dead_code;
		}

		if (i != 0 && type == spu_itype::STOPD)
		{
			return true;
		}
		if (type & spu_itype::branch)
		{
			if (type == spu_itype::BR && op.rt && op.rt != 127u)
			{
				return false;
			}

			auto results = op_branch_targets(addr, op);

			if (results[0] == umax)
			{
				switch (type)
				{
				case spu_itype::BIZ:
				case spu_itype::BINZ:
				case spu_itype::BIHZ:
				case spu_itype::BIHNZ:
				{
					results[0] = addr + 4;
					break;
				}
				default:
				{
					break;
				}
				}

				if (results[0] == umax)
				{
					break;
				}
			}
			else
			{
				switch (type)
				{
				case spu_itype::BR:
				case spu_itype::BRNZ:
				case spu_itype::BRZ:
				case spu_itype::BRHNZ:
				case spu_itype::BRHZ:
				case spu_itype::BRSL:
				{
					const s32 rel = bf_t<s32, 0, 18>::extract(static_cast<s32>(u32{op.i16} << 2));

					if (rel == 0 && !had_conditional && avoid_dead_code)
					{
						// Infinite loop 100%, detect that as invalid code
						return false;
					}

					// Detect "invalid" relative branches
					// Branch offsets that, although are the only way to get X code address using relative address
					// Rely on overflow/underflow of SPU memory bounds
					// Thus they would behave differently if SPU LS memory size was to increase (evolving the CELL architecture was the original plan)
					// Making them highly unlikely to be valid code

					if (rel < 0)
					{
						if (addr < 0u - rel)
						{
							return is_range_limited;
						}
					}
					else if (SPU_LS_SIZE - addr <= rel + 0u)
					{
						return is_range_limited;
					}

					if (type == spu_itype::BRSL)
					{
						// Insert a virtual return-to-next, because it is usually a call
						results[1] = addr + 4;
						std::swap(results[1], results[0]);
					}

					break;
				}
				default:
				{
					break;
				}
				}
			}

			for (usz res_i = 1; res_i < results.size(); res_i++)
			{
				const u32 route_pc = results[res_i];

				if (route_pc >= SPU_LS_SIZE)
				{
					continue;
				}

				if (route_pc < base_addr || route_pc >= base_addr + ls_ptr.size())
				{
					return is_range_limited;
				}

				// Test the validity of a single instruction of the optional target
				// This function can't be too slow and is unlikely to improve results by a great deal
				const u32 op0 = read_from_ptr<be_t<u32>>(ls_ptr, route_pc - base_addr);
				const spu_itype::type type0 = s_spu_itype.decode(op0);

				if (type0 == spu_itype::UNK || !op0)
				{
					return false;
				}

				had_conditional = true;
			}

			addr = spu_branch_target(results[0]);
			continue;
		}

		addr += 4;
	}

	return true;
}

u32 spu_thread::get_mfc_completed() const
{
	return ch_tag_mask & ~mfc_fence;
}

u32 evaluate_spin_optimization(std::span<u8> stats, u64 evaluate_time, const cfg::uint<0, 100>& wait_percent, bool inclined_for_responsiveness = false)
{
	ensure(stats.size() >= 2 && stats.size() <= 16);

	const u32 percent = wait_percent;

	// Predict whether or not to use operating system sleep based on history

	std::array<u8, 16> old_stats{};
	std::copy_n(stats.data(), stats.size(), old_stats.data());

	// Rotate history (prepare newest entry)
	stats[0] = 0;
	std::copy_n(old_stats.data(), stats.size() - 1, stats.data() + 1);

	u32 total_wait = 0;
	u32 zero_count = 0; // Try to ignore major inconsistencies
	u32 consecutive_zero = 0;
	u32 consecutive_zero_or_one = 0;
	u32 consecutive_zero_or_one_tally = 0;

	usz index = umax;

	for (u8 val : old_stats)
	{
		index++;

		if (index == stats.size())
		{
			break;
		}

		total_wait += val;

		if (val == 0)
		{
			if (consecutive_zero == index)
			{
				consecutive_zero++;
				consecutive_zero_or_one++;
				//consecutive_zero_or_one_tally += 0;
			}

			++zero_count;
		}

		if (val == 1)
		{
			if (consecutive_zero_or_one == index)
			{
				consecutive_zero_or_one++;
				consecutive_zero_or_one_tally++;
			}
		}
	}

	if (inclined_for_responsiveness)
	{
		total_wait /= 2;
	}

	// Add to chance if previous wait was long enough
	u32 add_count = 0;

	if (stats.size() == 4)
	{
		add_count = zero_count == 3 && total_wait >= 9 ? (total_wait - 8) * 40
			: zero_count == 2 && total_wait >= 8 ? (total_wait - 7) * 40 
			: zero_count == 1 && total_wait >= 7 ? (total_wait - 6) * 40
			: zero_count == 0 && total_wait >= 4 ? (total_wait - 3) * 40
			: 0;
	}
	else
	{
		add_count = zero_count >= 12 && total_wait >= 80 ? (total_wait - 80) * 30
			: zero_count >= 7 && total_wait >= 30 ? (total_wait - 30) * 10 
			: zero_count >= 4 && total_wait >= 20 ? (total_wait - 20) * 10
			: zero_count >= 0 && total_wait >= 10 ? (total_wait - 10) * 10
			: 0;
	}

	if (stats.size() == 16 && (consecutive_zero >= 2 || (consecutive_zero_or_one >= 3 && consecutive_zero_or_one_tally < consecutive_zero_or_one * 2 / 3)))
	{
		// Thread is back to action after some sleep
		add_count = 0;
	}

	if (inclined_for_responsiveness && std::count(old_stats.data(), old_stats.data() + 3, 0) >= 2)
	{
		add_count = 0;
	}

	// Evalute its value (shift-right to ensure its randomness with different CPUs)
	const u32 busy_waiting_switch = ((evaluate_time >> 8) % 100 + add_count < percent) ? 1 : 0;

	thread_local usz g_system_wait = 0, g_busy_wait = 0;

	if (busy_waiting_switch)
	{
		g_busy_wait++;
	}
	else
	{
		g_system_wait++;
	}

	if ((g_system_wait + g_busy_wait) && (g_system_wait + g_busy_wait) % 200 == 0)
	{
		spu_log.trace("SPU wait: count=%d. switch=%d, spin=%d, busy=%d, system=%d, {%d, %d, %d, %d}", total_wait, busy_waiting_switch, !"TODO: Spin", +g_busy_wait, +g_system_wait, old_stats[0], old_stats[1], old_stats[2], old_stats[3]);
	}

	if ((g_system_wait + g_busy_wait) % 5000 == 0)
	{
		g_system_wait = 0;
		g_busy_wait = 0;
	}

	return busy_waiting_switch;
}

bool spu_thread::process_mfc_cmd()
{
	// Stall infinitely if MFC queue is full
	while (mfc_size >= 16) [[unlikely]]
	{
		// Reset MFC timestamp in the case of full queue
		mfc_last_timestamp = 0;

		if (test_stopped())
		{
			return false;
		}

		// Process MFC commands
		do_mfc();

		if (mfc_size < 16)
		{
			break;
		}

		auto old = state.add_fetch(cpu_flag::wait);

		if (is_stopped(old))
		{
			return false;
		}

		thread_ctrl::wait_on(state, old);
	}

	spu::scheduler::concurrent_execution_watchdog watchdog(*this);
	spu_log.trace("DMAC: (%s)", ch_mfc_cmd);

	switch (ch_mfc_cmd.cmd)
	{
	case MFC_SDCRT_CMD:
	case MFC_SDCRTST_CMD:
		return true;
	case MFC_GETLLAR_CMD:
	{
		perf_meter<"GETLLAR"_u64> perf0;

		const u32 addr = ch_mfc_cmd.eal & -128;
		const auto& data = vm::_ref<spu_rdata_t>(addr);

		if (addr == last_faddr)
		{
			// TODO: make this configurable and possible to disable
			spu_log.trace(u8"GETLLAR after fail: addr=0x%x, time=%u c", last_faddr, (perf0.get() - last_ftsc));
		}

		if (addr == last_faddr && perf0.get() - last_ftsc < 1000 && (vm::reservation_acquire(addr) & -128) == last_ftime)
		{
			rtime = last_ftime;
			raddr = last_faddr;
			last_ftime = 0;
			mov_rdata(_ref<spu_rdata_t>(ch_mfc_cmd.lsa & 0x3ff80), rdata);

			ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);
			return true;
		}
		else
		{
			// Silent failure
			last_faddr = 0;
		}

		if (raddr)
		{
			if (raddr != addr)
			{
				// Last check for event before we replace the reservation with a new one
				if (reservation_check(raddr, rdata))
				{
					set_events(SPU_EVENT_LR);
				}
			}
			else
			{
				// Check if we can reuse our existing reservation
				auto& res = vm::reservation_acquire(addr);
				const u64 this_time = res;

				if (this_time % 128 == 0 && cmp_rdata(rdata, data))
				{
					mov_rdata(_ref<spu_rdata_t>(ch_mfc_cmd.lsa & 0x3ff80), rdata);
					ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);

					// Need to check twice for it to be accurate, the code is before and not after this check for:
					// 1. Reduce time between reservation accesses so TSX panelty would be lowered
					// 2. Increase the chance of change detection: if GETLLAR has been called again new data is probably wanted
					if (this_time == res && cmp_rdata(rdata, data))
					{
						if (this_time != rtime)
						{
							// Reservation was lost but the data itself remains unchanged so try to ignore it
							set_events(SPU_EVENT_LR);
							rtime = this_time;
						}

						if ([&]() -> bool
						{
							// Validation that it is indeed GETLLAR spinning (large time window is intentional)
							if (last_getllar_addr != addr || last_getllar_gpr1 != gpr[1]._u32[3] || perf0.get() - last_gtsc >= 5'000 || (interrupts_enabled && ch_events.load().mask))
							{
								// Seemingly not
								getllar_busy_waiting_switch = umax;
								getllar_spin_count = 0;
								return true;
							}

							if (last_getllar != pc || last_getllar_lsa != ch_mfc_cmd.lsa)
							{
								getllar_busy_waiting_switch = umax;
								getllar_spin_count = 0;
								return true;
							}

							// Check if LSA points to an OUT buffer on the stack from a caller - unlikely to be a loop
							if (last_getllar_lsa >= SPU_LS_SIZE - 0x10000 && last_getllar_lsa > last_getllar_gpr1)
							{
								auto cs = dump_callstack_list();

								if (!cs.empty() && last_getllar_lsa > cs[0].second)
								{
									getllar_busy_waiting_switch = umax;
									getllar_spin_count = 0;
									return true;
								}
							}

							getllar_spin_count = std::min<u32>(getllar_spin_count + 1, u16{umax});

							if (getllar_busy_waiting_switch == umax && getllar_spin_count == 4)
							{
								// Hidden value to force busy waiting (100 to 1 are dynamically adjusted, 0 is not)
								if (!g_cfg.core.spu_getllar_spin_optimization_disabled)
								{
									getllar_evaluate_time = perf0.get();
									auto& history = getllar_wait_time[(addr % SPU_LS_SIZE) / 128];

									getllar_busy_waiting_switch =
										evaluate_spin_optimization({ history.data(), history.size() }, getllar_evaluate_time, g_cfg.core.spu_getllar_busy_waiting_percentage);
								}
								else
								{
									getllar_busy_waiting_switch = 1;
								}
							}
							// Don't be stubborn, force operating sleep if too much time has passed
							else if (getllar_busy_waiting_switch == 1 && perf0.get() > getllar_evaluate_time && perf0.get() - getllar_evaluate_time >= 400'000)
							{
								// Hidden value to force busy waiting
								if (!g_cfg.core.spu_getllar_spin_optimization_disabled)
								{
									spu_log.trace("SPU wait for 0x%x", addr);
									getllar_wait_time[(addr % SPU_LS_SIZE) / 128].front() = 1;
									getllar_busy_waiting_switch = 0;
								}
							}

							// Either 1 or umax
							return getllar_busy_waiting_switch != 0;
						}())
						{
							if (g_cfg.core.mfc_debug)
							{
								auto& dump = mfc_history[mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
								dump.cmd = ch_mfc_cmd;
								dump.cmd.eah = pc;
								dump.block_hash = block_hash;
								std::memcpy(dump.data, rdata, 128);
							}

							last_getllar = pc;
							last_getllar_gpr1 = gpr[1]._u32[3];
							last_getllar_lsa = ch_mfc_cmd.lsa;

							if (getllar_busy_waiting_switch == 1)
							{
								getllar_wait_time[(addr % SPU_LS_SIZE) / 128].front() = 0;

#if defined(ARCH_X64)
								if (utils::has_um_wait())
								{
									if (utils::has_waitpkg())
									{
										__tpause(std::min<u32>(getllar_spin_count, 10) * 500, 0x1);
									}
									else
									{
										struct check_wait_t
										{
											static FORCE_INLINE bool needs_wait(u64 rtime, const atomic_t<u64>& mem_rtime) noexcept
											{
												return rtime == mem_rtime;
											}
										};

										// Provide the first X64 cache line of the reservation to be tracked
										__mwaitx<check_wait_t>(std::min<u32>(getllar_spin_count, 17) * 500, 0xf0, std::addressof(data), +rtime, vm::reservation_acquire(addr));
									}
								}
								else
#endif
								{
									busy_wait(300);
								}

								if (getllar_spin_count == 3)
								{
									// Check other reservations in other threads
									lv2_obj::notify_all();
								}

								// Reset perf
								perf0.restart();
							}

							last_gtsc = perf0.get();
							return true;
						}

						// Spinning, might as well yield cpu resources
						state += cpu_flag::wait;

						usz cache_line_waiter_index = umax;

						if (auto wait_var = vm::reservation_notifier_begin_wait(addr, rtime))
						{
							cache_line_waiter_index = register_cache_line_waiter(addr);
							utils::bless<atomic_t<u32>>(&wait_var->raw().wait_flag)->wait(1, atomic_wait_timeout{100'000});
							vm::reservation_notifier_end_wait(*wait_var);
						}

						deregister_cache_line_waiter(cache_line_waiter_index);

						static_cast<void>(test_stopped());

						// Quick check if there were reservation changes
						const u64 new_time = res;

						if (new_time % 128 == 0 && cmp_rdata(rdata, data) && res == new_time && cmp_rdata(rdata, data))
						{
							if (g_cfg.core.mfc_debug)
							{
								auto& dump = mfc_history[mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
								dump.cmd = ch_mfc_cmd;
								dump.cmd.eah = pc;
								dump.block_hash = block_hash;
								std::memcpy(dump.data, rdata, 128);
							}

							if (new_time != rtime)
							{
								// Reservation was lost but the data itself remains unchanged so try to ignore it
								set_events(SPU_EVENT_LR);
								rtime = new_time;
							}

							u8& val = getllar_wait_time[(addr % SPU_LS_SIZE) / 128].front();
							val = static_cast<u8>(std::min<u32>(val + 1, u8{umax}));

							// Reset perf
							perf0.restart();
							last_gtsc = perf0.get();
							return true;
						}

						static atomic_t<u32> g_changed, g_unchanged;

						if (new_time == this_time && res == this_time)
						{
							spu_log.trace("RTIME unchanged on address 0x%x", addr);

							g_unchanged++;

							// Notify threads manually, memory data has likely changed and broke the reservation for others
							if (vm::reservation_notifier_count(addr) && res == new_time)
							{
								vm::reservation_notifier_notify(addr);
							}
						}
						else
						{
							g_changed++;
						}

						if ((g_changed + g_unchanged) % 200 == 0)
						{
							spu_log.trace("SPU GETLLAR wait on RTIME stats: unchanged=%d, changed=%d", +g_unchanged, +g_changed);
						}
					}
				}

				if (this_time == rtime)
				{
					// Notify threads manually, memory data has likely changed and broke the reservation for others
					if (vm::reservation_notifier_count(addr) && res == this_time)
					{
						vm::reservation_notifier_notify(addr);
					}
				}

				// We can't, LR needs to be set now
				set_events(SPU_EVENT_LR);
				static_cast<void>(test_stopped());

				// Reset perf
				perf0.restart();
			}

			last_getllar = pc;
			last_gtsc = perf0.get();
		}

		last_getllar_addr = addr;
		getllar_spin_count = 0;
		getllar_busy_waiting_switch = umax;

		u64 ntime = 0;
		rsx::reservation_lock rsx_lock(addr, 128);

		for (u64 i = 0; i != umax; [&]()
		{
			if (state & cpu_flag::pause)
			{
				auto& sdata = *vm::get_super_ptr<spu_rdata_t>(addr);

				const bool ok = cpu_thread::if_suspended<0>(this, {&ntime}, [&]
				{
					// Guaranteed success
					ntime = vm::reservation_acquire(addr);
					mov_rdata_nt(rdata, sdata);
				});

				// Exit loop
				if (ok && (ntime & 127) == 0)
				{
					atomic_fence_seq_cst();
					i = -1;
					return;
				}
			}

			if (i < 24) [[likely]]
			{
				i++;
				busy_wait(300);
			}
			else
			{
				state += cpu_flag::wait + cpu_flag::temp;
				std::this_thread::yield();
				static_cast<void>(check_state());
			}
		}())
		{
			ntime = vm::reservation_acquire(addr);

			if (ntime & vm::rsrv_unique_lock)
			{
				// There's an on-going reservation store, wait
				continue;
			}

			u64 test_mask = -1;

			if (ntime & 127)
			{
				// Try to use TSX to obtain data atomically
				if (!g_use_rtm || !spu_getllar_tx(addr, rdata, this, ntime & -128))
				{
					// See previous ntime check.
					continue;
				}
			}
			else
			{
				mov_rdata(rdata, data);
			}

			if (u64 time0 = vm::reservation_acquire(addr); (ntime & test_mask) != (time0 & test_mask))
			{
				// Reservation data has been modified recently
				if (time0 & vm::rsrv_unique_lock) i += 12;
				continue;
			}

			if (!cmp_rdata(rdata, data))
			{
				i += 2;
				continue;
			}

			if (i >= 15 && g_cfg.core.perf_report) [[unlikely]]
			{
				perf_log.warning("GETLLAR: took too long: %u", i);
			}

			break;
		}

		raddr = addr;
		rtime = ntime;
		mov_rdata(_ref<spu_rdata_t>(ch_mfc_cmd.lsa & 0x3ff80), rdata);

		ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);

		if (g_cfg.core.mfc_debug)
		{
			auto& dump = mfc_history[mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
			dump.cmd = ch_mfc_cmd;
			dump.cmd.eah = pc;
			dump.block_hash = block_hash;
			std::memcpy(dump.data, rdata, 128);
		}

		return true;
	}

	case MFC_PUTLLC_CMD:
	{
		// Avoid logging useless commands if there is no reservation
		const bool dump = g_cfg.core.mfc_debug && raddr;

		const bool is_spurs_task_wait = pc == 0x11e4 && spurs_addr != 0u - 0x80u;

		if (!is_spurs_task_wait || spurs_addr != raddr || spurs_waited)
		{
			//
		}
		else if ((_ref<u8>(0x100 + 0x73) & (1u << index)) == 0 && (static_cast<u8>(rdata[0x73]) & (1u << index)) != 0)
		{
			// Wait for other threads to complete their tasks (temporarily)
			u32 max_run = group->max_run;

			auto [prev_running, ok] = spurs_entered_wait ? std::make_pair(+group->spurs_running, false) :
				group->spurs_running.fetch_op([max_run, num = group->max_num](u32& x)
			{
				if (x >= max_run && max_run < num)
				{
					x--;
					return true;
				}

				return false;
			});

			if (ok || spurs_entered_wait)
			{
				lv2_obj::prepare_for_sleep(*this);

				if (ok)
				{
					if (prev_running == max_run)
					{
						group->spurs_running.notify_one();

						if (group->spurs_running == max_run - 1)
						{
							// Try to let another thread slip in and take over execution
							thread_ctrl::wait_for(300);

							// Update value
							prev_running = group->spurs_running + 1;
						}
					}

					// Restore state
					prev_running--;
				}

				const u64 before = get_system_time();
				u64 current = before;

				spurs_waited = true;
				spurs_entered_wait = true;

				// Wait the duration of one and a half tasks
				const u64 spurs_wait_time = std::clamp<u64>(spurs_average_task_duration / spurs_task_count_to_calculate * 3 / 2, 10'000, 100'000);
				spurs_wait_duration_last = spurs_wait_time;

				if (spurs_last_task_timestamp)
				{
					const u64 avg_entry = spurs_average_task_duration / spurs_task_count_to_calculate;
					spurs_average_task_duration -= avg_entry;
					spurs_average_task_duration += std::min<u64>(45'000, current - spurs_last_task_timestamp);
					spu_log.trace("duration: %d, avg=%d", current - spurs_last_task_timestamp, spurs_average_task_duration / spurs_task_count_to_calculate);
					spurs_last_task_timestamp = 0;
				}

				while (true)
				{
					if (is_stopped() || current - before >= spurs_wait_time)
					{
						// Timed-out
						group->spurs_running++;
						break;
					}

					if (prev_running >= max_run)
					{
						thread_ctrl::wait_on(group->spurs_running, prev_running, spurs_wait_time - (current - before));
					}

					max_run = group->max_run;

					prev_running = group->spurs_running.fetch_op([max_run](u32& x)
					{
						if (x < max_run)
						{
							x++;
							return true;
						}

						return false;
					}).first;

					if (prev_running < max_run)
					{
						break;
					}

					current = get_system_time();
				}

				state += cpu_flag::temp;
				static_cast<void>(test_stopped());
			}
		}

		if (do_putllc(ch_mfc_cmd))
		{
			ch_atomic_stat.set_value(MFC_PUTLLC_SUCCESS);

			if (is_spurs_task_wait)
			{
				const bool is_idle = (_ref<u8>(0x100 + 0x73) & (1u << index)) != 0;
				const bool was_idle = (static_cast<u8>(rdata[0x73]) & (1u << index)) != 0;

				if (!was_idle && is_idle)
				{
					const u32 prev_running = group->spurs_running.fetch_op([](u32& x)
					{
						if (x)
						{
							x--;
							return true;
						}

						return false;
					}).first;

					if (prev_running)
					{
						spurs_entered_wait = true;
					}

					if (prev_running == group->max_run && prev_running < group->max_num)
					{
						group->spurs_running.notify_one();
					}
				}
				else if (was_idle && !is_idle)
				{
					// Cleanup
					const u64 current = get_system_time();

					if (spurs_last_task_timestamp)
					{
						const u64 avg_entry = spurs_average_task_duration / spurs_task_count_to_calculate;
						spurs_average_task_duration -= avg_entry;
						spurs_average_task_duration += std::min<u64>(45'000, current - spurs_last_task_timestamp);
						spu_log.trace("duration: %d, avg=%d", current - spurs_last_task_timestamp, spurs_average_task_duration / spurs_task_count_to_calculate);
						spurs_last_task_timestamp = 0;
					}

					spurs_last_task_timestamp = current;
					spurs_waited = false;
					spurs_entered_wait = false;
				}
			}
		}
		else
		{
			ch_atomic_stat.set_value(MFC_PUTLLC_FAILURE);
		}

		if (dump)
		{
			auto& dump = mfc_history[mfc_dump_idx++ % max_mfc_dump_idx];
			dump.cmd = ch_mfc_cmd;
			dump.cmd.eah = pc;
			dump.cmd.tag = static_cast<u32>(ch_atomic_stat.get_value()); // Use tag as atomic status
			dump.block_hash = block_hash;
			std::memcpy(dump.data, _ptr<u8>(ch_mfc_cmd.lsa & 0x3ff80), 128);
		}

		static_cast<void>(test_stopped());
		return true;
	}
	case MFC_PUTLLUC_CMD:
	{
		if (g_cfg.core.mfc_debug)
		{
			auto& dump = mfc_history[mfc_dump_idx++ % max_mfc_dump_idx];
			dump.cmd = ch_mfc_cmd;
			dump.cmd.eah = pc;
			dump.block_hash = block_hash;
			std::memcpy(dump.data, _ptr<u8>(ch_mfc_cmd.lsa & 0x3ff80), 128);
		}

		do_putlluc(ch_mfc_cmd);
		ch_atomic_stat.set_value(MFC_PUTLLUC_SUCCESS);
		static_cast<void>(test_stopped());
		return true;
	}
	case MFC_PUTQLLUC_CMD:
	{
		if (g_cfg.core.mfc_debug)
		{
			auto& dump = mfc_history[mfc_dump_idx++ % max_mfc_dump_idx];
			dump.cmd = ch_mfc_cmd;
			dump.cmd.eah = pc;
			dump.block_hash = block_hash;
			std::memcpy(dump.data, _ptr<u8>(ch_mfc_cmd.lsa & 0x3ff80), 128);
		}

		const u32 mask = utils::rol32(1, ch_mfc_cmd.tag);

		if ((mfc_barrier | mfc_fence) & mask) [[unlikely]]
		{
			mfc_queue[mfc_size++] = ch_mfc_cmd;
			mfc_fence |= mask;
		}
		else
		{
			do_putlluc(ch_mfc_cmd);
		}

		return true;
	}
	case MFC_SNDSIG_CMD:
	case MFC_SNDSIGB_CMD:
	case MFC_SNDSIGF_CMD:
	{
		if (ch_mfc_cmd.size != 4)
		{
			break;
		}

		[[fallthrough]];
	}
	case MFC_PUT_CMD:
	case MFC_PUTB_CMD:
	case MFC_PUTF_CMD:
	case MFC_PUTR_CMD:
	case MFC_PUTRB_CMD:
	case MFC_PUTRF_CMD:
	case MFC_GET_CMD:
	case MFC_GETB_CMD:
	case MFC_GETF_CMD:
	case MFC_SDCRZ_CMD:
	{
		if (ch_mfc_cmd.size <= 0x4000) [[likely]]
		{
			if (do_dma_check(ch_mfc_cmd)) [[likely]]
			{
				if (!g_cfg.core.mfc_transfers_shuffling)
				{
					if (ch_mfc_cmd.size)
					{
						do_dma_transfer(this, ch_mfc_cmd, ls);
					}

					return true;
				}

				if (!state.test_and_set(cpu_flag::pending))
					mfc_last_timestamp = get_system_time();
			}

			mfc_queue[mfc_size++] = ch_mfc_cmd;
			mfc_fence |= utils::rol32(1, ch_mfc_cmd.tag);

			if (ch_mfc_cmd.cmd & MFC_BARRIER_MASK)
			{
				mfc_barrier |= utils::rol32(1, ch_mfc_cmd.tag);
			}

			return true;
		}

		break;
	}
	case MFC_PUTL_CMD:
	case MFC_PUTLB_CMD:
	case MFC_PUTLF_CMD:
	case MFC_PUTRL_CMD:
	case MFC_PUTRLB_CMD:
	case MFC_PUTRLF_CMD:
	case MFC_GETL_CMD:
	case MFC_GETLB_CMD:
	case MFC_GETLF_CMD:
	{
		if (ch_mfc_cmd.size <= 0x4000) [[likely]]
		{
			auto& cmd = mfc_queue[mfc_size];
			cmd = ch_mfc_cmd;

			//if (g_cfg.core.mfc_debug)
			//{
			//  TODO: This needs a disambiguator with list elements dumping
			//	auto& dump = mfc_history[mfc_dump_idx++ % max_mfc_dump_idx];
			//	dump.cmd = ch_mfc_cmd;
			//	dump.cmd.eah = pc;
			//	std::memcpy(dump.data, _ptr<u8>(ch_mfc_cmd.eah & 0x3fff0), std::min<u32>(ch_mfc_cmd.size, 128));
			//}

			if (do_dma_check(cmd)) [[likely]]
			{
				if (!g_cfg.core.mfc_transfers_shuffling)
				{
					if (!cmd.size || do_list_transfer(cmd)) [[likely]]
					{
						return true;
					}
				}
				else
				{
					if (!state.test_and_set(cpu_flag::pending))
						mfc_last_timestamp = get_system_time();
				}
			}

			mfc_size++;
			mfc_fence |= utils::rol32(1, cmd.tag);

			if (cmd.cmd & MFC_BARRIER_MASK)
			{
				mfc_barrier |= utils::rol32(1, cmd.tag);
			}

			if (check_mfc_interrupts(pc + 4))
			{
				do_mfc(false);
				spu_runtime::g_escape(this);
			}

			return true;
		}

		break;
	}
	case MFC_BARRIER_CMD:
	case MFC_EIEIO_CMD:
	case MFC_SYNC_CMD:
	{
		if (mfc_size == 0)
		{
			atomic_fence_seq_cst();
		}
		else
		{
			mfc_queue[mfc_size++] = ch_mfc_cmd;
			mfc_barrier |= -1;
			mfc_fence |= utils::rol32(1, ch_mfc_cmd.tag);
		}

		return true;
	}
	default:
	{
		break;
	}
	}

	fmt::throw_exception("Unknown command (cmd=%s, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)",
		ch_mfc_cmd.cmd, ch_mfc_cmd.lsa, ch_mfc_cmd.eal, ch_mfc_cmd.tag, ch_mfc_cmd.size);
}

bool spu_thread::reservation_check(u32 addr, const decltype(rdata)& data) const
{
	if (!addr)
	{
		// No reservation to be lost in the first place
		return false;
	}

	if ((vm::reservation_acquire(addr) & -128) != rtime)
	{
		return true;
	}

	if ((addr >> 28) < 2 || (addr >> 28) == 0xd)
	{
		// Always-allocated memory does not need strict checking (vm::main or vm::stack)
		return !cmp_rdata(data, *vm::get_super_ptr<decltype(rdata)>(addr));
	}

	// Ensure data is allocated (HACK: would raise LR event if not)
	// Set range_lock first optimistically
	range_lock->store(u64{128} << 32 | addr);

	u64 lock_val = *std::prev(std::end(vm::g_range_lock_set));
	u64 old_lock = 0;

	while (lock_val != old_lock)
	{
		// Since we want to read data, let's check readability first
		if (!(lock_val & vm::range_readable))
		{
			// Only one abnormal operation is "unreadable"
			if ((lock_val >> vm::range_pos) == (vm::range_locked >> vm::range_pos))
			{
				// All page flags are untouched and can be read safely
				if (!vm::check_addr(addr))
				{
					// Assume our memory is being (de)allocated
					range_lock->release(0);
					break;
				}

				// g_shmem values are unchanged too
				const u64 is_shmem = vm::g_shmem[addr >> 16];

				const u64 test_addr = is_shmem ? (is_shmem | static_cast<u16>(addr)) / 128 : u64{addr} / 128;
				const u64 lock_addr = lock_val / 128;

				if (test_addr == lock_addr)
				{
					// Our reservation is locked
					range_lock->release(0);
					break;
				}

				break;
			}
		}

		// Fallback to normal range check
		const u64 lock_addr = static_cast<u32>(lock_val);
		const u32 lock_size = static_cast<u32>(lock_val << 3 >> 35);

		if (lock_addr + lock_size <= addr || lock_addr >= addr + 128)
		{
			// We are outside locked range, so page flags are unaffected
			if (!vm::check_addr(addr))
			{
				range_lock->release(0);
				break;
			}
		}
		else if (!(lock_val & vm::range_readable))
		{
			range_lock->release(0);
			break;
		}

		old_lock = std::exchange(lock_val, *std::prev(std::end(vm::g_range_lock_set)));
	}

	if (!range_lock->load()) [[unlikely]]
	{
		return true;
	}

	const bool res = cmp_rdata(data, vm::_ref<decltype(rdata)>(addr));

	range_lock->release(0);
	return !res;
}

bool spu_thread::reservation_check(u32 addr, u32 hash, atomic_t<u64, 64>* range_lock)
{
	if ((addr >> 28) < 2 || (addr >> 28) == 0xd)
	{
		// Always-allocated memory does not need strict checking (vm::main or vm::stack)
		return compute_rdata_hash32(*vm::get_super_ptr<decltype(rdata)>(addr)) == hash;
	}

	// Ensure data is allocated (HACK: would raise LR event if not)
	// Set range_lock first optimistically
	range_lock->store(u64{128} << 32 | addr);

	u64 lock_val = *std::prev(std::end(vm::g_range_lock_set));
	u64 old_lock = 0;

	while (lock_val != old_lock)
	{
		// Since we want to read data, let's check readability first
		if (!(lock_val & vm::range_readable))
		{
			// Only one abnormal operation is "unreadable"
			if ((lock_val >> vm::range_pos) == (vm::range_locked >> vm::range_pos))
			{
				// All page flags are untouched and can be read safely
				if (!vm::check_addr(addr))
				{
					// Assume our memory is being (de)allocated
					range_lock->release(0);
					break;
				}

				// g_shmem values are unchanged too
				const u64 is_shmem = vm::g_shmem[addr >> 16];

				const u64 test_addr = is_shmem ? (is_shmem | static_cast<u16>(addr)) / 128 : u64{addr} / 128;
				const u64 lock_addr = lock_val / 128;

				if (test_addr == lock_addr)
				{
					// Our reservation is locked
					range_lock->release(0);
					break;
				}

				break;
			}
		}

		// Fallback to normal range check
		const u64 lock_addr = static_cast<u32>(lock_val);
		const u32 lock_size = static_cast<u32>(lock_val << 3 >> 35);

		if (lock_addr + lock_size <= addr || lock_addr >= addr + 128)
		{
			// We are outside locked range, so page flags are unaffected
			if (!vm::check_addr(addr))
			{
				range_lock->release(0);
				break;
			}
		}
		else if (!(lock_val & vm::range_readable))
		{
			range_lock->release(0);
			break;
		}

		old_lock = std::exchange(lock_val, *std::prev(std::end(vm::g_range_lock_set)));
	}

	if (!range_lock->load()) [[unlikely]]
	{
		return true;
	}

	const bool res = compute_rdata_hash32(*vm::get_super_ptr<decltype(rdata)>(addr)) == hash;

	range_lock->release(0);
	return !res;
}

usz spu_thread::register_cache_line_waiter(u32 addr)
{
	const u64 value = u64{compute_rdata_hash32(rdata)} << 32 | addr;

	for (usz i = 0; i < std::size(g_spu_waiters_by_value); i++)
	{
		auto [old, ok] = g_spu_waiters_by_value[i].fetch_op([value](u64& x)
		{
			if (x == 0)
			{
				x = value + 1;
				return true;
			}

			if ((x & -128) == value)
			{
				x++;
				return true;
			}

			return false;
		});

		if (ok)
		{
			return i;
		}
	}

	return umax;
}


void spu_thread::deregister_cache_line_waiter(usz index)
{
	if (index == umax)
	{
		return;
	}

	g_spu_waiters_by_value[index].fetch_op([](u64& x)
	{
		x--;

		if ((x & 127) == 0)
		{
			x = 0;
		}

		return false;
	});
}

std::pair<u32, u32> spu_thread::read_dec() const
{
	const u64 res = ch_dec_value - (is_dec_frozen ? 0 : (get_timebased_time() - ch_dec_start_timestamp));
	return {static_cast<u32>(res), static_cast<u32>(res >> 32)};
}

spu_thread::ch_events_t spu_thread::get_events(u64 mask_hint, bool waiting, bool reading)
{
	if (auto mask1 = ch_events.load().mask; mask1 & ~SPU_EVENT_IMPLEMENTED)
	{
		fmt::throw_exception("SPU Events not implemented (mask=0x%x)", mask1);
	}

retry:
	u32 collect = 0;

	// Check reservation status and set SPU_EVENT_LR if lost
	if (mask_hint & SPU_EVENT_LR)
	{
		if (reservation_check(raddr, rdata))
		{
			collect |= SPU_EVENT_LR;
			raddr = 0;
		}
	}

	// SPU Decrementer Event on underflow (use the upper 32-bits to determine it)
	if (mask_hint & SPU_EVENT_TM)
	{
		if (const u64 res = read_dec().second)
		{
			// Set next event to the next time the decrementer underflows
			ch_dec_start_timestamp -= res << 32;
			collect |= SPU_EVENT_TM;
		}
	}

	if (collect)
	{
		set_events(collect);
	}

	auto [res, ok] = ch_events.fetch_op([&](ch_events_t& events)
	{
		if (!reading)
			return false;
		if (waiting)
			events.waiting = !events.count;

		events.count = false;
		return true;
	});

	if (reading && res.locks && mask_hint & (SPU_EVENT_S1 | SPU_EVENT_S2))
	{
		busy_wait(100);
		goto retry;
	}

	return res;
}

void spu_thread::set_events(u32 bits)
{
	if (ch_events.atomic_op([&](ch_events_t& events)
	{
		events.events |= bits;

		// If one masked event was fired, set the channel count (even if the event bit was already 1)
		if (events.mask & bits)
		{
			events.count = true;
			return !!events.waiting && (bits & (SPU_EVENT_S1 | SPU_EVENT_S2));
		}

		return false;
	}))
	{
		notify();
	}
}

void spu_thread::set_interrupt_status(bool enable)
{
	if (enable)
	{
		// Detect enabling interrupts with events masked
		if (auto mask = ch_events.load().mask; mask & SPU_EVENT_INTR_BUSY_CHECK)
		{
			if (g_cfg.core.spu_decoder != spu_decoder_type::_static && g_cfg.core.spu_decoder != spu_decoder_type::dynamic)
			{
				fmt::throw_exception("SPU Interrupts not implemented (mask=0x%x): Use [%s] SPU decoder", mask, spu_decoder_type::dynamic);
			}

			spu_log.trace("SPU Interrupts (mask=0x%x) are using CPU busy checking mode", mask);

			// Process interrupts in cpu_work()
			if (state.none_of(cpu_flag::pending))
			{
				state += cpu_flag::pending;
			}
		}
	}

	interrupts_enabled = enable;
}

u32 spu_thread::get_ch_count(u32 ch)
{
	if (ch < 128) spu_log.trace("get_ch_count(ch=%s)", spu_ch_name[ch]);

	switch (ch)
	{
	case SPU_WrOutMbox:       return ch_out_mbox.get_count() ^ 1;
	case SPU_WrOutIntrMbox:   return ch_out_intr_mbox.get_count() ^ 1;
	case SPU_RdInMbox:        return ch_in_mbox.get_count();
	case MFC_RdTagStat:       return ch_tag_stat.get_count();
	case MFC_RdListStallStat: return ch_stall_stat.get_count();
	case MFC_WrTagUpdate:     return 1;
	case SPU_RdSigNotify1:    return ch_snr1.get_count();
	case SPU_RdSigNotify2:    return ch_snr2.get_count();
	case MFC_RdAtomicStat:    return ch_atomic_stat.get_count();
	case SPU_RdEventStat:     return static_cast<u32>(get_events().count);
	case MFC_Cmd:             return 16 - mfc_size;

	// Channels with a constant count of 1:
	case SPU_WrEventMask:
	case SPU_WrEventAck:
	case SPU_WrDec:
	case SPU_RdDec:
	case SPU_RdEventMask:
	case SPU_RdMachStat:
	case SPU_WrSRR0:
	case SPU_RdSRR0:
	case SPU_Set_Bkmk_Tag:
	case SPU_PM_Start_Ev:
	case SPU_PM_Stop_Ev:
	case MFC_RdTagMask:
	case MFC_LSA:
	case MFC_EAH:
	case MFC_EAL:
	case MFC_Size:
	case MFC_TagID:
	case MFC_WrTagMask:
	case MFC_WrListStallAck:
		return 1;
	default: break;
	}

	ensure(ch < 128u);
	spu_log.error("Unknown/illegal channel in RCHCNT (ch=%s)", spu_ch_name[ch]);
	return 0; // Default count
}

s64 spu_thread::get_ch_value(u32 ch)
{
	if (ch < 128) spu_log.trace("get_ch_value(ch=%s)", spu_ch_name[ch]);

	auto read_channel = [&](spu_channel& channel) -> s64
	{
		if (channel.get_count() == 0)
		{
			state += cpu_flag::wait + cpu_flag::temp;
		}

		if (state & cpu_flag::pending)
		{
			do_mfc();
		}

		// Reset GETLLAR metadata
		last_getllar_addr = umax;

		const s64 out = channel.pop_wait(*this);

		if (state & cpu_flag::wait)
		{
			wakeup_delay();
		}

		static_cast<void>(test_stopped());
		return out;
	};

	switch (ch)
	{
	case SPU_RdSRR0:
	{
		return srr0;
	}
	case SPU_RdInMbox:
	{
		if (ch_in_mbox.get_count() == 0)
		{
			state += cpu_flag::wait;
		}

		while (true)
		{
			if (state & cpu_flag::pending)
			{
				do_mfc();
			}

			const auto [old_count, out] = ch_in_mbox.pop_wait(*this);

			if (old_count)
			{
				if (old_count == 4 /* SPU_IN_MBOX_THRESHOLD */) // TODO: check this
				{
					int_ctrl[2].set(SPU_INT2_STAT_SPU_MAILBOX_THRESHOLD_INT);
				}

				check_state();
				return out;
			}

			auto old = +state;

			if (is_stopped(old))
			{
				return -1;
			}

			thread_ctrl::wait_on(state, old);
		}

		fmt::throw_exception("Unreachable"); // Fix unannotated fallthrough warning
	}

	case MFC_RdTagStat:
	{
		if (state & cpu_flag::pending)
		{
			do_mfc();
		}

		if (u32 out; ch_tag_stat.try_read(out))
		{
			ch_tag_stat.set_value(0, false);
			return out;
		}

		return read_channel(ch_tag_stat);
	}

	case MFC_RdTagMask:
	{
		return ch_tag_mask;
	}

	case SPU_RdSigNotify1:
	{
		return read_channel(ch_snr1);
	}

	case SPU_RdSigNotify2:
	{
		return read_channel(ch_snr2);
	}

	case MFC_RdAtomicStat:
	{
		if (u32 out; ch_atomic_stat.try_read(out))
		{
			ch_atomic_stat.set_value(0, false);
			return out;
		}

		// Will stall infinitely
		return read_channel(ch_atomic_stat);
	}

	case MFC_RdListStallStat:
	{
		if (u32 out; ch_stall_stat.try_read(out))
		{
			ch_stall_stat.set_value(0, false);
			return out;
		}

		// Will stall infinitely
		return read_channel(ch_stall_stat);
	}

	case SPU_RdDec:
	{
		u32 out = read_dec().first;

		//Polling: We might as well hint to the scheduler to slot in another thread since this one is counting down
		if (g_cfg.core.spu_loop_detection && out > spu::scheduler::native_jiffy_duration_us)
		{
			state += cpu_flag::wait;
			std::this_thread::yield();
		}

		return out;
	}

	case SPU_RdEventMask:
	{
		return ch_events.load().mask;
	}

	case SPU_RdEventStat:
	{
		const u32 mask1 = ch_events.load().mask;

		auto events = get_events(mask1, false, true);

		if (events.count)
		{
			return events.events & mask1;
		}

		spu_function_logger logger(*this, "MFC Events read");

		lv2_obj::prepare_for_sleep(*this);

		using resrv_ptr = std::add_pointer_t<const decltype(rdata)>;

		resrv_mem = vm::get_super_ptr<decltype(rdata)>(raddr);
		std::shared_ptr<utils::shm> rdata_shm;

		const u32 old_raddr = raddr;

		// Does not need to safe-access reservation if LR is the only event masked
		// Because it's either an access violation or a live-lock if an invalid memory is passed
		if (raddr && mask1 > SPU_EVENT_LR)
		{
			auto area = vm::get(vm::any, raddr);

			if (area && (area->flags & vm::preallocated))
			{
				if (!vm::check_addr(raddr))
				{
					resrv_mem = nullptr;
				}
			}
			else if (area)
			{
				// Ensure possession over reservation memory so it won't be de-allocated
				auto [base_addr, shm_] = area->peek(raddr);

				if (shm_)
				{
					const u32 data_offs = raddr - base_addr;
					rdata_shm = std::move(shm_);
					resrv_mem = reinterpret_cast<resrv_ptr>(rdata_shm->get() + data_offs);
				}
			}

			if (!resrv_mem)
			{
				spu_log.error("A dangling reservation address has been found while reading SPU_RdEventStat channel. (addr=0x%x, events_mask=0x%x)", raddr, mask1);
				raddr = 0;
				set_events(SPU_EVENT_LR);
			}
		}

		const usz seed = (utils::get_tsc() >> 8) % 100;

#ifdef __linux__
		const bool reservation_busy_waiting = false;
#else
		const bool reservation_busy_waiting = (seed + ((raddr == spurs_addr) ? 50u : 0u)) < g_cfg.core.spu_reservation_busy_waiting_percentage;
#endif
		usz cache_line_waiter_index = umax;

		auto check_cache_line_waiter = [&]()
		{
			if (cache_line_waiter_index == umax)
			{
				return true;
			}

			if ((g_spu_waiters_by_value[cache_line_waiter_index] & -128) == 0)
			{
				deregister_cache_line_waiter(cache_line_waiter_index);
				cache_line_waiter_index = umax;
				return false;
			}

			return true;
		};

		const bool is_LR_wait = raddr && mask1 & SPU_EVENT_LR;

		auto& history = eventstat_wait_time[(raddr % SPU_LS_SIZE) / 128];

		if (is_LR_wait)
		{
			const u32 spu_group_restart = group ? +group->stop_count : 0;

			// Check if waiting session changed
			if (eventstat_raddr != raddr || eventstat_block_counter != block_counter || last_getllar != eventstat_getllar || eventstat_spu_group_restart != spu_group_restart)
			{
				eventstat_raddr = raddr;
				eventstat_block_counter = block_counter;
				eventstat_getllar = last_getllar;
				eventstat_spu_group_restart = spu_group_restart;
				eventstat_spin_count = 0;
				eventstat_evaluate_time = get_system_time();
				eventstat_busy_waiting_switch = umax;
			}
			else
			{
				u8& val = history.front();
				val = static_cast<u8>(std::min<u32>(val + 1, u8{umax}));
			}
		}
		else
		{
			eventstat_busy_waiting_switch = 0;
			eventstat_raddr = 0;
			eventstat_block_counter = 0;
		}

		if (eventstat_busy_waiting_switch == umax)
		{
			bool value = false;

			if (is_LR_wait && g_cfg.core.spu_reservation_busy_waiting_enabled)
			{
				// Make single-threaded groups inclined for busy-waiting
				value = evaluate_spin_optimization({ history.data(), history.size() }, eventstat_evaluate_time, g_cfg.core.spu_reservation_busy_waiting_percentage, group && group->max_num == 1) != 0;
			}

			eventstat_busy_waiting_switch = value ? 1 : 0;
		}
		
		for (bool is_first = true; !events.count; events = get_events(mask1 & ~SPU_EVENT_LR, true, true), is_first = false)
		{
			const auto old = +state;

			if (is_stopped(old))
			{
				if (cache_line_waiter_index != umax)
				{
					g_spu_waiters_by_value[cache_line_waiter_index].release(0);
				}

				return -1;
			}

			// Optimized check
			if (is_LR_wait)
			{
				if (cache_line_waiter_index == umax)
				{
					cache_line_waiter_index = register_cache_line_waiter(raddr);
				}

				bool set_lr = false;

				if (!vm::check_addr(raddr) || rtime != vm::reservation_acquire(raddr))
				{
					set_lr = true;
				}
				else if (!cmp_rdata(rdata, *resrv_mem))
				{
					// Notify threads manually, memory data has likely changed and broke the reservation for others
					if (vm::reservation_notifier_count(raddr) && vm::reservation_acquire(raddr) == rtime)
					{
						vm::reservation_notifier_notify(raddr);
					}

					set_lr = true;
					spu_log.trace("SPU Reservation Wait: Did not encounter notification (raddr=0x%x)", raddr);
				}

				if (set_lr)
				{
					raddr = 0;
					set_events(SPU_EVENT_LR);
					continue;
				}

				if (!is_first && eventstat_busy_waiting_switch != 1)
				{
					u8& val = history.front();
					val = static_cast<u8>(std::min<u32>(val + 1, u8{umax}));
				}
			}

			if (eventstat_busy_waiting_switch == 1)
			{
				// Don't be stubborn, force operating sleep if too much time has passed
				const u64 time_since = get_system_time() - eventstat_evaluate_time;

				if (time_since >= (utils::get_thread_count() >= 9 ? 50'000 : 3000))
				{
					spu_log.trace("SPU RdEventStat wait for 0x%x failed", raddr);
					history.front() = 2;
					eventstat_busy_waiting_switch = 0;
					continue;
				}

#if defined(ARCH_X64)
				if (utils::has_um_wait())
				{
					if (utils::has_waitpkg())
					{
						__tpause(static_cast<u32>(std::min<u64>(eventstat_spin_count, 10) * 500), 0x1);
					}
					else
					{
						struct check_wait_t
						{
							static FORCE_INLINE bool needs_wait(u64 rtime, const atomic_t<u64>& mem_rtime) noexcept
							{
								return rtime == mem_rtime;
							}
						};

						// Provide the first X64 cache line of the reservation to be tracked
						__mwaitx<check_wait_t>(static_cast<u32>(std::min<u64>(eventstat_spin_count, 17) * 500), 0xf0, std::addressof(*resrv_mem), +rtime, vm::reservation_acquire(raddr));
					}
				}
				else
#endif
				{
					busy_wait(300);
				}

				// Check other reservations in other threads
				lv2_obj::notify_all();

				eventstat_spin_count++;
				continue;
			}

			if (raddr && (mask1 & ~SPU_EVENT_TM) == SPU_EVENT_LR)
			{
				if (u32 max_threads = std::min<u32>(g_cfg.core.max_spurs_threads, group ? group->max_num : u32{umax}); group && group->max_run != max_threads)
				{
					constexpr std::string_view spurs_suffix = "CellSpursKernelGroup"sv;

					if (group->name.ends_with(spurs_suffix) && !group->name.substr(0, group->name.size() - spurs_suffix.size()).ends_with("_libsail"))
					{
						// Hack: don't run more SPURS threads than specified.
						if (u32 old = atomic_storage<u32>::exchange(group->max_run, max_threads); old > max_threads)
						{
							spu_log.success("HACK: '%s' (0x%x) limited to %u threads.", group->name, group->id, max_threads);
						}
						else if (u32 running = group->spurs_running; old < max_threads && running >= old && running < max_threads)
						{
							group->spurs_running.notify_all();
						}
					}
				}

				// Don't busy-wait with TSX - memory is sensitive
				if (g_use_rtm || !reservation_busy_waiting)
				{
					if (u32 work_count = g_spu_work_count)
					{
						const u32 true_free = utils::sub_saturate<u32>(utils::get_thread_count(), 10);

						if (work_count > true_free)
						{
							// SPU thread count estimation
							const u32 thread_count = (group ? g_raw_spu_ctr + group->max_num : g_raw_spu_ctr + 3);

							if (thread_count && seed % thread_count < work_count - true_free)
							{
								// Make the SPU wait longer for other threads to do the work
								thread_ctrl::wait_for(200);
								continue;
							}
						}
					}

					// Check other reservations in other threads
					lv2_obj::notify_all();

					if (raddr - spurs_addr <= 0x80 && !g_cfg.core.spu_accurate_reservations && mask1 == SPU_EVENT_LR)
					{
						// Wait with extended timeout, in this situation we have notifications for nearly all writes making it possible
						// Abort notifications are handled specially for performance reasons
						if (auto wait_var = vm::reservation_notifier_begin_wait(raddr, rtime))
						{
							if (check_cache_line_waiter())
							{
								utils::bless<atomic_t<u32>>(&wait_var->raw().wait_flag)->wait(1, atomic_wait_timeout{300'000});
							}

							vm::reservation_notifier_end_wait(*wait_var);
						}

						continue;
					}

					const u32 _raddr = this->raddr;
#ifdef __linux__

					if (auto wait_var = vm::reservation_notifier_begin_wait(_raddr, rtime))
					{
						if (check_cache_line_waiter())
						{
							utils::bless<atomic_t<u32>>(&wait_var->raw().wait_flag)->wait(1, atomic_wait_timeout{50'000});
						}

						vm::reservation_notifier_end_wait(*wait_var);
					}
#else
					static thread_local bool s_tls_try_notify = false;
					s_tls_try_notify = false;

					const auto wait_cb = mask1 != SPU_EVENT_LR ? nullptr : +[](u64 attempts) -> bool
					{
						const auto _this = static_cast<spu_thread*>(cpu_thread::get_current());
						AUDIT(_this->get_class() == thread_class::spu);

						const auto old = +_this->state;

						if (is_stopped(old))
						{
							return false;
						}

						if (!attempts)
						{
							// Skip checks which have been done already
							return true;
						}

						bool set_lr = false;
						const u32 raddr = _this->raddr;

						if (!raddr)
						{
							return true;
						}

						if (!vm::check_addr(raddr))
						{
							set_lr = true;
						}
						else if (!cmp_rdata(_this->rdata, *_this->resrv_mem))
						{
							// Notify threads manually, memory data has likely changed and broke the reservation for others
							if (vm::reservation_notifier_count(raddr) >= 2 && vm::reservation_acquire(raddr) == _this->rtime)
							{
								s_tls_try_notify = true;
							}

							set_lr = true;
						}

						if (set_lr)
						{
							_this->raddr = 0;
							_this->set_events(SPU_EVENT_LR);
							return false;
						}

						// Check other reservations in other threads
						lv2_obj::notify_all();

						return true;
					};

					if (auto wait_var = vm::reservation_notifier_begin_wait(_raddr, rtime))
					{
						if (check_cache_line_waiter())
						{
							atomic_wait_engine::set_one_time_use_wait_callback(wait_cb);
							utils::bless<atomic_t<u32>>(&wait_var->raw().wait_flag)->wait(1, atomic_wait_timeout{80'000});
						}

						vm::reservation_notifier_end_wait(*wait_var);
					}

					if (s_tls_try_notify && vm::reservation_notifier_count(_raddr) && vm::reservation_acquire(_raddr) == rtime)
					{
						vm::reservation_notifier_notify(_raddr);
					}
#endif
				}
				else
				{
					busy_wait();
				}

				continue;
			}

			thread_ctrl::wait_on(state, old, 100);
		}

		deregister_cache_line_waiter(cache_line_waiter_index);

		wakeup_delay();

		if (is_paused(state - cpu_flag::suspend))
		{
			if (!raddr && old_raddr)
			{
				// Restore reservation address temporarily for debugging use
				raddr = old_raddr;
				check_state();
				raddr = 0;
			}
		}

		check_state();
		return events.events & mask1;
	}

	case SPU_RdMachStat:
	{
		// Return SPU Interrupt status in LSB
		return u32{interrupts_enabled} | (u32{get_type() == spu_type::isolated} << 1);
	}
	}

	fmt::throw_exception("Unknown/illegal channel in RDCH (ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");
}

bool spu_thread::set_ch_value(u32 ch, u32 value)
{
	if (ch < 128) spu_log.trace("set_ch_value(ch=%s, value=0x%x)", spu_ch_name[ch], value);

	switch (ch)
	{
	case SPU_WrSRR0:
	{
		srr0 = value & 0x3fffc;
		return true;
	}

	case SPU_WrOutIntrMbox:
	{
		// Reset GETLLAR metadata
		last_getllar_addr = umax;

		if (get_type() >= spu_type::raw)
		{
			if (state & cpu_flag::pending)
			{
				do_mfc();
			}

			if (ch_out_intr_mbox.get_count())
			{
				state += cpu_flag::wait;
			}

			if (!ch_out_intr_mbox.push_wait(*this, value))
			{
				return false;
			}

			int_ctrl[2].set(SPU_INT2_STAT_MAILBOX_INT);
			wakeup_delay();
			check_state();
			return true;
		}

		state += cpu_flag::wait;

		lv2_obj::notify_all_t notify;

		const u32 code = value >> 24;
		{
			if (code < 64)
			{
				/* ===== sys_spu_thread_send_event (used by spu_printf) ===== */

				u32 spup = code & 63;
				u32 data = 0;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_spu_thread_send_event(value=0x%x, spup=%d): Out_MBox is empty", value, spup);
				}

				spu_log.trace("sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);

				spu_function_logger logger(*this, "sys_spu_thread_send_event");

				shared_ptr<lv2_event_queue> queue;
				{
					std::lock_guard lock(group->mutex);

					if (ch_in_mbox.get_count())
					{
						// TODO: Check this
						spu_log.error("sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x): In_MBox is not empty (%d)", spup, (value & 0x00ffffff), data, ch_in_mbox.get_count());
						ch_in_mbox.set_values(1, CELL_EBUSY);
						return true;
					}

					// Reserve a place in the inbound mailbox
					ch_in_mbox.set_values(1, CELL_OK);
					queue = this->spup[spup];
				}

				const auto res = !queue ? CELL_ENOTCONN :
					queue->send(SYS_SPU_THREAD_EVENT_USER_KEY, lv2_id, (u64{spup} << 32) | (value & 0x00ffffff), data);

				if (res == CELL_ENOTCONN)
				{
					spu_log.warning("sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x): error (%s)", spup, (value & 0x00ffffff), data, res);
				}

				if (res == CELL_EAGAIN)
				{
					// Restore mailboxes state
					ch_out_mbox.set_value(data);
					ch_in_mbox.try_pop(data);
					return false;
				}

				atomic_storage<u32>::release(ch_in_mbox.values.raw().value0, res);
				return true;
			}
			else if (code < 128)
			{
				/* ===== sys_spu_thread_throw_event ===== */

				u32 spup = code & 63;
				u32 data = 0;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_spu_thread_throw_event(value=0x%x, spup=%d): Out_MBox is empty", value, spup);
				}

				spu_log.trace("sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);

				spu_function_logger logger(*this, "sys_spu_thread_throw_event");

				shared_ptr<lv2_event_queue> queue;
				{
					std::lock_guard lock{group->mutex};
					queue = this->spup[spup];
				}

				// TODO: check passing spup value
				if (auto res = queue ? queue->send(SYS_SPU_THREAD_EVENT_USER_KEY, lv2_id, (u64{spup} << 32) | (value & 0x00ffffff), data) : CELL_ENOTCONN)
				{
					if (res == CELL_EAGAIN)
					{
						ch_out_mbox.set_value(data);
						return false;
					}

					spu_log.warning("sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x) failed (error=%s)", spup, (value & 0x00ffffff), data, res);
				}

				return true;
			}
			else if (code == 128)
			{
				/* ===== sys_event_flag_set_bit ===== */

				u32 flag = value & 0xffffff;
				u32 data = 0;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_event_flag_set_bit(value=0x%x (flag=%d)): Out_MBox is empty", value, flag);
				}

				spu_log.trace("sys_event_flag_set_bit(id=%d, value=0x%x (flag=%d))", data, value, flag);

				spu_function_logger logger(*this, "sys_event_flag_set_bit");

				{
					std::lock_guard lock(group->mutex);

					if (ch_in_mbox.get_count())
					{
						// TODO: Check this
						spu_log.error("sys_event_flag_set_bit(value=0x%x (flag=%d)): In_MBox is not empty (%d)", value, flag, ch_in_mbox.get_count());
						ch_in_mbox.set_values(1, CELL_EBUSY);
						return true;
					}

					// Reserve a place in the inbound mailbox
					ch_in_mbox.set_values(1, CELL_OK);
				}

				// Use the syscall to set flag
				const auto res = 0u + sys_event_flag_set(*this, data, 1ull << flag);

				if (res == CELL_EAGAIN)
				{
					// Restore mailboxes state
					ch_out_mbox.set_value(data);
					ch_in_mbox.try_pop(data);
					return false;
				}

				atomic_storage<u32>::release(ch_in_mbox.values.raw().value0, res);
				return true;
			}
			else if (code == 192)
			{
				/* ===== sys_event_flag_set_bit_impatient ===== */

				u32 flag = value & 0xffffff;
				u32 data = 0;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_event_flag_set_bit_impatient(value=0x%x (flag=%d)): Out_MBox is empty", value, flag);
				}

				spu_log.trace("sys_event_flag_set_bit_impatient(id=%d, value=0x%x (flag=%d))", data, value, flag);

				spu_function_logger logger(*this, "sys_event_flag_set_bit_impatient");

				// Use the syscall to set flag
				if (sys_event_flag_set(*this, data, 1ull << flag) + 0u == CELL_EAGAIN)
				{
					ch_out_mbox.set_value(data);
					return false;
				}

				return true;
			}
			else
			{
				fmt::throw_exception("SPU_WrOutIntrMbox: unknown data (value=0x%x, Out_MBox=%s)", value, ch_out_mbox);
			}
		}
	}

	case SPU_WrOutMbox:
	{
		if (state & cpu_flag::pending)
		{
			do_mfc();
		}

		if (ch_out_mbox.get_count())
		{
			state += cpu_flag::wait;
		}

		if (!ch_out_mbox.push_wait(*this, value))
		{
			return false;
		}

		check_state();
		return true;
	}

	case MFC_WrTagMask:
	{
		ch_tag_mask = value;

		if (ch_tag_upd)
		{
			const u32 completed = get_mfc_completed();

			if (completed && ch_tag_upd == MFC_TAG_UPDATE_ANY)
			{
				ch_tag_stat.set_value(completed);
				ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			}
			else if (completed == value && ch_tag_upd == MFC_TAG_UPDATE_ALL)
			{
				ch_tag_stat.set_value(completed);
				ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			}
		}

		return true;
	}

	case MFC_WrTagUpdate:
	{
		if (value > MFC_TAG_UPDATE_ALL)
		{
			break;
		}

		const u32 completed = get_mfc_completed();

		if (!value)
		{
			ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			ch_tag_stat.set_value(completed);
		}
		else if (completed && value == MFC_TAG_UPDATE_ANY)
		{
			ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			ch_tag_stat.set_value(completed);
		}
		else if (completed == ch_tag_mask && value == MFC_TAG_UPDATE_ALL)
		{
			ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			ch_tag_stat.set_value(completed);
		}
		else
		{
			ch_tag_upd = value;
		}

		return true;
	}

	case MFC_LSA:
	{
		ch_mfc_cmd.lsa = value;
		return true;
	}

	case MFC_EAH:
	{
		ch_mfc_cmd.eah = value;
		return true;
	}

	case MFC_EAL:
	{
		ch_mfc_cmd.eal = value;
		return true;
	}

	case MFC_Size:
	{
		ch_mfc_cmd.size = static_cast<u16>(std::min<u32>(value, 0xffff));
		return true;
	}

	case MFC_TagID:
	{
		ch_mfc_cmd.tag = value & 0x1f;
		return true;
	}

	case MFC_Cmd:
	{
		ch_mfc_cmd.cmd = MFC(value & 0xff);
		return process_mfc_cmd();
	}

	case MFC_WrListStallAck:
	{
		value &= 0x1f;

		// Reset stall status for specified tag
		const u32 tag_mask = utils::rol32(1, value);

		if (ch_stall_mask & tag_mask)
		{
			ch_stall_mask &= ~tag_mask;

			for (u32 i = 0; i < mfc_size; i++)
			{
				if (mfc_queue[i].tag == (value | 0x80))
				{
					// Unset stall bit
					mfc_queue[i].tag &= 0x7f;
				}
			}

			do_mfc(true);
		}

		return true;
	}

	case SPU_WrDec:
	{
		get_events(SPU_EVENT_TM); // Don't discard possibly occured old event
		ch_dec_start_timestamp = get_timebased_time();
		ch_dec_value = value;
		is_dec_frozen = false;
		return true;
	}

	case SPU_WrEventMask:
	{
		get_events(value | static_cast<u32>(ch_events.load().mask));

		if (ch_events.atomic_op([&](ch_events_t& events)
		{
			events.mask = value;

			if (events.events & events.mask)
			{
				events.count = true;
				return true;
			}

			return !!events.count;
		}))
		{
			// Check interrupts in case count is 1
			if (check_mfc_interrupts(pc + 4))
			{
				spu_runtime::g_escape(this);
			}
		}

		return true;
	}

	case SPU_WrEventAck:
	{
		// "Collect" events before final acknowledgment
		get_events(value | static_cast<u32>(ch_events.load().mask));

		bool freeze_dec = false;

		const bool check_intr = ch_events.atomic_op([&](ch_events_t& events)
		{
			events.events &= ~value;

			freeze_dec = !!((value & SPU_EVENT_TM) & ~events.mask);

			if (events.events & events.mask)
			{
				events.count = true;
				return true;
			}

			return !!events.count;
		});

		if (!is_dec_frozen && freeze_dec)
		{
			// Save current time, this will be the reported value until the decrementer resumes
			ch_dec_value = read_dec().first;
			is_dec_frozen = true;
		}

		if (check_intr)
		{
			// Check interrupts in case count is 1
			if (check_mfc_interrupts(pc + 4))
			{
				spu_runtime::g_escape(this);
			}
		}

		return true;
	}

	case SPU_Set_Bkmk_Tag:
	case SPU_PM_Start_Ev:
	case SPU_PM_Stop_Ev:
	{
		return true;
	}
	}

	fmt::throw_exception("Unknown/illegal channel in WRCH (ch=%d [%s], value=0x%x)", ch, ch < 128 ? spu_ch_name[ch] : "???", value);
}

extern void resume_spu_thread_group_from_waiting(spu_thread& spu)
{
	const auto group = spu.group;

	std::lock_guard lock(group->mutex);

	if (group->run_state == SPU_THREAD_GROUP_STATUS_WAITING)
	{
		group->run_state = SPU_THREAD_GROUP_STATUS_RUNNING;
	}
	else if (group->run_state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
	{
		group->run_state = SPU_THREAD_GROUP_STATUS_SUSPENDED;
		spu.state += cpu_flag::signal;
		spu.state.notify_one();
		return;
	}

	for (auto& thread : group->threads)
	{
		if (thread)
		{
			if (thread.get() == &spu)
			{
				constexpr auto flags = cpu_flag::suspend + cpu_flag::signal;
				ensure(((thread->state ^= flags) & flags) == cpu_flag::signal);
			}
			else
			{
				thread->state -= cpu_flag::suspend;
			}

			thread->state.notify_one();
		}
	}
}

bool spu_thread::stop_and_signal(u32 code)
{
	spu_log.trace("stop_and_signal(code=0x%x)", code);

	auto set_status_npc = [&]()
	{
		status_npc.atomic_op([&](status_npc_sync_var& state)
		{
			state.status = (state.status & 0xffff) | (code << 16);
			state.status |= SPU_STATUS_STOPPED_BY_STOP;
			state.status &= ~SPU_STATUS_RUNNING;
			state.npc = (pc + 4) | +interrupts_enabled;
		});
	};

	if (get_type() >= spu_type::raw)
	{
		// Save next PC and current SPU Interrupt Status
		state += cpu_flag::stop + cpu_flag::wait + cpu_flag::ret;
		set_status_npc();

		status_npc.notify_one();

		int_ctrl[2].set(SPU_INT2_STAT_SPU_STOP_AND_SIGNAL_INT);
		check_state();
		return true;
	}

	auto get_queue = [this](u32 spuq) -> const shared_ptr<lv2_event_queue>&
	{
		for (auto& v : this->spuq)
		{
			if (spuq == v.first)
			{
				if (lv2_obj::check(v.second))
				{
					return v.second;
				}
			}
		}

		static const shared_ptr<lv2_event_queue> empty;
		return empty;
	};

	switch (code)
	{
	case 0x001:
	{
		state += cpu_flag::wait;
		std::this_thread::sleep_for(1ms); // hack
		check_state();
		return true;
	}

	case 0x002:
	{
		state += cpu_flag::ret;
		return true;
	}

	case SYS_SPU_THREAD_STOP_RECEIVE_EVENT:
	{
		/* ===== sys_spu_thread_receive_event ===== */

		u32 spuq = 0;

		if (!ch_out_mbox.try_read(spuq))
		{
			fmt::throw_exception("sys_spu_thread_receive_event(): Out_MBox is empty");
		}

		if (u32 count = ch_in_mbox.get_count())
		{
			spu_log.error("sys_spu_thread_receive_event(): In_MBox is not empty (%d)", count);
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		struct clear_mbox
		{
			spu_thread& _this;

			~clear_mbox() noexcept
			{
				if (cpu_flag::again - _this.state)
				{
					u32 val = 0;
					_this.ch_out_mbox.try_pop(val);
				}
			}
		} clear{*this};

		spu_log.trace("sys_spu_thread_receive_event(spuq=0x%x)", spuq);

		if (!group->has_scheduler_context /*|| group->type & 0xf00*/)
		{
			spu_log.error("sys_spu_thread_receive_event(): Incompatible group type = 0x%x", group->type);
			return ch_in_mbox.set_values(1, CELL_EINVAL), true;
		}

		lv2_obj::prepare_for_sleep(*this);

		spu_function_logger logger(*this, "sys_spu_thread_receive_event");

		shared_ptr<lv2_event_queue> queue;

		while (true)
		{
			// Check group status (by actually checking thread status), wait if necessary
			while (true)
			{
				const auto old = +state;

				if (is_stopped(old))
				{
					state += cpu_flag::again;
					return false;
				}

				if (!is_paused(old))
				{
					// The group is not suspended (anymore)
					break;
				}

				thread_ctrl::wait_on(state, old);
			}

			reader_lock{group->mutex}, queue = get_queue(spuq);

			if (!queue)
			{
				return ch_in_mbox.set_values(1, CELL_EINVAL), true;
			}

			// Lock queue's mutex first, then group's mutex
			std::scoped_lock lock(queue->mutex, group->mutex);

			if (is_stopped())
			{
				state += cpu_flag::again;
				return false;
			}

			if (Emu.IsStarting())
			{
				// Deregister lv2_obj::g_to_sleep entry (savestates related)
				lv2_obj::sleep(*this);
			}

			if (group->run_state >= SPU_THREAD_GROUP_STATUS_WAITING && group->run_state <= SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
			{
				// Try again
				ensure(state & cpu_flag::suspend);
				continue;
			}

			if (queue != get_queue(spuq))
			{
				// Try again
				continue;
			}

			if (!queue->exists)
			{
				return ch_in_mbox.set_values(1, CELL_EINVAL), true;
			}

			if (queue->events.empty())
			{
				lv2_obj::emplace(queue->sq, this);
				group->run_state = SPU_THREAD_GROUP_STATUS_WAITING;
				group->waiter_spu_index = index;

				for (auto& thread : group->threads)
				{
					if (thread)
					{
						thread->state += cpu_flag::suspend;
					}
				}

				// Wait
				break;
			}
			else
			{
				// Return the event immediately
				const auto event = queue->events.front();
				const auto data1 = static_cast<u32>(std::get<1>(event));
				const auto data2 = static_cast<u32>(std::get<2>(event));
				const auto data3 = static_cast<u32>(std::get<3>(event));
				ch_in_mbox.set_values(4, CELL_OK, data1, data2, data3);
				queue->events.pop_front();
				return true;
			}
		}

		lv2_obj::notify_all();

		while (auto old = +state)
		{
			if (old & cpu_flag::signal && state.test_and_reset(cpu_flag::signal))
			{
				break;
			}

			if (is_stopped(old))
			{
				std::lock_guard qlock(queue->mutex);

				old = state.fetch_sub(cpu_flag::signal);

				if (old & cpu_flag::signal)
				{
					break;
				}

				state += cpu_flag::again;
				return false;
			}

			thread_ctrl::wait_on(state, old);
		}

		wakeup_delay();
		return true;
	}

	case SYS_SPU_THREAD_STOP_TRY_RECEIVE_EVENT:
	{
		/* ===== sys_spu_thread_tryreceive_event ===== */

		u32 spuq = 0;

		if (!ch_out_mbox.try_pop(spuq))
		{
			fmt::throw_exception("sys_spu_thread_tryreceive_event(): Out_MBox is empty");
		}

		if (u32 count = ch_in_mbox.get_count())
		{
			spu_log.error("sys_spu_thread_tryreceive_event(): In_MBox is not empty (%d)", count);
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		spu_log.trace("sys_spu_thread_tryreceive_event(spuq=0x%x)", spuq);

		shared_ptr<lv2_event_queue> queue;

		reader_lock{group->mutex}, queue = get_queue(spuq);

		std::unique_lock<shared_mutex> qlock, group_lock;

		while (true)
		{
			if (!queue)
			{
				return ch_in_mbox.set_values(1, CELL_EINVAL), true;
			}

			// Lock queue's mutex first, then group's mutex
			qlock = std::unique_lock{queue->mutex};
			group_lock = std::unique_lock{group->mutex};

			if (const auto& queue0 = get_queue(spuq); queue != queue0)
			{
				// Keep atleast one reference of the pointer so mutex unlock can work
				const auto old_ref = std::exchange(queue, queue0);
				group_lock.unlock();
				qlock.unlock();
			}
			else
			{
				break;
			}
		}

		if (!queue->exists)
		{
			return ch_in_mbox.set_values(1, CELL_EINVAL), true;
		}

		if (queue->events.empty())
		{
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		const auto event = queue->events.front();
		const auto data1 = static_cast<u32>(std::get<1>(event));
		const auto data2 = static_cast<u32>(std::get<2>(event));
		const auto data3 = static_cast<u32>(std::get<3>(event));
		ch_in_mbox.set_values(4, CELL_OK, data1, data2, data3);
		queue->events.pop_front();
		return true;
	}

	case SYS_SPU_THREAD_STOP_SWITCH_SYSTEM_MODULE:
	{
		fmt::throw_exception("SYS_SPU_THREAD_STOP_SWITCH_SYSTEM_MODULE (op=0x%x, Out_MBox=%s)", code, _ref<u32>(pc), ch_out_mbox);
		return true;
	}

	case SYS_SPU_THREAD_STOP_YIELD:
	{
		// SPU thread group yield (TODO)
		if (ch_out_mbox.get_count())
		{
			fmt::throw_exception("STOP code 0x100: Out_MBox is not empty");
		}

		atomic_fence_seq_cst();
		return true;
	}

	case SYS_SPU_THREAD_STOP_GROUP_EXIT:
	{
		/* ===== sys_spu_thread_group_exit ===== */

		state += cpu_flag::wait;

		u32 value = 0;

		if (!ch_out_mbox.try_pop(value))
		{
			fmt::throw_exception("sys_spu_thread_group_exit(): Out_MBox is empty");
		}

		spu_log.trace("sys_spu_thread_group_exit(status=0x%x)", value);

		spu_function_logger logger(*this, "sys_spu_thread_group_exit");

		while (true)
		{
			// Check group status (by actually checking thread status), wait if necessary
			while (true)
			{
				const auto old = +state;

				if (is_stopped(old))
				{
					ch_out_mbox.set_value(value);
					state += cpu_flag::again;
					return false;
				}

				if (!is_paused(old))
				{
					// The group is not suspended (anymore)
					break;
				}

				thread_ctrl::wait_on(state, old);
			}

			std::lock_guard lock(group->mutex);

			if (auto _state = +group->run_state;
				_state >= SPU_THREAD_GROUP_STATUS_WAITING && _state <= SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
			{
				// We can't exit while we are waiting on an SPU event
				continue;
			}

			if (std::exchange(group->set_terminate, true))
			{
				// Whoever terminated first decides the error status + cause
				return true;
			}

			for (auto& thread : group->threads)
			{
				if (thread)
				{
					thread->state.fetch_op([](bs_t<cpu_flag>& flags)
					{
						if (flags & cpu_flag::stop)
						{
							// In case the thread raised the ret flag itself at some point do not raise it again
							return false;
						}

						flags += cpu_flag::stop + cpu_flag::ret;
						return true;
					});
				}
			}

			group->exit_status = value;
			group->join_state = SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT;
			set_status_npc();
			break;
		}

		u32 prev_resv = 0;

		for (auto& thread : group->threads)
		{
			if (thread)
			{
				// Notify threads, guess which threads need a notification by checking cpu_flag::ret (redundant notification can only occur if thread has called an exit syscall itself as well)
				if (thread.get() != this && thread->state & cpu_flag::ret)
				{
					thread_ctrl::notify(*thread);

					if (u32 resv = atomic_storage<u32>::load(thread->raddr))
					{
						if (prev_resv && prev_resv != resv)
						{
							// Batch reservation notifications if possible
							vm::reservation_notifier_notify(prev_resv);
						}

						prev_resv = resv;
					}
				}
			}
		}

		if (prev_resv)
		{
			vm::reservation_notifier_notify(prev_resv);
		}

		check_state();
		return true;
	}

	case SYS_SPU_THREAD_STOP_THREAD_EXIT:
	{
		/* ===== sys_spu_thread_exit ===== */

		state += cpu_flag::wait;

		u32 value;

		if (!ch_out_mbox.try_pop(value))
		{
			fmt::throw_exception("sys_spu_thread_exit(): Out_MBox is empty");
		}

		spu_function_logger logger(*this, "sys_spu_thread_exit");

		spu_log.trace("sys_spu_thread_exit(status=0x%x)", value);
		last_exit_status.release(value);
		set_status_npc();
		state += cpu_flag::stop + cpu_flag::ret;
		check_state();
		return true;
	}
	}

	fmt::throw_exception("Unknown STOP code: 0x%x (op=0x%x, Out_MBox=%s)", code, _ref<u32>(pc), ch_out_mbox);
}

void spu_thread::halt()
{
	spu_log.trace("halt()");

	if (get_type() >= spu_type::raw)
	{
		state += cpu_flag::stop + cpu_flag::wait;

		status_npc.atomic_op([this](status_npc_sync_var& state)
		{
			state.status |= SPU_STATUS_STOPPED_BY_HALT;
			state.status &= ~SPU_STATUS_RUNNING;
			state.npc = pc | +interrupts_enabled;
		});

		status_npc.notify_one();

		int_ctrl[2].set(SPU_INT2_STAT_SPU_HALT_OR_STEP_INT);

		spu_runtime::g_escape(this);
	}

	spu_log.fatal("Halt");
	spu_runtime::g_escape(this);
}

void spu_thread::fast_call(u32 ls_addr)
{
	// LS:0x0: this is originally the entry point of the interrupt handler, but interrupts are not implemented
	_ref<u32>(0) = 0x00000002; // STOP 2

	auto old_pc = pc;
	auto old_lr = gpr[0]._u32[3];
	auto old_stack = gpr[1]._u32[3]; // only saved and restored (may be wrong)

	pc = ls_addr;
	gpr[0]._u32[3] = 0x0;

	cpu_task();

	state -= cpu_flag::ret;

	pc = old_pc;
	gpr[0]._u32[3] = old_lr;
	gpr[1]._u32[3] = old_stack;
}

spu_exec_object spu_thread::capture_memory_as_elf(std::span<spu_memory_segment_dump_data> segs, u32 pc_hint)
{
	spu_exec_object spu_exec;
	spu_exec.set_error(elf_error::ok);

	std::vector<u8> all_data(SPU_LS_SIZE);

	for (auto& seg : segs)
	{
		std::vector<uchar> data(seg.segment_size);

		if (auto [vm_addr, ok] = vm::try_get_addr(seg.src_addr); ok)
		{
			if (!vm::try_access(vm_addr, data.data(), seg.segment_size, false))
			{
				spu_log.error("capture_memory_as_elf(): Failed to read {0x%x..0x%x}, aborting capture.", +vm_addr, vm_addr + seg.segment_size - 1);
				spu_exec.set_error(elf_error::stream_data);
				return spu_exec;
			}
		}
		else
		{
			std::memcpy(data.data(), seg.src_addr, data.size());
		}

		std::memcpy(all_data.data() + seg.ls_addr, data.data(), data.size());

		auto& prog = spu_exec.progs.emplace_back(SYS_SPU_SEGMENT_TYPE_COPY, seg.flags & 0x7, seg.ls_addr, seg.segment_size, 8, std::move(data));

		prog.p_paddr = prog.p_vaddr;

		spu_log.success("Segment: p_type=0x%x, p_vaddr=0x%x, p_filesz=0x%x, p_memsz=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz);
	}


	u32 pc0 = pc_hint;

	if (pc_hint != umax)
	{
		for (pc0 = pc_hint; pc0; pc0 -= 4)
		{
			const u32 op = read_from_ptr<be_t<u32>>(all_data, pc0 - 4);

			// Try to find function entry (if they are placed sequentially search for BI $LR of previous function)
			if (!op || op == 0x35000000u || s_spu_itype.decode(op) == spu_itype::UNK)
			{
				if (is_exec_code(pc0, { all_data.data(), SPU_LS_SIZE }))
					break;
			}
		}
	}
	else
	{
		for (pc0 = 0; pc0 < SPU_LS_SIZE; pc0 += 4)
		{
			// Try to find a function entry (very basic)
			if (is_exec_code(pc0, { all_data.data(), SPU_LS_SIZE }))
				break;
		}
	}

	spu_exec.header.e_entry = pc0;

	return spu_exec;
}

bool spu_thread::capture_state()
{
	ensure(state & cpu_flag::wait);

	// Save data as an executable segment, even the SPU stack
	// In the past, an optimization was made here to save only non-zero chunks of data
	// But Ghidra didn't like accessing memory out of chunks (pretty common)
	// So it has been reverted
	spu_memory_segment_dump_data single_seg{.ls_addr = 0, .src_addr = ls, .segment_size = SPU_LS_SIZE};
	spu_exec_object spu_exec = capture_memory_as_elf({&single_seg, 1}, pc);

	std::string name;

	if (get_type() == spu_type::threaded)
	{
		name = *spu_tname.load();

		if (name.empty())
		{
			// TODO: Maybe add thread group name here
			fmt::append(name, "SPU.0x%07x", lv2_id);
		}
	}
	else
	{
		fmt::append(name, "RawSPU.%u", lv2_id);
	}

	name = vfs::escape(name, true);
	std::replace(name.begin(), name.end(), ' ', '_');

	auto get_filename = [&]() -> std::string
	{
		return fs::get_cache_dir() + "spu_progs/" + Emu.GetTitleID() + "_" + vfs::escape(name, true) + '_' + date_time::current_time_narrow() + "_capture.elf";
	};

	auto elf_path = get_filename();

	if (fs::exists(elf_path))
	{
		// Wait 1 second so current_time_narrow() will return a different string
		std::this_thread::sleep_for(1s);

		if (elf_path = get_filename(); fs::exists(elf_path))
		{
			spu_log.error("Failed to create '%s' (error=%s)", elf_path, fs::g_tls_error);
			return false;
		}
	}

	fs::file temp(elf_path, fs::rewrite);

	if (!temp)
	{
		spu_log.error("Failed to create file '%s' (error=%s)", elf_path, fs::g_tls_error);
		return false;
	}

	auto data = spu_exec.save();

	if (temp.write(data.data(), data.size()) != data.size())
	{
		spu_log.error("Failed to write file '%s' (error=%s)", elf_path, fs::g_tls_error);
		return false;
	}

	spu_log.success("SPU Local Storage image saved to '%s'", elf_path);

	if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit || g_cfg.core.spu_decoder == spu_decoder_type::llvm)
	{
		return true;
	}

	auto& rewind = rewind_captures[current_rewind_capture_idx++ % rewind_captures.size()];

	if (rewind)
	{
		spu_log.error("Due to resource limits the 16th SPU rewind capture is being overwritten with a new one. (free the most recent by loading it)");
		rewind.reset();
	}

	rewind = std::make_shared<utils::serial>();
	(*rewind)(std::span(spu_exec.progs[0].bin.data(), spu_exec.progs[0].bin.size())); // span serialization doesn't remember size which is what we need
	serialize_common(*rewind);

	// TODO: Save and restore decrementer state properly

	spu_log.success("SPU rewind image has been saved in memory. (%d free slots left)", std::count_if(rewind_captures.begin(), rewind_captures.end(), FN(!x.operator bool())));
	return true;
}

bool spu_thread::try_load_debug_capture()
{
	if (cpu_flag::wait - state)
	{
		return false;
	}

	auto rewind = std::move(rewind_captures[(current_rewind_capture_idx - 1) % rewind_captures.size()]);

	if (!rewind)
	{
		return false;
	}

	rewind->set_reading_state();
	(*rewind)(std::span(ls, SPU_LS_SIZE)); // span serialization doesn't remember size which is what we need
	serialize_common(*rewind);
	current_rewind_capture_idx--;

	spu_log.success("Last SPU rewind image has been loaded.");
	return true;
}

void spu_thread::wakeup_delay(u32 div) const
{
	if (g_cfg.core.spu_wakeup_delay_mask & (1u << index))
		thread_ctrl::wait_for_accurate(utils::aligned_div(+g_cfg.core.spu_wakeup_delay, div));
}

spu_function_logger::spu_function_logger(spu_thread& spu, const char* func) noexcept
	: spu(spu)
{
	spu.current_func = func;
	spu.start_time = get_system_time();
}

spu_thread::thread_name_t::operator std::string() const
{
	std::string full_name = fmt::format("%s[0x%07x]", [](spu_type type) -> std::string_view
	{
		switch (type)
		{
		case spu_type::threaded: return "SPU"sv;
		case spu_type::raw: return "RawSPU"sv;
		case spu_type::isolated: return "Iso"sv;
		default: fmt::throw_exception("Unreachable");
		}
	}(_this->get_type()), _this->lv2_id);

	if (const std::string name = *_this->spu_tname.load(); !name.empty())
	{
		fmt::append(full_name, " %s", name);
	}

	return full_name;
}

spu_thread::spu_prio_t spu_thread::priority_t::load() const
{
	if (_this->get_type() != spu_type::threaded || !_this->group->has_scheduler_context)
	{
		spu_thread::spu_prio_t prio{};
		prio.prio = s32{smax};
		return prio;
	}

	return _this->group->prio;
}

s64 spu_channel::pop_wait(cpu_thread& spu, bool pop)
{
	u64 old = data.fetch_op([&](u64& data)
	{
		if (data & bit_count) [[likely]]
		{
			if (pop)
			{
				data = 0;
				return true;
			}

			return false;
		}

		data = (pop ? bit_occupy : 0) | bit_wait;

		if (pop)
		{
			jostling_value.release(bit_occupy);
		}

		return true;
	}).first;

	if (old & bit_count)
	{
		return static_cast<u32>(old);
	}

	for (int i = 0; i < 10; i++)
	{
		busy_wait();

		if (!(data & bit_wait))
		{
			return static_cast<u32>(pop ? jostling_value.exchange(0) : +data);
		}
	}

	lv2_obj::notify_all();

	const u32 wait_on_val = static_cast<u32>(((pop ? bit_occupy : 0) | bit_wait) >> 32);

	while (true)
	{
		thread_ctrl::wait_on(utils::bless<atomic_t<u32>>(&data)[1], wait_on_val);
		old = data;

		if (!(old & bit_wait))
		{
			return static_cast<u32>(pop ? jostling_value.exchange(0) : +data);
		}

		if (spu.is_stopped())
		{
			// Abort waiting and test if a value has been received
			if (pop)
			{
				if (u64 v = jostling_value.exchange(0); !(v & bit_occupy))
				{
					return static_cast<u32>(v);
				}

				ensure(data.fetch_and(~(bit_wait | bit_occupy)) & bit_wait);
			}
			else
			{
				data.bit_test_reset(off_wait);
			}

			return -1;
		}
	}
}

// Waiting for channel push state availability, actually pushing if specified
bool spu_channel::push_wait(cpu_thread& spu, u32 value, bool push)
{
	u64 state{};
	data.fetch_op([&](u64& data)
	{
		if (data & bit_count) [[unlikely]]
		{
			jostling_value.release(push ? (bit_occupy | value) : static_cast<u32>(data));
			data |= (push ? bit_occupy : 0) | bit_wait;
		}
		else if (push)
		{
			data = bit_count | value;
		}
		else
		{
			state = data;
			return false;
		}

		state = data;
		return true;
	});

	for (int i = 0; i < 10; i++)
	{
		if (!(state & bit_wait))
		{
			return true;
		}

		busy_wait();
		state = data;
	}

	while (true)
	{
		if (!(state & bit_wait))
		{
			return true;
		}

		if (spu.is_stopped())
		{
			return !data.bit_test_reset(off_wait);
		}

		thread_ctrl::wait_on(utils::bless<atomic_t<u32>>(&data)[1], u32(state >> 32));
		state = data;
	}
}

std::pair<u32, u32> spu_channel_4_t::pop_wait(cpu_thread& spu, bool pop_value)
{
	auto old = values.fetch_op([&](sync_var_t& data)
	{
		if (data.count != 0)
		{
			if (!pop_value)
			{
				return;
			}

			data.waiting = 0;
			data.count--;

			data.value0 = data.value1;
			data.value1 = data.value2;
			data.value2 = this->value3;
		}
		else
		{
			data.waiting = (pop_value ? bit_occupy : 0) | bit_wait;
			jostling_value.release(pop_value ? jostling_flag : 0);
		}
	});

	if (old.count)
	{
		return {old.count, old.value0};
	}

	old.waiting = (pop_value ? bit_occupy : 0) | bit_wait;

	for (int i = 0; i < 10; i++)
	{
		busy_wait();

		if (!atomic_storage<u8>::load(values.raw().waiting))
		{
			return {1, static_cast<u32>(pop_value ? jostling_value.exchange(0) : 0)};
		}
	}

	while (true)
	{
		thread_ctrl::wait_on(utils::bless<atomic_t<u32>>(&values)[0], u32(u64(std::bit_cast<u128>(old))));
		old = values;

		if (~old.waiting & bit_wait)
		{
			// Count of 1 because a value has been inserted and popped in the same step.
			return {1, static_cast<u32>(pop_value ? jostling_value.exchange(0) : 0)};
		}

		if (spu.is_stopped())
		{
			if (pop_value)
			{
				// Abort waiting and test if a value has been received
				if (u64 v = jostling_value.exchange(0); !(v & jostling_flag))
				{
					return {1, static_cast<u32>(v)};
				}
			}

			if (~atomic_storage<u8>::exchange(values.raw().waiting, 0) & bit_wait)
			{
				// Count of 1 because a value has been inserted and popped in the same step.
				return {1, static_cast<u32>(pop_value ? jostling_value.exchange(0) : 0)};
			}

			return {};
		}
	}
}

spu_channel_op_state spu_channel_4_t::push(u32 value, bool postpone_notify)
{
	while (true)
	{
		value3.release(value);
		const auto [old, pushed_to_data] = values.fetch_op([&](sync_var_t& data)
		{
			if (data.waiting & bit_occupy)
			{
				return false;
			}

			switch (data.count++)
			{
			case 0: data.value0 = value; break;
			case 1: data.value1 = value; break;
			case 2: data.value2 = value; break;
			default:
			{
				data.count = 4;
				data.value3_inval++; // Ensure the SPU reads the most recent value3 write in try_pop by re-loading
				break;
			}
			}

			return true;
		});

		if (!pushed_to_data)
		{
			// Insert the pending value in special storage for waiting SPUs, leave no time in which the channel has data
			if (!jostling_value.compare_and_swap_test(jostling_flag, value))
			{
				// Other thread has inserted a value through jostling_value, retry
				continue;
			}
		}

		if (old.waiting & bit_wait)
		{
			// Turn off waiting bit manually (must succeed because waiting bit can only be resetted by the thread pushing to jostling_value)
			if (~atomic_storage<u8>::exchange(values.raw().waiting, 0) & bit_wait)
			{
				// Could be fatal or at emulation stopping, to be checked by the caller
				return { old.count, old.count, false, false };
			}

			if (!postpone_notify)
			{
				utils::bless<atomic_t<u32>>(&values)[0].notify_one();
			}
		}

		return { old.count, std::min<u8>(static_cast<u8>(old.count + 1), 4), !!(old.waiting & bit_wait), true };
	}
}

template <>
void fmt_class_string<spu_channel>::format(std::string& out, u64 arg)
{
	const auto& ch = get_object(arg);

	u32 data = 0;

	if (ch.try_read(data))
	{
		fmt::append(out, "0x%08x", data);
	}
	else
	{
		out += "empty";
	}
}

template <>
void fmt_class_string<spu_channel_4_t>::format(std::string& out, u64 arg)
{
	const auto& ch = get_object(arg);

	u32 vals[4]{};
	const uint count = ch.try_read(vals);

	if (count == 0u)
	{
		out += "empty\n\n";
		return;
	}

	fmt::append(out, "data:");

	if (count > 1u)
	{
		out += '\n';
	}
	else
	{
		out += ' ';
	}

	out += "{ ";

	for (u32 i = 0; i < count;)
	{
		fmt::append(out, "0x%x", vals[i]);

		if (++i != count)
		{
			out += ", ";
		}
	}

	out += " }\n";

	if (count <= 1u)
	{
		// Keep the amount of lines consistent
		out += '\n';
	}
}

DECLARE(spu_thread::g_raw_spu_ctr){};
DECLARE(spu_thread::g_raw_spu_id){};
DECLARE(spu_thread::g_spu_work_count){};
DECLARE(spu_thread::g_spu_waiters_by_value){};
