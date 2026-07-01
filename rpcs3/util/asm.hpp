#pragma once

#include "util/types.hpp"
#include "util/tsc.hpp"
#include "util/atomic.hpp"
#include "util/sysinfo.hpp"
#include <functional>
#include <thread>

#ifdef ARCH_X64
#ifdef _MSC_VER
#include <intrin.h>
#include <immintrin.h>
#else
#include <immintrin.h>
#include <x86intrin.h>
#endif
#endif

namespace utils
{
	// Try to prefetch to Level 2 cache since it's not split to data/code on most processors
	template <typename T>
	constexpr void prefetch_exec(T func)
	{
		if (std::is_constant_evaluated())
		{
			return;
		}

		const u64 value = reinterpret_cast<u64>(func);
		const void* ptr = reinterpret_cast<const void*>(value);

#ifdef ARCH_X64
		return _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T1);
#else
		return __builtin_prefetch(ptr, 0, 2);
#endif
	}

	// Try to prefetch to Level 1 cache
	constexpr void prefetch_read(const void* ptr)
	{
		if (std::is_constant_evaluated())
		{
			return;
		}

#ifdef ARCH_X64
		return _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#else
		return __builtin_prefetch(ptr, 0, 3);
#endif
	}

	constexpr void prefetch_write(const void* ptr)
	{
		if (std::is_constant_evaluated())
		{
			return;
		}

#if defined(ARCH_X64)
		return _m_prefetchw(const_cast<void*>(ptr));
#else
		return __builtin_prefetch(ptr, 1, 3);
#endif
	}

	constexpr u32 popcnt128(const u128& v)
	{
#ifdef _MSC_VER
		return std::popcount(v.lo) + std::popcount(v.hi);
#else
		return std::popcount(v);
#endif
	}

	constexpr u64 umulh64(u64 x, u64 y)
	{
#ifdef _MSC_VER
		if (std::is_constant_evaluated())
#endif
		{
			return static_cast<u64>((u128{x} * u128{y}) >> 64);
		}

#ifdef _MSC_VER
		return __umulh(x, y);
#endif
	}

	inline s64 mulh64(s64 x, s64 y)
	{
#ifdef _MSC_VER
		return __mulh(x, y);
#else
		return (s128{x} * s128{y}) >> 64;
#endif
	}

	inline s64 div128(s64 high, s64 low, s64 divisor, s64* remainder = nullptr)
	{
#ifdef _MSC_VER
		s64 rem = 0;
		s64 r = _div128(high, low, divisor, &rem);

		if (remainder)
		{
			*remainder = rem;
		}
#else
		const s128 x = (u128{static_cast<u64>(high)} << 64) | u64(low);
		const s128 r = x / divisor;

		if (remainder)
		{
			*remainder = x % divisor;
		}
#endif
		return r;
	}

	inline u64 udiv128(u64 high, u64 low, u64 divisor, u64* remainder = nullptr)
	{
#ifdef _MSC_VER
		u64 rem = 0;
		u64 r = _udiv128(high, low, divisor, &rem);

		if (remainder)
		{
			*remainder = rem;
		}
#else
		const u128 x = (u128{high} << 64) | low;
		const u128 r = x / divisor;

		if (remainder)
		{
			*remainder = x % divisor;
		}
#endif
		return r;
	}

#ifdef _MSC_VER
	inline u128 operator/(u128 lhs, u64 rhs)
	{
		u64 rem = 0;
		return _udiv128(lhs.hi, lhs.lo, rhs, &rem);
	}
#endif

	constexpr u32 ctz128(u128 arg)
	{
#ifdef _MSC_VER
		if (!arg.lo)
			return std::countr_zero(arg.hi) + 64u;
		else
			return std::countr_zero(arg.lo);
#else
		return std::countr_zero(arg);
#endif
	}

	constexpr u32 clz128(u128 arg)
	{
#ifdef _MSC_VER
		if (arg.hi)
			return std::countl_zero(arg.hi);
		else
			return std::countl_zero(arg.lo) + 64;
#else
		return std::countl_zero(arg);
#endif
	}

	inline void pause()
	{
#if defined(ARCH_ARM64)
		__asm__ volatile("isb" ::: "memory");
#elif defined(ARCH_X64)
		_mm_pause();
#else
#error "Missing utils::pause() implementation"
#endif
	}

	// The hardware clock on many arm timers run south of 100mhz
	// and the busy waits in RPCS3 were written assuming an x86 machine
	// with hardware timers that run around 3GHz.
	// For instance, on the snapdragon 8 gen 2, the hardware timer runs at 19.2mhz.
	// This means that a busy wait that would have taken nanoseconds on x86 will run for
	// many microseconds on many arm machines. 
#ifdef ARCH_ARM64

	inline u64 arm_timer_scale = 1;

	inline void init_arm_timer_scale()
	{
		u64 freq = 0;
		asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
		
		// Try to scale hardware timer to match 3GHz
		u64 timer_scale = freq / 30000000;
		if (timer_scale)
			arm_timer_scale = timer_scale;
	}
#endif
		
	inline void busy_wait(u64 cycles = 3000)
	{
#ifdef ARCH_ARM64
		const u64 stop = get_tsc() + ((cycles / 100) * arm_timer_scale);
#else
		const u64 stop = get_tsc() + cycles;
#endif
		do pause();
		while (get_tsc() < stop);
	}

#ifdef ARCH_X64
	inline u64 get_wait_cycles(u64 timeout_us, u64 tsc_freq)
	{
		constexpr u64 max_timeout = u64{umax};

		if (!tsc_freq)
		{
			return 0;
		}

		if (timeout_us == max_timeout)
		{
			return max_timeout;
		}

		const u64 seconds = timeout_us / 1'000'000;
		const u64 micros = timeout_us % 1'000'000;

		if (seconds > max_timeout / tsc_freq)
		{
			return max_timeout;
		}

		const u64 sec_cycles = seconds * tsc_freq;
		const u64 cycles_per_us = tsc_freq / 1'000'000;

		if (micros && cycles_per_us > max_timeout / micros)
		{
			return max_timeout;
		}

		const u64 us_cycles = micros * cycles_per_us + (micros * (tsc_freq % 1'000'000)) / 1'000'000;
		return sec_cycles > max_timeout - us_cycles ? max_timeout : sec_cycles + us_cycles;
	}
#endif

	template <typename T, usz Align>
#if defined(ARCH_X64) && !defined(_MSC_VER)
	__attribute__((target("waitpkg,mwaitx")))
#endif
	inline void spin_on_cacheline_once(const atomic_t<T, Align>& var, T old_value, u64 timeout_us)
	{
		const void* addr = &var.raw();

#if defined(ARCH_ARM64)
		// WFE will wake from the periodic event stream, so the explicit timeout is ignored on ARM.
		(void)timeout_us;

		using wait_type = std::remove_cvref_t<decltype(var.raw())>;
		using raw_type = std::conditional_t<sizeof(wait_type) == 8, u64,
			std::conditional_t<sizeof(wait_type) == 4, u32,
			std::conditional_t<sizeof(wait_type) == 2, u16, u8>>>;

		static_assert(sizeof(wait_type) <= 8, "Unsupported atomic size for spin_on_cacheline_once");

		raw_type value{};
		const auto* wait_addr = static_cast<const volatile raw_type*>(addr);

		if constexpr (sizeof(raw_type) == 1) __asm__ volatile("ldaxrb %w0, %1" : "=r"(value) : "Q"(*wait_addr) : "memory");
		else if constexpr (sizeof(raw_type) == 2) __asm__ volatile("ldaxrh %w0, %1" : "=r"(value) : "Q"(*wait_addr) : "memory");
		else if constexpr (sizeof(raw_type) == 4) __asm__ volatile("ldaxr %w0, %1" : "=r"(value) : "Q"(*wait_addr) : "memory");
		else if constexpr (sizeof(raw_type) == 8) __asm__ volatile("ldaxr %x0, %1" : "=r"(value) : "Q"(*wait_addr) : "memory");

		if (std::bit_cast<wait_type>(value) != old_value)
		{
			__asm__ volatile("clrex" ::: "memory");
			return;
		}

		__asm__ volatile("wfe" ::: "memory");
		__asm__ volatile("clrex" ::: "memory");
#elif defined(ARCH_X64)
		static const bool use_umwait = has_waitpkg();
		static const bool use_waitx = has_waitx();

		const u64 cycles = get_wait_cycles(timeout_us, get_tsc_freq());

		if (use_umwait && cycles)
		{
			_umonitor(const_cast<void*>(addr));

			if (var.load() != old_value)
			{
				return;
			}

			constexpr u64 max_timeout = u64{umax};
			const u64 now = get_tsc();
			const u64 deadline = cycles > max_timeout - now ? max_timeout : now + cycles;
			_umwait(0, deadline);
		}
		else if (use_waitx && cycles)
		{
			_mm_monitorx(const_cast<void*>(addr), 0, 0);

			if (var.load() != old_value)
			{
				return;
			}

			constexpr u32 timer_enable = 2;
			_mm_mwaitx(timer_enable, 0, cycles > u32{umax} ? u32{umax} : static_cast<u32>(cycles));
		}
		else
		{
			std::this_thread::yield();
		}
#else
		(void)addr;
		(void)old_value;
		(void)timeout_us;

		std::this_thread::yield();
#endif
	}

	template <typename T, usz Align, typename Pred>
#if defined(ARCH_X64) && !defined(_MSC_VER)
	__attribute__((target("waitpkg,mwaitx")))
#endif
	inline void spin_wait(const atomic_t<T, Align>& var, Pred predicate)
	{
#ifdef ARCH_X64
		static const bool use_umwait = has_waitpkg();
		static const bool use_waitx = has_waitx();
#endif

		const auto read_mem = [&]()
		{
			return var.load();
		};

		const void* addr = &var.raw();

		while (true)
		{
			if (predicate(read_mem()))
			{
				return;
			}

#if defined(ARCH_ARM64)
			using value_type = decltype(read_mem());
			using wait_type = std::remove_cvref_t<decltype(var.raw())>;

			wait_type value{};
			const auto* wait_addr = static_cast<const volatile wait_type*>(addr);

			if constexpr (sizeof(wait_type) == 1) __asm__ volatile("ldaxrb %w0, %1" : "=r"(value) : "Q"(*wait_addr) : "memory");
			else if constexpr (sizeof(wait_type) == 2) __asm__ volatile("ldaxrh %w0, %1" : "=r"(value) : "Q"(*wait_addr) : "memory");
			else if constexpr (sizeof(wait_type) == 4) __asm__ volatile("ldaxr %w0, %1" : "=r"(value) : "Q"(*wait_addr) : "memory");
			else if constexpr (sizeof(wait_type) == 8) __asm__ volatile("ldaxr %x0, %1" : "=r"(value) : "Q"(*wait_addr) : "memory");
			else static_assert(sizeof(wait_type) <= 8, "Unsupported atomic size for spin_wait");

			if (predicate(static_cast<value_type>(value)))
			{
				__asm__ volatile("clrex" ::: "memory");
				return;
			}

			__asm__ volatile("wfe" ::: "memory");
			__asm__ volatile("clrex" ::: "memory");

#elif defined(ARCH_X64)
			if (use_umwait)
			{
				_umonitor(const_cast<void*>(addr));
				if (predicate(read_mem()))
				{
					return;
				}

				_umwait(0, ~0ULL);
			}
			else if (use_waitx)
			{
				_mm_monitorx(const_cast<void*>(addr), 0, 0);
				if (predicate(read_mem()))
				{
					return;
				}

				_mm_mwaitx(0, 0, 0);
			}
			else
			{
				pause();
			}
#else
			pause();
#endif
		}
	}

	// Align to power of 2
	template <typename T, typename U>
		requires std::is_unsigned_v<T>
	constexpr std::make_unsigned_t<std::common_type_t<T, U>> align(T value, U align)
	{
		return static_cast<std::make_unsigned_t<std::common_type_t<T, U>>>((value + (align - 1)) & (T{0} - align));
	}

	// General purpose aligned division, the result is rounded up not truncated
	template <typename T>
		requires std::is_unsigned_v<T>
	constexpr T aligned_div(T value, std::type_identity_t<T> align)
	{
		return static_cast<T>(value / align + T{!!(value % align)});
	}

	// General purpose aligned division, the result is rounded to nearest
	template <typename T>
		requires std::is_integral_v<T>
	constexpr T rounded_div(T value, std::type_identity_t<T> align)
	{
		if constexpr (std::is_unsigned_v<T>)
		{
			return static_cast<T>(value / align + T{(value % align) > (align / 2)});
		}

		return static_cast<T>(value / align + (value > 0 ? T{(value % align) > (align / 2)} : 0 - T{(value % align) < (align / 2)}));
	}

	// Multiplying by ratio, semi-resistant to overflows
	template <UnsignedInt T>
	constexpr T rational_mul(T value, std::type_identity_t<T> numerator, std::type_identity_t<T> denominator)
	{
		if constexpr (sizeof(T) <= sizeof(u64) / 2)
		{
			return static_cast<T>(value * u64{numerator} / u64{denominator});
		}

		return static_cast<T>(value / denominator * numerator + (value % denominator) * numerator / denominator);
	}

	template <UnsignedInt T>
	constexpr T add_saturate(T addend1, T addend2)
	{
		return static_cast<T>(~addend1) < addend2 ? T{umax} : static_cast<T>(addend1 + addend2);
	}

	template <UnsignedInt T>
	constexpr T sub_saturate(T minuend, T subtrahend)
	{
		return minuend < subtrahend ? T{0} : static_cast<T>(minuend - subtrahend);
	}

	template <UnsignedInt T>
	constexpr T mul_saturate(T factor1, T factor2)
	{
		return factor1 > 0 && T{umax} / factor1 < factor2 ? T{umax} : static_cast<T>(factor1 * factor2);
	}

	inline void trigger_write_page_fault(void* ptr)
	{
#if defined(ARCH_X64) && !defined(_MSC_VER)
		__asm__ volatile("lock orl $0, 0(%0)" :: "r" (ptr));
#elif defined(ARCH_ARM64)
		u32 value = 0;
		u32* u32_ptr = static_cast<u32*>(ptr);
		__asm__ volatile("ldset %w0, %w0, %1" : "+r"(value), "=Q"(*u32_ptr) : "r"(value));
#else
		*static_cast<atomic_t<u32> *>(ptr) += 0;
#endif
	}

	inline void trap()
	{
#ifdef _M_X64
		__debugbreak();
#elif defined(ARCH_X64)
		__asm__ volatile("int3");
#elif defined(ARCH_ARM64)
		__asm__ volatile("brk 0x42");
#else
#error "Missing utils::trap() implementation"
#endif
	}

	inline void* get_thread_storage_pointer() noexcept
	{
	#if defined(_WIN64)

	    // TEB
	    return (void*)__readgsqword(0x30);

	#elif defined(_WIN32)

	    // TEB
	    return (void*)__readfsdword(0x18);

	#elif defined(__aarch64__) && defined(__APPLE__)

	    // Apple Silicon
	    void* tp;
	    asm volatile("mrs %0, TPIDRRO_EL0" : "=r"(tp));
	    return tp;

	#elif defined(__aarch64__)

	    // Linux/BSD AArch64
	    void* tp;
	    asm volatile("mrs %0, TPIDR_EL0" : "=r"(tp));
	    return tp;

	#elif defined(__x86_64__)

	    // SysV x86-64 (Linux, BSD, etc.)
	    void* tp;
	    asm volatile("mov %%fs:0, %0" : "=r"(tp));
	    return tp;

	#elif defined(__i386__)

	    // Linux/BSD i386
	    void* tp;
	    asm volatile("mov %%gs:0, %0" : "=r"(tp));
	    return tp;
	#else
	#error Unsupported platform
	#endif
	}

	inline usz get_thread_storage_offset(const void* ptr) noexcept
	{
		const usz ptr_addr = reinterpret_cast<usz>(ptr);
		const usz base = reinterpret_cast<usz>(get_thread_storage_pointer());

		const usz ret_val = ptr_addr - base;

		while (!ptr || ret_val > 0x100000000)
		{
			utils::trap();
		}

		return ret_val;
	}

} // namespace utils

using utils::busy_wait;

#ifdef _MSC_VER
using utils::operator/;
#endif
