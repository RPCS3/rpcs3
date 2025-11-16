#pragma once

namespace utils
{
	// Hack. Pointer cast util to workaround UB. Use with extreme care.
	template <typename T, typename U>
	[[nodiscard]] T* bless(U* ptr)
	{
#ifdef _MSC_VER
		return (T*)ptr;
#elif defined(ARCH_X64)
		T* result;
		__asm__("movq %1, %0;" : "=r" (result) : "r" (ptr) : "memory");
		return result;
#elif defined(ARCH_ARM64)
		T* result;
		__asm__("mov %0, %1" : "=r" (result) : "r" (ptr) : "memory");
		return result;
#else
#error "Missing utils::bless() implementation"
#endif
	}
}
