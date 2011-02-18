#ifndef XBYAK_XBYAK_UTIL_H_
#define XBYAK_XBYAK_UTIL_H_

/**
	utility class and functions for Xbyak
	@note this header is UNDER CONSTRUCTION!
*/
#include "xbyak.h"

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
	#ifndef __GNUC_PREREQ
    	#define __GNUC_PREREQ(major, minor) (((major) << 16) + (minor))
	#endif
	#if __GNUC_PREREQ(4, 3) && !defined(__APPLE__)
		#include <cpuid.h>
	#else
		#if defined(__APPLE__) && defined(XBYAK32) // avoid err : can't find a register in class `BREG' while reloading `asm'
			#define __cpuid(eaxIn, a, b, c, d) __asm__ __volatile__("pushl %%ebx\ncpuid\nmovl %%ebp, %%esi\npopl %%ebx" : "=a"(a), "=S"(b), "=c"(c), "=d"(d) : "0"(eaxIn))
		#else
			#define __cpuid(eaxIn, a, b, c, d) __asm__ __volatile__("cpuid\n" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(eaxIn))
		#endif
	#endif
#endif

#ifdef _MSC_VER
extern "C" unsigned __int64 __xgetbv(int);
#endif

namespace Xbyak { namespace util {

/**
	CPU detection class
*/
class Cpu {
	unsigned int type_;
	unsigned int get32bitAsBE(const char *x) const
	{
		return x[0] | (x[1] << 8) | (x[2] << 16) | (x[3] << 24);
	}
public:
	static inline void getCpuid(unsigned int eaxIn, unsigned int data[4])
	{
#ifdef _WIN32
		__cpuid(reinterpret_cast<int*>(data), eaxIn);
#else
		__cpuid(eaxIn, data[0], data[1], data[2], data[3]);
#endif
	}
	static inline uint64 getXfeature()
	{
#ifdef _MSC_VER
		return __xgetbv(0);
#else
		unsigned int eax, edx;
		__asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
		return ((uint64)edx << 32) | eax;
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
		tAESNI = 1 << 10,
		tSSE5 = 1 << 11,
		tOSXSACE = 1 << 12,
		tPCLMULQDQ = 1 << 13,
		tAVX = 1 << 14,
		tFMA = 1 << 15,

		t3DN = 1 << 16,
		tE3DN = 1 << 17,
		tSSE4a = 1 << 18,
		tRDTSCP = 1 << 19,

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
		if (data[2] == get32bitAsBE(amd)) {
			type_ |= tAMD;
			getCpuid(0x80000001, data);
			if (data[3] & (1U << 31)) type_ |= t3DN;
			if (data[3] & (1U << 15)) type_ |= tCMOV;
			if (data[3] & (1U << 30)) type_ |= tE3DN;
			if (data[3] & (1U << 22)) type_ |= tMMX2;
			if (data[3] & (1U << 27)) type_ |= tRDTSCP;
		}
		if (data[2] == get32bitAsBE(intel)) {
			type_ |= tINTEL;
			getCpuid(0x80000001, data);
			if (data[3] & (1U << 27)) type_ |= tRDTSCP;
		}
		getCpuid(1, data);
		if (data[2] & (1U << 0)) type_ |= tSSE3;
		if (data[2] & (1U << 9)) type_ |= tSSSE3;
		if (data[2] & (1U << 19)) type_ |= tSSE41;
		if (data[2] & (1U << 20)) type_ |= tSSE42;
		if (data[2] & (1U << 23)) type_ |= tPOPCNT;
		if (data[2] & (1U << 25)) type_ |= tAESNI;
		if (data[2] & (1U << 1)) type_ |= tPCLMULQDQ;
		if (data[2] & (1U << 27)) type_ |= tOSXSACE;

		if (type_ & tOSXSACE) {
			// check XFEATURE_ENABLED_MASK[2:1] = '11b'
			uint64 bv = getXfeature();
			if ((bv & 6) == 6) {
				if (data[2] & (1U << 28)) type_ |= tAVX;
				if (data[2] & (1U << 12)) type_ |= tFMA;
			}
		}

		if (data[3] & (1U << 15)) type_ |= tCMOV;
		if (data[3] & (1U << 23)) type_ |= tMMX;
		if (data[3] & (1U << 25)) type_ |= tMMX2 | tSSE;
		if (data[3] & (1U << 26)) type_ |= tSSE2;
	}
	bool has(Type type) const
	{
		return (type & type_) != 0;
	}
};

class Clock {
public:
	static inline uint64 getRdtsc()
	{
#ifdef _MSC_VER
		return __rdtsc();
#else
		unsigned int eax, edx;
		__asm__ volatile("rdtsc" : "=a"(eax), "=d"(edx));
		return ((uint64)edx << 32) | eax;
#endif
	}
	Clock()
		: clock_(0)
		, count_(0)
	{
	}
	void begin()
	{
		clock_ -= getRdtsc();
	}
	void end()
	{
		clock_ += getRdtsc();
		count_++;
	}
	int getCount() const { return count_; }
	uint64 getClock() const { return clock_; }
	void clear() { count_ = 0; clock_ = 0; }
private:
	uint64 clock_;
	int count_;
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

/**
	get eip to out register
	@note out is not esp
*/
template<class T>
void setEipTo(T *self, const Xbyak::Reg32& out)
{
#if 0
	self->call("@f");
self->L("@@");
	self->pop(out);
#else
	int idx = out.getIdx();
	switch (idx) {
	case Xbyak::Operand::EAX:
		self->call((void*)local::set_eip_to_eax);
		break;
	case Xbyak::Operand::ECX:
		self->call((void*)local::set_eip_to_ecx);
		break;
	case Xbyak::Operand::EDX:
		self->call((void*)local::set_eip_to_edx);
		break;
	case Xbyak::Operand::EBX:
		self->call((void*)local::set_eip_to_ebx);
		break;
	case Xbyak::Operand::ESI:
		self->call((void*)local::set_eip_to_esi);
		break;
	case Xbyak::Operand::EDI:
		self->call((void*)local::set_eip_to_edi);
		break;
	case Xbyak::Operand::EBP:
		self->call((void*)local::set_eip_to_ebp);
		break;
	default:
		assert(0);
	}
#endif
}
#endif

} } // end of util
#endif
