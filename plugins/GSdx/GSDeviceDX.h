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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSVector.h"
#include "GSDevice.h"
#include "GSAlignedClass.h"

class GSDeviceDX : public GSDevice
{
public:
	#pragma pack(push, 1)

	__aligned16 struct VSConstantBuffer
	{
		GSVector4 VertexScale;
		GSVector4 VertexOffset;
		GSVector4 TextureScale;

		struct VSConstantBuffer() 
		{
			VertexScale = GSVector4::zero();
			VertexOffset = GSVector4::zero();
			TextureScale = GSVector4::zero();
		}

		__forceinline bool Update(const VSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			GSVector4i b0 = b[0];
			GSVector4i b1 = b[1];
			GSVector4i b2 = b[2];

			if(!((a[0] == b0) & (a[1] == b1) & (a[2] == b2)).alltrue())
			{
				a[0] = b0;
				a[1] = b1;
				a[2] = b2;

				return true;
			}

			return false;
		}
	};

	struct VSSelector
	{
		union
		{
			struct
			{
				uint32 bppz:2;
				uint32 tme:1;
				uint32 fst:1;
				uint32 logz:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x1f;}

		VSSelector() : key(0) {}
	};

	__aligned16 struct PSConstantBuffer
	{
		GSVector4 FogColor_AREF;
		GSVector4 HalfTexel;
		GSVector4 WH;
		GSVector4 MinMax;
		GSVector4 MinF_TA;
		GSVector4i MskFix;

		struct PSConstantBuffer() 
		{
			FogColor_AREF = GSVector4::zero();
			HalfTexel = GSVector4::zero();
			WH = GSVector4::zero();
			MinMax = GSVector4::zero();
			MinF_TA = GSVector4::zero();
			MskFix = GSVector4i::zero();
		}

		__forceinline bool Update(const PSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			GSVector4i b0 = b[0];
			GSVector4i b1 = b[1];
			GSVector4i b2 = b[2];
			GSVector4i b3 = b[3];
			GSVector4i b4 = b[4];
			GSVector4i b5 = b[5];

			if(!((a[0] == b0) /*& (a[1] == b1)*/ & (a[2] == b2) & (a[3] == b3) & (a[4] == b4) & (a[5] == b5)).alltrue()) // if WH matches HalfTexel does too
			{
				a[0] = b0;
				a[1] = b1;
				a[2] = b2;
				a[3] = b3;
				a[4] = b4;
				a[5] = b5;

				return true;
			}

			return false;
		}
	};

	struct GSSelector
	{
		union
		{
			struct
			{
				uint32 iip:1;
				uint32 prim:2;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x7;}

		GSSelector() : key(0) {}
	};

	struct PSSelector
	{
		union
		{
			struct
			{
				uint32 fst:1;
				uint32 wms:2;
				uint32 wmt:2;
				uint32 fmt:3;
				uint32 aem:1;
				uint32 tfx:3;
				uint32 tcc:1;
				uint32 atst:3;
				uint32 fog:1;
				uint32 clr1:1;
				uint32 fba:1;
				uint32 aout:1;
				uint32 rt:1;
				uint32 ltf:1;
				uint32 colclip:2;
			};

			uint32 key;
		};

		operator uint32() {return key & 0xffffff;}

		PSSelector() : key(0) {}
	};

	struct PSSamplerSelector
	{
		union
		{
			struct
			{
				uint32 tau:1;
				uint32 tav:1;
				uint32 ltf:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x7;}

		PSSamplerSelector() : key(0) {}
	};

	struct OMDepthStencilSelector
	{
		union
		{
			struct
			{
				uint32 ztst:2;
				uint32 zwe:1;
				uint32 date:1;
				uint32 fba:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x1f;}

		OMDepthStencilSelector() : key(0) {}
	};

	struct OMBlendSelector
	{
		union
		{
			struct
			{
				uint32 abe:1;
				uint32 a:2;
				uint32 b:2;
				uint32 c:2;
				uint32 d:2;
				uint32 wr:1;
				uint32 wg:1;
				uint32 wb:1;
				uint32 wa:1;
				uint32 negative:1;
			};

			struct
			{
				uint32 _pad:1;
				uint32 abcd:8;
				uint32 wrgba:4;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x3fff;}

		OMBlendSelector() : key(0) {}

		bool IsCLR1() const
		{
			return (key & 0x19f) == 0x93; // abe == 1 && a == 1 && b == 2 && d == 1
		}
	};

	#pragma pack(pop)

public:
	GSDeviceDX() {};
	virtual ~GSDeviceDX() {}

	virtual bool Create(GSWnd* wnd)
	{
		return __super::Create( wnd );
	}
	
	virtual bool Reset(int w, int h)
	{
		return __super::Reset( w, h );
	}
	
	//virtual void Present(const GSVector4i& r, int shader);
	//virtual void Flip() {}
	
	virtual void SetupIA(const void* vertices, int count, int prim) = 0;
	virtual void SetupVS(VSSelector sel, const VSConstantBuffer* cb) = 0;
	virtual void SetupGS(GSSelector sel) = 0;
	virtual void SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel) = 0;
	virtual void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix) = 0;

	virtual void SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPT1 (&iaVertices)[4], bool datm)=0;
};
