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

class GSTextureFX : public GSAlignedClass<16>
{
public:
	#pragma pack(push, 1)

	enum
	{
		FMT_32,
		FMT_24,
		FMT_16,
		FMT_8H,
		FMT_4HL,
		FMT_4HH,
		FMT_8,
	};

	__declspec(align(16)) struct VSConstantBuffer
	{
		GSVector4 VertexScale;
		GSVector4 VertexOffset;
		GSVector2 TextureScale;
		float _pad[2];

		struct VSConstantBuffer() 
		{
			memset(this, 0, sizeof(*this));
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
				uint32 prim:2;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x7f;}

		VSSelector() : key(0) {}
	};

	__declspec(align(16)) struct PSConstantBuffer
	{
		GSVector4 FogColor_AREF;
		GSVector4 HalfTexel;
		GSVector4 WH;
		GSVector4 MinMax;
		GSVector4 MinF_TA;
		GSVector4i MskFix;

		struct PSConstantBuffer() 
		{
			memset(this, 0, sizeof(*this));
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

			if(!((a[0] == b0) & (a[1] == b1) & (a[2] == b2) & (a[3] == b3) & (a[4] == b4) & (a[5] == b5)).alltrue())
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
				uint32 ate:1;
				uint32 atst:3;
				uint32 fog:1;
				uint32 clr1:1;
				uint32 fba:1;
				uint32 aout:1;
				uint32 rt:1;
				uint32 ltf:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x7fffff;}

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
				uint32 zte:1;
				uint32 ztst:2;
				uint32 zwe:1;
				uint32 date:1;
				uint32 fba:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x3f;}

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
			};

			uint32 key;
		};

		operator uint32() {return key & 0x1fff;}

		OMBlendSelector() : key(0) {}
	};

	#pragma pack(pop)

protected:
	GSDevice* m_dev;

public:
	GSTextureFX();
	virtual ~GSTextureFX() {}

	virtual bool Create(GSDevice* dev);
	
	virtual void SetupIA(const void* vertices, int count, int prim) = 0;
	virtual void SetupVS(VSSelector sel, const VSConstantBuffer* cb) = 0;
	virtual void SetupGS(GSSelector sel) = 0;
	virtual void SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, GSTexture* tex, GSTexture* pal) = 0;
	virtual void UpdatePS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel) = 0;
	virtual void SetupRS(int w, int h, const GSVector4i& scissor) = 0;
	virtual void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix, GSTexture* rt, GSTexture* ds) = 0;
	virtual void UpdateOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix) = 0;
};
