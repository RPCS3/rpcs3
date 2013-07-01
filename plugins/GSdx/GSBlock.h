/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GS.h"
#include "GSTables.h"
#include "GSVector.h"

class GSBlock
{
	#if _M_SSE >= 0x501
	static const GSVector8i m_r16mask;
	#else
	static const GSVector4i m_r16mask;
	#endif
	static const GSVector4i m_r8mask;
	static const GSVector4i m_r4mask;

	#if _M_SSE >= 0x501
	static const GSVector8i m_xxxa;
	static const GSVector8i m_xxbx;
	static const GSVector8i m_xgxx;
	static const GSVector8i m_rxxx;
	#else
	static const GSVector4i m_xxxa;
	static const GSVector4i m_xxbx;
	static const GSVector4i m_xgxx;
	static const GSVector4i m_rxxx;
	#endif

	static const GSVector4i m_uw8hmask0;
	static const GSVector4i m_uw8hmask1;
	static const GSVector4i m_uw8hmask2;
	static const GSVector4i m_uw8hmask3;

public:
	template<int i, int alignment, uint32 mask> __forceinline static void WriteColumn32(uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		const uint8* RESTRICT s0 = &src[srcpitch * 0];
		const uint8* RESTRICT s1 = &src[srcpitch * 1];

		#if _M_SSE >= 0x501

		GSVector8i v0, v1;

		if(alignment == 32)
		{
			v0 = GSVector8i::load<true>(s0).acbd();
			v1 = GSVector8i::load<true>(s1).acbd();

			GSVector8i::sw64(v0, v1);
		}
		else
		{
			if(alignment == 16)
			{
				v0 = GSVector8i::load(&s0[0], &s0[16]).acbd();
				v1 = GSVector8i::load(&s1[0], &s1[16]).acbd();

				GSVector8i::sw64(v0, v1);
			}
			else
			{
				//v0 = GSVector8i::load(&s0[0], &s0[16], &s0[8], &s0[24]);
				//v1 = GSVector8i::load(&s1[0], &s1[16], &s1[8], &s1[24]);

				GSVector4i v4 = GSVector4i::load(&s0[0], &s1[0]);
				GSVector4i v5 = GSVector4i::load(&s0[8], &s1[8]);
				GSVector4i v6 = GSVector4i::load(&s0[16], &s1[16]);
				GSVector4i v7 = GSVector4i::load(&s0[24], &s1[24]);

				if(mask == 0xffffffff)
				{
					// just write them out directly

					((GSVector4i*)dst)[i * 4 + 0] = v4;
					((GSVector4i*)dst)[i * 4 + 1] = v5;
					((GSVector4i*)dst)[i * 4 + 2] = v6;
					((GSVector4i*)dst)[i * 4 + 3] = v7;

					return;
				}

				v0 = GSVector8i::cast(v4).insert<1>(v5);
				v1 = GSVector8i::cast(v6).insert<1>(v7);
			}
		}

		if(mask == 0xffffffff)
		{
			((GSVector8i*)dst)[i * 2 + 0] = v0;
			((GSVector8i*)dst)[i * 2 + 1] = v1;
		}
		else 
		{
			GSVector8i v2((int)mask);

			if(mask == 0xff000000 || mask == 0x00ffffff)
			{
				((GSVector8i*)dst)[i * 2 + 0] = ((GSVector8i*)dst)[i * 2 + 0].blend8(v0, v2);
				((GSVector8i*)dst)[i * 2 + 1] = ((GSVector8i*)dst)[i * 2 + 1].blend8(v1, v2);
			}
			else
			{
				((GSVector8i*)dst)[i * 2 + 0] = ((GSVector8i*)dst)[i * 2 + 0].blend(v0, v2);
				((GSVector8i*)dst)[i * 2 + 1] = ((GSVector8i*)dst)[i * 2 + 1].blend(v1, v2);
			}
		}

		#else

		GSVector4i v0, v1, v2, v3;

		if(alignment != 0)
		{
			v0 = GSVector4i::load<true>(&s0[0]);
			v1 = GSVector4i::load<true>(&s0[16]);
			v2 = GSVector4i::load<true>(&s1[0]);
			v3 = GSVector4i::load<true>(&s1[16]);

			GSVector4i::sw64(v0, v2, v1, v3);
		}
		else
		{
			v0 = GSVector4i::load(&s0[0], &s1[0]);
			v1 = GSVector4i::load(&s0[8], &s1[8]);
			v2 = GSVector4i::load(&s0[16], &s1[16]);
			v3 = GSVector4i::load(&s0[24], &s1[24]);
		}

		if(mask == 0xffffffff)
		{
			((GSVector4i*)dst)[i * 4 + 0] = v0;
			((GSVector4i*)dst)[i * 4 + 1] = v1;
			((GSVector4i*)dst)[i * 4 + 2] = v2;
			((GSVector4i*)dst)[i * 4 + 3] = v3;
		}
		else
		{
			GSVector4i v4((int)mask);

			#if _M_SSE >= 0x401

			if(mask == 0xff000000 || mask == 0x00ffffff)
			{
				((GSVector4i*)dst)[i * 4 + 0] = ((GSVector4i*)dst)[i * 4 + 0].blend8(v0, v4);
				((GSVector4i*)dst)[i * 4 + 1] = ((GSVector4i*)dst)[i * 4 + 1].blend8(v1, v4);
				((GSVector4i*)dst)[i * 4 + 2] = ((GSVector4i*)dst)[i * 4 + 2].blend8(v2, v4);
				((GSVector4i*)dst)[i * 4 + 3] = ((GSVector4i*)dst)[i * 4 + 3].blend8(v3, v4);
			}
			else
			{

			#endif

			((GSVector4i*)dst)[i * 4 + 0] = ((GSVector4i*)dst)[i * 4 + 0].blend(v0, v4);
			((GSVector4i*)dst)[i * 4 + 1] = ((GSVector4i*)dst)[i * 4 + 1].blend(v1, v4);
			((GSVector4i*)dst)[i * 4 + 2] = ((GSVector4i*)dst)[i * 4 + 2].blend(v2, v4);
			((GSVector4i*)dst)[i * 4 + 3] = ((GSVector4i*)dst)[i * 4 + 3].blend(v3, v4);

			#if _M_SSE >= 0x401

			}

			#endif
		}

		#endif
	}

	template<int i, int alignment> __forceinline static void WriteColumn16(uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		const uint8* RESTRICT s0 = &src[srcpitch * 0];
		const uint8* RESTRICT s1 = &src[srcpitch * 1];

		// for(int j = 0; j < 16; j++) {((uint16*)s0)[j] = columnTable16[0][j]; ((uint16*)s1)[j] = columnTable16[1][j];}

		#if _M_SSE >= 0x501

		GSVector8i v0, v1;

		if(alignment == 32)
		{
			v0 = GSVector8i::load<true>(s0);
			v1 = GSVector8i::load<true>(s1);

			GSVector8i::sw128(v0, v1);
			GSVector8i::sw16(v0, v1);
		}
		else
		{
			if(alignment == 16)
			{
				v0 = GSVector8i::load(&s0[0], &s1[0]);
				v1 = GSVector8i::load(&s0[16], &s1[16]);
			}
			else
			{
				v0 = GSVector8i::load(&s0[0], &s0[8], &s1[0], &s1[8]);
				v1 = GSVector8i::load(&s0[16], &s0[24], &s1[16], &s1[24]);
			}

			GSVector8i::sw16(v0, v1);
		}

		v0 = v0.acbd();
		v1 = v1.acbd();
		
		((GSVector8i*)dst)[i * 2 + 0] = v0;
		((GSVector8i*)dst)[i * 2 + 1] = v1;

		#else

		GSVector4i v0, v1, v2, v3;

		if(alignment != 0)
		{
			v0 = GSVector4i::load<true>(&s0[0]);
			v1 = GSVector4i::load<true>(&s0[16]);
			v2 = GSVector4i::load<true>(&s1[0]);
			v3 = GSVector4i::load<true>(&s1[16]);

			GSVector4i::sw16(v0, v1, v2, v3);
			GSVector4i::sw64(v0, v1, v2, v3);
		}
		else
		{
			v0 = GSVector4i::loadl(&s0[0]).upl16(GSVector4i::loadl(&s0[16]));
			v2 = GSVector4i::loadl(&s0[8]).upl16(GSVector4i::loadl(&s0[24]));
			v1 = GSVector4i::loadl(&s1[0]).upl16(GSVector4i::loadl(&s1[16]));
			v3 = GSVector4i::loadl(&s1[8]).upl16(GSVector4i::loadl(&s1[24]));

			GSVector4i::sw64(v0, v1, v2, v3);
		}

		((GSVector4i*)dst)[i * 4 + 0] = v0;
		((GSVector4i*)dst)[i * 4 + 1] = v2;
		((GSVector4i*)dst)[i * 4 + 2] = v1;
		((GSVector4i*)dst)[i * 4 + 3] = v3;

		#endif
	}

	template<int i, int alignment> __forceinline static void WriteColumn8(uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		// TODO: read unaligned as WriteColumn32 does and try saving a few shuffles

		#if _M_SSE >= 0x501

		GSVector4i v4 = GSVector4i::load<alignment != 0>(&src[srcpitch * 0]);
		GSVector4i v5 = GSVector4i::load<alignment != 0>(&src[srcpitch * 1]);
		GSVector4i v6 = GSVector4i::load<alignment != 0>(&src[srcpitch * 2]);
		GSVector4i v7 = GSVector4i::load<alignment != 0>(&src[srcpitch * 3]);

		GSVector8i v0(v4, v5);
		GSVector8i v1(v6, v7);

		if((i & 1) == 0)
		{
			v1 = v1.yxwz();
		}
		else 
		{
			v0 = v0.yxwz();
		}

		GSVector8i::sw8(v0, v1);
		GSVector8i::sw16(v0, v1);
		
		v0 = v0.acbd();
		v1 = v1.acbd();

		((GSVector8i*)dst)[i * 2 + 0] = v0;
		((GSVector8i*)dst)[i * 2 + 1] = v1;

		#else

		GSVector4i v0 = GSVector4i::load<alignment != 0>(&src[srcpitch * 0]);
		GSVector4i v1 = GSVector4i::load<alignment != 0>(&src[srcpitch * 1]);
		GSVector4i v2 = GSVector4i::load<alignment != 0>(&src[srcpitch * 2]);
		GSVector4i v3 = GSVector4i::load<alignment != 0>(&src[srcpitch * 3]);

		if((i & 1) == 0)
		{
			v2 = v2.yxwz();
			v3 = v3.yxwz();
		}
		else
		{
			v0 = v0.yxwz();
			v1 = v1.yxwz();
		}

		GSVector4i::sw8(v0, v2, v1, v3);
		GSVector4i::sw16(v0, v1, v2, v3);
		GSVector4i::sw64(v0, v1, v2, v3);

		((GSVector4i*)dst)[i * 4 + 0] = v0;
		((GSVector4i*)dst)[i * 4 + 1] = v2;
		((GSVector4i*)dst)[i * 4 + 2] = v1;
		((GSVector4i*)dst)[i * 4 + 3] = v3;

		#endif
	}

	template<int i, int alignment> __forceinline static void WriteColumn4(uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		//printf("WriteColumn4\n");

		// TODO: read unaligned as WriteColumn32 does and try saving a few shuffles

		// TODO: pshufb

		GSVector4i v0 = GSVector4i::load<alignment != 0>(&src[srcpitch * 0]);
		GSVector4i v1 = GSVector4i::load<alignment != 0>(&src[srcpitch * 1]);
		GSVector4i v2 = GSVector4i::load<alignment != 0>(&src[srcpitch * 2]);
		GSVector4i v3 = GSVector4i::load<alignment != 0>(&src[srcpitch * 3]);

		if((i & 1) == 0)
		{
			v2 = v2.yxwzlh();
			v3 = v3.yxwzlh();
		}
		else
		{
			v0 = v0.yxwzlh();
			v1 = v1.yxwzlh();
		}

		GSVector4i::sw4(v0, v2, v1, v3);
		GSVector4i::sw8(v0, v1, v2, v3);
		GSVector4i::sw8(v0, v2, v1, v3);
		GSVector4i::sw64(v0, v2, v1, v3);

		((GSVector4i*)dst)[i * 4 + 0] = v0;
		((GSVector4i*)dst)[i * 4 + 1] = v1;
		((GSVector4i*)dst)[i * 4 + 2] = v2;
		((GSVector4i*)dst)[i * 4 + 3] = v3;
	}

	template<int alignment, uint32 mask> static void WriteColumn32(int y, uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		switch((y >> 1) & 3)
		{
		case 0: WriteColumn32<0, alignment, mask>(dst, src, srcpitch); break;
		case 1: WriteColumn32<1, alignment, mask>(dst, src, srcpitch); break;
		case 2: WriteColumn32<2, alignment, mask>(dst, src, srcpitch); break;
		case 3: WriteColumn32<3, alignment, mask>(dst, src, srcpitch); break;
		default: __assume(0);
		}
	}

	template<int alignment> static void WriteColumn16(int y, uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		switch((y >> 1) & 3)
		{
		case 0: WriteColumn16<0, alignment>(dst, src, srcpitch); break;
		case 1: WriteColumn16<1, alignment>(dst, src, srcpitch); break;
		case 2: WriteColumn16<2, alignment>(dst, src, srcpitch); break;
		case 3: WriteColumn16<3, alignment>(dst, src, srcpitch); break;
		default: __assume(0);
		}
	}

	template<int alignment> static void WriteColumn8(int y, uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		switch((y >> 2) & 3)
		{
		case 0: WriteColumn8<0, alignment>(dst, src, srcpitch); break;
		case 1: WriteColumn8<1, alignment>(dst, src, srcpitch); break;
		case 2: WriteColumn8<2, alignment>(dst, src, srcpitch); break;
		case 3: WriteColumn8<3, alignment>(dst, src, srcpitch); break;
		default: __assume(0);
		}
	}

	template<int alignment> static void WriteColumn4(int y, uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		switch((y >> 2) & 3)
		{
		case 0: WriteColumn4<0, alignment>(dst, src, srcpitch); break;
		case 1: WriteColumn4<1, alignment>(dst, src, srcpitch); break;
		case 2: WriteColumn4<2, alignment>(dst, src, srcpitch); break;
		case 3: WriteColumn4<3, alignment>(dst, src, srcpitch); break;
		default: __assume(0);
		}
	}

	template<int alignment, uint32 mask> static void WriteBlock32(uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		WriteColumn32<0, alignment, mask>(dst, src, srcpitch);
		src += srcpitch * 2;
		WriteColumn32<1, alignment, mask>(dst, src, srcpitch);
		src += srcpitch * 2;
		WriteColumn32<2, alignment, mask>(dst, src, srcpitch);
		src += srcpitch * 2;
		WriteColumn32<3, alignment, mask>(dst, src, srcpitch);
	}

	template<int alignment> static void WriteBlock16(uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		WriteColumn16<0, alignment>(dst, src, srcpitch);
		src += srcpitch * 2;
		WriteColumn16<1, alignment>(dst, src, srcpitch);
		src += srcpitch * 2;
		WriteColumn16<2, alignment>(dst, src, srcpitch);
		src += srcpitch * 2;
		WriteColumn16<3, alignment>(dst, src, srcpitch);
	}

	template<int alignment> static void WriteBlock8(uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		WriteColumn8<0, alignment>(dst, src, srcpitch);
		src += srcpitch * 4;
		WriteColumn8<1, alignment>(dst, src, srcpitch);
		src += srcpitch * 4;
		WriteColumn8<2, alignment>(dst, src, srcpitch);
		src += srcpitch * 4;
		WriteColumn8<3, alignment>(dst, src, srcpitch);
	}

	template<int alignment> static void WriteBlock4(uint8* RESTRICT dst, const uint8* RESTRICT src, int srcpitch)
	{
		WriteColumn4<0, alignment>(dst, src, srcpitch);
		src += srcpitch * 4;
		WriteColumn4<1, alignment>(dst, src, srcpitch);
		src += srcpitch * 4;
		WriteColumn4<2, alignment>(dst, src, srcpitch);
		src += srcpitch * 4;
		WriteColumn4<3, alignment>(dst, src, srcpitch);
	}

	template<int i> __forceinline static void ReadColumn32(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		#if _M_SSE >= 0x501

		const GSVector8i* s = (const GSVector8i*)src;
		
		GSVector8i v0 = s[i * 2 + 0];
		GSVector8i v1 = s[i * 2 + 1];

		GSVector8i::sw128(v0, v1);
		GSVector8i::sw64(v0, v1);

		GSVector8i::store<true>(&dst[dstpitch * 0], v0);
		GSVector8i::store<true>(&dst[dstpitch * 1], v1);

		#else

		const GSVector4i* s = (const GSVector4i*)src;
		
		GSVector4i v0 = s[i * 4 + 0];
		GSVector4i v1 = s[i * 4 + 1];
		GSVector4i v2 = s[i * 4 + 2];
		GSVector4i v3 = s[i * 4 + 3];

		GSVector4i::sw64(v0, v1, v2, v3);

		GSVector4i* d0 = (GSVector4i*)&dst[dstpitch * 0];
		GSVector4i* d1 = (GSVector4i*)&dst[dstpitch * 1];

		GSVector4i::store<true>(&d0[0], v0);
		GSVector4i::store<true>(&d0[1], v1);
		GSVector4i::store<true>(&d1[0], v2);
		GSVector4i::store<true>(&d1[1], v3);

		#endif
	}

	template<int i> __forceinline static void ReadColumn16(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		#if _M_SSE >= 0x501

		const GSVector8i* s = (const GSVector8i*)src;
		
		GSVector8i v0 = s[i * 2 + 0].shuffle8(m_r16mask);
		GSVector8i v1 = s[i * 2 + 1].shuffle8(m_r16mask);

		GSVector8i::sw128(v0, v1);
		GSVector8i::sw32(v0, v1);

		v0 = v0.acbd();
		v1 = v1.acbd();

		GSVector8i::store<true>(&dst[dstpitch * 0], v0);
		GSVector8i::store<true>(&dst[dstpitch * 1], v1);

		#elif _M_SSE >= 0x301

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0 = s[i * 4 + 0].shuffle8(m_r16mask);
		GSVector4i v1 = s[i * 4 + 1].shuffle8(m_r16mask);
		GSVector4i v2 = s[i * 4 + 2].shuffle8(m_r16mask);
		GSVector4i v3 = s[i * 4 + 3].shuffle8(m_r16mask);

		GSVector4i::sw32(v0, v1, v2, v3);
		GSVector4i::sw64(v0, v1, v2, v3);

		GSVector4i* d0 = (GSVector4i*)&dst[dstpitch * 0];
		GSVector4i* d1 = (GSVector4i*)&dst[dstpitch * 1];

		GSVector4i::store<true>(&d0[0], v0);
		GSVector4i::store<true>(&d0[1], v2);
		GSVector4i::store<true>(&d1[0], v1);
		GSVector4i::store<true>(&d1[1], v3);

		#else

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0 = s[i * 4 + 0];
		GSVector4i v1 = s[i * 4 + 1];
		GSVector4i v2 = s[i * 4 + 2];
		GSVector4i v3 = s[i * 4 + 3];

		//for(int16 i = 0; i < 8; i++) {v0.i16[i] = i; v1.i16[i] = i + 8; v2.i16[i] = i + 16; v3.i16[i] = i + 24;}

		GSVector4i::sw16(v0, v1, v2, v3);
		GSVector4i::sw32(v0, v1, v2, v3);
		GSVector4i::sw16(v0, v2, v1, v3);

		GSVector4i* d0 = (GSVector4i*)&dst[dstpitch * 0];
		GSVector4i* d1 = (GSVector4i*)&dst[dstpitch * 1];

		GSVector4i::store<true>(&d0[0], v0);
		GSVector4i::store<true>(&d0[1], v1);
		GSVector4i::store<true>(&d1[0], v2);
		GSVector4i::store<true>(&d1[1], v3);

		#endif
	}

	template<int i> __forceinline static void ReadColumn8(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		//for(int j = 0; j < 64; j++) ((uint8*)src)[j] = (uint8)j;

		#if 0//_M_SSE >= 0x501

		const GSVector8i* s = (const GSVector8i*)src;
		
		GSVector8i v0 = s[i * 2 + 0];
		GSVector8i v1 = s[i * 2 + 1];

		GSVector8i::sw8(v0, v1);
		GSVector8i::sw16(v0, v1);
		GSVector8i::sw8(v0, v1);
		GSVector8i::sw128(v0, v1);
		GSVector8i::sw16(v0, v1);

		v0 = v0.acbd();
		v1 = v1.acbd();
		v1 = v1.yxwz();

		GSVector8i::storel(&dst[dstpitch * 0], v0);
		GSVector8i::storeh(&dst[dstpitch * 1], v0);
		GSVector8i::storel(&dst[dstpitch * 2], v1);
		GSVector8i::storeh(&dst[dstpitch * 3], v1);

		// TODO: not sure if this is worth it, not in this form, there should be a shorter path

		#elif _M_SSE >= 0x301

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0, v1, v2, v3;

		if((i & 1) == 0)
		{
			v0 = s[i * 4 + 0];
			v1 = s[i * 4 + 1];
			v2 = s[i * 4 + 2];
			v3 = s[i * 4 + 3];
		}
		else
		{
			v2 = s[i * 4 + 0];
			v3 = s[i * 4 + 1];
			v0 = s[i * 4 + 2];
			v1 = s[i * 4 + 3];
		}

		v0 = v0.shuffle8(m_r8mask);
		v1 = v1.shuffle8(m_r8mask);
		v2 = v2.shuffle8(m_r8mask);
		v3 = v3.shuffle8(m_r8mask);

		GSVector4i::sw16(v0, v1, v2, v3);
		GSVector4i::sw32(v0, v1, v3, v2);

		GSVector4i::store<true>(&dst[dstpitch * 0], v0);
		GSVector4i::store<true>(&dst[dstpitch * 1], v3);
		GSVector4i::store<true>(&dst[dstpitch * 2], v1);
		GSVector4i::store<true>(&dst[dstpitch * 3], v2);

		#else

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0 = s[i * 4 + 0];
		GSVector4i v1 = s[i * 4 + 1];
		GSVector4i v2 = s[i * 4 + 2];
		GSVector4i v3 = s[i * 4 + 3];

		GSVector4i::sw8(v0, v1, v2, v3);
		GSVector4i::sw16(v0, v1, v2, v3);
		GSVector4i::sw8(v0, v2, v1, v3);
		GSVector4i::sw64(v0, v1, v2, v3);

		if((i & 1) == 0)
		{
			v2 = v2.yxwz();
			v3 = v3.yxwz();
		}
		else
		{
			v0 = v0.yxwz();
			v1 = v1.yxwz();
		}

		GSVector4i::store<true>(&dst[dstpitch * 0], v0);
		GSVector4i::store<true>(&dst[dstpitch * 1], v1);
		GSVector4i::store<true>(&dst[dstpitch * 2], v2);
		GSVector4i::store<true>(&dst[dstpitch * 3], v3);

		#endif
	}

	template<int i> __forceinline static void ReadColumn4(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		//printf("ReadColumn4\n");

		#if _M_SSE >= 0x301

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0 = s[i * 4 + 0].xzyw();
		GSVector4i v1 = s[i * 4 + 1].xzyw();
		GSVector4i v2 = s[i * 4 + 2].xzyw();
		GSVector4i v3 = s[i * 4 + 3].xzyw();

		GSVector4i::sw64(v0, v1, v2, v3);
		GSVector4i::sw4(v0, v2, v1, v3);
		GSVector4i::sw8(v0, v1, v2, v3);

		v0 = v0.shuffle8(m_r4mask);
		v1 = v1.shuffle8(m_r4mask);
		v2 = v2.shuffle8(m_r4mask);
		v3 = v3.shuffle8(m_r4mask);

		if((i & 1) == 0)
		{
			GSVector4i::sw16rh(v0, v1, v2, v3);
		}
		else
		{
			GSVector4i::sw16rl(v0, v1, v2, v3);
		}

		GSVector4i::store<true>(&dst[dstpitch * 0], v0);
		GSVector4i::store<true>(&dst[dstpitch * 1], v1);
		GSVector4i::store<true>(&dst[dstpitch * 2], v2);
		GSVector4i::store<true>(&dst[dstpitch * 3], v3);

		#else

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0 = s[i * 4 + 0];
		GSVector4i v1 = s[i * 4 + 1];
		GSVector4i v2 = s[i * 4 + 2];
		GSVector4i v3 = s[i * 4 + 3];

		GSVector4i::sw32(v0, v1, v2, v3);
		GSVector4i::sw32(v0, v1, v2, v3);
		GSVector4i::sw4(v0, v2, v1, v3);
		GSVector4i::sw8(v0, v1, v2, v3);
		GSVector4i::sw16(v0, v2, v1, v3);

		v0 = v0.xzyw();
		v1 = v1.xzyw();
		v2 = v2.xzyw();
		v3 = v3.xzyw();

		GSVector4i::sw64(v0, v1, v2, v3);

		if((i & 1) == 0)
		{
			v2 = v2.yxwzlh();
			v3 = v3.yxwzlh();
		}
		else
		{
			v0 = v0.yxwzlh();
			v1 = v1.yxwzlh();
		}

		GSVector4i::store<true>(&dst[dstpitch * 0], v0);
		GSVector4i::store<true>(&dst[dstpitch * 1], v1);
		GSVector4i::store<true>(&dst[dstpitch * 2], v2);
		GSVector4i::store<true>(&dst[dstpitch * 3], v3);

		#endif
	}

	static void ReadColumn32(int y, const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		switch((y >> 1) & 3)
		{
		case 0: ReadColumn32<0>(src, dst, dstpitch); break;
		case 1: ReadColumn32<1>(src, dst, dstpitch); break;
		case 2: ReadColumn32<2>(src, dst, dstpitch); break;
		case 3: ReadColumn32<3>(src, dst, dstpitch); break;
		default: __assume(0);
		}
	}

	static void ReadColumn16(int y, const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		switch((y >> 1) & 3)
		{
		case 0: ReadColumn16<0>(src, dst, dstpitch); break;
		case 1: ReadColumn16<1>(src, dst, dstpitch); break;
		case 2: ReadColumn16<2>(src, dst, dstpitch); break;
		case 3: ReadColumn16<3>(src, dst, dstpitch); break;
		default: __assume(0);
		}
	}

	static void ReadColumn8(int y, const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		switch((y >> 2) & 3)
		{
		case 0: ReadColumn8<0>(src, dst, dstpitch); break;
		case 1: ReadColumn8<1>(src, dst, dstpitch); break;
		case 2: ReadColumn8<2>(src, dst, dstpitch); break;
		case 3: ReadColumn8<3>(src, dst, dstpitch); break;
		default: __assume(0);
		}
	}

	static void ReadColumn4(int y, const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		switch((y >> 2) & 3)
		{
		case 0: ReadColumn4<0>(src, dst, dstpitch); break;
		case 1: ReadColumn4<1>(src, dst, dstpitch); break;
		case 2: ReadColumn4<2>(src, dst, dstpitch); break;
		case 3: ReadColumn4<3>(src, dst, dstpitch); break;
		default: __assume(0);
		}
	}

	static void ReadBlock32(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		ReadColumn32<0>(src, dst, dstpitch);
		dst += dstpitch * 2;
		ReadColumn32<1>(src, dst, dstpitch);
		dst += dstpitch * 2;
		ReadColumn32<2>(src, dst, dstpitch);
		dst += dstpitch * 2;
		ReadColumn32<3>(src, dst, dstpitch);
	}

	static void ReadBlock16(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		ReadColumn16<0>(src, dst, dstpitch);
		dst += dstpitch * 2;
		ReadColumn16<1>(src, dst, dstpitch);
		dst += dstpitch * 2;
		ReadColumn16<2>(src, dst, dstpitch);
		dst += dstpitch * 2;
		ReadColumn16<3>(src, dst, dstpitch);
	}

	static void ReadBlock8(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		ReadColumn8<0>(src, dst, dstpitch);
		dst += dstpitch * 4;
		ReadColumn8<1>(src, dst, dstpitch);
		dst += dstpitch * 4;
		ReadColumn8<2>(src, dst, dstpitch);
		dst += dstpitch * 4;
		ReadColumn8<3>(src, dst, dstpitch);
	}

	static void ReadBlock4(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		ReadColumn4<0>(src, dst, dstpitch);
		dst += dstpitch * 4;
		ReadColumn4<1>(src, dst, dstpitch);
		dst += dstpitch * 4;
		ReadColumn4<2>(src, dst, dstpitch);
		dst += dstpitch * 4;
		ReadColumn4<3>(src, dst, dstpitch);
	}

	__forceinline static void ReadBlock4P(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		//printf("ReadBlock4P\n");

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0, v1, v2, v3;

		GSVector4i mask(0x0f0f0f0f);

		for(int i = 0; i < 2; i++)
		{
			// col 0, 2

			v0 = s[i * 8 + 0];
			v1 = s[i * 8 + 1];
			v2 = s[i * 8 + 2];
			v3 = s[i * 8 + 3];

			GSVector4i::sw8(v0, v1, v2, v3);
			GSVector4i::sw16(v0, v1, v2, v3);
			GSVector4i::sw8(v0, v2, v1, v3);

			GSVector4i::store<true>(&dst[dstpitch * 0 +  0], (v0 & mask));
			GSVector4i::store<true>(&dst[dstpitch * 0 + 16], (v1 & mask));
			GSVector4i::store<true>(&dst[dstpitch * 1 +  0], (v2 & mask));
			GSVector4i::store<true>(&dst[dstpitch * 1 + 16], (v3 & mask));

			dst += dstpitch * 2;

			GSVector4i::store<true>(&dst[dstpitch * 0 +  0], (v0.andnot(mask)).yxwz() >> 4);
			GSVector4i::store<true>(&dst[dstpitch * 0 + 16], (v1.andnot(mask)).yxwz() >> 4);
			GSVector4i::store<true>(&dst[dstpitch * 1 +  0], (v2.andnot(mask)).yxwz() >> 4);
			GSVector4i::store<true>(&dst[dstpitch * 1 + 16], (v3.andnot(mask)).yxwz() >> 4);

			dst += dstpitch * 2;

			// col 1, 3

			v0 = s[i * 8 + 4];
			v1 = s[i * 8 + 5];
			v2 = s[i * 8 + 6];
			v3 = s[i * 8 + 7];

			GSVector4i::sw8(v0, v1, v2, v3);
			GSVector4i::sw16(v0, v1, v2, v3);
			GSVector4i::sw8(v0, v2, v1, v3);

			GSVector4i::store<true>(&dst[dstpitch * 0 +  0], (v0 & mask).yxwz());
			GSVector4i::store<true>(&dst[dstpitch * 0 + 16], (v1 & mask).yxwz());
			GSVector4i::store<true>(&dst[dstpitch * 1 +  0], (v2 & mask).yxwz());
			GSVector4i::store<true>(&dst[dstpitch * 1 + 16], (v3 & mask).yxwz());

			dst += dstpitch * 2;

			GSVector4i::store<true>(&dst[dstpitch * 0 +  0], (v0.andnot(mask)) >> 4);
			GSVector4i::store<true>(&dst[dstpitch * 0 + 16], (v1.andnot(mask)) >> 4);
			GSVector4i::store<true>(&dst[dstpitch * 1 +  0], (v2.andnot(mask)) >> 4);
			GSVector4i::store<true>(&dst[dstpitch * 1 + 16], (v3.andnot(mask)) >> 4);

			dst += dstpitch * 2;
		}
	}

	__forceinline static void ReadBlock8HP(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		#if _M_SSE >= 0x501

		uint8* RESTRICT d0 = &dst[dstpitch * 0];
		uint8* RESTRICT d1 = &dst[dstpitch * 4];

		const GSVector8i* s = (const GSVector8i*)src;
		
		GSVector8i v0, v1, v2, v3;
		GSVector4i v4, v5;

		v0 = s[0].acbd();
		v1 = s[1].acbd();
		v2 = s[2].acbd();
		v3 = s[3].acbd();

		v0 = (v0 >> 24).ps32(v1 >> 24).pu16((v2 >> 24).ps32(v3 >> 24));

		v4 = v0.extract<0>();
		v5 = v0.extract<1>();

		GSVector4i::storel(&d0[dstpitch * 0], v4);
		GSVector4i::storel(&d0[dstpitch * 1], v5);
		GSVector4i::storeh(&d0[dstpitch * 2], v4);
		GSVector4i::storeh(&d0[dstpitch * 3], v5);

		v0 = s[4].acbd();
		v1 = s[5].acbd();
		v2 = s[6].acbd();
		v3 = s[7].acbd();

		v0 = (v0 >> 24).ps32(v1 >> 24).pu16((v2 >> 24).ps32(v3 >> 24));

		v4 = v0.extract<0>();
		v5 = v0.extract<1>();

		GSVector4i::storel(&d1[dstpitch * 0], v4);
		GSVector4i::storel(&d1[dstpitch * 1], v5);
		GSVector4i::storeh(&d1[dstpitch * 2], v4);
		GSVector4i::storeh(&d1[dstpitch * 3], v5);

		#else

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0, v1, v2, v3;

		for(int i = 0; i < 4; i++)
		{
			v0 = s[i * 4 + 0];
			v1 = s[i * 4 + 1];
			v2 = s[i * 4 + 2];
			v3 = s[i * 4 + 3];

			GSVector4i::sw64(v0, v1, v2, v3);

			v0 = ((v0 >> 24).ps32(v1 >> 24)).pu16((v2 >> 24).ps32(v3 >> 24));

			GSVector4i::storel(dst, v0);

			dst += dstpitch;

			GSVector4i::storeh(dst, v0);

			dst += dstpitch;
		}

		#endif
	}

	__forceinline static void ReadBlock4HLP(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		#if _M_SSE >= 0x501

		uint8* RESTRICT d0 = &dst[dstpitch * 0];
		uint8* RESTRICT d1 = &dst[dstpitch * 4];

		const GSVector8i* s = (const GSVector8i*)src;
		
		GSVector8i v0, v1, v2, v3;
		GSVector4i v4, v5;
		GSVector8i mask(0x0f0f0f0f);

		v0 = s[0].acbd();
		v1 = s[1].acbd();
		v2 = s[2].acbd();
		v3 = s[3].acbd();

		v0 = (v0 >> 24).ps32(v1 >> 24).pu16((v2 >> 24).ps32(v3 >> 24)) & mask;

		v4 = v0.extract<0>();
		v5 = v0.extract<1>();

		GSVector4i::storel(&d0[dstpitch * 0], v4);
		GSVector4i::storel(&d0[dstpitch * 1], v5);
		GSVector4i::storeh(&d0[dstpitch * 2], v4);
		GSVector4i::storeh(&d0[dstpitch * 3], v5);

		v0 = s[4].acbd();
		v1 = s[5].acbd();
		v2 = s[6].acbd();
		v3 = s[7].acbd();

		v0 = (v0 >> 24).ps32(v1 >> 24).pu16((v2 >> 24).ps32(v3 >> 24)) & mask;

		v4 = v0.extract<0>();
		v5 = v0.extract<1>();

		GSVector4i::storel(&d1[dstpitch * 0], v4);
		GSVector4i::storel(&d1[dstpitch * 1], v5);
		GSVector4i::storeh(&d1[dstpitch * 2], v4);
		GSVector4i::storeh(&d1[dstpitch * 3], v5);

		#else

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0, v1, v2, v3;

		GSVector4i mask(0x0f0f0f0f);

		for(int i = 0; i < 4; i++)
		{
			v0 = s[i * 4 + 0];
			v1 = s[i * 4 + 1];
			v2 = s[i * 4 + 2];
			v3 = s[i * 4 + 3];

			GSVector4i::sw64(v0, v1, v2, v3);

			v0 = ((v0 >> 24).ps32(v1 >> 24)).pu16((v2 >> 24).ps32(v3 >> 24)) & mask;

			GSVector4i::storel(dst, v0);

			dst += dstpitch;

			GSVector4i::storeh(dst, v0);

			dst += dstpitch;
		}

		#endif
	}

	__forceinline static void ReadBlock4HHP(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch)
	{
		#if _M_SSE >= 0x501

		uint8* RESTRICT d0 = &dst[dstpitch * 0];
		uint8* RESTRICT d1 = &dst[dstpitch * 4];

		const GSVector8i* s = (const GSVector8i*)src;
		
		GSVector8i v0, v1, v2, v3;
		GSVector4i v4, v5;

		v0 = s[0].acbd();
		v1 = s[1].acbd();
		v2 = s[2].acbd();
		v3 = s[3].acbd();

		v0 = (v0 >> 28).ps32(v1 >> 28).pu16((v2 >> 28).ps32(v3 >> 28));

		v4 = v0.extract<0>();
		v5 = v0.extract<1>();

		GSVector4i::storel(&d0[dstpitch * 0], v4);
		GSVector4i::storel(&d0[dstpitch * 1], v5);
		GSVector4i::storeh(&d0[dstpitch * 2], v4);
		GSVector4i::storeh(&d0[dstpitch * 3], v5);

		v0 = s[4].acbd();
		v1 = s[5].acbd();
		v2 = s[6].acbd();
		v3 = s[7].acbd();

		v0 = (v0 >> 28).ps32(v1 >> 28).pu16((v2 >> 28).ps32(v3 >> 28));

		v4 = v0.extract<0>();
		v5 = v0.extract<1>();

		GSVector4i::storel(&d1[dstpitch * 0], v4);
		GSVector4i::storel(&d1[dstpitch * 1], v5);
		GSVector4i::storeh(&d1[dstpitch * 2], v4);
		GSVector4i::storeh(&d1[dstpitch * 3], v5);

		#else

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0, v1, v2, v3;

		for(int i = 0; i < 4; i++)
		{
			v0 = s[i * 4 + 0];
			v1 = s[i * 4 + 1];
			v2 = s[i * 4 + 2];
			v3 = s[i * 4 + 3];

			GSVector4i::sw64(v0, v1, v2, v3);

			v0 = ((v0 >> 28).ps32(v1 >> 28)).pu16((v2 >> 28).ps32(v3 >> 28));

			GSVector4i::storel(dst, v0);

			dst += dstpitch;

			GSVector4i::storeh(dst, v0);

			dst += dstpitch;
		}

		#endif
	}

	template<bool AEM, class V> __forceinline static V Expand24to32(const V& c, const V& TA0)
	{
		return c | (AEM ? TA0.andnot(c == V::zero()) : TA0); // TA0 & (c != GSVector4i::zero())
	}

	template<bool AEM, class V> __forceinline static V Expand16to32(const V& c, const V& TA0, const V& TA1)
	{
		return ((c & m_rxxx) << 3) | ((c & m_xgxx) << 6) | ((c & m_xxbx) << 9) | (AEM ? TA0.blend8(TA1, c.sra16(15)).andnot(c == V::zero()) : TA0.blend(TA1, c.sra16(15)));
	}

	template<bool AEM> static void ExpandBlock24(const uint32* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const GIFRegTEXA& TEXA)
	{
		#if _M_SSE >= 0x501

		const GSVector8i* s = (const GSVector8i*)src;

		GSVector8i TA0(TEXA.TA0 << 24);
		GSVector8i mask = GSVector8i::x00ffffff();

		for(int i = 0; i < 4; i++, dst += dstpitch * 2)
		{
			GSVector8i v0 = s[i * 2 + 0] & mask;
			GSVector8i v1 = s[i * 2 + 1] & mask;

			GSVector8i* d0 = (GSVector8i*)&dst[dstpitch * 0];
			GSVector8i* d1 = (GSVector8i*)&dst[dstpitch * 1];

			d0[0] = Expand24to32<AEM>(v0, TA0);
			d1[0] = Expand24to32<AEM>(v1, TA0);
		}

		#else

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i TA0(TEXA.TA0 << 24);
		GSVector4i mask = GSVector4i::x00ffffff();

		for(int i = 0; i < 4; i++, dst += dstpitch * 2)
		{
			GSVector4i v0 = s[i * 4 + 0] & mask;
			GSVector4i v1 = s[i * 4 + 1] & mask;
			GSVector4i v2 = s[i * 4 + 2] & mask;
			GSVector4i v3 = s[i * 4 + 3] & mask;

			GSVector4i* d0 = (GSVector4i*)&dst[dstpitch * 0];
			GSVector4i* d1 = (GSVector4i*)&dst[dstpitch * 1];

			d0[0] = Expand24to32<AEM>(v0, TA0);
			d0[1] = Expand24to32<AEM>(v1, TA0);
			d1[0] = Expand24to32<AEM>(v2, TA0);
			d1[1] = Expand24to32<AEM>(v3, TA0);
		}

		#endif
	}

	template<bool AEM> static void ExpandBlock16(const uint16* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const GIFRegTEXA& TEXA) // do not inline, uses too many xmm regs
	{
		#if _M_SSE >= 0x501
		
		const GSVector8i* s = (const GSVector8i*)src;

		GSVector8i TA0(TEXA.TA0 << 24);
		GSVector8i TA1(TEXA.TA1 << 24);

		for(int i = 0; i < 8; i++, dst += dstpitch)
		{
			GSVector8i v = s[i].acbd();

			((GSVector8i*)dst)[0] = Expand16to32<AEM>(v.upl16(v), TA0, TA1);
			((GSVector8i*)dst)[1] = Expand16to32<AEM>(v.uph16(v), TA0, TA1);
		}

		#else
		
		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i TA0(TEXA.TA0 << 24);
		GSVector4i TA1(TEXA.TA1 << 24);

		for(int i = 0; i < 8; i++, dst += dstpitch)
		{
			GSVector4i v0 = s[i * 2 + 0];

			((GSVector4i*)dst)[0] = Expand16to32<AEM>(v0.upl16(v0), TA0, TA1);
			((GSVector4i*)dst)[1] = Expand16to32<AEM>(v0.uph16(v0), TA0, TA1);

			GSVector4i v1 = s[i * 2 + 1];

			((GSVector4i*)dst)[2] = Expand16to32<AEM>(v1.upl16(v1), TA0, TA1);
			((GSVector4i*)dst)[3] = Expand16to32<AEM>(v1.uph16(v1), TA0, TA1);
		}

		#endif
	}

	__forceinline static void ExpandBlock8_32(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		for(int j = 0; j < 16; j++, dst += dstpitch)
		{
			((const GSVector4i*)src)[j].gather32_8(pal, (GSVector4i*)dst);
		}
	}

	__forceinline static void ExpandBlock8_16(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		for(int j = 0; j < 16; j++, dst += dstpitch)
		{
			((const GSVector4i*)src)[j].gather16_8(pal, (GSVector4i*)dst);
		}
	}

	__forceinline static void ExpandBlock4_32(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint64* RESTRICT pal)
	{
		for(int j = 0; j < 16; j++, dst += dstpitch)
		{
			((const GSVector4i*)src)[j].gather64_8(pal, (GSVector4i*)dst);
		}
	}

	__forceinline static void ExpandBlock4_16(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint64* RESTRICT pal)
	{
		for(int j = 0; j < 16; j++, dst += dstpitch)
		{
			((const GSVector4i*)src)[j].gather32_8(pal, (GSVector4i*)dst);
		}
	}

	__forceinline static void ExpandBlock8H_32(uint32* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		for(int j = 0; j < 8; j++, dst += dstpitch)
		{
			const GSVector4i* s = (const GSVector4i*)src;

			((GSVector4i*)dst)[0] = (s[j * 2 + 0] >> 24).gather32_32<>(pal);
			((GSVector4i*)dst)[1] = (s[j * 2 + 1] >> 24).gather32_32<>(pal);
		}
	}

	__forceinline static void ExpandBlock8H_16(uint32* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		for(int j = 0; j < 8; j++, dst += dstpitch)
		{
			#if _M_SSE >= 0x401

			const GSVector4i* s = (const GSVector4i*)src;

			GSVector4i v0 = (s[j * 2 + 0] >> 24).gather32_32<>(pal);
			GSVector4i v1 = (s[j * 2 + 1] >> 24).gather32_32<>(pal);

			((GSVector4i*)dst)[0] = v0.pu32(v1);

			#else

			for(int i = 0; i < 8; i++)
			{
				((uint16*)dst)[i] = (uint16)pal[src[j * 8 + i] >> 24];
			}

			#endif
		}
	}

	__forceinline static void ExpandBlock4HL_32(uint32* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		for(int j = 0; j < 8; j++, dst += dstpitch)
		{
			const GSVector4i* s = (const GSVector4i*)src;

			((GSVector4i*)dst)[0] = ((s[j * 2 + 0] >> 24) & 0xf).gather32_32<>(pal);
			((GSVector4i*)dst)[1] = ((s[j * 2 + 1] >> 24) & 0xf).gather32_32<>(pal);
		}
	}

	__forceinline static void ExpandBlock4HL_16(uint32* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		for(int j = 0; j < 8; j++, dst += dstpitch)
		{
			#if _M_SSE >= 0x401

			const GSVector4i* s = (const GSVector4i*)src;

			GSVector4i v0 = ((s[j * 2 + 0] >> 24) & 0xf).gather32_32<>(pal);
			GSVector4i v1 = ((s[j * 2 + 1] >> 24) & 0xf).gather32_32<>(pal);

			((GSVector4i*)dst)[0] = v0.pu32(v1);

			#else

			for(int i = 0; i < 8; i++)
			{
				((uint16*)dst)[i] = (uint16)pal[(src[j * 8 + i] >> 24) & 0xf];
			}

			#endif
		}
	}

	__forceinline static void ExpandBlock4HH_32(uint32* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		for(int j = 0; j < 8; j++, dst += dstpitch)
		{
			const GSVector4i* s = (const GSVector4i*)src;

			((GSVector4i*)dst)[0] = (s[j * 2 + 0] >> 28).gather32_32<>(pal);
			((GSVector4i*)dst)[1] = (s[j * 2 + 1] >> 28).gather32_32<>(pal);
		}
	}

	__forceinline static void ExpandBlock4HH_16(uint32* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		for(int j = 0; j < 8; j++, dst += dstpitch)
		{
			#if _M_SSE >= 0x401

			const GSVector4i* s = (const GSVector4i*)src;

			GSVector4i v0 = (s[j * 2 + 0] >> 28).gather32_32<>(pal);
			GSVector4i v1 = (s[j * 2 + 1] >> 28).gather32_32<>(pal);

			((GSVector4i*)dst)[0] = v0.pu32(v1);

			#else

			for(int i = 0; i < 8; i++)
			{
				((uint16*)dst)[i] = (uint16)pal[src[j * 8 + i] >> 28];
			}

			#endif
		}
	}

	__forceinline static void UnpackAndWriteBlock24(const uint8* RESTRICT src, int srcpitch, uint8* RESTRICT dst)
	{
		#if _M_SSE >= 0x501

		const uint8* RESTRICT s0 = &src[srcpitch * 0];
		const uint8* RESTRICT s1 = &src[srcpitch * 1];
		const uint8* RESTRICT s2 = &src[srcpitch * 2];
		const uint8* RESTRICT s3 = &src[srcpitch * 3];

		GSVector8i v0, v1, v2, v3, v4, v5, v6;
		GSVector8i mask = GSVector8i::x00ffffff();

		v4 = GSVector8i::load(s0, s0 + 8, s2, s2 + 8);
		v5 = GSVector8i::load(s0 + 16, s1, s2 + 16, s3);
		v6 = GSVector8i::load(s1 + 8, s1 + 16, s3 + 8, s3 + 16);
		
		v0 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>())).acbd();
		v4 = v4.srl<12>(v5);
		v1 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>())).acbd();
		v4 = v5.srl<8>(v6);
		v2 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>())).acbd();
		v4 = v6.srl<4>();
		v3 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>())).acbd();

		GSVector8i::sw64(v0, v2, v1, v3);

		((GSVector8i*)dst)[0] = ((GSVector8i*)dst)[0].blend8(v0, mask);
		((GSVector8i*)dst)[1] = ((GSVector8i*)dst)[1].blend8(v2, mask);
		((GSVector8i*)dst)[2] = ((GSVector8i*)dst)[2].blend8(v1, mask);
		((GSVector8i*)dst)[3] = ((GSVector8i*)dst)[3].blend8(v3, mask);

		src += srcpitch * 4;

		s0 = &src[srcpitch * 0];
		s1 = &src[srcpitch * 1];
		s2 = &src[srcpitch * 2];
		s3 = &src[srcpitch * 3];

		v4 = GSVector8i::load(s0, s0 + 8, s2, s2 + 8);
		v5 = GSVector8i::load(s0 + 16, s1, s2 + 16, s3);
		v6 = GSVector8i::load(s1 + 8, s1 + 16, s3 + 8, s3 + 16);
		
		v0 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>())).acbd();
		v4 = v4.srl<12>(v5);
		v1 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>())).acbd();
		v4 = v5.srl<8>(v6);
		v2 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>())).acbd();
		v4 = v6.srl<4>();
		v3 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>())).acbd();

		GSVector8i::sw64(v0, v2, v1, v3);

		((GSVector8i*)dst)[4] = ((GSVector8i*)dst)[4].blend8(v0, mask);
		((GSVector8i*)dst)[5] = ((GSVector8i*)dst)[5].blend8(v2, mask);
		((GSVector8i*)dst)[6] = ((GSVector8i*)dst)[6].blend8(v1, mask);
		((GSVector8i*)dst)[7] = ((GSVector8i*)dst)[7].blend8(v3, mask);

		#else

		GSVector4i v0, v1, v2, v3, v4, v5, v6;
		GSVector4i mask = GSVector4i::x00ffffff();

		for(int i = 0; i < 4; i++, src += srcpitch * 2)
		{
			v4 = GSVector4i::load<false>(src);
			v5 = GSVector4i::load(src + 16, src + srcpitch);
			v6 = GSVector4i::load<false>(src + srcpitch + 8);

			v0 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>()));
			v4 = v4.srl<12>(v5);
			v1 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>()));
			v4 = v5.srl<8>(v6);
			v2 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>()));
			v4 = v6.srl<4>();
			v3 = v4.upl32(v4.srl<3>()).upl64(v4.srl<6>().upl32(v4.srl<9>()));

			GSVector4i::sw64(v0, v2, v1, v3);

			((GSVector4i*)dst)[i * 4 + 0] = ((GSVector4i*)dst)[i * 4 + 0].blend8(v0, mask);
			((GSVector4i*)dst)[i * 4 + 1] = ((GSVector4i*)dst)[i * 4 + 1].blend8(v1, mask);
			((GSVector4i*)dst)[i * 4 + 2] = ((GSVector4i*)dst)[i * 4 + 2].blend8(v2, mask);
			((GSVector4i*)dst)[i * 4 + 3] = ((GSVector4i*)dst)[i * 4 + 3].blend8(v3, mask);
		}

		#endif
	}

	__forceinline static void UnpackAndWriteBlock8H(const uint8* RESTRICT src, int srcpitch, uint8* RESTRICT dst)
	{
		GSVector4i v4, v5, v6, v7;

		#if _M_SSE >= 0x501

		GSVector8i v0, v1, v2, v3;
		GSVector8i mask = GSVector8i::xff000000();

		v4 = GSVector4i::loadl(&src[srcpitch * 0]);
		v5 = GSVector4i::loadl(&src[srcpitch * 1]);
		v6 = GSVector4i::loadl(&src[srcpitch * 2]);
		v7 = GSVector4i::loadl(&src[srcpitch * 3]);

		v2 = GSVector8i::cast(v4.upl16(v5));
		v3 = GSVector8i::cast(v6.upl16(v7));
			
		v0 = v2.u8to32c() << 24;
		v1 = v2.bbbb().u8to32c() << 24;
		v2 = v3.u8to32c() << 24;
		v3 = v3.bbbb().u8to32c() << 24;

		((GSVector8i*)dst)[0] = ((GSVector8i*)dst)[0].blend8(v0, mask);
		((GSVector8i*)dst)[1] = ((GSVector8i*)dst)[1].blend8(v1, mask);
		((GSVector8i*)dst)[2] = ((GSVector8i*)dst)[2].blend8(v2, mask);
		((GSVector8i*)dst)[3] = ((GSVector8i*)dst)[3].blend8(v3, mask);

		src += srcpitch * 4;

		v4 = GSVector4i::loadl(&src[srcpitch * 0]);
		v5 = GSVector4i::loadl(&src[srcpitch * 1]);
		v6 = GSVector4i::loadl(&src[srcpitch * 2]);
		v7 = GSVector4i::loadl(&src[srcpitch * 3]);

		v2 = GSVector8i::cast(v4.upl16(v5));
		v3 = GSVector8i::cast(v6.upl16(v7));
			
		v0 = v2.u8to32c() << 24;
		v1 = v2.bbbb().u8to32c() << 24;
		v2 = v3.u8to32c() << 24;
		v3 = v3.bbbb().u8to32c() << 24;

		((GSVector8i*)dst)[4] = ((GSVector8i*)dst)[4].blend8(v0, mask);
		((GSVector8i*)dst)[5] = ((GSVector8i*)dst)[5].blend8(v1, mask);
		((GSVector8i*)dst)[6] = ((GSVector8i*)dst)[6].blend8(v2, mask);
		((GSVector8i*)dst)[7] = ((GSVector8i*)dst)[7].blend8(v3, mask);

		#elif _M_SSE >= 0x301

		GSVector4i v0, v1, v2, v3;
		GSVector4i mask = GSVector4i::xff000000();
		GSVector4i mask0 = m_uw8hmask0;
		GSVector4i mask1 = m_uw8hmask1;
		GSVector4i mask2 = m_uw8hmask2;
		GSVector4i mask3 = m_uw8hmask3;

		for(int i = 0; i < 4; i++, src += srcpitch * 2)
		{
			v4 = GSVector4i::load(src, src + srcpitch);

			v0 = v4.shuffle8(mask0);
			v1 = v4.shuffle8(mask1);
			v2 = v4.shuffle8(mask2);
			v3 = v4.shuffle8(mask3);

			((GSVector4i*)dst)[i * 4 + 0] = ((GSVector4i*)dst)[i * 4 + 0].blend8(v0, mask);
			((GSVector4i*)dst)[i * 4 + 1] = ((GSVector4i*)dst)[i * 4 + 1].blend8(v1, mask);
			((GSVector4i*)dst)[i * 4 + 2] = ((GSVector4i*)dst)[i * 4 + 2].blend8(v2, mask);
			((GSVector4i*)dst)[i * 4 + 3] = ((GSVector4i*)dst)[i * 4 + 3].blend8(v3, mask);
		}

		#else

		GSVector4i v0, v1, v2, v3;
		GSVector4i mask = GSVector4i::xff000000();

		for(int i = 0; i < 4; i++, src += srcpitch * 2)
		{
			v4 = GSVector4i::loadl(&src[srcpitch * 0]);
			v5 = GSVector4i::loadl(&src[srcpitch * 1]);

			v6 = v4.upl16(v5);

			v4 = v6.upl8(v6);
			v5 = v6.uph8(v6);

			v0 = v4.upl16(v4);
			v1 = v4.uph16(v4);
			v2 = v5.upl16(v5);
			v3 = v5.uph16(v5);
			
			((GSVector4i*)dst)[i * 4 + 0] = ((GSVector4i*)dst)[i * 4 + 0].blend8(v0, mask);
			((GSVector4i*)dst)[i * 4 + 1] = ((GSVector4i*)dst)[i * 4 + 1].blend8(v1, mask);
			((GSVector4i*)dst)[i * 4 + 2] = ((GSVector4i*)dst)[i * 4 + 2].blend8(v2, mask);
			((GSVector4i*)dst)[i * 4 + 3] = ((GSVector4i*)dst)[i * 4 + 3].blend8(v3, mask);
		}

		#endif
	}

	__forceinline static void UnpackAndWriteBlock4HL(const uint8* RESTRICT src, int srcpitch, uint8* RESTRICT dst)
	{
		//printf("4HL\n");

		if(0)
		{
			uint8* s = (uint8*)src;
			for(int j = 0; j < 8; j++, s += srcpitch)
				for(int i = 0; i < 4; i++) s[i] = (columnTable32[j][i*2] & 0x0f) | (columnTable32[j][i*2+1] << 4);
		}

		GSVector4i v4, v5, v6, v7;

		#if _M_SSE >= 0x501

		GSVector8i v0, v1, v2, v3;
		GSVector8i mask(0x0f000000);

		v6 = GSVector4i(*(uint32*)&src[srcpitch * 0], *(uint32*)&src[srcpitch * 2], *(uint32*)&src[srcpitch * 1], *(uint32*)&src[srcpitch * 3]);

		v4 = v6.upl8(v6 >> 4);
		v5 = v6.uph8(v6 >> 4);

		v2 = GSVector8i::cast(v4.upl16(v5));
		v3 = GSVector8i::cast(v4.uph16(v5));
			
		v0 = v2.u8to32c() << 24;
		v1 = v2.bbbb().u8to32c() << 24;
		v2 = v3.u8to32c() << 24;
		v3 = v3.bbbb().u8to32c() << 24;

		((GSVector8i*)dst)[0] = ((GSVector8i*)dst)[0].blend(v0, mask);
		((GSVector8i*)dst)[1] = ((GSVector8i*)dst)[1].blend(v1, mask);
		((GSVector8i*)dst)[2] = ((GSVector8i*)dst)[2].blend(v2, mask);
		((GSVector8i*)dst)[3] = ((GSVector8i*)dst)[3].blend(v3, mask);

		src += srcpitch * 4;

		v6 = GSVector4i(*(uint32*)&src[srcpitch * 0], *(uint32*)&src[srcpitch * 2], *(uint32*)&src[srcpitch * 1], *(uint32*)&src[srcpitch * 3]);

		v4 = v6.upl8(v6 >> 4);
		v5 = v6.uph8(v6 >> 4);

		v2 = GSVector8i::cast(v4.upl16(v5));
		v3 = GSVector8i::cast(v4.uph16(v5));
			
		v0 = v2.u8to32c() << 24;
		v1 = v2.bbbb().u8to32c() << 24;
		v2 = v3.u8to32c() << 24;
		v3 = v3.bbbb().u8to32c() << 24;

		((GSVector8i*)dst)[4] = ((GSVector8i*)dst)[4].blend(v0, mask);
		((GSVector8i*)dst)[5] = ((GSVector8i*)dst)[5].blend(v1, mask);
		((GSVector8i*)dst)[6] = ((GSVector8i*)dst)[6].blend(v2, mask);
		((GSVector8i*)dst)[7] = ((GSVector8i*)dst)[7].blend(v3, mask);

		#elif _M_SSE >= 0x301

		GSVector4i v0, v1, v2, v3;
		GSVector4i mask = GSVector4i(0x0f000000);
		GSVector4i mask0 = m_uw8hmask0;
		GSVector4i mask1 = m_uw8hmask1;
		GSVector4i mask2 = m_uw8hmask2;
		GSVector4i mask3 = m_uw8hmask3;

		for(int i = 0; i < 2; i++, src += srcpitch * 4)
		{
			GSVector4i v(*(uint32*)&src[srcpitch * 0], *(uint32*)&src[srcpitch * 1], *(uint32*)&src[srcpitch * 2], *(uint32*)&src[srcpitch * 3]);

			v4 = v.upl8(v >> 4);
			v5 = v.uph8(v >> 4);

			v0 = v4.shuffle8(mask0);
			v1 = v4.shuffle8(mask1);
			v2 = v4.shuffle8(mask2);
			v3 = v4.shuffle8(mask3);

			((GSVector4i*)dst)[i * 8 + 0] = ((GSVector4i*)dst)[i * 8 + 0].blend(v0, mask);
			((GSVector4i*)dst)[i * 8 + 1] = ((GSVector4i*)dst)[i * 8 + 1].blend(v1, mask);
			((GSVector4i*)dst)[i * 8 + 2] = ((GSVector4i*)dst)[i * 8 + 2].blend(v2, mask);
			((GSVector4i*)dst)[i * 8 + 3] = ((GSVector4i*)dst)[i * 8 + 3].blend(v3, mask);

			v0 = v5.shuffle8(mask0);
			v1 = v5.shuffle8(mask1);
			v2 = v5.shuffle8(mask2);
			v3 = v5.shuffle8(mask3);

			((GSVector4i*)dst)[i * 8 + 4] = ((GSVector4i*)dst)[i * 8 + 4].blend(v0, mask);
			((GSVector4i*)dst)[i * 8 + 5] = ((GSVector4i*)dst)[i * 8 + 5].blend(v1, mask);
			((GSVector4i*)dst)[i * 8 + 6] = ((GSVector4i*)dst)[i * 8 + 6].blend(v2, mask);
			((GSVector4i*)dst)[i * 8 + 7] = ((GSVector4i*)dst)[i * 8 + 7].blend(v3, mask);
		}

		#else

		GSVector4i v0, v1, v2, v3;
		GSVector4i mask = GSVector4i(0x0f000000);

		for(int i = 0; i < 2; i++, src += srcpitch * 4)
		{
			GSVector4i v(*(uint32*)&src[srcpitch * 0], *(uint32*)&src[srcpitch * 2], *(uint32*)&src[srcpitch * 1], *(uint32*)&src[srcpitch * 3]);

			v4 = v.upl8(v >> 4);
			v5 = v.uph8(v >> 4);

			v6 = v4.upl16(v5);
			v7 = v4.uph16(v5);

			v4 = v6.upl8(v6);
			v5 = v6.uph8(v6);
			v6 = v7.upl8(v7);
			v7 = v7.uph8(v7);

			v0 = v4.upl16(v4);
			v1 = v4.uph16(v4);
			v2 = v5.upl16(v5);
			v3 = v5.uph16(v5);

			((GSVector4i*)dst)[i * 8 + 0] = ((GSVector4i*)dst)[i * 8 + 0].blend(v0, mask);
			((GSVector4i*)dst)[i * 8 + 1] = ((GSVector4i*)dst)[i * 8 + 1].blend(v1, mask);
			((GSVector4i*)dst)[i * 8 + 2] = ((GSVector4i*)dst)[i * 8 + 2].blend(v2, mask);
			((GSVector4i*)dst)[i * 8 + 3] = ((GSVector4i*)dst)[i * 8 + 3].blend(v3, mask);

			v0 = v6.upl16(v6);
			v1 = v6.uph16(v6);
			v2 = v7.upl16(v7);
			v3 = v7.uph16(v7);

			((GSVector4i*)dst)[i * 8 + 4] = ((GSVector4i*)dst)[i * 8 + 4].blend(v0, mask);
			((GSVector4i*)dst)[i * 8 + 5] = ((GSVector4i*)dst)[i * 8 + 5].blend(v1, mask);
			((GSVector4i*)dst)[i * 8 + 6] = ((GSVector4i*)dst)[i * 8 + 6].blend(v2, mask);
			((GSVector4i*)dst)[i * 8 + 7] = ((GSVector4i*)dst)[i * 8 + 7].blend(v3, mask);
		}

		#endif
	}

	__forceinline static void UnpackAndWriteBlock4HH(const uint8* RESTRICT src, int srcpitch, uint8* RESTRICT dst)
	{
		GSVector4i v4, v5, v6, v7;

		#if _M_SSE >= 0x501

		GSVector8i v0, v1, v2, v3;
		GSVector8i mask = GSVector8i::xf0000000();

		v6 = GSVector4i(*(uint32*)&src[srcpitch * 0], *(uint32*)&src[srcpitch * 2], *(uint32*)&src[srcpitch * 1], *(uint32*)&src[srcpitch * 3]);

		v4 = (v6 << 4).upl8(v6);
		v5 = (v6 << 4).uph8(v6);

		v2 = GSVector8i::cast(v4.upl16(v5));
		v3 = GSVector8i::cast(v4.uph16(v5));
			
		v0 = v2.u8to32c() << 24;
		v1 = v2.bbbb().u8to32c() << 24;
		v2 = v3.u8to32c() << 24;
		v3 = v3.bbbb().u8to32c() << 24;

		((GSVector8i*)dst)[0] = ((GSVector8i*)dst)[0].blend(v0, mask);
		((GSVector8i*)dst)[1] = ((GSVector8i*)dst)[1].blend(v1, mask);
		((GSVector8i*)dst)[2] = ((GSVector8i*)dst)[2].blend(v2, mask);
		((GSVector8i*)dst)[3] = ((GSVector8i*)dst)[3].blend(v3, mask);

		src += srcpitch * 4;

		v6 = GSVector4i(*(uint32*)&src[srcpitch * 0], *(uint32*)&src[srcpitch * 2], *(uint32*)&src[srcpitch * 1], *(uint32*)&src[srcpitch * 3]);

		v4 = (v6 << 4).upl8(v6);
		v5 = (v6 << 4).uph8(v6);

		v2 = GSVector8i::cast(v4.upl16(v5));
		v3 = GSVector8i::cast(v4.uph16(v5));
			
		v0 = v2.u8to32c() << 24;
		v1 = v2.bbbb().u8to32c() << 24;
		v2 = v3.u8to32c() << 24;
		v3 = v3.bbbb().u8to32c() << 24;

		((GSVector8i*)dst)[4] = ((GSVector8i*)dst)[4].blend(v0, mask);
		((GSVector8i*)dst)[5] = ((GSVector8i*)dst)[5].blend(v1, mask);
		((GSVector8i*)dst)[6] = ((GSVector8i*)dst)[6].blend(v2, mask);
		((GSVector8i*)dst)[7] = ((GSVector8i*)dst)[7].blend(v3, mask);

		#elif _M_SSE >= 0x301

		GSVector4i v0, v1, v2, v3;
		GSVector4i mask = GSVector4i::xf0000000();
		GSVector4i mask0 = m_uw8hmask0;
		GSVector4i mask1 = m_uw8hmask1;
		GSVector4i mask2 = m_uw8hmask2;
		GSVector4i mask3 = m_uw8hmask3;

		for(int i = 0; i < 2; i++, src += srcpitch * 4)
		{
			GSVector4i v(*(uint32*)&src[srcpitch * 0], *(uint32*)&src[srcpitch * 1], *(uint32*)&src[srcpitch * 2], *(uint32*)&src[srcpitch * 3]);

			v4 = (v << 4).upl8(v);
			v5 = (v << 4).uph8(v);

			v0 = v4.shuffle8(mask0);
			v1 = v4.shuffle8(mask1);
			v2 = v4.shuffle8(mask2);
			v3 = v4.shuffle8(mask3);

			((GSVector4i*)dst)[i * 8 + 0] = ((GSVector4i*)dst)[i * 8 + 0].blend(v0, mask);
			((GSVector4i*)dst)[i * 8 + 1] = ((GSVector4i*)dst)[i * 8 + 1].blend(v1, mask);
			((GSVector4i*)dst)[i * 8 + 2] = ((GSVector4i*)dst)[i * 8 + 2].blend(v2, mask);
			((GSVector4i*)dst)[i * 8 + 3] = ((GSVector4i*)dst)[i * 8 + 3].blend(v3, mask);

			v0 = v5.shuffle8(mask0);
			v1 = v5.shuffle8(mask1);
			v2 = v5.shuffle8(mask2);
			v3 = v5.shuffle8(mask3);

			((GSVector4i*)dst)[i * 8 + 4] = ((GSVector4i*)dst)[i * 8 + 4].blend(v0, mask);
			((GSVector4i*)dst)[i * 8 + 5] = ((GSVector4i*)dst)[i * 8 + 5].blend(v1, mask);
			((GSVector4i*)dst)[i * 8 + 6] = ((GSVector4i*)dst)[i * 8 + 6].blend(v2, mask);
			((GSVector4i*)dst)[i * 8 + 7] = ((GSVector4i*)dst)[i * 8 + 7].blend(v3, mask);
		}

		#else

		GSVector4i v0, v1, v2, v3;
		GSVector4i mask = GSVector4i::xf0000000();

		for(int i = 0; i < 2; i++, src += srcpitch * 4)
		{
			GSVector4i v(*(uint32*)&src[srcpitch * 0], *(uint32*)&src[srcpitch * 2], *(uint32*)&src[srcpitch * 1], *(uint32*)&src[srcpitch * 3]);

			v4 = (v << 4).upl8(v);
			v5 = (v << 4).uph8(v);

			v6 = v4.upl16(v5);
			v7 = v4.uph16(v5);

			v4 = v6.upl8(v6);
			v5 = v6.uph8(v6);
			v6 = v7.upl8(v7);
			v7 = v7.uph8(v7);

			v0 = v4.upl16(v4);
			v1 = v4.uph16(v4);
			v2 = v5.upl16(v5);
			v3 = v5.uph16(v5);

			((GSVector4i*)dst)[i * 8 + 0] = ((GSVector4i*)dst)[i * 8 + 0].blend(v0, mask);
			((GSVector4i*)dst)[i * 8 + 1] = ((GSVector4i*)dst)[i * 8 + 1].blend(v1, mask);
			((GSVector4i*)dst)[i * 8 + 2] = ((GSVector4i*)dst)[i * 8 + 2].blend(v2, mask);
			((GSVector4i*)dst)[i * 8 + 3] = ((GSVector4i*)dst)[i * 8 + 3].blend(v3, mask);

			v0 = v6.upl16(v6);
			v1 = v6.uph16(v6);
			v2 = v7.upl16(v7);
			v3 = v7.uph16(v7);

			((GSVector4i*)dst)[i * 8 + 4] = ((GSVector4i*)dst)[i * 8 + 4].blend(v0, mask);
			((GSVector4i*)dst)[i * 8 + 5] = ((GSVector4i*)dst)[i * 8 + 5].blend(v1, mask);
			((GSVector4i*)dst)[i * 8 + 6] = ((GSVector4i*)dst)[i * 8 + 6].blend(v2, mask);
			((GSVector4i*)dst)[i * 8 + 7] = ((GSVector4i*)dst)[i * 8 + 7].blend(v3, mask);
		}

		#endif
	}

	template<bool AEM> __forceinline static void ReadAndExpandBlock24(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const GIFRegTEXA& TEXA)
	{
		#if _M_SSE >= 0x501

		const GSVector8i* s = (const GSVector8i*)src;
		
		GSVector8i TA0(TEXA.TA0 << 24);
		GSVector8i mask = GSVector8i::x00ffffff();

		GSVector8i v0, v1, v2, v3;

		v0 = s[0] & mask;
		v1 = s[1] & mask;
		v2 = s[2] & mask;
		v3 = s[3] & mask;

		GSVector8i::sw128(v0, v1);
		GSVector8i::sw64(v0, v1);
		GSVector8i::sw128(v2, v3);
		GSVector8i::sw64(v2, v3);

		*(GSVector8i*)&dst[dstpitch * 0] = Expand24to32<AEM>(v0, TA0);
		*(GSVector8i*)&dst[dstpitch * 1] = Expand24to32<AEM>(v1, TA0);
		*(GSVector8i*)&dst[dstpitch * 2] = Expand24to32<AEM>(v2, TA0);
		*(GSVector8i*)&dst[dstpitch * 3] = Expand24to32<AEM>(v3, TA0);

		v0 = s[4] & mask;
		v1 = s[5] & mask;
		v2 = s[6] & mask;
		v3 = s[7] & mask;

		GSVector8i::sw128(v0, v1);
		GSVector8i::sw64(v0, v1);
		GSVector8i::sw128(v2, v3);
		GSVector8i::sw64(v2, v3);

		dst += dstpitch * 4;

		*(GSVector8i*)&dst[dstpitch * 0] = Expand24to32<AEM>(v0, TA0);
		*(GSVector8i*)&dst[dstpitch * 1] = Expand24to32<AEM>(v1, TA0);
		*(GSVector8i*)&dst[dstpitch * 2] = Expand24to32<AEM>(v2, TA0);
		*(GSVector8i*)&dst[dstpitch * 3] = Expand24to32<AEM>(v3, TA0);

		#else

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i TA0(TEXA.TA0 << 24);
		GSVector4i mask = GSVector4i::x00ffffff();

		for(int i = 0; i < 4; i++, dst += dstpitch * 2)
		{
			GSVector4i v0 = s[i * 4 + 0];
			GSVector4i v1 = s[i * 4 + 1];
			GSVector4i v2 = s[i * 4 + 2];
			GSVector4i v3 = s[i * 4 + 3];

			GSVector4i::sw64(v0, v1, v2, v3);

			v0 &= mask;
			v1 &= mask;
			v2 &= mask;
			v3 &= mask;

			GSVector4i* d0 = (GSVector4i*)&dst[dstpitch * 0];
			GSVector4i* d1 = (GSVector4i*)&dst[dstpitch * 1];

			d0[0] = Expand24to32<AEM>(v0, TA0);
			d0[1] = Expand24to32<AEM>(v1, TA0);
			d1[0] = Expand24to32<AEM>(v2, TA0);
			d1[1] = Expand24to32<AEM>(v3, TA0);
		}

		#endif
	}

	template<bool AEM> __forceinline static void ReadAndExpandBlock16(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const GIFRegTEXA& TEXA)
	{
		#if _M_SSE >= 0x501

		const GSVector8i* s = (const GSVector8i*)src;

		GSVector8i TA0(TEXA.TA0 << 24);
		GSVector8i TA1(TEXA.TA1 << 24);

		for(int i = 0; i < 4; i++, dst += dstpitch * 2)
		{
			GSVector8i v0 = s[i * 2 + 0].shuffle8(m_r16mask);
			GSVector8i v1 = s[i * 2 + 1].shuffle8(m_r16mask);

			GSVector8i::sw128(v0, v1);
			GSVector8i::sw32(v0, v1);

			GSVector8i* d0 = (GSVector8i*)&dst[dstpitch * 0];
			GSVector8i* d1 = (GSVector8i*)&dst[dstpitch * 1];

			d0[0] = Expand16to32<AEM>(v0.upl16(v0), TA0, TA1);
			d0[1] = Expand16to32<AEM>(v0.uph16(v0), TA0, TA1);
			d1[0] = Expand16to32<AEM>(v1.upl16(v1), TA0, TA1);
			d1[1] = Expand16to32<AEM>(v1.uph16(v1), TA0, TA1);
		}

		#elif 0 // not faster
		
		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i TA0(TEXA.TA0 << 24);
		GSVector4i TA1(TEXA.TA1 << 24);

		for(int i = 0; i < 4; i++, dst += dstpitch * 2)
		{
			GSVector4i v0 = s[i * 4 + 0];
			GSVector4i v1 = s[i * 4 + 1];
			GSVector4i v2 = s[i * 4 + 2];
			GSVector4i v3 = s[i * 4 + 3];

			GSVector4i::sw16(v0, v1, v2, v3);
			GSVector4i::sw32(v0, v1, v2, v3);
			GSVector4i::sw16(v0, v2, v1, v3);

			GSVector4i* d0 = (GSVector4i*)&dst[dstpitch * 0];

			d0[0] = Expand16to32<AEM>(v0.upl16(v0), TA0, TA1);
			d0[1] = Expand16to32<AEM>(v0.uph16(v0), TA0, TA1);
			d0[2] = Expand16to32<AEM>(v1.upl16(v1), TA0, TA1);
			d0[3] = Expand16to32<AEM>(v1.uph16(v1), TA0, TA1);
			
			GSVector4i* d1 = (GSVector4i*)&dst[dstpitch * 1];

			d1[0] = Expand16to32<AEM>(v2.upl16(v2), TA0, TA1);
			d1[1] = Expand16to32<AEM>(v2.uph16(v2), TA0, TA1);
			d1[2] = Expand16to32<AEM>(v3.upl16(v3), TA0, TA1);
			d1[3] = Expand16to32<AEM>(v3.uph16(v3), TA0, TA1);
		}

		#else
		
		__aligned(uint16, 32) block[16 * 8];
	
		ReadBlock16(src, (uint8*)block, sizeof(block) / 8);

		ExpandBlock16<AEM>(block, dst, dstpitch, TEXA);

		#endif
	}

	__forceinline static void ReadAndExpandBlock8_32(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		//printf("ReadAndExpandBlock8_32\n");

		#if _M_SSE >= 0x401

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0, v1, v2, v3;
		GSVector4i mask = m_r8mask;

		for(int i = 0; i < 2; i++)
		{
			v0 = s[i * 8 + 0].shuffle8(mask);
			v1 = s[i * 8 + 1].shuffle8(mask);
			v2 = s[i * 8 + 2].shuffle8(mask);
			v3 = s[i * 8 + 3].shuffle8(mask);

			GSVector4i::sw16(v0, v1, v2, v3);
			GSVector4i::sw32(v0, v1, v3, v2);

			v0.gather32_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v3.gather32_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v1.gather32_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v2.gather32_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;

			v2 = s[i * 8 + 4].shuffle8(mask);
			v3 = s[i * 8 + 5].shuffle8(mask);
			v0 = s[i * 8 + 6].shuffle8(mask);
			v1 = s[i * 8 + 7].shuffle8(mask);

			GSVector4i::sw16(v0, v1, v2, v3);
			GSVector4i::sw32(v0, v1, v3, v2);

			v0.gather32_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v3.gather32_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v1.gather32_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v2.gather32_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
		}

		#else

		__aligned(uint8, 32) block[16 * 16];

		ReadBlock8(src, (uint8*)block, sizeof(block) / 16);

		ExpandBlock8_32(block, dst, dstpitch, pal);

		#endif
	}

	// TODO: ReadAndExpandBlock8_16

	__forceinline static void ReadAndExpandBlock4_32(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint64* RESTRICT pal)
	{
		//printf("ReadAndExpandBlock4_32\n");

		#if _M_SSE >= 0x401

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0, v1, v2, v3;
		GSVector4i mask = m_r4mask;

		for(int i = 0; i < 2; i++)
		{
			v0 = s[i * 8 + 0].xzyw();
			v1 = s[i * 8 + 1].xzyw();
			v2 = s[i * 8 + 2].xzyw();
			v3 = s[i * 8 + 3].xzyw();

			GSVector4i::sw64(v0, v1, v2, v3);
			GSVector4i::sw4(v0, v2, v1, v3);
			GSVector4i::sw8(v0, v1, v2, v3);

			v0 = v0.shuffle8(mask);
			v1 = v1.shuffle8(mask);
			v2 = v2.shuffle8(mask);
			v3 = v3.shuffle8(mask);

			GSVector4i::sw16rh(v0, v1, v2, v3);

			v0.gather64_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v1.gather64_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v2.gather64_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v3.gather64_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;

			v0 = s[i * 8 + 4].xzyw();
			v1 = s[i * 8 + 5].xzyw();
			v2 = s[i * 8 + 6].xzyw();
			v3 = s[i * 8 + 7].xzyw();

			GSVector4i::sw64(v0, v1, v2, v3);
			GSVector4i::sw4(v0, v2, v1, v3);
			GSVector4i::sw8(v0, v1, v2, v3);

			v0 = v0.shuffle8(mask);
			v1 = v1.shuffle8(mask);
			v2 = v2.shuffle8(mask);
			v3 = v3.shuffle8(mask);

			GSVector4i::sw16rl(v0, v1, v2, v3);

			v0.gather64_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v1.gather64_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v2.gather64_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
			v3.gather64_8<>(pal, (GSVector4i*)dst);
			dst += dstpitch;
		}

		#else

		__aligned(uint8, 32) block[(32 / 2) * 16];

		ReadBlock4(src, (uint8*)block, sizeof(block) / 16);

		ExpandBlock4_32(block, dst, dstpitch, pal);

		#endif
	}

	// TODO: ReadAndExpandBlock4_16

	__forceinline static void ReadAndExpandBlock8H_32(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		//printf("ReadAndExpandBlock8H_32\n");

		#if _M_SSE >= 0x401

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0, v1, v2, v3;

		for(int i = 0; i < 4; i++)
		{
			v0 = s[i * 4 + 0];
			v1 = s[i * 4 + 1];
			v2 = s[i * 4 + 2];
			v3 = s[i * 4 + 3];

			GSVector4i::sw64(v0, v1, v2, v3);

			(v0 >> 24).gather32_32<>(pal, (GSVector4i*)&dst[0]);
			(v1 >> 24).gather32_32<>(pal, (GSVector4i*)&dst[16]);

			dst += dstpitch;

			(v2 >> 24).gather32_32<>(pal, (GSVector4i*)&dst[0]);
			(v3 >> 24).gather32_32<>(pal, (GSVector4i*)&dst[16]);

			dst += dstpitch;
		}

		#else

		__aligned(uint32, 32) block[8 * 8];

		ReadBlock32(src, (uint8*)block, sizeof(block) / 8);

		ExpandBlock8H_32(block, dst, dstpitch, pal);

		#endif
	}

	// TODO: ReadAndExpandBlock8H_16

	__forceinline static void ReadAndExpandBlock4HL_32(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		//printf("ReadAndExpandBlock4HL_32\n");

		#if _M_SSE >= 0x401

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0, v1, v2, v3;

		for(int i = 0; i < 4; i++)
		{
			v0 = s[i * 4 + 0];
			v1 = s[i * 4 + 1];
			v2 = s[i * 4 + 2];
			v3 = s[i * 4 + 3];

			GSVector4i::sw64(v0, v1, v2, v3);

			((v0 >> 24) & 0xf).gather32_32<>(pal, (GSVector4i*)&dst[0]);
			((v1 >> 24) & 0xf).gather32_32<>(pal, (GSVector4i*)&dst[16]);

			dst += dstpitch;

			((v2 >> 24) & 0xf).gather32_32<>(pal, (GSVector4i*)&dst[0]);
			((v3 >> 24) & 0xf).gather32_32<>(pal, (GSVector4i*)&dst[16]);

			dst += dstpitch;
		}

		#else

		__aligned(uint32, 32) block[8 * 8];

		ReadBlock32(src, (uint8*)block, sizeof(block) / 8);

		ExpandBlock4HL_32(block, dst, dstpitch, pal);

		#endif
	}

	// TODO: ReadAndExpandBlock4HL_16

	__forceinline static void ReadAndExpandBlock4HH_32(const uint8* RESTRICT src, uint8* RESTRICT dst, int dstpitch, const uint32* RESTRICT pal)
	{
		//printf("ReadAndExpandBlock4HH_32\n");

		#if _M_SSE >= 0x401

		const GSVector4i* s = (const GSVector4i*)src;

		GSVector4i v0, v1, v2, v3;

		for(int i = 0; i < 4; i++)
		{
			v0 = s[i * 4 + 0];
			v1 = s[i * 4 + 1];
			v2 = s[i * 4 + 2];
			v3 = s[i * 4 + 3];

			GSVector4i::sw64(v0, v1, v2, v3);

			(v0 >> 28).gather32_32<>(pal, (GSVector4i*)&dst[0]);
			(v1 >> 28).gather32_32<>(pal, (GSVector4i*)&dst[16]);

			dst += dstpitch;

			(v2 >> 28).gather32_32<>(pal, (GSVector4i*)&dst[0]);
			(v3 >> 28).gather32_32<>(pal, (GSVector4i*)&dst[16]);

			dst += dstpitch;
		}

		#else

		__aligned(uint32, 32) block[8 * 8];

		ReadBlock32(src, (uint8*)block, sizeof(block) / 8);

		ExpandBlock4HH_32(block, dst, dstpitch, pal);

		#endif
	}

	// TODO: ReadAndExpandBlock4HH_16
};
