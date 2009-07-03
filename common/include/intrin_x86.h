/*
	Compatibility <intrin_x86.h> header for GCC -- GCC equivalents of intrinsic
	Microsoft Visual C++ functions. Originally developed for the ReactOS
	(<http://www.reactos.org/>) and TinyKrnl (<http://www.tinykrnl.org/>)
	projects.

	Copyright (c) 2006 KJK::Hyperion <hackbunny@reactos.com>

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

#ifndef KJK_INTRIN_X86_H_
#define KJK_INTRIN_X86_H_

/*
	FIXME: review all "memory" clobbers, add/remove to match Visual C++
	behavior: some "obvious" memory barriers are not present in the Visual C++
	implementation - e.g. __stosX; on the other hand, some memory barriers that
	*are* present could have been missed
*/

/*
	NOTE: this is a *compatibility* header. Some functions may look wrong at
	first, but they're only "as wrong" as they would be on Visual C++. Our
	priority is compatibility

	NOTE: unlike most people who write inline asm for GCC, I didn't pull the
	constraints and the uses of __volatile__ out of my... hat. Do not touch
	them. I hate cargo cult programming

	NOTE: be very careful with declaring "memory" clobbers. Some "obvious"
	barriers aren't there in Visual C++ (e.g. __stosX)

	NOTE: review all intrinsics with a return value, add/remove __volatile__
	where necessary. If an intrinsic whose value is ignored generates a no-op
	under Visual C++, __volatile__ must be omitted; if it always generates code
	(for example, if it has side effects), __volatile__ must be specified. GCC
	will only optimize out non-volatile asm blocks with outputs, so input-only
	blocks are safe. Oddities such as the non-volatile 'rdmsr' are intentional
	and follow Visual C++ behavior

	NOTE: on GCC 4.1.0, please use the __sync_* built-ins for barriers and
	atomic operations. Test the version like this:

	#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100
		...

	Pay attention to the type of barrier. Make it match with what Visual C++
	would use in the same case
*/


#ifndef __SIZE_T
#define __SIZE_T
typedef unsigned int size_t;
#endif

/*** Stack frame juggling ***/
#define _ReturnAddress() (__builtin_return_address(0))
#define _AddressOfReturnAddress() (&(((void **)(__builtin_frame_address(0)))[1]))
/* TODO: __getcallerseflags but how??? */


/*** Atomic operations ***/

#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100
#define _ReadWriteBarrier() __sync_synchronize()
#else
static void __inline__ __attribute__((always_inline)) _MemoryBarrier(void)
{
	__asm__ __volatile__("" : : : "memory");
}
#define _ReadWriteBarrier() _MemoryBarrier()
#endif

/* BUGBUG: GCC only supports full barriers */
#define _ReadBarrier _ReadWriteBarrier
#define _WriteBarrier _ReadWriteBarrier

#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100

static __inline__ __attribute__((always_inline)) char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) long long _InterlockedCompareExchange64(volatile long long * const Destination, const long long Exchange, const long long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchange(volatile long * const Target, const long Value)
{
	/* NOTE: __sync_lock_test_and_set would be an acquire barrier, so we force a full barrier */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

static __inline__ __attribute__((always_inline)) void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
	/* NOTE: ditto */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchangeAdd(volatile long * const Addend, const long Value)
{
	return __sync_fetch_and_add(Addend, Value);
}

static __inline__ __attribute__((always_inline)) char _InterlockedAnd8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_and(value, mask);
}

static __inline__ __attribute__((always_inline)) short _InterlockedAnd16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_and(value, mask);
}

static __inline__ __attribute__((always_inline)) long _InterlockedAnd(volatile long * const value, const long mask)
{
	return __sync_fetch_and_and(value, mask);
}

static __inline__ __attribute__((always_inline)) char _InterlockedOr8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_or(value, mask);
}

static __inline__ __attribute__((always_inline)) short _InterlockedOr16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_or(value, mask);
}

static __inline__ __attribute__((always_inline)) long _InterlockedOr(volatile long * const value, const long mask)
{
	return __sync_fetch_and_or(value, mask);
}

static __inline__ __attribute__((always_inline)) char _InterlockedXor8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_xor(value, mask);
}

static __inline__ __attribute__((always_inline)) short _InterlockedXor16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_xor(value, mask);
}

static __inline__ __attribute__((always_inline)) long _InterlockedXor(volatile long * const value, const long mask)
{
	return __sync_fetch_and_xor(value, mask);
}

#else

static __inline__ __attribute__((always_inline)) char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand)
{
	char retval = Comperand;
	__asm__("lock; cmpxchgb %b[Exchange], %[Destination]" : [retval] "+a" (retval) : [Destination] "m" (*Destination), [Exchange] "q" (Exchange) : "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand)
{
	short retval = Comperand;
	__asm__("lock; cmpxchgw %w[Exchange], %[Destination]" : [retval] "+a" (retval) : [Destination] "m" (*Destination), [Exchange] "q" (Exchange): "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand)
{
	long retval = Comperand;
	__asm__("lock; cmpxchgl %k[Exchange], %[Destination]" : [retval] "+a" (retval) : [Destination] "m" (*Destination), [Exchange] "q" (Exchange): "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long long _InterlockedCompareExchange64(volatile long long * const Destination, const long long Exchange, const long long Comperand)
{
	long long retval = Comperand;

	__asm__
	(
		"cmpxchg8b %[Destination]" :
		[retval] "+A" (retval) :
			[Destination] "m" (*Destination),
			"b" ((unsigned long)((Exchange >>  0) & 0xFFFFFFFF)),
			"c" ((unsigned long)((Exchange >> 32) & 0xFFFFFFFF)) :
		"memory"
	);

	return retval;
}

static __inline__ __attribute__((always_inline)) void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
	void * retval = (void *)Comperand;
	__asm__("lock; cmpxchgl %k[Exchange], %[Destination]" : [retval] "=a" (retval) : "[retval]" (retval), [Destination] "m" (*Destination), [Exchange] "q" (Exchange) : "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchange(volatile long * const Target, const long Value)
{
	long retval = Value;
	__asm__("xchgl %[retval], %[Target]" : [retval] "+r" (retval) : [Target] "m" (*Target) : "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
	void * retval = Value;
	__asm__("xchgl %[retval], %[Target]" : [retval] "+r" (retval) : [Target] "m" (*Target) : "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchangeAdd(volatile long * const Addend, const long Value)
{
	long retval = Value;
	__asm__("lock; xaddl %[retval], %[Addend]" : [retval] "+r" (retval) : [Addend] "m" (*Addend) : "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) char _InterlockedAnd8(volatile char * const value, const char mask)
{
	char x;
	char y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange8(value, x & mask, x);
	}
	while(y != x);

	return y;
}

static __inline__ __attribute__((always_inline)) short _InterlockedAnd16(volatile short * const value, const short mask)
{
	short x;
	short y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange16(value, x & mask, x);
	}
	while(y != x);

	return y;
}

static __inline__ __attribute__((always_inline)) long _InterlockedAnd(volatile long * const value, const long mask)
{
	long x;
	long y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange(value, x & mask, x);
	}
	while(y != x);

	return y;
}

static __inline__ __attribute__((always_inline)) char _InterlockedOr8(volatile char * const value, const char mask)
{
	char x;
	char y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange8(value, x | mask, x);
	}
	while(y != x);

	return y;
}

static __inline__ __attribute__((always_inline)) short _InterlockedOr16(volatile short * const value, const short mask)
{
	short x;
	short y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange16(value, x | mask, x);
	}
	while(y != x);

	return y;
}

static __inline__ __attribute__((always_inline)) long _InterlockedOr(volatile long * const value, const long mask)
{
	long x;
	long y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange(value, x | mask, x);
	}
	while(y != x);

	return y;
}

static __inline__ __attribute__((always_inline)) char _InterlockedXor8(volatile char * const value, const char mask)
{
	char x;
	char y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange8(value, x ^ mask, x);
	}
	while(y != x);

	return y;
}

static __inline__ __attribute__((always_inline)) short _InterlockedXor16(volatile short * const value, const short mask)
{
	short x;
	short y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange16(value, x ^ mask, x);
	}
	while(y != x);

	return y;
}

static __inline__ __attribute__((always_inline)) long _InterlockedXor(volatile long * const value, const long mask)
{
	long x;
	long y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange(value, x ^ mask, x);
	}
	while(y != x);

	return y;
}

#endif

static __inline__ __attribute__((always_inline)) long _InterlockedAddLargeStatistic(volatile long long * const Addend, const long Value)
{
	__asm__
	(
		"lock; add %[Value], %[Lo32];"
		"jae LABEL%=;"
		"lock; adc $0, %[Hi32];"
		"LABEL%=:;" :
		[Lo32] "=m" (*((volatile long *)(Addend) + 0)), [Hi32] "=m" (*((volatile long *)(Addend) + 1)) :
		[Value] "ir" (Value)
	);

	return Value;
}

static __inline__ __attribute__((always_inline)) long _InterlockedDecrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, -1) - 1;
}

static __inline__ __attribute__((always_inline)) long _InterlockedIncrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, 1) + 1;
}

static __inline__ __attribute__((always_inline)) unsigned char _interlockedbittestandreset(volatile long * a, const long b)
{
	unsigned char retval;
	__asm__("lock; btrl %[b], %[a]; setb %b[retval]" : [retval] "=r" (retval), [a] "=m" (a) : [b] "Ir" (b) : "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _interlockedbittestandset(volatile long * a, const long b)
{
	unsigned char retval;
	__asm__("lock; btsl %[b], %[a]; setc %b[retval]" : [retval] "=r" (retval), [a] "=m" (a) : [b] "Ir" (b) : "memory");
	return retval;
}


/*** String operations ***/
/* NOTE: we don't set a memory clobber in the __stosX functions because Visual C++ doesn't */
static __inline__ __attribute__((always_inline)) void __stosb(unsigned char * Dest, const unsigned char Data, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; stosb" :
		[Dest] "=D" (Dest), [Count] "=c" (Count) :
		"[Dest]" (Dest), "a" (Data), "[Count]" (Count)
	);
}

static __inline__ __attribute__((always_inline)) void __stosw(unsigned short * Dest, const unsigned short Data, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; stosw" :
		[Dest] "=D" (Dest), [Count] "=c" (Count) :
		"[Dest]" (Dest), "a" (Data), "[Count]" (Count)
	);
}

static __inline__ __attribute__((always_inline)) void __stosd(unsigned long * Dest, const unsigned long Data, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; stosl" :
		[Dest] "=D" (Dest), [Count] "=c" (Count) :
		"[Dest]" (Dest), "a" (Data), "[Count]" (Count)
	);
}

static __inline__ __attribute__((always_inline)) void __movsb(unsigned char * Destination, const unsigned char * Source, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; movsb" :
		[Destination] "=D" (Destination), [Source] "=S" (Source), [Count] "=c" (Count) :
		"[Destination]" (Destination), "[Source]" (Source), "[Count]" (Count)
	);
}

static __inline__ __attribute__((always_inline)) void __movsw(unsigned short * Destination, const unsigned short * Source, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; movsw" :
		[Destination] "=D" (Destination), [Source] "=S" (Source), [Count] "=c" (Count) :
		"[Destination]" (Destination), "[Source]" (Source), "[Count]" (Count)
	);
}

static __inline__ __attribute__((always_inline)) void __movsd(unsigned long * Destination, const unsigned long * Source, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; movsd" :
		[Destination] "=D" (Destination), [Source] "=S" (Source), [Count] "=c" (Count) :
		"[Destination]" (Destination), "[Source]" (Source), "[Count]" (Count)
	);
}


/*** FS segment addressing ***/
static __inline__ __attribute__((always_inline)) void __writefsbyte(const unsigned long Offset, const unsigned char Data)
{
	__asm__("movb %b[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}

static __inline__ __attribute__((always_inline)) void __writefsword(const unsigned long Offset, const unsigned short Data)
{
	__asm__("movw %w[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}

static __inline__ __attribute__((always_inline)) void __writefsdword(const unsigned long Offset, const unsigned long Data)
{
	__asm__("movl %k[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}

static __inline__ __attribute__((always_inline)) unsigned char __readfsbyte(const unsigned long Offset)
{
	unsigned char value;
	__asm__("movb %%fs:%a[Offset], %b[value]" : [value] "=q" (value) : [Offset] "irm" (Offset));
	return value;
}

static __inline__ __attribute__((always_inline)) unsigned short __readfsword(const unsigned long Offset)
{
	unsigned short value;
	__asm__("movw %%fs:%a[Offset], %w[value]" : [value] "=q" (value) : [Offset] "irm" (Offset));
	return value;
}

static __inline__ __attribute__((always_inline)) unsigned long __readfsdword(const unsigned long Offset)
{
	unsigned long value;
	__asm__("movl %%fs:%a[Offset], %k[value]" : [value] "=q" (value) : [Offset] "irm" (Offset));
	return value;
}

static __inline__ __attribute__((always_inline)) void __incfsbyte(const unsigned long Offset)
{
	__asm__("incb %%fs:%a[Offset]" : : [Offset] "ir" (Offset));
}

static __inline__ __attribute__((always_inline)) void __incfsword(const unsigned long Offset)
{
	__asm__("incw %%fs:%a[Offset]" : : [Offset] "ir" (Offset));
}

static __inline__ __attribute__((always_inline)) void __incfsdword(const unsigned long Offset)
{
	__asm__("incl %%fs:%a[Offset]" : : [Offset] "ir" (Offset));
}

/* NOTE: the bizarre implementation of __addfsxxx mimics the broken Visual C++ behavior */
static __inline__ __attribute__((always_inline)) void __addfsbyte(const unsigned long Offset, const unsigned char Data)
{
	if(!__builtin_constant_p(Offset))
		__asm__("addb %k[Offset], %%fs:%a[Offset]" : : [Offset] "r" (Offset));
	else
		__asm__("addb %b[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}

static __inline__ __attribute__((always_inline)) void __addfsword(const unsigned long Offset, const unsigned short Data)
{
	if(!__builtin_constant_p(Offset))
		__asm__("addw %k[Offset], %%fs:%a[Offset]" : : [Offset] "r" (Offset));
	else
		__asm__("addw %w[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}

static __inline__ __attribute__((always_inline)) void __addfsdword(const unsigned long Offset, const unsigned int Data)
{
	if(!__builtin_constant_p(Offset))
		__asm__("addl %k[Offset], %%fs:%a[Offset]" : : [Offset] "r" (Offset));
	else
		__asm__("addl %k[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}


/*** Bit manipulation ***/
static  __inline__ __attribute__((always_inline)) unsigned char _BitScanForward(unsigned long * const Index, const unsigned long Mask)
{
	__asm__("bsfl %[Mask], %[Index]" : [Index] "=r" (*Index) : [Mask] "mr" (Mask));
	return Mask ? 1 : 0;
}

static  __inline__ __attribute__((always_inline)) unsigned char _BitScanReverse(unsigned long * const Index, const unsigned long Mask)
{
	__asm__("bsrl %[Mask], %[Index]" : [Index] "=r" (*Index) : [Mask] "mr" (Mask));
	return Mask ? 1 : 0;
}

/* NOTE: again, the bizarre implementation follows Visual C++ */
static  __inline__ __attribute__((always_inline)) unsigned char _bittest(const long * const a, const long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("bt %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*(a + (b / 32))), [b] "Ir" (b % 32));
	else
		__asm__("bt %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*a), [b] "r" (b));

	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _bittestandcomplement(long * const a, const long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("btc %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*(a + (b / 32))), [b] "Ir" (b % 32));
	else
		__asm__("btc %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*a), [b] "r" (b));

	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _bittestandreset(long * const a, const long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("btr %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*(a + (b / 32))), [b] "Ir" (b % 32));
	else
		__asm__("btr %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*a), [b] "r" (b));

	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _bittestandset(long * const a, const long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("bts %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*(a + (b / 32))), [b] "Ir" (b % 32));
	else
		__asm__("bts %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*a), [b] "r" (b));

	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _rotl8(const unsigned char value, const unsigned char shift)
{
	unsigned char retval;
	__asm__("rolb %b[shift], %b[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned short _rotl16(const unsigned short value, const unsigned char shift)
{
	unsigned short retval;
	__asm__("rolw %b[shift], %w[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _rotr8(const unsigned char value, const unsigned char shift)
{
	unsigned char retval;
	__asm__("rorb %b[shift], %b[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned short _rotr16(const unsigned short value, const unsigned char shift)
{
	unsigned short retval;
	__asm__("rorw %b[shift], %w[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}

/*
	NOTE: in __ll_lshift, __ll_rshift and __ull_rshift we use the "A"
	constraint (edx:eax) for the Mask argument, because it's the only way GCC
	can pass 64-bit operands around - passing the two 32 bit parts separately
	just confuses it. Also we declare Bit as an int and then truncate it to
	match Visual C++ behavior
*/
static __inline__ __attribute__((always_inline)) unsigned long long __ll_lshift(const unsigned long long Mask, const int Bit)
{
	unsigned long long retval = Mask;

	__asm__
	(
		"shldl %b[Bit], %%eax, %%edx; sall %b[Bit], %%eax" :
		"+A" (retval) :
		[Bit] "Nc" ((unsigned char)((unsigned long)Bit) & 0xFF)
	);

	return retval;
}

static __inline__ __attribute__((always_inline)) long long __ll_rshift(const long long Mask, const int Bit)
{
	unsigned long long retval = Mask;

	__asm__
	(
		"shldl %b[Bit], %%eax, %%edx; sarl %b[Bit], %%eax" :
		"+A" (retval) :
		[Bit] "Nc" ((unsigned char)((unsigned long)Bit) & 0xFF)
	);

	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned long long __ull_rshift(const unsigned long long Mask, int Bit)
{
	unsigned long long retval = Mask;

	__asm__
	(
		"shrdl %b[Bit], %%eax, %%edx; shrl %b[Bit], %%eax" :
		"+A" (retval) :
		[Bit] "Nc" ((unsigned char)((unsigned long)Bit) & 0xFF)
	);

	return retval;
}


/*** 64-bit math ***/
static __inline__ __attribute__((always_inline)) long long __emul(const int a, const int b)
{
	long long retval;
	__asm__("imull %[b]" : "=A" (retval) : [a] "a" (a), [b] "rm" (b));
	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned long long __emulu(const unsigned int a, const unsigned int b)
{
	unsigned long long retval;
	__asm__("mull %[b]" : "=A" (retval) : [a] "a" (a), [b] "rm" (b));
	return retval;
}


/*** Port I/O ***/
static __inline__ __attribute__((always_inline)) unsigned char __inbyte(const unsigned short Port)
{
	unsigned char byte;
	__asm__ __volatile__("inb %w[Port], %b[byte]" : [byte] "=a" (byte) : [Port] "Nd" (Port));
	return byte;
}

static __inline__ __attribute__((always_inline)) unsigned short __inword(const unsigned short Port)
{
	unsigned short word;
	__asm__ __volatile__("inw %w[Port], %w[word]" : [word] "=a" (word) : [Port] "Nd" (Port));
	return word;
}

static __inline__ __attribute__((always_inline)) unsigned long __indword(const unsigned short Port)
{
	unsigned long dword;
	__asm__ __volatile__("inl %w[Port], %k[dword]" : [dword] "=a" (dword) : [Port] "Nd" (Port));
	return dword;
}

static __inline__ __attribute__((always_inline)) void __inbytestring(unsigned short Port, unsigned char * Buffer, unsigned long Count)
{
	__asm__ __volatile__
	(
		"rep; insb" :
		[Buffer] "=D" (Buffer), [Count] "=c" (Count) :
		"d" (Port), "[Buffer]" (Buffer), "[Count]" (Count) :
		"memory"
	);
}

static __inline__ __attribute__((always_inline)) void __inwordstring(unsigned short Port, unsigned short * Buffer, unsigned long Count)
{
	__asm__ __volatile__
	(
		"rep; insw" :
		[Buffer] "=D" (Buffer), [Count] "=c" (Count) :
		"d" (Port), "[Buffer]" (Buffer), "[Count]" (Count) :
		"memory"
	);
}

static __inline__ __attribute__((always_inline)) void __indwordstring(unsigned short Port, unsigned long * Buffer, unsigned long Count)
{
	__asm__ __volatile__
	(
		"rep; insl" :
		[Buffer] "=D" (Buffer), [Count] "=c" (Count) :
		"d" (Port), "[Buffer]" (Buffer), "[Count]" (Count) :
		"memory"
	);
}

static __inline__ __attribute__((always_inline)) void __outbyte(unsigned short const Port, const unsigned char Data)
{
	__asm__ __volatile__("outb %b[Data], %w[Port]" : : [Port] "Nd" (Port), [Data] "a" (Data));
}

static __inline__ __attribute__((always_inline)) void __outword(unsigned short const Port, const unsigned short Data)
{
	__asm__ __volatile__("outw %w[Data], %w[Port]" : : [Port] "Nd" (Port), [Data] "a" (Data));
}

static __inline__ __attribute__((always_inline)) void __outdword(unsigned short const Port, const unsigned long Data)
{
	__asm__ __volatile__("outl %k[Data], %w[Port]" : : [Port] "Nd" (Port), [Data] "a" (Data));
}

static __inline__ __attribute__((always_inline)) void __outbytestring(unsigned short const Port, const unsigned char * const Buffer, const unsigned long Count)
{
	__asm__ __volatile__("rep; outsb" : : [Port] "d" (Port), [Buffer] "S" (Buffer), "c" (Count));
}

static __inline__ __attribute__((always_inline)) void __outwordstring(unsigned short const Port, const unsigned short * const Buffer, const unsigned long Count)
{
	__asm__ __volatile__("rep; outsw" : : [Port] "d" (Port), [Buffer] "S" (Buffer), "c" (Count));
}

static __inline__ __attribute__((always_inline)) void __outdwordstring(unsigned short const Port, const unsigned long * const Buffer, const unsigned long Count)
{
	__asm__ __volatile__("rep; outsl" : : [Port] "d" (Port), [Buffer] "S" (Buffer), "c" (Count));
}


/*** System information ***/
static __inline__ __attribute__((always_inline)) void __cpuid(int CPUInfo[], const int InfoType)
{
	__asm__ __volatile__("cpuid" : "=a" (CPUInfo[0]), "=b" (CPUInfo[1]), "=c" (CPUInfo[2]), "=d" (CPUInfo[3]) : "a" (InfoType));
}

static __inline__ __attribute__((always_inline)) unsigned long long __rdtsc(void)
{
	unsigned long long retval;
	__asm__ __volatile__("rdtsc" : "=A"(retval));
	return retval;
}


/*** Interrupts ***/
static __inline__ __attribute__((always_inline)) void __debugbreak(void)
{
	__asm__("int $3");
}

static __inline__ __attribute__((always_inline)) void __int2c(void)
{
	__asm__("int $0x2c");
}

static __inline__ __attribute__((always_inline)) void _disable(void)
{
	__asm__("cli");
}

static __inline__ __attribute__((always_inline)) void _enable(void)
{
	__asm__("sti");
}


/*** Protected memory management ***/
static __inline__ __attribute__((always_inline)) unsigned long __readcr0(void)
{
	unsigned long value;
	__asm__ __volatile__("mov %%cr0, %[value]" : [value] "=q" (value));
	return value;
}

static __inline__ __attribute__((always_inline)) unsigned long __readcr2(void)
{
	unsigned long value;
	__asm__ __volatile__("mov %%cr2, %[value]" : [value] "=q" (value));
	return value;
}

static __inline__ __attribute__((always_inline)) unsigned long __readcr3(void)
{
	unsigned long value;
	__asm__ __volatile__("mov %%cr3, %[value]" : [value] "=q" (value));
	return value;
}

static __inline__ __attribute__((always_inline)) unsigned long __readcr4(void)
{
	unsigned long value;
	__asm__ __volatile__("mov %%cr4, %[value]" : [value] "=q" (value));
	return value;
}

static __inline__ __attribute__((always_inline)) void __writecr0(const unsigned long long Data)
{
	__asm__("mov %[Data], %%cr0" : : [Data] "q" ((const unsigned long)(Data & 0xFFFFFFFF)) : "memory");
}

static __inline__ __attribute__((always_inline)) void __writecr3(const unsigned long long Data)
{
	__asm__("mov %[Data], %%cr3" : : [Data] "q" ((const unsigned long)(Data & 0xFFFFFFFF)) : "memory");
}

static __inline__ __attribute__((always_inline)) void __writecr4(const unsigned long long Data)
{
	__asm__("mov %[Data], %%cr4" : : [Data] "q" ((const unsigned long)(Data & 0xFFFFFFFF)) : "memory");
}

static __inline__ __attribute__((always_inline)) void __invlpg(void * const Address)
{
	__asm__("invlpg %[Address]" : : [Address] "m" (*((unsigned char *)(Address))));
}


/*** System operations ***/
static __inline__ __attribute__((always_inline)) unsigned long long __readmsr(const int reg)
{
	unsigned long long retval;
	__asm__ __volatile__("rdmsr" : "=A" (retval) : "c" (reg));
	return retval;
}

static __inline__ __attribute__((always_inline)) void __writemsr(const unsigned long Register, const unsigned long long Value)
{
	__asm__ __volatile__("wrmsr" : : "A" (Value), "c" (Register));
}

static __inline__ __attribute__((always_inline)) unsigned long long __readpmc(const int counter)
{
	unsigned long long retval;
	__asm__ __volatile__("rdpmc" : "=A" (retval) : "c" (counter));
	return retval;
}

/* NOTE: an immediate value for 'a' will raise an ICE in Visual C++ */
static __inline__ __attribute__((always_inline)) unsigned long __segmentlimit(const unsigned long a)
{
	unsigned long retval;
	__asm__ __volatile__("lsl %[a], %[retval]" : [retval] "=r" (retval) : [a] "rm" (a));
	return retval;
}

static __inline__ __attribute__((always_inline)) void __wbinvd(void)
{
	__asm__ __volatile__("wbinvd");
}

#endif /* KJK_INTRIN_X86_H_ */

/* EOF */
