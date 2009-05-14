#pragma once

// NOTE: x64 version of the _mm_set_* functions are terrible, first they store components into memory then reload in one piece (VS2008 SP1)

#pragma pack(push, 1)

template<class T> class GSVector2T
{
public:
	union 
	{
		struct {T x, y;}; 
		struct {T r, g;}; 
		struct {T v[2];};
	};

	GSVector2T()
	{
	}
	
	GSVector2T(T x, T y) 
	{
		this->x = x; 
		this->y = y;
	}

	GSVector2T(const GSVector2T& v) 
	{
		*this = v;
	}

	const GSVector2T& operator = (const GSVector2T& v) 
	{
		_mm_storel_epi64((__m128i*)this, _mm_loadl_epi64((__m128i*)&v));

		return *this;
	}
};

typedef GSVector2T<float> GSVector2;
typedef GSVector2T<int> GSVector2i;

class GSVector4;

__declspec(align(16)) class GSVector4i
{
public:
	union 
	{
		struct {int x, y, z, w;}; 
		struct {int r, g, b, a;};
		struct {int left, top, right, bottom;}; 
		int v[4];
		float f32[4];
		int8 i8[16];
		int16 i16[8];
		int32 i32[4];
		int64  i64[2];
		uint8 u8[16];
		uint16 u16[8];
		uint32 u32[4];
		uint64 u64[2];
		__m128i m;
	};

	GSVector4i() 
	{
	}

	GSVector4i(int x, int y, int z, int w) 
	{
		// 4 gprs

		// m = _mm_set_epi32(w, z, y, x); 

		// 2 gprs

		GSVector4i xz = load(x).upl32(load(z));
		GSVector4i yw = load(y).upl32(load(w));

		*this = xz.upl32(yw);
	}

	GSVector4i(int x, int y) 
	{
		*this = load(x).upl32(load(y));
	}

	GSVector4i(short s0, short s1, short s2, short s3, short s4, short s5, short s6, short s7) 
	{
		m = _mm_set_epi16(s7, s6, s5, s4, s3, s2, s1, s0);
	}

	GSVector4i(char b0, char b1, char b2, char b3, char b4, char b5, char b6, char b7, char b8, char b9, char b10, char b11, char b12, char b13, char b14, char b15) 
	{
		m = _mm_set_epi8(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0);
	}

	GSVector4i(const GSVector4i& v) 
	{
		m = v.m;
	}

	explicit GSVector4i(const GSVector2i& v)
	{
		m = _mm_loadl_epi64((__m128i*)&v);
	}

	explicit GSVector4i(int i) 
	{
		m = _mm_set1_epi32(i);
	}

	explicit GSVector4i(__m128i m)
	{
		this->m = m;
	}

	explicit GSVector4i(const GSVector4& v)
	{
		*this = v;
	}

	void operator = (const GSVector4i& v) 
	{
		m = v.m;
	}

	void operator = (const GSVector4& v);

	void operator = (int i) 
	{
		m = _mm_set1_epi32(i);
	}

	void operator = (__m128i m) 
	{
		this->m = m;
	}

	operator __m128i() const 
	{
		return m;
	}

	// rect

	int width() const
	{
		return right - left;
	}

	int height() const
	{
		return bottom - top;
	}

	bool rempty() const
	{
		return (*this < zwzw()).mask() != 0x00ff;
	}

	GSVector4i runion(const GSVector4i& a) const 
	{
		#if _M_SSE >= 0x401

		return min_i32(a).upl64(max_i32(a).srl<8>());

		#else

		return GSVector4i(min(x, a.x), min(y, a.y), max(z, a.z), max(x, a.w));

		#endif
	}

	GSVector4i rintersect(const GSVector4i& a) const 
	{
		return sat_i32(a);
	}

	GSVector4i fit(int arx, int ary) const;

	GSVector4i fit(int preset) const;

	operator LPCRECT() const
	{
		return (LPCRECT)this;
	}

	operator LPRECT()
	{
		return (LPRECT)this;
	}

	//

	uint32 rgba32() const
	{
		GSVector4i v = *this;

		v = v.ps32(v);
		v = v.pu16(v);

		return (uint32)store(v);
	}

	static GSVector4i cast(const GSVector4& v);

	#if _M_SSE >= 0x401

	GSVector4i sat_i8(const GSVector4i& a, const GSVector4i& b) const 
	{
		return max_i8(a).min_i8(b);
	}

	GSVector4i sat_i8(const GSVector4i& a) const 
	{
		return max_i8(a.xyxy()).min_i8(a.zwzw());
	}

	#endif

	GSVector4i sat_i16(const GSVector4i& a, const GSVector4i& b) const 
	{
		return max_i16(a).min_i16(b);
	}

	GSVector4i sat_i16(const GSVector4i& a) const 
	{
		return max_i16(a.xyxy()).min_i16(a.zwzw());
	}

	#if _M_SSE >= 0x401

	GSVector4i sat_i32(const GSVector4i& a, const GSVector4i& b) const 
	{
		return max_i32(a).min_i32(b);
	}

	GSVector4i sat_i32(const GSVector4i& a) const 
	{
		return max_i32(a.xyxy()).min_i32(a.zwzw());
	}

	#else

	GSVector4i sat_i32(const GSVector4i& a, const GSVector4i& b) const 
	{
		GSVector4i v;

		v.x = min(max(x, a.x), b.x);
		v.y = min(max(y, a.y), b.y);
		v.z = min(max(z, a.z), b.z);
		v.w = min(max(x, a.w), b.w);

		return v;
	}

	GSVector4i sat_i32(const GSVector4i& a) const 
	{
		GSVector4i v;

		v.x = min(max(x, a.x), b.z);
		v.y = min(max(y, a.y), b.w);
		v.z = min(max(z, a.x), b.z);
		v.w = min(max(x, a.y), b.w);

		return v;
	}

	#endif

	GSVector4i sat_u8(const GSVector4i& a, const GSVector4i& b) const 
	{
		return max_u8(a).min_u8(b);
	}

	GSVector4i sat_u8(const GSVector4i& a) const 
	{
		return max_u8(a.xyxy()).min_u8(a.zwzw());
	}

	#if _M_SSE >= 0x401

	GSVector4i sat_u16(const GSVector4i& a, const GSVector4i& b) const 
	{
		return max_u16(a).min_u16(b);
	}

	GSVector4i sat_u16(const GSVector4i& a) const 
	{
		return max_u16(a.xyxy()).min_u16(a.zwzw());
	}

	#endif

	#if _M_SSE >= 0x401

	GSVector4i sat_u32(const GSVector4i& a, const GSVector4i& b) const 
	{
		return max_u32(a).min_u32(b);
	}

	GSVector4i sat_u32(const GSVector4i& a) const 
	{
		return max_u32(a.xyxy()).min_u32(a.zwzw());
	}

	#endif

	#if _M_SSE >= 0x401

	GSVector4i min_i8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epi8(m, a));
	}

	GSVector4i max_i8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epi8(m, a));
	}

	#endif

	GSVector4i min_i16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epi16(m, a));
	}

	GSVector4i max_i16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epi16(m, a));
	}

	#if _M_SSE >= 0x401

	GSVector4i min_i32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epi32(m, a));
	}

	GSVector4i max_i32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epi32(m, a));
	}

	#endif

	GSVector4i min_u8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epu8(m, a));
	}

	GSVector4i max_u8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epu8(m, a));
	}

	#if _M_SSE >= 0x401

	GSVector4i min_u16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epu16(m, a));
	}

	GSVector4i max_u16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epu16(m, a));
	}

	GSVector4i min_u32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epu32(m, a));
	}

	GSVector4i max_u32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epu32(m, a));
	}

	#endif

	static int min_i16(int a, int b)
	{
		 return store(load(a).min_i16(load(b)));
	}

	GSVector4i clamp8() const
	{
		return pu16().upl8();
	}

	GSVector4i blend8(const GSVector4i& a, const GSVector4i& mask) const
	{
		#if _M_SSE >= 0x401

		return GSVector4i(_mm_blendv_epi8(m, a, mask));

		#else

		return GSVector4i(_mm_or_si128(_mm_andnot_si128(mask, m), _mm_and_si128(mask, a)));

		#endif
	}

	#if _M_SSE >= 0x401

	template<int mask> GSVector4i blend16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_blend_epi16(m, a, mask));
	}

	#endif

	GSVector4i blend(const GSVector4i& a, const GSVector4i& mask) const
	{
		return GSVector4i(_mm_or_si128(_mm_andnot_si128(mask, m), _mm_and_si128(mask, a)));
	}

	GSVector4i mix16(const GSVector4i& a) const
	{
		#if _M_SSE >= 0x401

		return blend16<0xaa>(a);
		
		#else
		
		return blend8(a, GSVector4i::xffff0000());

		#endif
	}

	#if _M_SSE >= 0x301

	GSVector4i shuffle8(const GSVector4i& mask) const
	{
		return GSVector4i(_mm_shuffle_epi8(m, mask));
	}

	#endif

	GSVector4i ps16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_packs_epi16(m, a));
	}

	GSVector4i ps16() const
	{
		return GSVector4i(_mm_packs_epi16(m, m));
	}

	GSVector4i pu16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_packus_epi16(m, a));
	}

	GSVector4i pu16() const
	{
		return GSVector4i(_mm_packus_epi16(m, m));
	}

	GSVector4i ps32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_packs_epi32(m, a));
	}
	
	GSVector4i ps32() const
	{
		return GSVector4i(_mm_packs_epi32(m, m));
	}
	
	#if _M_SSE >= 0x401

	GSVector4i pu32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_packus_epi32(m, a));
	}

	GSVector4i pu32() const
	{
		return GSVector4i(_mm_packus_epi32(m, m));
	}

	#endif

	GSVector4i upl8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpacklo_epi8(m, a));
	}

	GSVector4i uph8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpackhi_epi8(m, a));
	}

	GSVector4i upl16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpacklo_epi16(m, a));
	}

	GSVector4i uph16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpackhi_epi16(m, a));
	}

	GSVector4i upl32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpacklo_epi32(m, a));
	}

	GSVector4i uph32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpackhi_epi32(m, a));
	}

	GSVector4i upl64(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpacklo_epi64(m, a));
	}

	GSVector4i uph64(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpackhi_epi64(m, a));
	}

	GSVector4i upl8() const
	{
		#if 0 // _M_SSE >= 0x401 // TODO: compiler bug

		return GSVector4i(_mm_cvtepu8_epi16(m));
		
		#else

		return GSVector4i(_mm_unpacklo_epi8(m, _mm_setzero_si128()));

		#endif
	}

	GSVector4i uph8() const
	{
		return GSVector4i(_mm_unpackhi_epi8(m, _mm_setzero_si128()));
	}

	GSVector4i upl16() const
	{
		#if 0 //_M_SSE >= 0x401 // TODO: compiler bug

		return GSVector4i(_mm_cvtepu16_epi32(m));
		
		#else

		return GSVector4i(_mm_unpacklo_epi16(m, _mm_setzero_si128()));

		#endif
	}

	GSVector4i uph16() const
	{
		return GSVector4i(_mm_unpackhi_epi16(m, _mm_setzero_si128()));
	}

	GSVector4i upl32() const
	{
		#if 0 //_M_SSE >= 0x401 // TODO: compiler bug

		return GSVector4i(_mm_cvtepu32_epi64(m));
		
		#else

		return GSVector4i(_mm_unpacklo_epi32(m, _mm_setzero_si128()));

		#endif
	}

	GSVector4i uph32() const
	{
		return GSVector4i(_mm_unpackhi_epi32(m, _mm_setzero_si128()));
	}

	GSVector4i upl64() const
	{
		return GSVector4i(_mm_unpacklo_epi64(m, _mm_setzero_si128()));
	}

	GSVector4i uph64() const
	{
		return GSVector4i(_mm_unpackhi_epi64(m, _mm_setzero_si128()));
	}

	#if _M_SSE >= 0x401

	// WARNING!!!
	//
	// MSVC (2008, 2010 ctp) believes that there is a "mem, reg" form of the pmovz/sx* instructions,
	// turning these intrinsics into a minefield, don't spill regs when using them...

	GSVector4i i8to16() const
	{
		return GSVector4i(_mm_cvtepi8_epi16(m));
	}

	GSVector4i u8to16() const
	{
		return GSVector4i(_mm_cvtepu8_epi16(m));
	}

	GSVector4i i8to32() const
	{
		return GSVector4i(_mm_cvtepi8_epi32(m));
	}

	GSVector4i u8to32() const
	{
		return GSVector4i(_mm_cvtepu8_epi32(m));
	}

	GSVector4i i8to64() const
	{
		return GSVector4i(_mm_cvtepi8_epi64(m));
	}

	GSVector4i u8to64() const
	{
		return GSVector4i(_mm_cvtepu16_epi64(m));
	}

	GSVector4i i16to32() const
	{
		return GSVector4i(_mm_cvtepi16_epi32(m));
	}

	GSVector4i u16to32() const
	{
		return GSVector4i(_mm_cvtepu16_epi32(m));
	}

	GSVector4i i16to64() const
	{
		return GSVector4i(_mm_cvtepi16_epi64(m));
	}

	GSVector4i u16to64() const
	{
		return GSVector4i(_mm_cvtepu16_epi64(m));
	}

	GSVector4i i32to64() const
	{
		return GSVector4i(_mm_cvtepi32_epi64(m));
	}

	GSVector4i u32to64() const
	{
		return GSVector4i(_mm_cvtepu32_epi64(m));
	}

	#else

	GSVector4i u8to16() const
	{
		return upl8();
	}

	GSVector4i u8to32() const
	{
		return upl8().upl16();
	}

	GSVector4i u8to64() const
	{
		return upl8().upl16().upl32();
	}

	GSVector4i u16to32() const
	{
		return upl16();
	}

	GSVector4i u16to64() const
	{
		return upl16().upl32();
	}

	GSVector4i u32to64() const
	{
		return upl32();
	}

	#endif

	template<int i> GSVector4i srl() const
	{
		#pragma warning(push)
		#pragma warning(disable: 4556)

		return GSVector4i(_mm_srli_si128(m, i));

		#pragma warning(pop)
	}

	template<int i> GSVector4i srl(const GSVector4i& v)
	{
		#if _M_SSE >= 0x301

		return GSVector4i(_mm_alignr_epi8(v.m, m, i));

		#else

		if(i == 0) return *this;
		else if(i < 16) return srl<i>() | v.sll<16 - i>();
		else if(i == 16) return v;
		else if(i < 32) return v.srl<i - 16>();
		else return zero();

		#endif
	}

	template<int i> GSVector4i sll() const
	{
		#pragma warning(push)
		#pragma warning(disable: 4556)

		return GSVector4i(_mm_slli_si128(m, i));

		#pragma warning(pop)
	}

	GSVector4i sra16(int i) const
	{
		return GSVector4i(_mm_srai_epi16(m, i));
	}

	GSVector4i sra32(int i) const
	{
		return GSVector4i(_mm_srai_epi32(m, i));
	}

	GSVector4i sll16(int i) const
	{
		return GSVector4i(_mm_slli_epi16(m, i));
	}

	GSVector4i sll32(int i) const
	{
		return GSVector4i(_mm_slli_epi32(m, i));
	}

	GSVector4i sll64(int i) const
	{
		return GSVector4i(_mm_slli_epi64(m, i));
	}

	GSVector4i srl16(int i) const
	{
		return GSVector4i(_mm_srli_epi16(m, i));
	}

	GSVector4i srl32(int i) const
	{
		return GSVector4i(_mm_srli_epi32(m, i));
	}

	GSVector4i srl64(int i) const
	{
		return GSVector4i(_mm_srli_epi64(m, i));
	}

	GSVector4i add8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_add_epi8(m, v.m));
	}

	GSVector4i add16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_add_epi16(m, v.m));
	}

	GSVector4i add32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_add_epi32(m, v.m));
	}

	GSVector4i adds8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_adds_epi8(m, v.m));
	}

	GSVector4i adds16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_adds_epi16(m, v.m));
	}

	GSVector4i addus8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_adds_epu8(m, v.m));
	}

	GSVector4i addus16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_adds_epu16(m, v.m));
	}

	GSVector4i sub8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_sub_epi8(m, v.m));
	}

	GSVector4i sub16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_sub_epi16(m, v.m));
	}

	GSVector4i sub32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_sub_epi32(m, v.m));
	}

	GSVector4i subs8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_subs_epi8(m, v.m));
	}

	GSVector4i subs16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_subs_epi16(m, v.m));
	}

	GSVector4i subus8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_subs_epu8(m, v.m));
	}

	GSVector4i subus16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_subs_epu16(m, v.m));
	}

	GSVector4i avg8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_avg_epu8(m, v.m));
	}

	GSVector4i avg16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_avg_epu16(m, v.m));
	}

	GSVector4i mul16hs(const GSVector4i& v) const
	{
		return GSVector4i(_mm_mulhi_epi16(m, v.m));
	}

	GSVector4i mul16hu(const GSVector4i& v) const
	{
		return GSVector4i(_mm_mulhi_epu16(m, v.m));
	}

	GSVector4i mul16l(const GSVector4i& v) const
	{
		return GSVector4i(_mm_mullo_epi16(m, v.m));
	}

	#if _M_SSE >= 0x301

	GSVector4i mul16hrs(const GSVector4i& v) const
	{
		return GSVector4i(_mm_mulhrs_epi16(m, v.m));
	}

	#endif

	template<int shift> GSVector4i lerp16(const GSVector4i& a, const GSVector4i& f) const
	{
		// (a - this) * f << shift + this

		return add16(a.sub16(*this).modulate16<shift>(f));
	}

	template<int shift> static GSVector4i lerp16(const GSVector4i& a, const GSVector4i& b, const GSVector4i& c)
	{
		// (a - b) * c << shift

		return a.sub16(b).modulate16<shift>(c);
	}

	template<int shift> static GSVector4i lerp16(const GSVector4i& a, const GSVector4i& b, const GSVector4i& c, const GSVector4i& d)
	{
		// (a - b) * c << shift + d

		return d.add16(a.sub16(b).modulate16<shift>(c));
	}

	template<int shift> GSVector4i modulate16(const GSVector4i& f) const
	{
		// a * f << shift

		#if _M_SSE >= 0x301

		if(shift == 0) 
		{
			return mul16hrs(f);
		}

		#endif

		return sll16(shift + 1).mul16hs(f);
	}

	bool eq(const GSVector4i& v) const
	{
		#if _M_SSE >= 0x401
		// pxor, ptest, je
		GSVector4i t = *this ^ v;
		return _mm_testz_si128(t, t) != 0;
		#else
		// pcmpeqd, pmovmskb, cmp, je
		return eq32(v).alltrue();
		#endif
	}

	GSVector4i eq8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpeq_epi8(m, v.m));
	}

	GSVector4i eq16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpeq_epi16(m, v.m));
	}

	GSVector4i eq32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpeq_epi32(m, v.m));
	}

	GSVector4i neq8(const GSVector4i& v) const
	{
		return ~eq8(v);
	}

	GSVector4i neq16(const GSVector4i& v) const
	{
		return ~eq16(v);
	}

	GSVector4i neq32(const GSVector4i& v) const
	{
		return ~eq32(v);
	}

	GSVector4i gt8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpgt_epi8(m, v.m));
	}

	GSVector4i gt16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpgt_epi16(m, v.m));
	}

	GSVector4i gt32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpgt_epi32(m, v.m));
	}

	GSVector4i lt8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmplt_epi8(m, v.m));
	}

	GSVector4i lt16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmplt_epi16(m, v.m));
	}

	GSVector4i lt32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmplt_epi32(m, v.m));
	}

	GSVector4i andnot(const GSVector4i& v) const
	{
		return GSVector4i(_mm_andnot_si128(v.m, m));
	}

	int mask() const
	{
		return _mm_movemask_epi8(m);
	}

	bool alltrue() const
	{
		return _mm_movemask_epi8(m) == 0xffff;
	}

	bool anytrue() const
	{
		return _mm_movemask_epi8(m) != 0x0000;
	}

	#if _M_SSE >= 0x401

	template<int i> GSVector4i insert8(int a) const
	{
		return GSVector4i(_mm_insert_epi8(m, a, i));
	}

	#endif

	template<int i> int extract8() const
	{
		#if _M_SSE >= 0x401
		return _mm_extract_epi8(m, i);
		#else
		return (int)u8[i];
		#endif
	}

	template<int i> GSVector4i insert16(int a) const
	{
		return GSVector4i(_mm_insert_epi16(m, a, i));
	}

	template<int i> int extract16() const
	{
		return _mm_extract_epi16(m, i);
	}

	#if _M_SSE >= 0x401

	template<int i> GSVector4i insert32(int a) const
	{
		return GSVector4i(_mm_insert_epi32(m, a, i));
	}

	#endif

	template<int i> int extract32() const
	{
		if(i == 0) return GSVector4i::store(*this);
		#if _M_SSE >= 0x401
		return _mm_extract_epi32(m, i);
		#else
		return i32[i];
		#endif
	}

	#ifdef _M_AMD64

	#if _M_SSE >= 0x401

	template<int i> GSVector4i insert64(__int64 a) const
	{
		return GSVector4i(_mm_insert_epi64(m, a, i));
	}

	#endif

	template<int i> __int64 extract64() const
	{
		if(i == 0) return GSVector4i::storeq(*this);
		#if _M_SSE >= 0x401
		return _mm_extract_epi64(m, i);
		#else
		return i64[i];
		#endif
	}

	#endif

	#if _M_SSE >= 0x401

	template<int src, class T> __forceinline GSVector4i gather8_4(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert8<1>((int)ptr[extract8<src + 0>() >> 4]);
		v = v.insert8<2>((int)ptr[extract8<src + 1>() & 0xf]);
		v = v.insert8<3>((int)ptr[extract8<src + 1>() >> 4]);
		v = v.insert8<4>((int)ptr[extract8<src + 2>() & 0xf]);
		v = v.insert8<5>((int)ptr[extract8<src + 2>() >> 4]);
		v = v.insert8<6>((int)ptr[extract8<src + 3>() & 0xf]);
		v = v.insert8<7>((int)ptr[extract8<src + 3>() >> 4]);
		v = v.insert8<8>((int)ptr[extract8<src + 4>() & 0xf]);
		v = v.insert8<9>((int)ptr[extract8<src + 4>() >> 4]);
		v = v.insert8<10>((int)ptr[extract8<src + 5>() & 0xf]);
		v = v.insert8<11>((int)ptr[extract8<src + 5>() >> 4]);
		v = v.insert8<12>((int)ptr[extract8<src + 6>() & 0xf]);
		v = v.insert8<13>((int)ptr[extract8<src + 6>() >> 4]);
		v = v.insert8<14>((int)ptr[extract8<src + 7>() & 0xf]);
		v = v.insert8<15>((int)ptr[extract8<src + 7>() >> 4]);

		return v;
	}

	template<class T> __forceinline GSVector4i gather8_8(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<0>()]);
		v = v.insert8<1>((int)ptr[extract8<1>()]);
		v = v.insert8<2>((int)ptr[extract8<2>()]);
		v = v.insert8<3>((int)ptr[extract8<3>()]);
		v = v.insert8<4>((int)ptr[extract8<4>()]);
		v = v.insert8<5>((int)ptr[extract8<5>()]);
		v = v.insert8<6>((int)ptr[extract8<6>()]);
		v = v.insert8<7>((int)ptr[extract8<7>()]);
		v = v.insert8<8>((int)ptr[extract8<8>()]);
		v = v.insert8<9>((int)ptr[extract8<9>()]);
		v = v.insert8<10>((int)ptr[extract8<10>()]);
		v = v.insert8<11>((int)ptr[extract8<11>()]);
		v = v.insert8<12>((int)ptr[extract8<12>()]);
		v = v.insert8<13>((int)ptr[extract8<13>()]);
		v = v.insert8<14>((int)ptr[extract8<14>()]);
		v = v.insert8<15>((int)ptr[extract8<15>()]);

		return v;
	}

	template<int dst, class T> __forceinline GSVector4i gather8_16(const T* ptr, const GSVector4i& a) const
	{
		GSVector4i v = a;

		v = v.insert8<dst + 0>((int)ptr[extract16<0>()]);
		v = v.insert8<dst + 1>((int)ptr[extract16<1>()]);
		v = v.insert8<dst + 2>((int)ptr[extract16<2>()]);
		v = v.insert8<dst + 3>((int)ptr[extract16<3>()]);
		v = v.insert8<dst + 4>((int)ptr[extract16<4>()]);
		v = v.insert8<dst + 5>((int)ptr[extract16<5>()]);
		v = v.insert8<dst + 6>((int)ptr[extract16<6>()]);
		v = v.insert8<dst + 7>((int)ptr[extract16<7>()]);

		return v;
	}

	template<int dst, class T> __forceinline GSVector4i gather8_32(const T* ptr, const GSVector4i& a) const
	{
		GSVector4i v = a;

		v = v.insert8<dst + 0>((int)ptr[extract32<0>()]);
		v = v.insert8<dst + 1>((int)ptr[extract32<1>()]);
		v = v.insert8<dst + 2>((int)ptr[extract32<2>()]);
		v = v.insert8<dst + 3>((int)ptr[extract32<3>()]);

		return v;
	}

	#endif

	template<int src, class T> __forceinline GSVector4i gather16_4(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert16<1>((int)ptr[extract8<src + 0>() >> 4]);
		v = v.insert16<2>((int)ptr[extract8<src + 1>() & 0xf]);
		v = v.insert16<3>((int)ptr[extract8<src + 1>() >> 4]);
		v = v.insert16<4>((int)ptr[extract8<src + 2>() & 0xf]);
		v = v.insert16<5>((int)ptr[extract8<src + 2>() >> 4]);
		v = v.insert16<6>((int)ptr[extract8<src + 3>() & 0xf]);
		v = v.insert16<7>((int)ptr[extract8<src + 3>() >> 4]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather16_8(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<src + 0>()]);
		v = v.insert16<1>((int)ptr[extract8<src + 1>()]);
		v = v.insert16<2>((int)ptr[extract8<src + 2>()]);
		v = v.insert16<3>((int)ptr[extract8<src + 3>()]);
		v = v.insert16<4>((int)ptr[extract8<src + 4>()]);
		v = v.insert16<5>((int)ptr[extract8<src + 5>()]);
		v = v.insert16<6>((int)ptr[extract8<src + 6>()]);
		v = v.insert16<7>((int)ptr[extract8<src + 7>()]);

		return v;
	}

	template<class T>__forceinline GSVector4i gather16_16(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract16<0>()]);
		v = v.insert16<1>((int)ptr[extract16<1>()]);
		v = v.insert16<2>((int)ptr[extract16<2>()]);
		v = v.insert16<3>((int)ptr[extract16<3>()]);
		v = v.insert16<4>((int)ptr[extract16<4>()]);
		v = v.insert16<5>((int)ptr[extract16<5>()]);
		v = v.insert16<6>((int)ptr[extract16<6>()]);
		v = v.insert16<7>((int)ptr[extract16<7>()]);

		return v;
	}

	template<class T1, class T2>__forceinline GSVector4i gather16_16(const T1* ptr1, const T2* ptr2) const
	{
		GSVector4i v;

		v = load((int)ptr2[ptr1[extract16<0>()]]);
		v = v.insert16<1>((int)ptr2[ptr1[extract16<1>()]]);
		v = v.insert16<2>((int)ptr2[ptr1[extract16<2>()]]);
		v = v.insert16<3>((int)ptr2[ptr1[extract16<3>()]]);
		v = v.insert16<4>((int)ptr2[ptr1[extract16<4>()]]);
		v = v.insert16<5>((int)ptr2[ptr1[extract16<5>()]]);
		v = v.insert16<6>((int)ptr2[ptr1[extract16<6>()]]);
		v = v.insert16<7>((int)ptr2[ptr1[extract16<7>()]]);

		return v;
	}

	template<int dst, class T> __forceinline GSVector4i gather16_32(const T* ptr, const GSVector4i& a) const
	{
		GSVector4i v = a;

		v = v.insert16<dst + 0>((int)ptr[extract32<0>()]);
		v = v.insert16<dst + 1>((int)ptr[extract32<1>()]);
		v = v.insert16<dst + 2>((int)ptr[extract32<2>()]);
		v = v.insert16<dst + 3>((int)ptr[extract32<3>()]);

		return v;
	}

	#if _M_SSE >= 0x401

	template<int src, class T> __forceinline GSVector4i gather32_4(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert32<1>((int)ptr[extract8<src + 0>() >> 4]);
		v = v.insert32<2>((int)ptr[extract8<src + 1>() & 0xf]);
		v = v.insert32<3>((int)ptr[extract8<src + 1>() >> 4]);
		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather32_8(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<src + 0>()]);
		v = v.insert32<1>((int)ptr[extract8<src + 1>()]);
		v = v.insert32<2>((int)ptr[extract8<src + 2>()]);
		v = v.insert32<3>((int)ptr[extract8<src + 3>()]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather32_16(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract16<src + 0>()]);
		v = v.insert32<1>((int)ptr[extract16<src + 1>()]);
		v = v.insert32<2>((int)ptr[extract16<src + 2>()]);
		v = v.insert32<3>((int)ptr[extract16<src + 3>()]);

		return v;
	}

	template<class T> __forceinline GSVector4i gather32_32(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract32<0>()]);
		v = v.insert32<1>((int)ptr[extract32<1>()]);
		v = v.insert32<2>((int)ptr[extract32<2>()]);
		v = v.insert32<3>((int)ptr[extract32<3>()]);

		return v;
	}

	template<class T1, class T2> __forceinline GSVector4i gather32_32(const T1* ptr1, const T2* ptr2) const
	{
		GSVector4i v;

		v = load((int)ptr2[ptr1[extract32<0>()]]);
		v = v.insert32<1>((int)ptr2[ptr1[extract32<1>()]]);
		v = v.insert32<2>((int)ptr2[ptr1[extract32<2>()]]);
		v = v.insert32<3>((int)ptr2[ptr1[extract32<3>()]]);

		return v;
	}

	#else

	template<int src, class T> __forceinline GSVector4i gather32_4(const T* ptr) const
	{
		return GSVector4i(
			(int)ptr[extract8<src + 0>() & 0xf],
			(int)ptr[extract8<src + 0>() >> 4],
			(int)ptr[extract8<src + 1>() & 0xf],
			(int)ptr[extract8<src + 1>() >> 4]);
	}

	template<int src, class T> __forceinline GSVector4i gather32_8(const T* ptr) const
	{
		return GSVector4i(
			(int)ptr[extract8<src + 0>()], 
			(int)ptr[extract8<src + 1>()],
			(int)ptr[extract8<src + 2>()],
			(int)ptr[extract8<src + 3>()]);
	}

	template<int src, class T> __forceinline GSVector4i gather32_16(const T* ptr) const
	{
		return GSVector4i(
			(int)ptr[extract16<src + 0>()],
			(int)ptr[extract16<src + 1>()],
			(int)ptr[extract16<src + 2>()],
			(int)ptr[extract16<src + 3>()]);
	}

	template<class T> __forceinline GSVector4i gather32_32(const T* ptr) const
	{
		return GSVector4i(
			(int)ptr[extract32<0>()], 
			(int)ptr[extract32<1>()], 
			(int)ptr[extract32<2>()],
			(int)ptr[extract32<3>()]);
	}

	template<class T1, class T2> __forceinline GSVector4i gather32_32(const T1* ptr1, const T2* ptr2) const
	{
		return GSVector4i(
			(int)ptr2[ptr1[extract32<0>()]], 
			(int)ptr2[ptr1[extract32<1>()]],
			(int)ptr2[ptr1[extract32<2>()]],
			(int)ptr2[ptr1[extract32<3>()]]);
	}

	#endif

	#if defined(_M_AMD64) && _M_SSE >= 0x401

	template<int src, class T> __forceinline GSVector4i gather64_4(const T* ptr) const
	{
		GSVector4i v;

		v = loadq((__int64)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert64<1>((__int64)ptr[extract8<src + 0>() >> 4]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_8(const T* ptr) const
	{
		GSVector4i v;

		v = loadq((__int64)ptr[extract8<src + 0>()]);
		v = v.insert64<1>((__int64)ptr[extract8<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_16(const T* ptr) const
	{
		GSVector4i v;

		v = loadq((__int64)ptr[extract16<src + 0>()]);
		v = v.insert64<1>((__int64)ptr[extract16<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_32(const T* ptr) const
	{
		GSVector4i v;

		v = loadq((__int64)ptr[extract32<src + 0>()]);
		v = v.insert64<1>((__int64)ptr[extract32<src + 1>()]);

		return v;
	}

	template<class T> __forceinline GSVector4i gather64_64(const T* ptr) const
	{
		GSVector4i v;

		v = loadq((__int64)ptr[extract64<0>()]);
		v = v.insert64<1>((__int64)ptr[extract64<1>()]);

		return v;
	}

	#else

	template<int src, class T> __forceinline GSVector4i gather64_4(const T* ptr) const
	{
		GSVector4i v;

		v = loadu(&ptr[extract8<src + 0>() & 0xf], &ptr[extract8<src + 0>() >> 4]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_8(const T* ptr) const
	{
		GSVector4i v;

		v = load(&ptr[extract8<src + 0>()], &ptr[extract8<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_16(const T* ptr) const
	{
		GSVector4i v;

		v = load(&ptr[extract16<src + 0>()], &ptr[extract16<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_32(const T* ptr) const
	{
		GSVector4i v;

		v = load(&ptr[extract32<src + 0>()], &ptr[extract32<src + 1>()]);

		return v;
	}

	#endif

	#if _M_SSE >= 0x401

	template<class T> __forceinline void gather8_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather8_4<0>(ptr);
		dst[1] = gather8_4<8>(ptr);
	}

	__forceinline void gather8_8(const uint8* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather8_8<>(ptr);
	}

	#endif

	template<class T> __forceinline void gather16_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather16_4<0>(ptr);
		dst[1] = gather16_4<4>(ptr);
		dst[2] = gather16_4<8>(ptr);
		dst[3] = gather16_4<12>(ptr);
	}

	template<class T> __forceinline void gather16_8(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather16_8<0>(ptr);
		dst[1] = gather16_8<8>(ptr);
	}

	template<class T> __forceinline void gather16_16(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather16_16<>(ptr);
	}

	template<class T> __forceinline void gather32_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather32_4<0>(ptr);
		dst[1] = gather32_4<2>(ptr);
		dst[2] = gather32_4<4>(ptr);
		dst[3] = gather32_4<6>(ptr);
		dst[4] = gather32_4<8>(ptr);
		dst[5] = gather32_4<10>(ptr);
		dst[6] = gather32_4<12>(ptr);
		dst[7] = gather32_4<14>(ptr);
	}

	template<class T> __forceinline void gather32_8(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather32_8<0>(ptr);
		dst[1] = gather32_8<4>(ptr);
		dst[2] = gather32_8<8>(ptr);
		dst[3] = gather32_8<12>(ptr);
	}

	template<class T> __forceinline void gather32_16(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather32_16<0>(ptr);
		dst[1] = gather32_16<4>(ptr);
	}

	template<class T> __forceinline void gather32_32(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather32_32<>(ptr);
	}

	template<class T> __forceinline void gather64_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather64_4<0>(ptr);
		dst[1] = gather64_4<1>(ptr);
		dst[2] = gather64_4<2>(ptr);
		dst[3] = gather64_4<3>(ptr);
		dst[4] = gather64_4<4>(ptr);
		dst[5] = gather64_4<5>(ptr);
		dst[6] = gather64_4<6>(ptr);
		dst[7] = gather64_4<7>(ptr);
		dst[8] = gather64_4<8>(ptr);
		dst[9] = gather64_4<9>(ptr);
		dst[10] = gather64_4<10>(ptr);
		dst[11] = gather64_4<11>(ptr);
		dst[12] = gather64_4<12>(ptr);
		dst[13] = gather64_4<13>(ptr);
		dst[14] = gather64_4<14>(ptr);
		dst[15] = gather64_4<15>(ptr);
	}

	template<class T> __forceinline void gather64_8(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather64_8<0>(ptr);
		dst[1] = gather64_8<2>(ptr);
		dst[2] = gather64_8<4>(ptr);
		dst[3] = gather64_8<6>(ptr);
		dst[4] = gather64_8<8>(ptr);
		dst[5] = gather64_8<10>(ptr);
		dst[6] = gather64_8<12>(ptr);
		dst[7] = gather64_8<14>(ptr);
	}

	template<class T> __forceinline void gather64_16(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather64_16<0>(ptr);
		dst[1] = gather64_16<2>(ptr);
		dst[2] = gather64_16<4>(ptr);
		dst[3] = gather64_16<8>(ptr);
	}

	template<class T> __forceinline void gather64_32(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather64_32<0>(ptr);
		dst[1] = gather64_32<2>(ptr);
	}

	#ifdef _M_AMD64

	template<class T> __forceinline void gather64_64(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather64_64<>(ptr);
	}

	#endif

	#if _M_SSE >= 0x401

	static GSVector4i loadnt(const void* p)
	{
		return GSVector4i(_mm_stream_load_si128((__m128i*)p));
	}

	#endif

	static GSVector4i loadl(const void* p)
	{
		return GSVector4i(_mm_loadl_epi64((__m128i*)p));
	}

	static GSVector4i loadh(const void* p)
	{
		return GSVector4i(_mm_castps_si128(_mm_loadh_pi(_mm_setzero_ps(), (__m64*)p)));
	}

	static GSVector4i loadh(const void* p, const GSVector4i& v)
	{
		return GSVector4i(_mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(v.m), (__m64*)p)));
	}

	static GSVector4i load(const void* pl, const void* ph)
	{
		return loadh(ph, loadl(pl));
	}
/*
	static GSVector4i load(const void* pl, const void* ph)
	{
		__m128i lo = _mm_loadl_epi64((__m128i*)pl);
		__m128i hi = _mm_loadl_epi64((__m128i*)ph);

		return GSVector4i(_mm_unpacklo_epi64(lo, hi));
	}
*/
	template<bool aligned> static GSVector4i load(const void* p)
	{
		return GSVector4i(aligned ? _mm_load_si128((__m128i*)p) : _mm_loadu_si128((__m128i*)p));
	}

	static GSVector4i load(int i)
	{
		return GSVector4i(_mm_cvtsi32_si128(i));
	}

	#ifdef _M_AMD64

	static GSVector4i loadq(__int64 i)
	{
		return GSVector4i(_mm_cvtsi64_si128(i));
	}

	#endif

	static void storent(void* p, const GSVector4i& v)
	{
		_mm_stream_si128((__m128i*)p, v.m);
	}

	static void storel(void* p, const GSVector4i& v)
	{
		_mm_storel_epi64((__m128i*)p, v.m);
	}

	static void storeh(void* p, const GSVector4i& v)
	{
		_mm_storeh_pi((__m64*)p, _mm_castsi128_ps(v.m));
	}

	static void store(void* pl, void* ph, const GSVector4i& v)
	{
		GSVector4i::storel(pl, v);
		GSVector4i::storeh(ph, v);
	}

	template<bool aligned> static void store(void* p, const GSVector4i& v)
	{
		if(aligned) _mm_store_si128((__m128i*)p, v.m);
		else _mm_storeu_si128((__m128i*)p, v.m);
	}

	static int store(const GSVector4i& v)
	{
		return _mm_cvtsi128_si32(v.m);
	}

	#ifdef _M_AMD64

	static __int64 storeq(const GSVector4i& v)
	{
		return _mm_cvtsi128_si64(v.m);
	}

	#endif

	__forceinline static void transpose(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		_MM_TRANSPOSE4_SI128(a.m, b.m, c.m, d.m);
	}

	__forceinline static void sw4(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		const __m128i epi32_0f0f0f0f = _mm_set1_epi32(0x0f0f0f0f);

		GSVector4i mask(epi32_0f0f0f0f);

		GSVector4i e = (b << 4).blend(a, mask);
		GSVector4i f = b.blend(a >> 4, mask);
		GSVector4i g = (d << 4).blend(c, mask);
		GSVector4i h = d.blend(c >> 4, mask);

		a = e.upl8(f);
		c = e.uph8(f);
		b = g.upl8(h);
		d = g.uph8(h);
	}

	__forceinline static void sw8(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = e.upl8(b);
		c = e.uph8(b);
		b = f.upl8(d);
		d = f.uph8(d);
	}

	__forceinline static void sw16(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = e.upl16(b);
		c = e.uph16(b);
		b = f.upl16(d);
		d = f.uph16(d);
	}

	__forceinline static void sw16rl(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = b.upl16(e);
		c = e.uph16(b);
		b = d.upl16(f);
		d = f.uph16(d);
	}

	__forceinline static void sw16rh(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = e.upl16(b);
		c = b.uph16(e);
		b = f.upl16(d);
		d = d.uph16(f);
	}

	__forceinline static void sw32(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = e.upl32(b);
		c = e.uph32(b);
		b = f.upl32(d);
		d = f.uph32(d);
	}

	__forceinline static void sw64(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = e.upl64(b);
		c = e.uph64(b);
		b = f.upl64(d);
		d = f.uph64(d);
	}

	__forceinline static bool compare(const void* dst, const void* src, int size)
	{
		ASSERT((size & 15) == 0);

		size >>= 4;

		GSVector4i* s = (GSVector4i*)src;
		GSVector4i* d = (GSVector4i*)dst;

		GSVector4i v = GSVector4i::xffffffff();

		for(int i = 0; i < size; i++)
		{
			v &= d[i] == s[i];
		}

		return v.alltrue();
	}

	__forceinline static bool update(const void* dst, const void* src, int size)
	{
		ASSERT((size & 15) == 0);

		size >>= 4;

		GSVector4i* s = (GSVector4i*)src;
		GSVector4i* d = (GSVector4i*)dst;

		GSVector4i v = GSVector4i::xffffffff();

		for(int i = 0; i < size; i++)
		{
			v &= d[i] == s[i];

			d[i] = s[i];
		}

		return v.alltrue();
	}

	void operator += (const GSVector4i& v) 
	{
		m = _mm_add_epi32(m, v);
	}

	void operator -= (const GSVector4i& v) 
	{
		m = _mm_sub_epi32(m, v);
	}

	void operator += (int i) 
	{
		*this += GSVector4i(i);
	}

	void operator -= (int i) 
	{
		*this -= GSVector4i(i);
	}

	void operator <<= (const int i) 
	{
		m = _mm_slli_epi32(m, i);
	}

	void operator >>= (const int i) 
	{
		m = _mm_srli_epi32(m, i);
	}

	void operator &= (const GSVector4i& v)
	{
		m = _mm_and_si128(m, v);
	}

	void operator |= (const GSVector4i& v) 
	{
		m = _mm_or_si128(m, v);
	}

	void operator ^= (const GSVector4i& v) 
	{
		m = _mm_xor_si128(m, v);
	}

	friend GSVector4i operator + (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_add_epi32(v1, v2));
	}

	friend GSVector4i operator - (const GSVector4i& v1, const GSVector4i& v2) 
	{
		return GSVector4i(_mm_sub_epi32(v1, v2));
	}

	friend GSVector4i operator + (const GSVector4i& v, int i)
	{
		return v + GSVector4i(i);
	}
	
	friend GSVector4i operator - (const GSVector4i& v, int i)
	{
		return v - GSVector4i(i);
	}
	
	friend GSVector4i operator << (const GSVector4i& v, const int i) 
	{
		return GSVector4i(_mm_slli_epi32(v, i));
	}
	
	friend GSVector4i operator >> (const GSVector4i& v, const int i) 
	{
		return GSVector4i(_mm_srli_epi32(v, i));
	}
	
	friend GSVector4i operator & (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_and_si128(v1, v2));
	}
	
	friend GSVector4i operator | (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_or_si128(v1, v2));
	}
	
	friend GSVector4i operator ^ (const GSVector4i& v1, const GSVector4i& v2) 
	{
		return GSVector4i(_mm_xor_si128(v1, v2));
	}
	
	friend GSVector4i operator & (const GSVector4i& v, int i) 
	{
		return v & GSVector4i(i);
	}
	
	friend GSVector4i operator | (const GSVector4i& v, int i) 
	{
		return v | GSVector4i(i);
	}
	
	friend GSVector4i operator ^ (const GSVector4i& v, int i) 
	{
		return v ^ GSVector4i(i);
	}
	
	friend GSVector4i operator ~ (const GSVector4i& v) 
	{
		return v ^ (v == v);
	}

	friend GSVector4i operator == (const GSVector4i& v1, const GSVector4i& v2) 
	{
		return GSVector4i(_mm_cmpeq_epi32(v1, v2));
	}
	
	friend GSVector4i operator != (const GSVector4i& v1, const GSVector4i& v2) 
	{
		return ~(v1 == v2);
	}
	
	friend GSVector4i operator > (const GSVector4i& v1, const GSVector4i& v2) 
	{
		return GSVector4i(_mm_cmpgt_epi32(v1, v2));
	}
	
	friend GSVector4i operator < (const GSVector4i& v1, const GSVector4i& v2) 
	{
		return GSVector4i(_mm_cmplt_epi32(v1, v2));
	}
	
	friend GSVector4i operator >= (const GSVector4i& v1, const GSVector4i& v2) 
	{
		return (v1 > v2) | (v1 == v2);
	}
	
	friend GSVector4i operator <= (const GSVector4i& v1, const GSVector4i& v2) 
	{
		return (v1 < v2) | (v1 == v2);
	}

	template<int i> GSVector4i shuffle() const
	{
		return GSVector4i(_mm_shuffle_epi32(m, _MM_SHUFFLE(i, i, i, i)));
	}

	#define VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
		GSVector4i xs##ys##zs##ws() const {return GSVector4i(_mm_shuffle_epi32(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		GSVector4i xs##ys##zs##ws##l() const {return GSVector4i(_mm_shufflelo_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		GSVector4i xs##ys##zs##ws##h() const {return GSVector4i(_mm_shufflehi_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		GSVector4i xs##ys##zs##ws##lh() const {return GSVector4i(_mm_shufflehi_epi16(_mm_shufflelo_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)), _MM_SHUFFLE(wn, zn, yn, xn)));} \

	#define VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, zs, zn) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, x, 0) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, y, 1) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, z, 2) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, w, 3) \

	#define VECTOR4i_SHUFFLE_2(xs, xn, ys, yn) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, x, 0) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, y, 1) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, z, 2) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, w, 3) \

	#define VECTOR4i_SHUFFLE_1(xs, xn) \
		GSVector4i xs##4##() const {return GSVector4i(_mm_shuffle_epi32(m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
		GSVector4i xs##4##l() const {return GSVector4i(_mm_shufflelo_epi16(m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
		GSVector4i xs##4##h() const {return GSVector4i(_mm_shufflehi_epi16(m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
		VECTOR4i_SHUFFLE_2(xs, xn, x, 0) \
		VECTOR4i_SHUFFLE_2(xs, xn, y, 1) \
		VECTOR4i_SHUFFLE_2(xs, xn, z, 2) \
		VECTOR4i_SHUFFLE_2(xs, xn, w, 3) \

	VECTOR4i_SHUFFLE_1(x, 0)
	VECTOR4i_SHUFFLE_1(y, 1)
	VECTOR4i_SHUFFLE_1(z, 2)
	VECTOR4i_SHUFFLE_1(w, 3)

	static GSVector4i zero() {return GSVector4i(_mm_setzero_si128());}

	static GSVector4i xffffffff() {return zero() == zero();}

	static GSVector4i x00000001() {return xffffffff().srl32(31);}
	static GSVector4i x00000003() {return xffffffff().srl32(30);}
	static GSVector4i x00000007() {return xffffffff().srl32(29);}
	static GSVector4i x0000000f() {return xffffffff().srl32(28);}
	static GSVector4i x0000001f() {return xffffffff().srl32(27);}
	static GSVector4i x0000003f() {return xffffffff().srl32(26);}
	static GSVector4i x0000007f() {return xffffffff().srl32(25);}
	static GSVector4i x000000ff() {return xffffffff().srl32(24);}
	static GSVector4i x000001ff() {return xffffffff().srl32(23);}
	static GSVector4i x000003ff() {return xffffffff().srl32(22);}
	static GSVector4i x000007ff() {return xffffffff().srl32(21);}
	static GSVector4i x00000fff() {return xffffffff().srl32(20);}
	static GSVector4i x00001fff() {return xffffffff().srl32(19);}
	static GSVector4i x00003fff() {return xffffffff().srl32(18);}
	static GSVector4i x00007fff() {return xffffffff().srl32(17);}
	static GSVector4i x0000ffff() {return xffffffff().srl32(16);}
	static GSVector4i x0001ffff() {return xffffffff().srl32(15);}
	static GSVector4i x0003ffff() {return xffffffff().srl32(14);}
	static GSVector4i x0007ffff() {return xffffffff().srl32(13);}
	static GSVector4i x000fffff() {return xffffffff().srl32(12);}
	static GSVector4i x001fffff() {return xffffffff().srl32(11);}
	static GSVector4i x003fffff() {return xffffffff().srl32(10);}
	static GSVector4i x007fffff() {return xffffffff().srl32( 9);}
	static GSVector4i x00ffffff() {return xffffffff().srl32( 8);}
	static GSVector4i x01ffffff() {return xffffffff().srl32( 7);}
	static GSVector4i x03ffffff() {return xffffffff().srl32( 6);}
	static GSVector4i x07ffffff() {return xffffffff().srl32( 5);}
	static GSVector4i x0fffffff() {return xffffffff().srl32( 4);}
	static GSVector4i x1fffffff() {return xffffffff().srl32( 3);}
	static GSVector4i x3fffffff() {return xffffffff().srl32( 2);}
	static GSVector4i x7fffffff() {return xffffffff().srl32( 1);}

	static GSVector4i x80000000() {return xffffffff().sll32(31);}
	static GSVector4i xc0000000() {return xffffffff().sll32(30);}
	static GSVector4i xe0000000() {return xffffffff().sll32(29);}
	static GSVector4i xf0000000() {return xffffffff().sll32(28);}
	static GSVector4i xf8000000() {return xffffffff().sll32(27);}
	static GSVector4i xfc000000() {return xffffffff().sll32(26);}
	static GSVector4i xfe000000() {return xffffffff().sll32(25);}
	static GSVector4i xff000000() {return xffffffff().sll32(24);}
	static GSVector4i xff800000() {return xffffffff().sll32(23);}
	static GSVector4i xffc00000() {return xffffffff().sll32(22);}
	static GSVector4i xffe00000() {return xffffffff().sll32(21);}
	static GSVector4i xfff00000() {return xffffffff().sll32(20);}
	static GSVector4i xfff80000() {return xffffffff().sll32(19);}
	static GSVector4i xfffc0000() {return xffffffff().sll32(18);}
	static GSVector4i xfffe0000() {return xffffffff().sll32(17);}
	static GSVector4i xffff0000() {return xffffffff().sll32(16);}
	static GSVector4i xffff8000() {return xffffffff().sll32(15);}
	static GSVector4i xffffc000() {return xffffffff().sll32(14);}
	static GSVector4i xffffe000() {return xffffffff().sll32(13);}
	static GSVector4i xfffff000() {return xffffffff().sll32(12);}
	static GSVector4i xfffff800() {return xffffffff().sll32(11);}
	static GSVector4i xfffffc00() {return xffffffff().sll32(10);}
	static GSVector4i xfffffe00() {return xffffffff().sll32( 9);}
	static GSVector4i xffffff00() {return xffffffff().sll32( 8);}
	static GSVector4i xffffff80() {return xffffffff().sll32( 7);}
	static GSVector4i xffffffc0() {return xffffffff().sll32( 6);}
	static GSVector4i xffffffe0() {return xffffffff().sll32( 5);}
	static GSVector4i xfffffff0() {return xffffffff().sll32( 4);}
	static GSVector4i xfffffff8() {return xffffffff().sll32( 3);}
	static GSVector4i xfffffffc() {return xffffffff().sll32( 2);}
	static GSVector4i xfffffffe() {return xffffffff().sll32( 1);}

	static GSVector4i x0001() {return xffffffff().srl16(15);}
	static GSVector4i x0003() {return xffffffff().srl16(14);}
	static GSVector4i x0007() {return xffffffff().srl16(13);}
	static GSVector4i x000f() {return xffffffff().srl16(12);}
	static GSVector4i x001f() {return xffffffff().srl16(11);}
	static GSVector4i x003f() {return xffffffff().srl16(10);}
	static GSVector4i x007f() {return xffffffff().srl16( 9);}
	static GSVector4i x00ff() {return xffffffff().srl16( 8);}
	static GSVector4i x01ff() {return xffffffff().srl16( 7);}
	static GSVector4i x03ff() {return xffffffff().srl16( 6);}
	static GSVector4i x07ff() {return xffffffff().srl16( 5);}
	static GSVector4i x0fff() {return xffffffff().srl16( 4);}
	static GSVector4i x1fff() {return xffffffff().srl16( 3);}
	static GSVector4i x3fff() {return xffffffff().srl16( 2);}
	static GSVector4i x7fff() {return xffffffff().srl16( 1);}

	static GSVector4i x8000() {return xffffffff().sll16(15);}
	static GSVector4i xc000() {return xffffffff().sll16(14);}
	static GSVector4i xe000() {return xffffffff().sll16(13);}
	static GSVector4i xf000() {return xffffffff().sll16(12);}
	static GSVector4i xf800() {return xffffffff().sll16(11);}
	static GSVector4i xfc00() {return xffffffff().sll16(10);}
	static GSVector4i xfe00() {return xffffffff().sll16( 9);}
	static GSVector4i xff00() {return xffffffff().sll16( 8);}
	static GSVector4i xff80() {return xffffffff().sll16( 7);}
	static GSVector4i xffc0() {return xffffffff().sll16( 6);}
	static GSVector4i xffe0() {return xffffffff().sll16( 5);}
	static GSVector4i xfff0() {return xffffffff().sll16( 4);}
	static GSVector4i xfff8() {return xffffffff().sll16( 3);}
	static GSVector4i xfffc() {return xffffffff().sll16( 2);}
	static GSVector4i xfffe() {return xffffffff().sll16( 1);}

	static GSVector4i xffffffff(const GSVector4i& v) {return v == v;}

	static GSVector4i x00000001(const GSVector4i& v) {return xffffffff(v).srl32(31);}
	static GSVector4i x00000003(const GSVector4i& v) {return xffffffff(v).srl32(30);}
	static GSVector4i x00000007(const GSVector4i& v) {return xffffffff(v).srl32(29);}
	static GSVector4i x0000000f(const GSVector4i& v) {return xffffffff(v).srl32(28);}
	static GSVector4i x0000001f(const GSVector4i& v) {return xffffffff(v).srl32(27);}
	static GSVector4i x0000003f(const GSVector4i& v) {return xffffffff(v).srl32(26);}
	static GSVector4i x0000007f(const GSVector4i& v) {return xffffffff(v).srl32(25);}
	static GSVector4i x000000ff(const GSVector4i& v) {return xffffffff(v).srl32(24);}
	static GSVector4i x000001ff(const GSVector4i& v) {return xffffffff(v).srl32(23);}
	static GSVector4i x000003ff(const GSVector4i& v) {return xffffffff(v).srl32(22);}
	static GSVector4i x000007ff(const GSVector4i& v) {return xffffffff(v).srl32(21);}
	static GSVector4i x00000fff(const GSVector4i& v) {return xffffffff(v).srl32(20);}
	static GSVector4i x00001fff(const GSVector4i& v) {return xffffffff(v).srl32(19);}
	static GSVector4i x00003fff(const GSVector4i& v) {return xffffffff(v).srl32(18);}
	static GSVector4i x00007fff(const GSVector4i& v) {return xffffffff(v).srl32(17);}
	static GSVector4i x0000ffff(const GSVector4i& v) {return xffffffff(v).srl32(16);}
	static GSVector4i x0001ffff(const GSVector4i& v) {return xffffffff(v).srl32(15);}
	static GSVector4i x0003ffff(const GSVector4i& v) {return xffffffff(v).srl32(14);}
	static GSVector4i x0007ffff(const GSVector4i& v) {return xffffffff(v).srl32(13);}
	static GSVector4i x000fffff(const GSVector4i& v) {return xffffffff(v).srl32(12);}
	static GSVector4i x001fffff(const GSVector4i& v) {return xffffffff(v).srl32(11);}
	static GSVector4i x003fffff(const GSVector4i& v) {return xffffffff(v).srl32(10);}
	static GSVector4i x007fffff(const GSVector4i& v) {return xffffffff(v).srl32( 9);}
	static GSVector4i x00ffffff(const GSVector4i& v) {return xffffffff(v).srl32( 8);}
	static GSVector4i x01ffffff(const GSVector4i& v) {return xffffffff(v).srl32( 7);}
	static GSVector4i x03ffffff(const GSVector4i& v) {return xffffffff(v).srl32( 6);}
	static GSVector4i x07ffffff(const GSVector4i& v) {return xffffffff(v).srl32( 5);}
	static GSVector4i x0fffffff(const GSVector4i& v) {return xffffffff(v).srl32( 4);}
	static GSVector4i x1fffffff(const GSVector4i& v) {return xffffffff(v).srl32( 3);}
	static GSVector4i x3fffffff(const GSVector4i& v) {return xffffffff(v).srl32( 2);}
	static GSVector4i x7fffffff(const GSVector4i& v) {return xffffffff(v).srl32( 1);}

	static GSVector4i x80000000(const GSVector4i& v) {return xffffffff(v).sll32(31);}
	static GSVector4i xc0000000(const GSVector4i& v) {return xffffffff(v).sll32(30);}
	static GSVector4i xe0000000(const GSVector4i& v) {return xffffffff(v).sll32(29);}
	static GSVector4i xf0000000(const GSVector4i& v) {return xffffffff(v).sll32(28);}
	static GSVector4i xf8000000(const GSVector4i& v) {return xffffffff(v).sll32(27);}
	static GSVector4i xfc000000(const GSVector4i& v) {return xffffffff(v).sll32(26);}
	static GSVector4i xfe000000(const GSVector4i& v) {return xffffffff(v).sll32(25);}
	static GSVector4i xff000000(const GSVector4i& v) {return xffffffff(v).sll32(24);}
	static GSVector4i xff800000(const GSVector4i& v) {return xffffffff(v).sll32(23);}
	static GSVector4i xffc00000(const GSVector4i& v) {return xffffffff(v).sll32(22);}
	static GSVector4i xffe00000(const GSVector4i& v) {return xffffffff(v).sll32(21);}
	static GSVector4i xfff00000(const GSVector4i& v) {return xffffffff(v).sll32(20);}
	static GSVector4i xfff80000(const GSVector4i& v) {return xffffffff(v).sll32(19);}
	static GSVector4i xfffc0000(const GSVector4i& v) {return xffffffff(v).sll32(18);}
	static GSVector4i xfffe0000(const GSVector4i& v) {return xffffffff(v).sll32(17);}
	static GSVector4i xffff0000(const GSVector4i& v) {return xffffffff(v).sll32(16);}
	static GSVector4i xffff8000(const GSVector4i& v) {return xffffffff(v).sll32(15);}
	static GSVector4i xffffc000(const GSVector4i& v) {return xffffffff(v).sll32(14);}
	static GSVector4i xffffe000(const GSVector4i& v) {return xffffffff(v).sll32(13);}
	static GSVector4i xfffff000(const GSVector4i& v) {return xffffffff(v).sll32(12);}
	static GSVector4i xfffff800(const GSVector4i& v) {return xffffffff(v).sll32(11);}
	static GSVector4i xfffffc00(const GSVector4i& v) {return xffffffff(v).sll32(10);}
	static GSVector4i xfffffe00(const GSVector4i& v) {return xffffffff(v).sll32( 9);}
	static GSVector4i xffffff00(const GSVector4i& v) {return xffffffff(v).sll32( 8);}
	static GSVector4i xffffff80(const GSVector4i& v) {return xffffffff(v).sll32( 7);}
	static GSVector4i xffffffc0(const GSVector4i& v) {return xffffffff(v).sll32( 6);}
	static GSVector4i xffffffe0(const GSVector4i& v) {return xffffffff(v).sll32( 5);}
	static GSVector4i xfffffff0(const GSVector4i& v) {return xffffffff(v).sll32( 4);}
	static GSVector4i xfffffff8(const GSVector4i& v) {return xffffffff(v).sll32( 3);}
	static GSVector4i xfffffffc(const GSVector4i& v) {return xffffffff(v).sll32( 2);}
	static GSVector4i xfffffffe(const GSVector4i& v) {return xffffffff(v).sll32( 1);}

	static GSVector4i x0001(const GSVector4i& v) {return xffffffff(v).srl16(15);}
	static GSVector4i x0003(const GSVector4i& v) {return xffffffff(v).srl16(14);}
	static GSVector4i x0007(const GSVector4i& v) {return xffffffff(v).srl16(13);}
	static GSVector4i x000f(const GSVector4i& v) {return xffffffff(v).srl16(12);}
	static GSVector4i x001f(const GSVector4i& v) {return xffffffff(v).srl16(11);}
	static GSVector4i x003f(const GSVector4i& v) {return xffffffff(v).srl16(10);}
	static GSVector4i x007f(const GSVector4i& v) {return xffffffff(v).srl16( 9);}
	static GSVector4i x00ff(const GSVector4i& v) {return xffffffff(v).srl16( 8);}
	static GSVector4i x01ff(const GSVector4i& v) {return xffffffff(v).srl16( 7);}
	static GSVector4i x03ff(const GSVector4i& v) {return xffffffff(v).srl16( 6);}
	static GSVector4i x07ff(const GSVector4i& v) {return xffffffff(v).srl16( 5);}
	static GSVector4i x0fff(const GSVector4i& v) {return xffffffff(v).srl16( 4);}
	static GSVector4i x1fff(const GSVector4i& v) {return xffffffff(v).srl16( 3);}
	static GSVector4i x3fff(const GSVector4i& v) {return xffffffff(v).srl16( 2);}
	static GSVector4i x7fff(const GSVector4i& v) {return xffffffff(v).srl16( 1);}

	static GSVector4i x8000(const GSVector4i& v) {return xffffffff(v).sll16(15);}
	static GSVector4i xc000(const GSVector4i& v) {return xffffffff(v).sll16(14);}
	static GSVector4i xe000(const GSVector4i& v) {return xffffffff(v).sll16(13);}
	static GSVector4i xf000(const GSVector4i& v) {return xffffffff(v).sll16(12);}
	static GSVector4i xf800(const GSVector4i& v) {return xffffffff(v).sll16(11);}
	static GSVector4i xfc00(const GSVector4i& v) {return xffffffff(v).sll16(10);}
	static GSVector4i xfe00(const GSVector4i& v) {return xffffffff(v).sll16( 9);}
	static GSVector4i xff00(const GSVector4i& v) {return xffffffff(v).sll16( 8);}
	static GSVector4i xff80(const GSVector4i& v) {return xffffffff(v).sll16( 7);}
	static GSVector4i xffc0(const GSVector4i& v) {return xffffffff(v).sll16( 6);}
	static GSVector4i xffe0(const GSVector4i& v) {return xffffffff(v).sll16( 5);}
	static GSVector4i xfff0(const GSVector4i& v) {return xffffffff(v).sll16( 4);}
	static GSVector4i xfff8(const GSVector4i& v) {return xffffffff(v).sll16( 3);}
	static GSVector4i xfffc(const GSVector4i& v) {return xffffffff(v).sll16( 2);}
	static GSVector4i xfffe(const GSVector4i& v) {return xffffffff(v).sll16( 1);}
};

__declspec(align(16)) class GSVector4
{
public:
	union 
	{
		struct {float x, y, z, w;}; 
		struct {float r, g, b, a;}; 
		struct {int left, top, right, bottom;}; 
		float v[4];
		float f32[4];
		int8 i8[16];
		int16 i16[8];
		int32 i32[4];
		int64 i64[2];
		uint8 u8[16];
		uint16 u16[8];
		uint32 u32[4];
		uint64 u64[2];
		__m128 m;
	};

	static const GSVector4 m_ps0123;
	static const GSVector4 m_ps4567;

	static const GSVector4 m_x3f800000;
	static const GSVector4 m_x4b000000;

	GSVector4()
	{
	}

	GSVector4(float x, float y, float z, float w)
	{
		m = _mm_set_ps(w, z, y, x);
	}

	GSVector4(float x, float y)
	{
		m = _mm_unpacklo_ps(_mm_load_ss(&x), _mm_load_ss(&y));
	}

	GSVector4(int x, int y, int z, int w)
	{
		m = _mm_cvtepi32_ps(_mm_set_epi32(w, z, y, x));
	}

	GSVector4(int x, int y)
	{
		m = _mm_cvtepi32_ps(_mm_unpacklo_epi32(_mm_cvtsi32_si128(x), _mm_cvtsi32_si128(y)));
	}

	GSVector4(const GSVector4& v) 
	{
		m = v.m;
	}

	explicit GSVector4(const GSVector2& v)
	{
		m = _mm_castsi128_ps(_mm_loadl_epi64((__m128i*)&v));
	}

	explicit GSVector4(float f)
	{
		m = _mm_set1_ps(f);
	}

	explicit GSVector4(__m128 m)
	{
		this->m = m;
	}

	explicit GSVector4(uint32 u32)
	{
		*this = GSVector4(GSVector4i::load((int)u32).u8to32());
	}

	explicit GSVector4(const GSVector4i& v)
	{
		*this = v;
	}

	void operator = (const GSVector4& v)
	{
		m = v.m;
	}

	void operator = (const GSVector4i& v);

	void operator = (float f)
	{
		m = _mm_set1_ps(f);
	}

	void operator = (__m128 m)
	{
		this->m = m;
	}

	void operator = (uint32 u32)
	{
		*this = GSVector4(GSVector4i::load((int)u32).u8to32());
	}

	operator __m128() const 
	{
		return m;
	}

	uint32 rgba32() const
	{
		return GSVector4i(*this).rgba32();
	}

	static GSVector4 cast(const GSVector4i& v);

	GSVector4 abs() const 
	{
		return *this & cast(GSVector4i::x7fffffff());
	}

	GSVector4 neg() const 
	{
		return *this ^ cast(GSVector4i::x80000000());
	}

	GSVector4 rcp() const 
	{
		return GSVector4(_mm_rcp_ps(m));
	}

	GSVector4 rcpnr() const 
	{
		GSVector4 v = rcp();

		return (v + v) - (v * v) * *this;
	}

	enum RoundMode {NearestInt = 8, NegInf = 9, PosInf = 10};

	template<int mode> GSVector4 round() const 
	{
		#if _M_SSE >= 0x401

		return GSVector4(_mm_round_ps(m, mode));

		#else

		GSVector4 a = *this;

		GSVector4 b = (a & cast(GSVector4i::x80000000())) | m_x4b000000;

		b = a + b - b;

		if((mode & 7) == (NegInf & 7))
		{
			return b - ((a < b) & m_x3f800000);
		}

		if((mode & 7) == (PosInf & 7))
		{
			return b + ((a > b) & m_x3f800000);
		}

		ASSERT((mode & 7) == (NearestInt & 7)); // other modes aren't implemented

		return b;

		#endif
	}

	GSVector4 floor() const 
	{
		return round<NegInf>();
	}

	GSVector4 ceil() const 
	{
		return round<PosInf>();
	}

	GSVector4 mod2x(const GSVector4& f, const int scale = 256) const 
	{
		return *this * (f * (2.0f / scale));
	}

	GSVector4 mod2x(float f, const int scale = 256) const 
	{
		return mod2x(GSVector4(f), scale);
	}

	GSVector4 madd(const GSVector4& a, const GSVector4& b) const
	{
		return *this * a + b; // TODO: _mm_fmadd_ps
	}

	GSVector4 msub(const GSVector4& a, const GSVector4& b) const
	{
		return *this * a + b; // TODO: _mm_fmsub_ps
	}

	GSVector4 nmadd(const GSVector4& a, const GSVector4& b) const
	{
		return b - *this * a; // TODO: _mm_fnmadd_ps
	}

	GSVector4 nmsub(const GSVector4& a, const GSVector4& b) const
	{
		return -b - *this * a; // TODO: _mm_fmnsub_ps
	}

	GSVector4 lerp(const GSVector4& v, const GSVector4& f) const 
	{
		return *this + (v - *this) * f;
	}

	GSVector4 lerp(const GSVector4& v, float f) const 
	{
		return lerp(v, GSVector4(f));
	}

	GSVector4 hadd() const
	{
		#if _M_SSE >= 0x300
		return GSVector4(_mm_hadd_ps(m, m));
		#else
		return xzxz() + ywyw();
		#endif
	}

	GSVector4 hadd(const GSVector4& v) const
	{
		#if _M_SSE >= 0x300
		return GSVector4(_mm_hadd_ps(m, v.m));
		#else
		return xzxz(v) + ywyw(v);
		#endif
	}

	#if _M_SSE >= 0x401
	template<int i> GSVector4 dp(const GSVector4& v) const 
	{
		return GSVector4(_mm_dp_ps(m, v.m, i));
	}
	#endif

	GSVector4 sat(const GSVector4& a, const GSVector4& b) const 
	{
		return GSVector4(_mm_min_ps(_mm_max_ps(m, a), b));
	}

	GSVector4 sat(const GSVector4& a) const 
	{
		return GSVector4(_mm_min_ps(_mm_max_ps(m, a.xyxy()), a.zwzw()));
	}

	GSVector4 sat(const float scale = 255) const 
	{
		return sat(zero(), GSVector4(scale));
	}

	GSVector4 clamp(const float scale = 255) const 
	{
		return minv(GSVector4(scale));
	}

	GSVector4 minv(const GSVector4& a) const
	{
		return GSVector4(_mm_min_ps(m, a));
	}

	GSVector4 maxv(const GSVector4& a) const
	{
		return GSVector4(_mm_max_ps(m, a));
	}

	GSVector4 blend8(const GSVector4& a, const GSVector4& mask)  const
	{
		#if _M_SSE >= 0x401

		return GSVector4(_mm_blendv_ps(m, a, mask));

		#else

		return GSVector4(_mm_or_ps(_mm_andnot_ps(mask, m), _mm_and_ps(mask, a)));

		#endif
	}

	GSVector4 upl(const GSVector4& a) const
	{
		return GSVector4(_mm_unpacklo_ps(m, a));
	}

	GSVector4 uph(const GSVector4& a) const
	{
		return GSVector4(_mm_unpackhi_ps(m, a));
	}

	GSVector4 l2h(const GSVector4& a) const
	{
		return GSVector4(_mm_movelh_ps(m, a));
	}	

	GSVector4 h2l(const GSVector4& a) const
	{
		return GSVector4(_mm_movehl_ps(m, a));
	}	

	GSVector4 andnot(const GSVector4& v) const
	{
		return GSVector4(_mm_andnot_ps(v.m, m));
	}

	int mask() const
	{
		return _mm_movemask_ps(m);
	}

	bool alltrue() const
	{
		return _mm_movemask_ps(m) == 0xf;
	}

	bool allfalse() const
	{
		return _mm_movemask_ps(m) == 0;
	}

	// TODO: insert

	template<int i> int extract() const
	{
		#if _M_SSE >= 0x401
		return _mm_extract_ps(m, i);
		#else
		return i32[i];
		#endif
	}

	static GSVector4 zero() 
	{
		return GSVector4(_mm_setzero_ps());
	}

	static GSVector4 xffffffff() 
	{
		return zero() == zero();
	}

	static GSVector4 ps0123()
	{
		return GSVector4(m_ps0123);
	}

	static GSVector4 ps4567()
	{
		return GSVector4(m_ps4567);
	}

	static GSVector4 loadl(const void* p)
	{
		return GSVector4(_mm_castpd_ps(_mm_load_sd((double*)p)));
	}

	static GSVector4 load(float f)
	{
		return GSVector4(_mm_load_ss(&f));
	}

	template<bool aligned> static GSVector4 load(const void* p)
	{
		return GSVector4i(aligned ? _mm_load_ps((__m128*)p) : _mm_loadu_ps((__m128*)p));
	}

	static void storel(void* p, const GSVector4& v)
	{
		_mm_store_sd((double*)p, _mm_castps_pd(v.m));
	}

	template<bool aligned> static void store(void* p, const GSVector4& v)
	{
		if(aligned) _mm_store_ps((__m128*)p, v.m);
		else _mm_storeu_ps((__m128*)p, v.m);
	}

	__forceinline static void expand(const GSVector4i& v, GSVector4& a, GSVector4& b, GSVector4& c, GSVector4& d)
	{
		GSVector4i mask = GSVector4i::x000000ff();

		a = v & mask;
		b = (v >> 8) & mask;
		c = (v >> 16) & mask;
		d = (v >> 24);
	}

	__forceinline static void transpose(GSVector4& a, GSVector4& b, GSVector4& c, GSVector4& d)
	{
		GSVector4 v0 = a.xyxy(b);
		GSVector4 v1 = c.xyxy(d);

		GSVector4 e = v0.xzxz(v1);
		GSVector4 f = v0.ywyw(v1);

		GSVector4 v2 = a.zwzw(b);
		GSVector4 v3 = c.zwzw(d);

		GSVector4 g = v2.xzxz(v3);
		GSVector4 h = v2.ywyw(v3);

		a = e;
		b = f;
		c = g;
		d = h;
/*
		GSVector4 v0 = a.xyxy(b);
		GSVector4 v1 = c.xyxy(d);
		GSVector4 v2 = a.zwzw(b);
		GSVector4 v3 = c.zwzw(d);

		a = v0.xzxz(v1);
		b = v0.ywyw(v1);
		c = v2.xzxz(v3);
		d = v2.ywyw(v3);
*/
/*
		GSVector4 v0 = a.upl(b);
		GSVector4 v1 = a.uph(b);
		GSVector4 v2 = c.upl(d);
		GSVector4 v3 = c.uph(d);

		a = v0.l2h(v2);
		b = v2.h2l(v0);
		c = v1.l2h(v3);
		d = v3.h2l(v1);
*/	}

	GSVector4 operator - () const
	{
		return neg();
	}

	void operator += (const GSVector4& v) 
	{
		m = _mm_add_ps(m, v);
	}

	void operator -= (const GSVector4& v) 
	{
		m = _mm_sub_ps(m, v);
	}

	void operator *= (const GSVector4& v) 
	{
		m = _mm_mul_ps(m, v);
	}

	void operator /= (const GSVector4& v) 
	{
		m = _mm_div_ps(m, v);
	}

	void operator += (float f) 
	{
		*this += GSVector4(f);
	}

	void operator -= (float f) 
	{
		*this -= GSVector4(f);
	}

	void operator *= (float f) 
	{
		*this *= GSVector4(f);
	}

	void operator /= (float f) 
	{
		*this /= GSVector4(f);
	}

	void operator &= (const GSVector4& v)
	{
		m = _mm_and_ps(m, v);
	}

	void operator |= (const GSVector4& v) 
	{
		m = _mm_or_ps(m, v);
	}

	void operator ^= (const GSVector4& v) 
	{
		m = _mm_xor_ps(m, v);
	}

	friend GSVector4 operator + (const GSVector4& v1, const GSVector4& v2) 
	{
		return GSVector4(_mm_add_ps(v1, v2));
	}

	friend GSVector4 operator - (const GSVector4& v1, const GSVector4& v2) 
	{
		return GSVector4(_mm_sub_ps(v1, v2));
	}

	friend GSVector4 operator * (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_mul_ps(v1, v2));
	}

	friend GSVector4 operator / (const GSVector4& v1, const GSVector4& v2) 
	{
		return GSVector4(_mm_div_ps(v1, v2));
	}

	friend GSVector4 operator + (const GSVector4& v, float f) 
	{
		return v + GSVector4(f);
	}

	friend GSVector4 operator - (const GSVector4& v, float f) 
	{
		return v - GSVector4(f);
	}

	friend GSVector4 operator * (const GSVector4& v, float f) 
	{
		return v * GSVector4(f);
	}

	friend GSVector4 operator / (const GSVector4& v, float f) 
	{
		return v / GSVector4(f);
	}

	friend GSVector4 operator & (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_and_ps(v1, v2));
	}
	
	friend GSVector4 operator | (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_or_ps(v1, v2));
	}
	
	friend GSVector4 operator ^ (const GSVector4& v1, const GSVector4& v2) 
	{
		return GSVector4(_mm_xor_ps(v1, v2));
	}

	friend GSVector4 operator == (const GSVector4& v1, const GSVector4& v2) 
	{
		return GSVector4(_mm_cmpeq_ps(v1, v2));
	}

	friend GSVector4 operator != (const GSVector4& v1, const GSVector4& v2) 
	{
		return GSVector4(_mm_cmpneq_ps(v1, v2));
	}

	friend GSVector4 operator > (const GSVector4& v1, const GSVector4& v2) 
	{
		return GSVector4(_mm_cmpgt_ps(v1, v2));
	}

	friend GSVector4 operator < (const GSVector4& v1, const GSVector4& v2) 
	{
		return GSVector4(_mm_cmplt_ps(v1, v2));
	}

	friend GSVector4 operator >= (const GSVector4& v1, const GSVector4& v2) 
	{
		return GSVector4(_mm_cmpge_ps(v1, v2));
	}

	friend GSVector4 operator <= (const GSVector4& v1, const GSVector4& v2) 
	{
		return GSVector4(_mm_cmple_ps(v1, v2));
	}

	template<int i> GSVector4 shuffle() const
	{
		return GSVector4(_mm_shuffle_ps(m, m, _MM_SHUFFLE(i, i, i, i)));
	}

	#define VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
		GSVector4 xs##ys##zs##ws() const {return GSVector4(_mm_shuffle_ps(m, m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		GSVector4 xs##ys##zs##ws(const GSVector4& v) const {return GSVector4(_mm_shuffle_ps(m, v.m, _MM_SHUFFLE(wn, zn, yn, xn)));} \

	#define VECTOR4_SHUFFLE_3(xs, xn, ys, yn, zs, zn) \
		VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, x, 0) \
		VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, y, 1) \
		VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, z, 2) \
		VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, w, 3) \

	#define VECTOR4_SHUFFLE_2(xs, xn, ys, yn) \
		VECTOR4_SHUFFLE_3(xs, xn, ys, yn, x, 0) \
		VECTOR4_SHUFFLE_3(xs, xn, ys, yn, y, 1) \
		VECTOR4_SHUFFLE_3(xs, xn, ys, yn, z, 2) \
		VECTOR4_SHUFFLE_3(xs, xn, ys, yn, w, 3) \

	#define VECTOR4_SHUFFLE_1(xs, xn) \
		GSVector4 xs##4##() const {return GSVector4(_mm_shuffle_ps(m, m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
		GSVector4 xs##4##(const GSVector4& v) const {return GSVector4(_mm_shuffle_ps(m, v.m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
		VECTOR4_SHUFFLE_2(xs, xn, x, 0) \
		VECTOR4_SHUFFLE_2(xs, xn, y, 1) \
		VECTOR4_SHUFFLE_2(xs, xn, z, 2) \
		VECTOR4_SHUFFLE_2(xs, xn, w, 3) \

	VECTOR4_SHUFFLE_1(x, 0)
	VECTOR4_SHUFFLE_1(y, 1)
	VECTOR4_SHUFFLE_1(z, 2)
	VECTOR4_SHUFFLE_1(w, 3)
};

#pragma pack(pop)
