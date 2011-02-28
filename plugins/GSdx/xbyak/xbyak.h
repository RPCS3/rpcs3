#ifndef XBYAK_XBYAK_H_
#define XBYAK_XBYAK_H_
/*!
	@file xbyak.h
	@brief Xbyak ; JIT assembler for x86(IA32)/x64 by C++
	@author herumi
	@version $Revision: 1.239 $
	@url http://homepage1.nifty.com/herumi/soft/xbyak.html
	@date $Date: 2011/02/07 06:09:35 $
	@note modified new BSD license
	http://www.opensource.org/licenses/bsd-license.php
*/

#include <stdio.h> // for debug print
#include <assert.h>
#include <map>
#include <string>
#include <algorithm>
#ifdef _WIN32
	#include <windows.h>
#elif defined(__GNUC__)
	#include <unistd.h>
	#include <sys/mman.h>
#endif

#ifdef __x86_64__
		#define XBYAK64_GCC
#elif defined(_WIN64)
		#define XBYAK64_WIN
#endif
#if !defined(XBYAK64) && !defined(XBYAK32)
	#if defined(XBYAK64_GCC) || defined(XBYAK64_WIN)
		#define XBYAK64
	#else
		#define XBYAK32
	#endif
#endif

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4514) /* remove inline function */
	#pragma warning(disable : 4786) /* identifier is too long */
	#pragma warning(disable : 4503) /* name is too long */
	#pragma warning(disable : 4127) /* constant expresison */
	#if (_MSC_VER <= 1200)
		#ifndef for
			#define for if(0);else for
			#pragma warning(disable : 4127) /* condition is constant(for "if" trick) */
		#endif
	#endif
#endif

namespace Xbyak {

#include "xbyak_bin2hex.h"

enum {
	DEFAULT_MAX_CODE_SIZE = 4096,
	VERSION = 0x2991, /* 0xABCD = A.BC(D) */
};
/*
#ifndef MIE_INTEGER_TYPE_DEFINED
#define MIE_INTEGER_TYPE_DEFINED
#ifdef _MSC_VER
	typedef unsigned __int64 uint64;
	typedef __int64 sint64;
#else
	typedef unsigned long long uint64;
	typedef long long sint64;
#endif
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
#endif
*/
#ifndef MIE_ALIGN
	#ifdef _MSC_VER
		#define MIE_ALIGN(x) __declspec(align(x))
	#else
		#define MIE_ALIGN(x) __attribute__((aligned(x)))
	#endif
#endif
#ifndef MIE_PACK // for shufps
	#define MIE_PACK(x, y, z, w) ((x) * 64 + (y) * 16 + (z) * 4 + (w))
#endif

enum Error {
	ERR_NONE = 0,
	ERR_BAD_ADDRESSING,
	ERR_CODE_IS_TOO_BIG,
	ERR_BAD_SCALE,
	ERR_ESP_CANT_BE_INDEX,
	ERR_BAD_COMBINATION,
	ERR_BAD_SIZE_OF_REGISTER,
	ERR_IMM_IS_TOO_BIG,
	ERR_BAD_ALIGN,
	ERR_LABEL_IS_REDEFINED,
	ERR_LABEL_IS_TOO_FAR,
	ERR_LABEL_IS_NOT_FOUND,
	ERR_CODE_ISNOT_COPYABLE,
	ERR_BAD_PARAMETER,
	ERR_CANT_PROTECT,
	ERR_CANT_USE_64BIT_DISP,
	ERR_OFFSET_IS_TOO_BIG,
	ERR_MEM_SIZE_IS_NOT_SPECIFIED,
	ERR_BAD_MEM_SIZE,
	ERR_BAD_ST_COMBINATION,
	ERR_OVER_LOCAL_LABEL,
	ERR_UNDER_LOCAL_LABEL,
	ERR_INTERNAL
};

static inline const char *ConvertErrorToString(Error err)
{
	static const char errTbl[][40] = {
		"none",
		"bad addressing",
		"code is too big",
		"bad scale",
		"esp can't be index",
		"bad combination",
		"bad size of register",
		"imm is too big",
		"bad align",
		"label is redefined",
		"label is too far",
		"label is not found",
		"code is not copyable",
		"bad parameter",
		"can't protect",
		"can't use 64bit disp(use (void*))",
		"offset is too big",
		"MEM size is not specified",
		"bad mem size",
		"bad st combination",
		"over local label",
		"under local label",
		"internal error",
	};
	if (err < 0 || err > ERR_INTERNAL) return 0;
	return errTbl[err];
}

namespace inner {

enum { debug = 1 };

static inline uint32 GetPtrDist(const void *p1, const void *p2)
{
	uint64 diff = static_cast<const char *>(p1) - static_cast<const char *>(p2);
#ifdef XBYAK64
	if (0x7FFFFFFFULL < diff && diff < 0xFFFFFFFF80000000ULL) throw ERR_OFFSET_IS_TOO_BIG;
#endif
	return static_cast<uint32>(diff);
}

static inline bool IsInDisp8(uint32 x) { return 0xFFFFFF80 <= x || x <= 0x7F; }
static inline bool IsInInt32(uint64 x) { return 0xFFFFFFFF80000000ULL <= x || x <= 0x7FFFFFFFU; }

}

class Operand {
private:
	const uint8 idx_;
	const uint8 kind_;
	const uint8 bit_;
	const uint8 ext8bit_; // 1 if spl/bpl/sil/dil, otherwise 0
	void operator=(Operand&);
public:
	enum Kind {
		NONE = 0,
		MEM = 1 << 1,
		IMM = 1 << 2,
		REG = 1 << 3,
		MMX = 1 << 4,
		XMM = 1 << 5,
		FPU = 1 << 6,
		YMM = 1 << 7
	};
	enum Code {
#ifdef XBYAK64
		RAX = 0, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15,
		R8D = 8, R9D, R10D, R11D, R12D, R13D, R14D, R15D,
		R8W = 8, R9W, R10W, R11W, R12W, R13W, R14W, R15W,
		R8B = 8, R9B, R10B, R11B, R12B, R13B, R14B, R15B,
		SPL = 4, BPL, SIL, DIL,
#endif
		EAX = 0, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
		AX = 0, CX, DX, BX, SP, BP, SI, DI,
		AL = 0, CL, DL, BL, AH, CH, DH, BH
	};
	Operand() : idx_(0), kind_(0), bit_(0), ext8bit_(0) { }
	Operand(int idx, Kind kind, int bit, int ext8bit = 0)
		: idx_(static_cast<uint8>(idx))
		, kind_(static_cast<uint8>(kind))
		, bit_(static_cast<uint8>(bit))
		, ext8bit_(static_cast<uint8>(ext8bit))
	{
		assert((bit_ & (bit_ - 1)) == 0); // bit must be power of two
	}
	Kind getKind() const { return static_cast<Kind>(kind_); }
	int getIdx() const { return idx_; }
	bool isNone() const { return kind_ == 0; }
	bool isMMX() const { return is(MMX); }
	bool isXMM() const { return is(XMM); }
	bool isYMM() const { return is(YMM); }
	bool isREG(int bit = 0) const { return is(REG, bit); }
	bool isMEM(int bit = 0) const { return is(MEM, bit); }
	bool isFPU() const { return is(FPU); }
	bool isExt8bit() const { return ext8bit_ != 0; }
	// any bit is accetable if bit == 0
	bool is(int kind, uint32 bit = 0) const
	{
		return (kind_ & kind) && (bit == 0 || (bit_ & bit)); // cf. you can set (8|16)
	}
	bool isBit(uint32 bit) const { return (bit_ & bit) != 0; }
	uint32 getBit() const { return bit_; }
	const char *toString() const
	{
		if (kind_ == REG) {
			if (ext8bit_) {
				static const char tbl[4][4] = { "spl", "bpl", "sil", "dil" };
				return tbl[idx_ - 4];
			}
			static const char tbl[4][16][5] = {
				{ "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh", "r8b", "r9b", "r10b",  "r11b", "r12b", "r13b", "r14b", "r15b" },
				{ "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w",  "r11w", "r12w", "r13w", "r14w", "r15w" },
				{ "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d",  "r11d", "r12d", "r13d", "r14d", "r15d" },
				{ "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10",  "r11", "r12", "r13", "r14", "r15" },
			};
			return tbl[bit_ == 8 ? 0 : bit_ == 16 ? 1 : bit_ == 32 ? 2 : 3][idx_];
		} else if (isYMM()) {
			static const char tbl[16][5] = { "ym0", "ym1", "ym2", "ym3", "ym4", "ym5", "ym6", "ym7", "ym8", "ym9", "ym10", "ym11", "ym12", "ym13", "ym14", "ym15" };
			return tbl[idx_];
		} else if (isXMM()) {
			static const char tbl[16][5] = { "xm0", "xm1", "xm2", "xm3", "xm4", "xm5", "xm6", "xm7", "xm8", "xm9", "xm10", "xm11", "xm12", "xm13", "xm14", "xm15" };
			return tbl[idx_];
		} else if (isMMX()) {
			static const char tbl[8][4] = { "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7" };
			return tbl[idx_];
		} else if (isFPU()) {
			static const char tbl[8][4] = { "st0", "st1", "st2", "st3", "st4", "st5", "st6", "st7" };
			return tbl[idx_];
		}
		throw ERR_INTERNAL;
	}
};

class Reg : public Operand {
	void operator=(const Reg&);
	bool hasRex() const { return isExt8bit() | isREG(64) | isExtIdx(); }
public:
	Reg() { }
	Reg(int idx, Kind kind, int bit = 0, int ext8bit = 0) : Operand(idx, kind, bit, ext8bit) { }
	Reg changeBit(int bit) const { return Reg(getIdx(), getKind(), bit, isExt8bit()); }
	bool isExtIdx() const { return getIdx() > 7; }
	uint8 getRex(const Reg& base = Reg()) const
	{
		return (hasRex() || base.hasRex()) ? uint8(0x40 | ((isREG(64) | base.isREG(64)) ? 8 : 0) | (isExtIdx() ? 4 : 0)| (base.isExtIdx() ? 1 : 0)) : 0;
	}
};

class Reg8 : public Reg {
	void operator=(const Reg8&);
public:
	explicit Reg8(int idx, int ext8bit = 0) : Reg(idx, Operand::REG, 8, ext8bit) { }
};

class Reg16 : public Reg {
	void operator=(const Reg16&);
public:
	explicit Reg16(int idx) : Reg(idx, Operand::REG, 16) { }
};

class Mmx : public Reg {
	void operator=(const Mmx&);
public:
	explicit Mmx(int idx, Kind kind = Operand::MMX, int bit = 64) : Reg(idx, kind, bit) { }
};

class Xmm : public Mmx {
	void operator=(const Xmm&);
public:
	explicit Xmm(int idx, Kind kind = Operand::XMM, int bit = 128) : Mmx(idx, kind, bit) { }
};

class Ymm : public Xmm {
	void operator=(const Ymm&);
public:
	explicit Ymm(int idx) : Xmm(idx, Operand::YMM, 256) { }
};

class Fpu : public Reg {
	void operator=(const Fpu&);
public:
	explicit Fpu(int idx) : Reg(idx, Operand::FPU, 32) { }
};

// register for addressing(32bit or 64bit)
class Reg32e : public Reg {
public:
	// [base_(this) + index_ * scale_ + disp_]
	const Reg index_;
	const int scale_; // 0(index is none), 1, 2, 4, 8
	const uint32 disp_;
private:
	friend class Address;
	friend Reg32e operator+(const Reg32e& a, const Reg32e& b)
	{
		if (a.scale_ == 0) {
			if (b.scale_ == 0) { // base + base
				if (b.getIdx() == Operand::ESP) { // [reg + esp] => [esp + reg]
					return Reg32e(b, a, 1, a.disp_ + b.disp_);
				} else {
					return Reg32e(a, b, 1, a.disp_ + b.disp_);
				}
			} else if (b.isNone()) { // base + index
				return Reg32e(a, b.index_, b.scale_, a.disp_ + b.disp_);
			}
		}
		throw ERR_BAD_ADDRESSING;
	}
	friend Reg32e operator*(const Reg32e& r, int scale)
	{
		if (r.scale_ == 0) {
			if (scale == 1) {
				return r;
			} else if (scale == 2 || scale == 4 || scale == 8) {
				return Reg32e(Reg(), r, scale, r.disp_);
			}
		}
		throw ERR_BAD_SCALE;
	}
	friend Reg32e operator+(const Reg32e& r, unsigned int disp)
	{
		return Reg32e(r, r.index_, r.scale_, r.disp_ + disp);
	}
	friend Reg32e operator-(const Reg32e& r, unsigned int disp)
	{
		return operator+(r, -static_cast<int>(disp));
	}
	void operator=(const Reg32e&);
public:
	explicit Reg32e(int idx, int bit)
		: Reg(idx, REG, bit)
		, index_()
		, scale_(0)
		, disp_(0)
	{
	}
	Reg32e(const Reg& base, const Reg& index, int scale, unsigned int disp)
		: Reg(base)
		, index_(index)
		, scale_(scale)
		, disp_(disp)
	{
		if (scale != 0 && scale != 1 && scale != 2 && scale != 4 && scale != 8) throw ERR_BAD_SCALE;
		if (!base.isNone() && !index.isNone() && base.getBit() != index.getBit()) throw ERR_BAD_COMBINATION;
		if (index.getIdx() == Operand::ESP) throw ERR_ESP_CANT_BE_INDEX;
	}
	Reg32e optimize() const // select smaller size
	{
		// [reg * 2] => [reg + reg]
		if (isNone() && !index_.isNone() && scale_ == 2) {
			const Reg index(index_.getIdx(), Operand::REG, index_.getBit());
			return Reg32e(index, index, 1, disp_);
		}
		return *this;
	}
};

struct Reg32 : public Reg32e {
	explicit Reg32(int idx) : Reg32e(idx, 32) {}
private:
	void operator=(const Reg32&);
};
#ifdef XBYAK64
struct Reg64 : public Reg32e {
	explicit Reg64(int idx) : Reg32e(idx, 64) {}
private:
	void operator=(const Reg64&);
};
struct RegRip {
	uint32 disp_;
	RegRip(unsigned int disp = 0) : disp_(disp) {}
	friend const RegRip operator+(const RegRip& r, unsigned int disp) {
		return RegRip(r.disp_ + disp);
	}
	friend const RegRip operator-(const RegRip& r, unsigned int disp) {
		return RegRip(r.disp_ - disp);
	}
};
#endif

class CodeArray {
	enum {
		ALIGN_PAGE_SIZE = 4096,
		MAX_FIXED_BUF_SIZE = 8
	};
	enum Type {
		FIXED_BUF, // use buf_(non alignment, non protect)
		USER_BUF, // use userPtr(non alignment, non protect)
		ALLOC_BUF // use new(alignment, protect)
	};
	void operator=(const CodeArray&);
	Type type_;
	uint8 *const allocPtr_; // for ALLOC_BUF
	uint8 buf_[MAX_FIXED_BUF_SIZE]; // for FIXED_BUF
protected:
	const size_t maxSize_;
	uint8 *const top_;
	size_t size_;
public:
	CodeArray(size_t maxSize = MAX_FIXED_BUF_SIZE, void *userPtr = 0)
		: type_(userPtr ? USER_BUF : maxSize <= MAX_FIXED_BUF_SIZE ? FIXED_BUF : ALLOC_BUF)
		, allocPtr_(type_ == ALLOC_BUF ? new uint8[maxSize + ALIGN_PAGE_SIZE] : 0)
		, maxSize_(maxSize)
		, top_(type_ == ALLOC_BUF ? getAlignedAddress(allocPtr_, ALIGN_PAGE_SIZE) : type_ == USER_BUF ? reinterpret_cast<uint8*>(userPtr) : buf_)
		, size_(0)
	{
		if (type_ == ALLOC_BUF && !protect(top_, maxSize, true)) {
			throw ERR_CANT_PROTECT;
		}
	}
	virtual ~CodeArray()
	{
		if (type_ == ALLOC_BUF) {
			protect(top_, maxSize_, false);
			delete[] allocPtr_;
		}
	}
	CodeArray(const CodeArray& rhs)
		: type_(rhs.type_)
		, allocPtr_(0)
		, maxSize_(rhs.maxSize_)
		, top_(buf_)
		, size_(rhs.size_)
	{
		if (type_ != FIXED_BUF) throw ERR_CODE_ISNOT_COPYABLE;
		for (size_t i = 0; i < size_; i++) top_[i] = rhs.top_[i];
	}
	void db(int code)
	{
		if (size_ >= maxSize_) throw ERR_CODE_IS_TOO_BIG;
		top_[size_++] = static_cast<uint8>(code);
	}
	void db(const uint8 *code, int codeSize)
	{
		for (int i = 0; i < codeSize; i++) db(code[i]);
	}
	void db(uint64 code, int codeSize)
	{
		if (codeSize > 8) throw ERR_BAD_PARAMETER;
		for (int i = 0; i < codeSize; i++) db(static_cast<uint8>(code >> (i * 8)));
	}
	void dw(uint32 code) { db(code, 2); }
	void dd(uint32 code) { db(code, 4); }
	const uint8 *getCode() const { return top_; }
	const uint8 *getCurr() const { return &top_[size_]; }
	size_t getSize() const { return size_; }
	void dump() const
	{
		const uint8 *p = getCode();
		size_t bufSize = getSize();
		size_t remain = bufSize;
		for (int i = 0; i < 4; i++) {
			size_t disp = 16;
			if (remain < 16) {
				disp = remain;
			}
			for (size_t j = 0; j < 16; j++) {
				if (j < disp) {
					printf("%02X", p[i * 16 + j]);
				}
			}
			putchar('\n');
			remain -= disp;
			if (remain <= 0) {
				break;
			}
		}
	}
	/*
		@param data [in] address of jmp data
		@param disp [in] offset from the next of jmp
		@param size [in] write size(1, 2, 4, 8)
	*/
	void rewrite(uint8 *data, uint64 disp, size_t size)
	{
		if (size != 1 && size != 2 && size != 4 && size != 8) throw ERR_BAD_PARAMETER;
		for (size_t i = 0; i < size; i++) {
			data[i] = static_cast<uint8>(disp >> (i * 8));
		}
	}
	void updateRegField(uint8 regIdx) const
	{
		*top_ = (*top_ & B11000111) | ((regIdx << 3) & B00111000);
	}
	/**
		change exec permission of memory
		@param addr [in] buffer address
		@param size [in] buffer size
		@param canExec [in] true(enable to exec), false(disable to exec)
		@return true(success), false(failure)
	*/
	static inline bool protect(const void *addr, size_t size, bool canExec)
	{
#if defined(_WIN32)
		DWORD oldProtect;
		return VirtualProtect(const_cast<void*>(addr), size, canExec ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE, &oldProtect) != 0;
#elif defined(__GNUC__)
		size_t pageSize = sysconf(_SC_PAGESIZE);
		size_t iaddr = reinterpret_cast<size_t>(addr);
		size_t roundAddr = iaddr & ~(pageSize - static_cast<size_t>(1));
		int mode = PROT_READ | PROT_WRITE | (canExec ? PROT_EXEC : 0);
		return mprotect(reinterpret_cast<void*>(roundAddr), size + (iaddr - roundAddr), mode) == 0;
#else
		return true;
#endif
	}
	/**
		get aligned memory pointer
		@param addr [in] address
		@param alingedSize [in] power of two
		@return aligned addr by alingedSize
	*/
	static inline uint8 *getAlignedAddress(uint8 *addr, size_t alignedSize = 16)
	{
		return reinterpret_cast<uint8*>((reinterpret_cast<size_t>(addr) + alignedSize - 1) & ~(alignedSize - static_cast<size_t>(1)));
	}
};

class Address : public Operand, public CodeArray {
	void operator=(const Address&);
	uint64 disp_;
	bool isOnlyDisp_;
	bool is64bitDisp_;
	uint8 rex_;
public:
	const bool is32bit_;
	Address(uint32 sizeBit, bool isOnlyDisp, uint64 disp, bool is32bit, bool is64bitDisp = false)
		: Operand(0, MEM, sizeBit)
		, CodeArray(6) // 6 = 1(ModRM) + 1(SIB) + 4(disp)
		, disp_(disp)
		, isOnlyDisp_(isOnlyDisp)
		, is64bitDisp_(is64bitDisp)
		, rex_(0)
		, is32bit_(is32bit)
	{
	}
	bool isOnlyDisp() const { return isOnlyDisp_; } // for mov eax
	uint64 getDisp() const { return disp_; }
	uint8 getRex() const { return rex_; }
	bool is64bitDisp() const { return is64bitDisp_; } // for moffset
	void setRex(uint8 rex) { rex_ = rex; }
};

class AddressFrame {
private:
	void operator=(const AddressFrame&);
public:
	const uint32 bit_;
	explicit AddressFrame(uint32 bit) : bit_(bit) { }
	Address operator[](const void *disp) const
	{
		size_t adr = reinterpret_cast<size_t>(disp);
#ifdef XBYAK64
		if (adr > 0xFFFFFFFFU) throw ERR_OFFSET_IS_TOO_BIG;
#endif
		Reg32e r(Reg(), Reg(), 0, static_cast<uint32>(adr));
		return operator[](r);
	}
#ifdef XBYAK64
	Address operator[](uint64 disp) const
	{
		return Address(64, true, disp, false, true);
	}
	Address operator[](const RegRip& addr) const
	{
		Address frame(64, true, addr.disp_, false);
		frame.db(B00000101);
		frame.dd(addr.disp_);
		return frame;
	}
#endif
	Address operator[](const Reg32e& in) const
	{
		const Reg32e& r = in.optimize();
		Address frame(bit_, (r.isNone() && r.index_.isNone()), r.disp_, r.isBit(32) || r.index_.isBit(32));
		enum {
			mod00 = 0, mod01 = 1, mod10 = 2
		};
		int mod;
		if (r.isNone() || ((r.getIdx() & 7) != Operand::EBP && r.disp_ == 0)) {
			mod = mod00;
		} else if (inner::IsInDisp8(r.disp_)) {
			mod = mod01;
		} else {
			mod = mod10;
		}
		const int base = r.isNone() ? Operand::EBP : (r.getIdx() & 7);
		/* ModR/M = [2:3:3] = [Mod:reg/code:R/M] */
		bool hasSIB = !r.index_.isNone() || (r.getIdx() & 7) == Operand::ESP;
#ifdef XBYAK64
		if (r.isNone() && r.index_.isNone()) hasSIB = true;
#endif
		if (!hasSIB) {
			frame.db((mod << 6) | base);
		} else {
			frame.db((mod << 6) | Operand::ESP);
			/* SIB = [2:3:3] = [SS:index:base(=rm)] */
			int index = r.index_.isNone() ? Operand::ESP : (r.index_.getIdx() & 7);
			int ss = (r.scale_ == 8) ? 3 : (r.scale_ == 4) ? 2 : (r.scale_ == 2) ? 1 : 0;
			frame.db((ss << 6) | (index << 3) | base);
		}
		if (mod == mod01) {
			frame.db(r.disp_);
		} else if (mod == mod10 || (mod == mod00 && r.isNone())) {
			frame.dd(r.disp_);
		}
		uint8 rex = ((r.getIdx() | r.index_.getIdx()) < 8) ? 0 : uint8(0x40 | ((r.index_.getIdx() >> 3) << 1) | (r.getIdx() >> 3));
		frame.setRex(rex);
		return frame;
	}
};

struct JmpLabel {
	uint8 *endOfJmp; /* end address of jmp */
	bool isShort;
};

class Label {
	CodeArray *base_;
	int anonymousCount_; // for @@, @f, @b
	enum {
		maxStack = 10
	};
	int stack_[maxStack];
	int stackPos_;
	int usedCount_;
	int localCount_; // for .***
	typedef std::map<const std::string, const uint8*> DefinedList;
	typedef std::multimap<const std::string, const JmpLabel> UndefinedList;
	DefinedList definedList_;
	UndefinedList undefinedList_;

	/*
		@@ --> @@.<num>
		@b --> @@.<num>
		@f --> @@.<num + 1>
		.*** -> .***.<num>
	*/
	std::string convertLabel(const char *label) const
	{
		std::string newLabel(label);
		if (newLabel == "@f" || newLabel == "@F") {
			newLabel = std::string("@@") + toStr(anonymousCount_ + 1);
		} else if (newLabel == "@b" || newLabel == "@B") {
			newLabel = std::string("@@") + toStr(anonymousCount_);
		} else if (*label == '.') {
			newLabel += toStr(localCount_);
		}
		return newLabel;
	}
public:
	Label()
		: base_(0)
		, anonymousCount_(0)
		, stackPos_(1)
		, usedCount_(0)
		, localCount_(0)
	{
	}
	void enterLocal()
	{
		if (stackPos_ == maxStack) throw ERR_OVER_LOCAL_LABEL;
		localCount_ = stack_[stackPos_++] = ++usedCount_;
	}
	void leaveLocal()
	{
		if (stackPos_ == 1) throw ERR_UNDER_LOCAL_LABEL;
		localCount_ = stack_[--stackPos_ - 1];
	}
	void set(CodeArray *base) { base_ = base; }
	void define(const char *label, const uint8 *address)
	{
		std::string newLabel(label);
		if (newLabel == "@@") {
			newLabel += toStr(++anonymousCount_);
		} else if (*label == '.') {
			newLabel += toStr(localCount_);
		}
		label = newLabel.c_str();
		// add label
		DefinedList::value_type item(label, address);
		std::pair<DefinedList::iterator, bool> ret = definedList_.insert(item);
		if (!ret.second) throw ERR_LABEL_IS_REDEFINED;
		// search undefined label
		for (;;) {
			UndefinedList::iterator itr = undefinedList_.find(label);
			if (itr == undefinedList_.end()) break;
			const JmpLabel *jmp = &itr->second;
			uint32 disp = inner::GetPtrDist(address, jmp->endOfJmp);
			if (jmp->isShort && !inner::IsInDisp8(disp)) throw ERR_LABEL_IS_TOO_FAR;
			size_t jmpSize = jmp->isShort ? 1 : 4;
			uint8 *data = jmp->endOfJmp - jmpSize;
			base_->rewrite(data, disp, jmpSize);
			undefinedList_.erase(itr);
		}
	}
	const uint8 *getAddress(const char *label) const
	{
		std::string newLabel = convertLabel(label);
		DefinedList::const_iterator itr = definedList_.find(newLabel);
		if (itr != definedList_.end()) {
			return itr->second;
		} else {
			return 0;
		}
	}
	void addUndefinedLabel(const char *label, const JmpLabel& jmp)
	{
		std::string newLabel = convertLabel(label);
		undefinedList_.insert(UndefinedList::value_type(newLabel, jmp));
	}
	bool hasUndefinedLabel() const
	{
		if (inner::debug) {
			for (UndefinedList::const_iterator i = undefinedList_.begin(); i != undefinedList_.end(); ++i) {
				fprintf(stderr, "undefined label:%s\n", i->first.c_str());
			}
		}
		return !undefinedList_.empty();
	}
	static inline std::string toStr(int num)
	{
		char buf[16];
#ifdef _WIN32
		#if _MSC_VER < 1400
			_snprintf
		#else
			_snprintf_s
		#endif
#else
		snprintf
#endif
		(buf, sizeof(buf), ".%08x", num);
		return buf;
	}
};

class CodeGenerator : public CodeArray {
public:
	enum LabelType {
		T_SHORT,
		T_NEAR,
		T_AUTO // T_SHORT if possible
	};
private:
	CodeGenerator operator=(const CodeGenerator&); // don't call
#ifdef XBYAK64
	enum { i32e = 32 | 64, BIT = 64 };
#else
	enum { i32e = 32, BIT = 32 };
#endif
	// (XMM, XMM|MEM)
	static inline bool isXMM_XMMorMEM(const Operand& op1, const Operand& op2)
	{
		return op1.isXMM() && (op2.isXMM() || op2.isMEM());
	}
	// (MMX, MMX|MEM) or (XMM, XMM|MEM)
	static inline bool isXMMorMMX_MEM(const Operand& op1, const Operand& op2)
	{
		return (op1.isMMX() && (op2.isMMX() || op2.isMEM())) || isXMM_XMMorMEM(op1, op2);
	}
	// (XMM, MMX|MEM)
	static inline bool isXMM_MMXorMEM(const Operand& op1, const Operand& op2)
	{
		return op1.isXMM() && (op2.isMMX() || op2.isMEM());
	}
	// (MMX, XMM|MEM)
	static inline bool isMMX_XMMorMEM(const Operand& op1, const Operand& op2)
	{
		return op1.isMMX() && (op2.isXMM() || op2.isMEM());
	}
	// (XMM, REG32|MEM)
	static inline bool isXMM_REG32orMEM(const Operand& op1, const Operand& op2)
	{
		return op1.isXMM() && (op2.isREG(i32e) || op2.isMEM());
	}
	// (REG32, XMM|MEM)
	static inline bool isREG32_XMMorMEM(const Operand& op1, const Operand& op2)
	{
		return op1.isREG(i32e) && (op2.isXMM() || op2.isMEM());
	}
	void rex(const Operand& op1, const Operand& op2 = Operand())
	{
		uint8 rex = 0;
		const Operand *p1 = &op1, *p2 = &op2;
		if (p1->isMEM()) std::swap(p1, p2);
		if (p1->isMEM()) throw ERR_BAD_COMBINATION;
		if (p2->isMEM()) {
			const Address& addr = static_cast<const Address&>(*p2);
			if (BIT == 64 && addr.is32bit_) db(0x67);
			rex = addr.getRex() | static_cast<const Reg&>(*p1).getRex();
		} else {
			// ModRM(reg, base);
			rex = static_cast<const Reg&>(op2).getRex(static_cast<const Reg&>(op1));
		}
		// except movsx(16bit, 32/64bit)
		if ((op1.isBit(16) && !op2.isBit(i32e)) || (op2.isBit(16) && !op1.isBit(i32e))) db(0x66);
		if (rex) db(rex);
	}
	enum AVXtype {
		PP_NONE = 1 << 0,
		PP_66 = 1 << 1,
		PP_F3 = 1 << 2,
		PP_F2 = 1 << 3,
		MM_RESERVED = 1 << 4,
		MM_0F = 1 << 5,
		MM_0F38 = 1 << 6,
		MM_0F3A = 1 << 7
	};
	void vex(bool r, int idx, bool is256, int type, bool x = false, bool b = false, int w = 1)
	{
		uint32 pp = (type & PP_66) ? 1 : (type & PP_F3) ? 2 : (type & PP_F2) ? 3 : 0;
		uint32 vvvv = (((~idx) & 15) << 3) | (is256 ? 4 : 0) | pp;
		if (!b && !x && !w && (type & MM_0F)) {
			db(0xC5); db((r ? 0 : 0x80) | vvvv);
		} else {
			uint32 mmmm = (type & MM_0F) ? 1 : (type & MM_0F38) ? 2 : (type & MM_0F3A) ? 3 : 0;
			db(0xC4); db((r ? 0 : 0x80) | (x ? 0 : 0x40) | (b ? 0 : 0x20) | mmmm); db((w << 7) | vvvv);
		}
	}
	Label label_;
	bool isInDisp16(uint32 x) const { return 0xFFFF8000 <= x || x <= 0x7FFF; }
	uint8 getModRM(int mod, int r1, int r2) const { return static_cast<uint8>((mod << 6) | ((r1 & 7) << 3) | (r2 & 7)); }
	void opModR(const Reg& reg1, const Reg& reg2, int code0, int code1 = NONE, int code2 = NONE)
	{
		rex(reg2, reg1);
		db(code0 | (reg1.isBit(8) ? 0 : 1)); if (code1 != NONE) db(code1); if (code2 != NONE) db(code2);
		db(getModRM(3, reg1.getIdx(), reg2.getIdx()));
	}
	void opModM(const Address& addr, const Reg& reg, int code0, int code1 = NONE, int code2 = NONE)
	{
		if (addr.is64bitDisp()) throw ERR_CANT_USE_64BIT_DISP;
		rex(addr, reg);
		db(code0 | (reg.isBit(8) ? 0 : 1)); if (code1 != NONE) db(code1); if (code2 != NONE) db(code2);
		addr.updateRegField(static_cast<uint8>(reg.getIdx()));
		db(addr.getCode(), static_cast<int>(addr.getSize()));
	}
	void opJmp(const char *label, LabelType type, uint8 shortCode, uint8 longCode, uint8 longPref)
	{
		const uint8 *address = label_.getAddress(label);
		if (address) { /* label exists */
			opJmp(address, type, shortCode, longCode, longPref);
		} else {
			const int shortHeaderSize = 1;
			const int shortJmpSize = shortHeaderSize + 1; /* +1 means 8-bit displacement */
			const int longHeaderSize = longPref ? 2 : 1;
			const int longJmpSize = longHeaderSize + 4; /* +4 means 32-bit displacement */
			uint8 *top = const_cast<uint8*>(getCurr());
			bool isShort = (type != T_NEAR);
			JmpLabel jmp;
			jmp.endOfJmp = top + (isShort ? shortJmpSize : longJmpSize);
			jmp.isShort = isShort;
			if (isShort) {
				db(shortCode);
				db(0);
			} else {
				if (longPref) db(longPref);
				db(longCode);
				dd(0);
			}
			label_.addUndefinedLabel(label, jmp);
		}
	}
	void opJmp(const void *addr, LabelType type, uint8 shortCode, uint8 longCode, uint8 longPref)
	{
		const int shortHeaderSize = 1;
		const int shortJmpSize = shortHeaderSize + 1; /* +1 means 8-bit displacement */
		const int longHeaderSize = longPref ? 2 : 1;
		const int longJmpSize = longHeaderSize + 4; /* +4 means 32-bit displacement */

		uint8 *top = const_cast<uint8*>(getCurr());
		uint32 disp = inner::GetPtrDist(addr, top);
		if (type != T_NEAR && inner::IsInDisp8(disp - shortJmpSize)) {
			db(shortCode);
			db(0);
			rewrite(top + shortHeaderSize, disp - shortJmpSize, 1);
		} else {
			if (type == T_SHORT) throw ERR_LABEL_IS_TOO_FAR;
			if (longPref) db(longPref);
			db(longCode);
			dd(0);
			rewrite(top + longHeaderSize, disp - longJmpSize, 4);
		}
	}
	/* preCode is for SSSE3/SSE4 */
	void opGen(const Operand& reg, const Operand& op, int code, int pref, bool isValid(const Operand&, const Operand&), int imm8 = NONE, int preCode = NONE)
	{
		if (isValid && !isValid(reg, op)) throw ERR_BAD_COMBINATION;
		if (pref != NONE) db(pref);
		if (op.isMEM()) {
			opModM(static_cast<const Address&>(op), static_cast<const Reg&>(reg), 0x0F, preCode, code);
		} else {
			opModR(static_cast<const Reg&>(reg), static_cast<const Reg&>(op), 0x0F, preCode, code);
		}
		if (imm8 != NONE) db(imm8);
	}
	void opMMX_IMM(const Mmx& mmx, int imm8, int code, int ext)
	{
		if (mmx.isXMM()) db(0x66);
		opModR(Reg32(ext), mmx, 0x0F, code);
		db(imm8);
	}
	void opMMX(const Mmx& mmx, const Operand& op, int code, int pref = 0x66, int imm8 = NONE, int preCode = NONE)
	{
		opGen(mmx, op, code, mmx.isXMM() ? pref : NONE, isXMMorMMX_MEM, imm8, preCode);
	}
	void opMovXMM(const Operand& op1, const Operand& op2, int code, int pref)
	{
		if (pref != NONE) db(pref);
		if (op1.isXMM() && op2.isMEM()) {
			opModM(static_cast<const Address&>(op2), static_cast<const Reg&>(op1), 0x0F, code);
		} else if (op1.isMEM() && op2.isXMM()) {
			opModM(static_cast<const Address&>(op1), static_cast<const Reg&>(op2), 0x0F, code | 1);
		} else {
			throw ERR_BAD_COMBINATION;
		}
	}
	void opExt(const Operand& op, const Mmx& mmx, int code, int imm, bool hasMMX2 = false)
	{
		if (hasMMX2 && op.isREG(i32e)) { /* pextrw is special */
			if (mmx.isXMM()) db(0x66);
			opModR(static_cast<const Reg&>(op), mmx, 0x0F, B11000101); db(imm);
		} else {
			opGen(mmx, op, code, 0x66, isXMM_REG32orMEM, imm, B00111010);
		}
	}
	void opR_ModM(const Operand& op, int bit, int ext, int code0, int code1 = NONE, int code2 = NONE, bool disableRex = false)
	{
		int opBit = op.getBit();
		if (disableRex && opBit == 64) opBit = 32;
		if (op.isREG(bit)) {
			opModR(Reg(ext, Operand::REG, opBit), static_cast<const Reg&>(op).changeBit(opBit), code0, code1, code2);
		} else if (op.isMEM()) {
			opModM(static_cast<const Address&>(op), Reg(ext, Operand::REG, opBit), code0, code1, code2);
		} else {
			throw ERR_BAD_COMBINATION;
		}
	}
	void opShift(const Operand& op, int imm, int ext)
	{
		verifyMemHasSize(op);
		opR_ModM(op, 0, ext, (B11000000 | ((imm == 1 ? 1 : 0) << 4)));
		if (imm != 1) db(imm);
	}
	void opShift(const Operand& op, const Reg8& cl, int ext)
	{
		if (cl.getIdx() != Operand::CL) throw ERR_BAD_COMBINATION;
		opR_ModM(op, 0, ext, B11010010);
	}
	void opModRM(const Operand& op1, const Operand& op2, bool condR, bool condM, int code0, int code1 = NONE, int code2 = NONE)
	{
		if (condR) {
			opModR(static_cast<const Reg&>(op1), static_cast<const Reg&>(op2), code0, code1, code2);
		} else if (condM) {
			opModM(static_cast<const Address&>(op2), static_cast<const Reg&>(op1), code0, code1, code2);
		} else {
			throw ERR_BAD_COMBINATION;
		}
	}
	void opShxd(const Operand& op, const Reg& reg, uint8 imm, int code, const Reg8 *cl = 0)
	{
		if (cl && cl->getIdx() != Operand::CL) throw ERR_BAD_COMBINATION;
		opModRM(reg, op, (op.isREG(16 | i32e) && op.getBit() == reg.getBit()), op.isMEM() && (reg.isREG(16 | i32e)), 0x0F, code | (cl ? 1 : 0));
		if (!cl) db(imm);
	}
	// (REG, REG|MEM), (MEM, REG)
	void opRM_RM(const Operand& op1, const Operand& op2, int code)
	{
		if (op1.isREG() && op2.isMEM()) {
			opModM(static_cast<const Address&>(op2), static_cast<const Reg&>(op1), code | 2);
		} else {
			opModRM(op2, op1, op1.isREG() && op1.getKind() == op2.getKind(), op1.isMEM() && op2.isREG(), code);
		}
	}
	// (REG|MEM, IMM)
	void opRM_I(const Operand& op, uint32 imm, int code, int ext)
	{
		verifyMemHasSize(op);
		uint32 immBit = inner::IsInDisp8(imm) ? 8 : isInDisp16(imm) ? 16 : 32;
		if (op.getBit() < immBit) throw ERR_IMM_IS_TOO_BIG;
		if (op.isREG(32|64) && immBit == 16) immBit = 32; /* don't use MEM16 if 32/64bit mode */
		if (op.isREG() && op.getIdx() == 0 && (op.getBit() == immBit || (op.isBit(64) && immBit == 32))) { // rax, eax, ax, al
			rex(op);
			db(code | 4 | (immBit == 8 ? 0 : 1));
		} else {
			int tmp = immBit < (std::min)(op.getBit(), 32U) ? 2 : 0;
			opR_ModM(op, 0, ext, B10000000 | tmp);
		}
		db(imm, immBit / 8);
	}
	void opIncDec(const Operand& op, int code, int ext)
	{
		verifyMemHasSize(op);
#ifndef XBYAK64
		if (op.isREG() && !op.isBit(8)) {
			rex(op); db(code | op.getIdx());
			return;
		}
#endif
		code = B11111110;
		if (op.isREG()) {
			opModR(Reg(ext, Operand::REG, op.getBit()), static_cast<const Reg&>(op), code);
		} else {
			opModM(static_cast<const Address&>(op), Reg(ext, Operand::REG, op.getBit()), code);
		}
	}
	void opPushPop(const Operand& op, int code, int ext, int alt)
	{
		if (op.isREG()) {
			if (op.isBit(16)) db(0x66);
			if (static_cast<const Reg&>(op).getIdx() >= 8) db(0x41);
			db(alt | (op.getIdx() & 7));
		} else if (op.isMEM()) {
			opModM(static_cast<const Address&>(op), Reg(ext, Operand::REG, op.getBit()), code);
		} else {
			throw ERR_BAD_COMBINATION;
		}
	}
	void verifyMemHasSize(const Operand& op) const
	{
		if (op.isMEM() && op.getBit() == 0) throw ERR_MEM_SIZE_IS_NOT_SPECIFIED;
	}
	void opMovxx(const Reg& reg, const Operand& op, uint8 code)
	{
		int w = op.isBit(16);
		bool cond = reg.isREG() && (reg.getBit() > op.getBit());
		opModRM(reg, op, cond && op.isREG(), cond && op.isMEM(), 0x0F, code | w);
	}
	#ifdef XBYAK64
	void opMovsxd(const Reg& reg, const Operand& op)
	{
		bool cond = reg.isREG() && (reg.getBit() > op.getBit());
		opModRM(reg, op, cond && op.isREG(), cond && op.isMEM(), 0x63);
	}
	#endif
	void opFpuMem(const Address& addr, uint8 m16, uint8 m32, uint8 m64, uint8 ext, uint8 m64ext)
	{
		if (addr.is64bitDisp()) throw ERR_CANT_USE_64BIT_DISP;
		uint8 code = addr.isBit(16) ? m16 : addr.isBit(32) ? m32 : addr.isBit(64) ? m64 : 0;
		if (!code) throw ERR_BAD_MEM_SIZE;
		if (m64ext && addr.isBit(64)) ext = m64ext;

		rex(addr, st0);
		db(code);
		addr.updateRegField(ext);
		db(addr.getCode(), static_cast<int>(addr.getSize()));
	}
	// like yasm not nasm
	// use code1 if reg1 == st0
	// use code2 if reg1 != st0 && reg2 == st0
	void opFpuFpu(const Fpu& reg1, const Fpu& reg2, uint32 code1, uint32 code2)
	{
		uint32 code = reg1.getIdx() == 0 ? code1 : reg2.getIdx() == 0 ? code2 : 0;
		if (!code) throw ERR_BAD_ST_COMBINATION;
		db(uint8(code >> 8));
		db(uint8(code | (reg1.getIdx() | reg2.getIdx())));
	}
	void opFpu(const Fpu& reg, uint8 code1, uint8 code2)
	{
		db(code1); db(code2 | reg.getIdx());
	}
public:
	unsigned int getVersion() const { return VERSION; }
	using CodeArray::db;
	const Mmx mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7;
	const Xmm xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	const Ymm ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
	const Xmm &xm0, &xm1, &xm2, &xm3, &xm4, &xm5, &xm6, &xm7;
	const Ymm &ym0, &ym1, &ym2, &ym3, &ym4, &ym5, &ym6, &ym7;
	const Reg32 eax, ecx, edx, ebx, esp, ebp, esi, edi;
	const Reg16 ax, cx, dx, bx, sp, bp, si, di;
	const Reg8 al, cl, dl, bl, ah, ch, dh, bh;
	const AddressFrame ptr, byte, word, dword, qword;
	const Fpu st0, st1, st2, st3, st4, st5, st6, st7;
	const Xmm* xmTbl[16];
	const Ymm* ymTbl[16];
#ifdef XBYAK64
	const Reg64 rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
	const Reg32 r8d, r9d, r10d, r11d, r12d, r13d, r14d, r15d;
	const Reg16 r8w, r9w, r10w, r11w, r12w, r13w, r14w, r15w;
	const Reg8 r8b, r9b, r10b, r11b, r12b, r13b, r14b, r15b;
	const Reg8 spl, bpl, sil, dil;
	const Xmm xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
	const Ymm ymm8, ymm9, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15;
	const Xmm &xm8, &xm9, &xm10, &xm11, &xm12, &xm13, &xm14, &xm15; // for my convenience
	const Ymm &ym8, &ym9, &ym10, &ym11, &ym12, &ym13, &ym14, &ym15;
	const RegRip rip;
#endif

	void L(const char *label)
	{
		label_.define(label, getCurr());
	}
	void inLocalLabel() { label_.enterLocal(); }
	void outLocalLabel() { label_.leaveLocal(); }
	void jmp(const char *label, LabelType type = T_AUTO)
	{
		opJmp(label, type, B11101011, B11101001, 0);
	}
	void jmp(const void *addr, LabelType type = T_AUTO)
	{
		opJmp(addr, type, B11101011, B11101001, 0);
	}
	void jmp(const Operand& op)
	{
		opR_ModM(op, BIT, 4, 0xFF, NONE, NONE, true);
	}
	void call(const Operand& op)
	{
		opR_ModM(op, 16 | i32e, 2, 0xFF, NONE, NONE, true);
	}
	// (REG|MEM, REG)
	void test(const Operand& op, const Reg& reg)
	{
		opModRM(reg, op, op.isREG() && (op.getKind() == reg.getKind()), op.isMEM(), B10000100);
	}
	// (REG|MEM, IMM)
	void test(const Operand& op, uint32 imm)
	{
		verifyMemHasSize(op);
		if (op.isREG() && op.getIdx() == 0) { // al, ax, eax
			rex(op);
			db(B10101000 | (op.isBit(8) ? 0 : 1));
		} else {
			opR_ModM(op, 0, 0, B11110110);
		}
		db(imm, (std::min)(op.getBit() / 8, 4U));
	}
	void ret(int imm = 0)
	{
		if (imm) {
			db(B11000010); dw(imm);
		} else {
			db(B11000011);
		}
	}
	// (REG16|REG32, REG16|REG32|MEM)
	void imul(const Reg& reg, const Operand& op)
	{
		opModRM(reg, op, op.isREG() && (reg.getKind() == op.getKind()), op.isMEM(), 0x0F, B10101111);
	}
	void imul(const Reg& reg, const Operand& op, int imm)
	{
		int s = inner::IsInDisp8(imm) ? 1 : 0;
		opModRM(reg, op, op.isREG() && (reg.getKind() == op.getKind()), op.isMEM(), B01101001 | (s << 1));
		int size = s ? 1 : reg.isREG(16) ? 2 : 4;
		db(imm, size);
	}
	void pop(const Operand& op)
	{
		opPushPop(op, B10001111, 0, B01011000);
	}
	void push(const Operand& op)
	{
		opPushPop(op, B11111111, 6, B01010000);
	}
	void push(const AddressFrame& af, uint32 imm)
	{
		if (af.bit_ == 8 && inner::IsInDisp8(imm)) {
			db(B01101010); db(imm);
		} else if (af.bit_ == 16 && isInDisp16(imm)) {
			db(0x66); db(B01101000); dw(imm);
		} else {
			db(B01101000); dd(imm);
		}
	}
	/* use "push(word, 4)" if you want "push word 4" */
	void push(uint32 imm)
	{
		if (inner::IsInDisp8(imm)) {
			push(byte, imm);
		} else {
			push(dword, imm);
		}
	}
	void bswap(const Reg32e& reg)
	{
		opModR(Reg32(1), reg, 0x0F);
	}
	void mov(const Operand& reg1, const Operand& reg2)
	{
		const Reg *reg = 0;
		const Address *addr = 0;
		uint8 code = 0;
		if (reg1.isREG() && reg1.getIdx() == 0 && reg2.isMEM()) { // mov eax|ax|al, [disp]
			reg = &static_cast<const Reg&>(reg1);
			addr= &static_cast<const Address&>(reg2);
			code = B10100000;
		} else
		if (reg1.isMEM() && reg2.isREG() && reg2.getIdx() == 0) { // mov [disp], eax|ax|al
			reg = &static_cast<const Reg&>(reg2);
			addr= &static_cast<const Address&>(reg1);
			code = B10100010;
		}
#ifdef XBYAK64
		if (addr && addr->is64bitDisp()) {
			if (code) {
				rex(*reg);
				db(reg1.isREG(8) ? 0xA0 : reg1.isREG() ? 0xA1 : reg2.isREG(8) ? 0xA2 : 0xA3);
				db(addr->getDisp(), 8);
			} else {
				throw ERR_BAD_COMBINATION;
			}
		} else
#else
		if (code && addr->isOnlyDisp()) {
			rex(*reg, *addr);
			db(code | (reg->isBit(8) ? 0 : 1));
			dd(static_cast<uint32>(addr->getDisp()));
		} else
#endif
		{
			opRM_RM(reg1, reg2, B10001000);
		}
	}
	void mov(const Operand& op,
#ifdef XBYAK64
	uint64
#else
	uint32
#endif
	imm)
	{
		verifyMemHasSize(op);
		if (op.isREG()) {
			rex(op);
			int code, size;
#ifdef XBYAK64
			if (op.isBit(64) && inner::IsInInt32(imm)) {
				db(B11000111);
				code = B11000000;
				size = 4;
			} else
#endif
			{
				code = B10110000 | ((op.isBit(8) ? 0 : 1) << 3);
				size = op.getBit() / 8;
			}

			db(code | (op.getIdx() & 7));
			db(imm, size);
		} else if (op.isMEM()) {
			opModM(static_cast<const Address&>(op), Reg(0, Operand::REG, op.getBit()), B11000110);
			int size = op.getBit() / 8; if (size > 4) size = 4;
			db(static_cast<uint32>(imm), size);
		} else {
			throw ERR_BAD_COMBINATION;
		}
	}
	void cmpxchg8b(const Address& addr) { opModM(addr, Reg32(1), 0x0F, B11000111); }
#ifdef XBYAK64
	void cmpxchg16b(const Address& addr) { opModM(addr, Reg64(1), 0x0F, B11000111); }
#endif
	void xadd(const Operand& op, const Reg& reg)
	{
		opModRM(reg, op, (op.isREG() && reg.isREG() && op.getBit() == reg.getBit()), op.isMEM(), 0x0F, B11000000 | (reg.isBit(8) ? 0 : 1));
	}
	void xchg(const Operand& op1, const Operand& op2)
	{
		const Operand *p1 = &op1, *p2 = &op2;
		if (p1->isMEM() || (p2->isREG(16 | i32e) && p2->getIdx() == 0)) {
			p1 = &op2; p2 = &op1;
		}
		if (p1->isMEM()) throw ERR_BAD_COMBINATION;
		if (p2->isREG() && (p1->isREG(16 | i32e) && p1->getIdx() == 0)
#ifdef XBYAK64
			&& (p2->getIdx() != 0 || !p1->isREG(32))
#endif
		) {
			rex(*p2, *p1); db(0x90 | (p2->getIdx() & 7));
			return;
		}
		opModRM(*p1, *p2, (p1->isREG() && p2->isREG() && (p1->getBit() == p2->getBit())), p2->isMEM(), B10000110 | (p1->isBit(8) ? 0 : 1));
	}
	void call(const char *label)
	{
		opJmp(label, T_NEAR, 0, B11101000, 0);
	}
	void call(const void *addr)
	{
		opJmp(addr, T_NEAR, 0, B11101000, 0);
	}
	// special case
	void movd(const Address& addr, const Mmx& mmx)
	{
		if (mmx.isXMM()) db(0x66);
		opModM(addr, mmx, 0x0F, B01111110);
	}
	void movd(const Reg32& reg, const Mmx& mmx)
	{
		if (mmx.isXMM()) db(0x66);
		opModR(mmx, reg, 0x0F, B01111110);
	}
	void movd(const Mmx& mmx, const Address& addr)
	{
		if (mmx.isXMM()) db(0x66);
		opModM(addr, mmx, 0x0F, B01101110);
	}
	void movd(const Mmx& mmx, const Reg32& reg)
	{
		if (mmx.isXMM()) db(0x66);
		opModR(mmx, reg, 0x0F, B01101110);
	}
	void movq2dq(const Xmm& xmm, const Mmx& mmx)
	{
		db(0xF3); opModR(xmm, mmx, 0x0F, B11010110);
	}
	void movdq2q(const Mmx& mmx, const Xmm& xmm)
	{
		db(0xF2); opModR(mmx, xmm, 0x0F, B11010110);
	}
	void movq(const Mmx& mmx, const Operand& op)
	{
		if (mmx.isXMM()) db(0xF3);
		opModRM(mmx, op, (mmx.getKind() == op.getKind()), op.isMEM(), 0x0F, mmx.isXMM() ? B01111110 : B01101111);
	}
	void movq(const Address& addr, const Mmx& mmx)
	{
		if (mmx.isXMM()) db(0x66);
		opModM(addr, mmx, 0x0F, mmx.isXMM() ? B11010110 : B01111111);
	}
#ifdef XBYAK64
	void movq(const Reg64& reg, const Mmx& mmx)
	{
		if (mmx.isXMM()) db(0x66);
		opModR(mmx, reg, 0x0F, B01111110);
	}
	void movq(const Mmx& mmx, const Reg64& reg)
	{
		if (mmx.isXMM()) db(0x66);
		opModR(mmx, reg, 0x0F, B01101110);
	}
	void pextrq(const Operand& op, const Xmm& xmm, uint8 imm)
	{
		if (!op.isREG(64) && !op.isMEM()) throw ERR_BAD_COMBINATION;
		opGen(Reg64(xmm.getIdx()), op, 0x16, 0x66, 0, imm, B00111010); // force to 64bit
	}
	void pinsrq(const Xmm& xmm, const Operand& op, uint8 imm)
	{
		if (!op.isREG(64) && !op.isMEM()) throw ERR_BAD_COMBINATION;
		opGen(Reg64(xmm.getIdx()), op, 0x22, 0x66, 0, imm, B00111010); // force to 64bit
	}
#endif
	// MMX2 : pextrw : reg, mmx/xmm, imm
	// SSE4 : pextrw, pextrb, pextrd, extractps : reg/mem, mmx/xmm, imm
	void pextrw(const Operand& op, const Mmx& xmm, uint8 imm) { opExt(op, xmm, 0x15, imm, true); }
	void pextrb(const Operand& op, const Xmm& xmm, uint8 imm) { opExt(op, xmm, 0x14, imm); }
	void pextrd(const Operand& op, const Xmm& xmm, uint8 imm) { opExt(op, xmm, 0x16, imm); }
	void extractps(const Operand& op, const Xmm& xmm, uint8 imm) { opExt(op, xmm, 0x17, imm); }
	void pinsrw(const Mmx& mmx, const Operand& op, int imm)
	{
		if (!op.isREG(32) && !op.isMEM()) throw ERR_BAD_COMBINATION;
		opGen(mmx, op, B11000100, mmx.isXMM() ? 0x66 : NONE, 0, imm);
	}
	void insertps(const Xmm& xmm, const Operand& op, uint8 imm) { opGen(xmm, op, 0x21, 0x66, isXMM_XMMorMEM, imm, B00111010); }
	void pinsrb(const Xmm& xmm, const Operand& op, uint8 imm) { opGen(xmm, op, 0x20, 0x66, isXMM_REG32orMEM, imm, B00111010); }
	void pinsrd(const Xmm& xmm, const Operand& op, uint8 imm) { opGen(xmm, op, 0x22, 0x66, isXMM_REG32orMEM, imm, B00111010); }

	void pmovmskb(const Reg32e& reg, const Mmx& mmx)
	{
		if (mmx.isXMM()) db(0x66);
		opModR(reg, mmx, 0x0F, B11010111);
	}
	void maskmovq(const Mmx& reg1, const Mmx& reg2)
	{
		if (!reg1.isMMX() || !reg2.isMMX()) throw ERR_BAD_COMBINATION;
		opModR(reg1, reg2, 0x0F, B11110111);
	}
	void lea(const Reg32e& reg, const Address& addr) { opModM(addr, reg, B10001101); }

	void movmskps(const Reg32e& reg, const Xmm& xmm) { opModR(reg, xmm, 0x0F, B01010000); }
	void movmskpd(const Reg32e& reg, const Xmm& xmm) { db(0x66); movmskps(reg, xmm); }
	void movntps(const Address& addr, const Xmm& xmm) { opModM(addr, Mmx(xmm.getIdx()), 0x0F, B00101011); }
	void movntdqa(const Xmm& xmm, const Address& addr) { db(0x66); opModM(addr, xmm, 0x0F, 0x38, 0x2A); }
	void lddqu(const Xmm& xmm, const Address& addr) { db(0xF2); opModM(addr, xmm, 0x0F, B11110000); }
	void movnti(const Address& addr, const Reg32e& reg) { opModM(addr, reg, 0x0F, B11000011); }
	void movntq(const Address& addr, const Mmx& mmx)
	{
		if (!mmx.isMMX()) throw ERR_BAD_COMBINATION;
		opModM(addr, mmx, 0x0F, B11100111);
	}
	void popcnt(const Reg& reg, const Operand& op)
	{
		bool is16bit = reg.isREG(16) && (op.isREG(16) || op.isMEM());
		if (!is16bit && !(reg.isREG(i32e) && (op.isREG(i32e) || op.isMEM()))) throw ERR_BAD_COMBINATION;
		if (is16bit) db(0x66);
		db(0xF3); opModRM(reg.changeBit(i32e == 32 ? 32 : reg.getBit()), op, op.isREG(), true, 0x0F, 0xB8);
	}
	void crc32(const Reg32e& reg, const Operand& op)
	{
		if (reg.isBit(32) && op.isBit(16)) db(0x66);
		db(0xF2);
		opModRM(reg, op, op.isREG(), op.isMEM(), 0x0F, 0x38, 0xF0 | (op.isBit(8) ? 0 : 1));
	}
	void vextractps(const Operand& op, const Xmm& xmm, uint8 imm)
	{
		if (!(op.isREG(32) || op.isMEM()) || xmm.isYMM()) throw ERR_BAD_COMBINATION;
		opAVX_X_XM_IMM(xmm, cvtReg(op, op.isREG(), Operand::XMM), MM_0F3A | PP_66, 0x17, false, 0, imm);
	}
	// support (x, x, x/m), (y, y, y/m)
	void opAVX_X_X_XM(const Xmm& xm1, const Operand& op1, const Operand& op2, int type, int code0, bool supportYMM, int w = -1)
	{
		const Xmm *xm2;
		const Operand *op;
		if (op2.isNone()) {
			xm2 = &xm1;
			op = &op1;
		} else {
			if (!(op1.isXMM() || (supportYMM && op1.isYMM()))) throw ERR_BAD_COMBINATION;
			xm2 = static_cast<const Xmm*>(&op1);
			op = &op2;
		}
		// (xm1, xm2, op)
		if (!((xm1.isXMM() && xm2->isXMM()) || (supportYMM && xm1.isYMM() && xm2->isYMM()))) throw ERR_BAD_COMBINATION;
		bool x, b;
		if (op->isMEM()) {
			const Address& addr = *static_cast<const Address*>(op);
			uint8 rex = addr.getRex();
			x = (rex & 2) != 0;
			b = (rex & 1) != 0;
			if (BIT == 64 && addr.is32bit_) db(0x67);
			if (BIT == 64 && w == -1) w = (rex & 4) ? 1 : 0;
		} else {
			x = false;
			b = static_cast<const Reg*>(op)->isExtIdx();
		}
		if (w == -1) w = 0;
		vex(xm1.isExtIdx(), xm2->getIdx(), xm1.isYMM(), type, x, b, w);
		db(code0);
		if (op->isMEM()) {
			const Address& addr = *static_cast<const Address*>(op);
			addr.updateRegField(static_cast<uint8>(xm1.getIdx()));
			db(addr.getCode(), static_cast<int>(addr.getSize()));
		} else {
			db(getModRM(3, xm1.getIdx(), op->getIdx()));
		}
	}
	// if cvt then return pointer to Xmm(idx) (or Ymm(idx)), otherwise return op
	const Operand& cvtReg(const Operand& op, bool cvt, Operand::Kind kind) const
	{
		if (!cvt) return op;
		return (kind == Operand::XMM) ? *xmTbl[op.getIdx()] : *ymTbl[op.getIdx()];
	}
	// support (x, x/m, imm), (y, y/m, imm)
	void opAVX_X_XM_IMM(const Xmm& xmm, const Operand& op, int type, int code, bool supportYMM, int w = -1, int imm = NONE)
	{
		opAVX_X_X_XM(xmm, xmm.isXMM() ? xm0 : ym0, op, type, code, supportYMM, w); if (imm != NONE) db((uint8)imm);
	}
	enum { NONE = 256 };
public:
	CodeGenerator(size_t maxSize = DEFAULT_MAX_CODE_SIZE, void *userPtr = 0)
		: CodeArray(maxSize, userPtr)
		, mm0(0), mm1(1), mm2(2), mm3(3), mm4(4), mm5(5), mm6(6), mm7(7)
		, xmm0(0), xmm1(1), xmm2(2), xmm3(3), xmm4(4), xmm5(5), xmm6(6), xmm7(7)
		, ymm0(0), ymm1(1), ymm2(2), ymm3(3), ymm4(4), ymm5(5), ymm6(6), ymm7(7)
		, xm0(xmm0), xm1(xmm1), xm2(xmm2), xm3(xmm3), xm4(xmm4), xm5(xmm5), xm6(xmm6), xm7(xmm7) // for my convenience
		, ym0(ymm0), ym1(ymm1), ym2(ymm2), ym3(ymm3), ym4(ymm4), ym5(ymm5), ym6(ymm6), ym7(ymm7) // for my convenience
		, eax(Operand::EAX), ecx(Operand::ECX), edx(Operand::EDX), ebx(Operand::EBX), esp(Operand::ESP), ebp(Operand::EBP), esi(Operand::ESI), edi(Operand::EDI)
		, ax(Operand::EAX), cx(Operand::ECX), dx(Operand::EDX), bx(Operand::EBX), sp(Operand::ESP), bp(Operand::EBP), si(Operand::ESI), di(Operand::EDI)
		, al(Operand::AL), cl(Operand::CL), dl(Operand::DL), bl(Operand::BL), ah(Operand::AH), ch(Operand::CH), dh(Operand::DH), bh(Operand::BH)
		, ptr(0), byte(8), word(16), dword(32), qword(64)
		, st0(0), st1(1), st2(2), st3(3), st4(4), st5(5), st6(6), st7(7)
#ifdef XBYAK64
		, rax(Operand::RAX), rcx(Operand::RCX), rdx(Operand::RDX), rbx(Operand::RBX), rsp(Operand::RSP), rbp(Operand::RBP), rsi(Operand::RSI), rdi(Operand::RDI), r8(Operand::R8), r9(Operand::R9), r10(Operand::R10), r11(Operand::R11), r12(Operand::R12), r13(Operand::R13), r14(Operand::R14), r15(Operand::R15)
		, r8d(Operand::R8D), r9d(Operand::R9D), r10d(Operand::R10D), r11d(Operand::R11D), r12d(Operand::R12D), r13d(Operand::R13D), r14d(Operand::R14D), r15d(Operand::R15D)
		, r8w(Operand::R8W), r9w(Operand::R9W), r10w(Operand::R10W), r11w(Operand::R11W), r12w(Operand::R12W), r13w(Operand::R13W), r14w(Operand::R14W), r15w(Operand::R15W)
		, r8b(Operand::R8B), r9b(Operand::R9B), r10b(Operand::R10B), r11b(Operand::R11B), r12b(Operand::R12B), r13b(Operand::R13B), r14b(Operand::R14B), r15b(Operand::R15B)
		, spl(Operand::SPL, 1), bpl(Operand::BPL, 1), sil(Operand::SIL, 1), dil(Operand::DIL, 1)
		, xmm8(8), xmm9(9), xmm10(10), xmm11(11), xmm12(12), xmm13(13), xmm14(14), xmm15(15)
		, ymm8(8), ymm9(9), ymm10(10), ymm11(11), ymm12(12), ymm13(13), ymm14(14), ymm15(15)
		, xm8(xmm8), xm9(xmm9), xm10(xmm10), xm11(xmm11), xm12(xmm12), xm13(xmm13), xm14(xmm14), xm15(xmm15) // for my convenience
		, ym8(ymm8), ym9(ymm9), ym10(ymm10), ym11(ymm11), ym12(ymm12), ym13(ymm13), ym14(ymm14), ym15(ymm15) // for my convenience
		, rip()
#endif
	{
		xmTbl[0] = &xm0; xmTbl[1] = &xm1; xmTbl[2] = &xm2; xmTbl[3] = &xm3;
		xmTbl[4] = &xm4; xmTbl[5] = &xm5; xmTbl[6] = &xm6; xmTbl[7] = &xm7;
		ymTbl[0] = &ym0; ymTbl[1] = &ym1; ymTbl[2] = &ym2; ymTbl[3] = &ym3;
		ymTbl[4] = &ym4; ymTbl[5] = &ym5; ymTbl[6] = &ym6; ymTbl[7] = &ym7;
#ifdef XBYAK64
		xmTbl[8] = &xm8; xmTbl[9] = &xm9; xmTbl[10] = &xm10; xmTbl[11] = &xm11;
		xmTbl[12] = &xm12; xmTbl[13] = &xm13; xmTbl[14] = &xm14; xmTbl[15] = &xm15;
		ymTbl[8] = &ym8; ymTbl[9] = &ym9; ymTbl[10] = &ym10; ymTbl[11] = &ym11;
		ymTbl[12] = &ym12; ymTbl[13] = &ym13; ymTbl[14] = &ym14; ymTbl[15] = &ym15;
#endif

		label_.set(this);
	}
	bool hasUndefinedLabel() const { return label_.hasUndefinedLabel(); }
	const uint8 *getCode() const
	{
		assert(!hasUndefinedLabel());
//		if (hasUndefinedLabel()) throw ERR_LABEL_IS_NOT_FOUND;
		return top_;
	}
#ifdef XBYAK_TEST
	void dump(bool doClear = true)
	{
		CodeArray::dump();
		if (doClear) size_ = 0;
	}
#endif

#ifndef XBYAK_DONT_READ_LIST
#include "xbyak_mnemonic.h"
	void align(int x = 16)
	{
		if (x != 4 && x != 8 && x != 16 && x != 32) throw ERR_BAD_ALIGN;
		while (size_t(getCurr()) % x) {
			nop();
		}
	}
#endif
};

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

} // end of namespace

#endif // XBYAK_XBYAK_H_
