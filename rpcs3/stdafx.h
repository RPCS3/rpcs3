#pragma once

#ifdef MSVC_CRT_MEMLEAK_DETECTION
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#define NOMINMAX
#ifndef QT_UI
#include <wx/wxprec.h>
#include <wx/config.h>
#include <wx/string.h>
#include <wx/propdlg.h>
#include <wx/dcclient.h>

#include <wx/wfstream.h>
#include <wx/spinctrl.h>
#include <wx/datetime.h>
#include <wx/filepicker.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/scrolbar.h>
#include <wx/frame.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/listctrl.h>
#include <wx/aui/auibook.h>
#endif

#if defined(MSVC_CRT_MEMLEAK_DETECTION) && defined(_DEBUG) && !defined(DBG_NEW)
	#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
	#define new DBG_NEW
#endif

// This header should be frontend-agnostic, so don't assume wx includes everything
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <vector>
#include <set>
#include <string>
#include <ostream>
#include <sstream>
#include <functional>
#include <algorithm>

#include "Utilities/GNU.h"

typedef unsigned int uint;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

union u128
{
	struct
	{
		u64 hi;
		u64 lo;
	};

	u64 _u64[2];
	u32 _u32[4];
	u16 _u16[8];
	u8  _u8[16];

	operator u64() const { return _u64[0]; }
	operator u32() const { return _u32[0]; }
	operator u16() const { return _u16[0]; }
	operator u8()  const { return _u8[0]; }

	operator bool() const { return _u64[0] != 0 || _u64[1] != 0; }

	static u128 From128(u64 hi, u64 lo)
	{
		u128 ret = { hi, lo };
		return ret;
	}

	static u128 From64(u64 src)
	{
		u128 ret = { 0, src };
		return ret;
	}

	static u128 From32(u32 src)
	{
		u128 ret;
		ret._u32[0] = src;
		ret._u32[1] = 0;
		ret._u32[2] = 0;
		ret._u32[3] = 0;
		return ret;
	}

	static u128 FromBit(u32 bit)
	{
		u128 ret;
		if (bit < 64)
		{
			ret.hi = 0;
			ret.lo = (u64)1 << bit;
		}
		else if (bit < 128)
		{
			ret.hi = (u64)1 << (bit - 64);
			ret.lo = 0;
		}
		else
		{
			ret.hi = 0;
			ret.lo = 0;
		}
		return ret;
	}

	bool operator == (const u128& right) const
	{
		return (lo == right.lo) && (hi == right.hi);
	}

	bool operator != (const u128& right) const
	{
		return (lo != right.lo) || (hi != right.hi);
	}

	u128 operator | (const u128& right) const
	{
		return From128(hi | right.hi, lo | right.lo);
	}

	u128 operator & (const u128& right) const
	{
		return From128(hi & right.hi, lo & right.lo);
	}

	u128 operator ^ (const u128& right) const
	{
		return From128(hi ^ right.hi, lo ^ right.lo);
	}

	u128 operator ~ () const
	{
		return From128(~hi, ~lo);
	}

	static __forceinline u128 byteswap(const u128 val)
	{
		u128 ret;
		ret.lo = _byteswap_uint64(val.hi);
		ret.hi = _byteswap_uint64(val.lo);
		return ret;
	}
};

union s128
{
	struct
	{
		s64 hi;
		s64 lo;
	};

	u64 _i64[2];
	u32 _i32[4];
	u16 _i16[8];
	u8  _i8[16];

	operator s64() const { return _i64[0]; }
	operator s32() const { return _i32[0]; }
	operator s16() const { return _i16[0]; }
	operator s8()  const { return _i8[0]; }

	operator bool() const { return _i64[0] != 0 || _i64[1] != 0; }

	static s128 From64(s64 src)
	{
		s128 ret = { src, 0 };
		return ret;
	}

	static s128 From32(s32 src)
	{
		s128 ret;
		ret._i32[0] = src;
		ret._i32[1] = 0;
		ret.hi = 0;
		return ret;
	}

	bool operator == (const s128& right) const
	{
		return (lo == right.lo) && (hi == right.hi);
	}

	bool operator != (const s128& right) const
	{
		return (lo != right.lo) || (hi != right.hi);
	}
};

//TODO: SSE style
/*
struct u128
{
	__m128 m_val;

	u128 GetValue128()
	{
		u128 ret;
		_mm_store_ps( (float*)&ret, m_val );
		return ret;
	}

	u64 GetValue64()
	{
		u64 ret;
		_mm_store_ps( (float*)&ret, m_val );
		return ret;
	}

	u32 GetValue32()
	{
		u32 ret;
		_mm_store_ps( (float*)&ret, m_val );
		return ret;
	}

	u16 GetValue16()
	{
		u16 ret;
		_mm_store_ps( (float*)&ret, m_val );
		return ret;
	}

	u8 GetValue8()
	{
		u8 ret;
		_mm_store_ps( (float*)&ret, m_val );
		return ret;
	}
};
*/

#define AlignAddr(addr, align) (((addr) + ((align) - 1)) & ~((align) - 1))

#include "Utilities/StrFmt.h"
#include "Utilities/BEType.h"

#define _PRGNAME_ "RPCS3"
#define _PRGVER_ "0.0.0.5"
