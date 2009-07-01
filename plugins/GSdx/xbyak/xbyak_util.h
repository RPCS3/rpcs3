#ifndef XBYAK_XBYAK_UTIL_H_
#define XBYAK_XBYAK_UTIL_H_

/**
	utility class for Xbyak
	@note this header is under construction
*/

#ifdef _WIN32
	#if (_MSC_VER < 1400) && defined(XBYAK32)
		static inline __declspec(naked) void __cpuid(int[4], int)
		{
			__asm {
				push	ebx
				push	esi
				mov		eax, dword ptr [esp + 4 * 2 + 8] // eaxIn
				cpuid
				mov		esi, dword ptr [esp + 4 * 2 + 4] // data
				mov		dword ptr [esi], eax
				mov		dword ptr [esi + 4], ebx
				mov		dword ptr [esi + 8], ecx
				mov		dword ptr [esi + 12], edx
				pop		esi
				pop		ebx
				ret
			}
		}
	#else
		#include <intrin.h> // for __cpuid
	#endif
#else
	#if __GNUC_PREREQ(4, 3)
		#include <cpuid.h>
	#else
		#define __cpuid(eaxIn, a, b, c, d) __asm__("cpuid\n" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(eaxIn))
	#endif
#endif

namespace Xbyak { namespace util {

/**
	CPU detection class
*/
class Cpu {
	unsigned int type_;
public:
	static inline void getCpuid(unsigned int eaxIn, unsigned int data[4])
	{
#ifdef _WIN32
		__cpuid(reinterpret_cast<int*>(data), eaxIn);
#else
		__cpuid(eaxIn, data[0], data[1], data[2], data[3]);
#endif
	}
	enum Type {
		NONE = 0,
		tMMX = 1 << 0,
		tMMX2 = 1 << 1,
		tCMOV = 1 << 2,
		tSSE = 1 << 3,
		tSSE2 = 1 << 4,
		tSSE3 = 1 << 5,
		tSSSE3 = 1 << 6,
		tSSE41 = 1 << 7,
		tSSE42 = 1 << 8,
		tPOPCNT = 1 << 9,

		t3DN = 1 << 16,
		tE3DN = 1 << 17,
		tSSE4a = 1 << 18,
		tSSE5 = 1 << 11,

		tINTEL = 1 << 24,
		tAMD = 1 << 25
	};
	Cpu()
		: type_(NONE)
	{
		unsigned int data[4];
		getCpuid(0, data);
		static const char intel[] = "ntel";
		static const char amd[] = "cAMD";
		if (data[2] == *reinterpret_cast<const unsigned int*>(amd)) {
			type_ |= tAMD;
			getCpuid(0x80000001, data);
			if (data[3] & (1 << 31)) type_ |= t3DN;
			if (data[3] & (1 << 15)) type_ |= tCMOV;
			if (data[3] & (1 << 30)) type_ |= tE3DN;
			if (data[3] & (1 << 22)) type_ |= tMMX2;
		}
		if (data[2] == *reinterpret_cast<const unsigned int*>(intel)) {
			type_ |= tINTEL;
		}
		getCpuid(1, data);
		if (data[2] & (1 << 0)) type_ |= tSSE3;
		if (data[2] & (1 << 9)) type_ |= tSSSE3;
		if (data[2] & (1 << 19)) type_ |= tSSE41;
		if (data[2] & (1 << 20)) type_ |= tSSE42;
		if (data[2] & (1 << 23)) type_ |= tPOPCNT;

		if (data[3] & (1 << 15)) type_ |= tCMOV;
		if (data[3] & (1 << 23)) type_ |= tMMX;
		if (data[3] & (1 << 25)) type_ |= tMMX2 | tSSE;
		if (data[3] & (1 << 26)) type_ |= tSSE2;
	}
	bool has(Type type) const
	{
		return (type & type_) != 0;
	}
};

#ifdef XBYAK32

namespace local {
#ifdef _WIN32
	#define XBYAK_LOCAL_DEFINE_SET_EIP_TO_REG(x) static inline __declspec(naked) void set_eip_to_ ## x() { \
	__asm { mov x, dword ptr [esp] } __asm { ret } \
	}
#else
	#define XBYAK_LOCAL_DEFINE_SET_EIP_TO_REG(x) static inline void set_eip_to_ ## x() { \
	__asm__ volatile("movl (%esp), %" #x); \
	}
#endif

XBYAK_LOCAL_DEFINE_SET_EIP_TO_REG(eax)
XBYAK_LOCAL_DEFINE_SET_EIP_TO_REG(ecx)
XBYAK_LOCAL_DEFINE_SET_EIP_TO_REG(edx)
XBYAK_LOCAL_DEFINE_SET_EIP_TO_REG(ebx)
XBYAK_LOCAL_DEFINE_SET_EIP_TO_REG(esi)
XBYAK_LOCAL_DEFINE_SET_EIP_TO_REG(edi)
XBYAK_LOCAL_DEFINE_SET_EIP_TO_REG(ebp)

#undef XBYAK_LOCAL_DEFINE_SET_EIP_TO_REG
} // end of local

template<class Gen>
struct EnableSetEip : public Gen {
	EnableSetEip(size_t maxSize = DEFAULT_MAX_CODE_SIZE, void *userPtr = 0)
		: Gen(maxSize, userPtr)
	{
	}
	/**
		get pid to out register
		@note out = eax or ecx or edx
	*/
	void setEipTo(const Xbyak::Reg32& out)
	{
#if 0
		Gen::call(Gen::getCurr() + 5);
		Gen::pop(out);
#else
		int idx = out.getIdx();
		switch (idx) {
		case Xbyak::Operand::EAX:
			Gen::call((void*)local::set_eip_to_eax);
			break;
		case Xbyak::Operand::ECX:
			Gen::call((void*)local::set_eip_to_ecx);
			break;
		case Xbyak::Operand::EDX:
			Gen::call((void*)local::set_eip_to_edx);
			break;
		case Xbyak::Operand::EBX:
			Gen::call((void*)local::set_eip_to_ebx);
			break;
		case Xbyak::Operand::ESI:
			Gen::call((void*)local::set_eip_to_esi);
			break;
		case Xbyak::Operand::EDI:
			Gen::call((void*)local::set_eip_to_edi);
			break;
		case Xbyak::Operand::EBP:
			Gen::call((void*)local::set_eip_to_ebp);
			break;
		default:
			assert(0);
		}
#endif
	}
};
#endif

} } // end of util
#endif

