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

#include "stdafx.h"
#include "GSSetupPrimCodeGenerator.h"

#if _M_SSE >= 0x500 && !(defined(_M_AMD64) || defined(_WIN64))

using namespace Xbyak;

void GSSetupPrimCodeGenerator::Generate()
{
	if((m_en.z || m_en.f) && !m_sel.sprite || m_en.t || m_en.c && m_sel.iip)
	{
		for(int i = 0; i < 5; i++)
		{
			vmovaps(Xmm(3 + i), ptr[&m_shift[i]]);
		}
	}

	Depth();

	Texture();

	Color();

	ret();
}

void GSSetupPrimCodeGenerator::Depth()
{
	if(!m_en.z && !m_en.f)
	{
		return;
	}

	if(!m_sel.sprite)
	{
		// GSVector4 p = dscan.p;

		vmovaps(xmm0, ptr[edx + 16]);

		if(m_en.f)
		{
			// GSVector4 df = p.wwww();

			vshufps(xmm1, xmm0, xmm0, _MM_SHUFFLE(3, 3, 3, 3));

			// m_local.d4.f = GSVector4i(df * 4.0f).xxzzlh();

			vmulps(xmm2, xmm1, xmm3);
			vcvttps2dq(xmm2, xmm2);
			vpshuflw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
			vpshufhw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
			vmovdqa(ptr[&m_local.d4.f], xmm2);

			for(int i = 0; i < 4; i++)
			{
				// m_local.d[i].f = GSVector4i(df * m_shift[i]).xxzzlh();

				vmulps(xmm2, xmm1, Xmm(4 + i));
				vcvttps2dq(xmm2, xmm2);
				vpshuflw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
				vpshufhw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
				vmovdqa(ptr[&m_local.d[i].f], xmm2);
			}
		}

		if(m_en.z)
		{
			// GSVector4 dz = p.zzzz();

			vshufps(xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

			// m_local.d4.z = dz * 4.0f;

			vmulps(xmm1, xmm0, xmm3);
			vmovdqa(ptr[&m_local.d4.z], xmm1);

			for(int i = 0; i < 4; i++)
			{
				// m_local.d[i].z = dz * m_shift[i];

				vmulps(xmm1, xmm0, Xmm(4 + i));
				vmovdqa(ptr[&m_local.d[i].z], xmm1);
			}
		}
	}
	else
	{
		// GSVector4 p = vertices[0].p;

		vmovaps(xmm0, ptr[ecx + 16]);

		if(m_en.f)
		{
			// m_local.p.f = GSVector4i(p).zzzzh().zzzz();

			vcvttps2dq(xmm1, xmm0);
			vpshufhw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
			vpshufd(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
			vmovdqa(ptr[&m_local.p.f], xmm1);
		}

		if(m_en.z)
		{
			// GSVector4 z = p.zzzz();

			vshufps(xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

			if(m_sel.zoverflow)
			{
				// m_local.p.z = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001());

				vbroadcastss(xmm1, ptr[&GSVector4::m_half]);
				vmulps(xmm1, xmm0);
				vcvttps2dq(xmm1, xmm1);
				vpslld(xmm1, 1);

				vcvttps2dq(xmm0, xmm0);
				vpcmpeqd(xmm2, xmm2);
				vpsrld(xmm2, 31);
				vpand(xmm0, xmm2);

				vpor(xmm0, xmm1);
			}
			else
			{
				// m_local.p.z = GSVector4i(z);

				vcvttps2dq(xmm0, xmm0);
			}

			vmovdqa(ptr[&m_local.p.z], xmm0);
		}
	}
}

void GSSetupPrimCodeGenerator::Texture()
{
	if(!m_en.t)
	{
		return;
	}

	// GSVector4 t = dscan.t;

	vmovaps(xmm0, ptr[edx + 32]);

	vmulps(xmm1, xmm0, xmm3);

	if(m_sel.fst)
	{
		// m_local.d4.st = GSVector4i(t * 4.0f);

		vcvttps2dq(xmm1, xmm1);
		vmovdqa(ptr[&m_local.d4.st], xmm1);
	}
	else
	{
		// m_local.d4.stq = t * 4.0f;

		vmovaps(ptr[&m_local.d4.stq], xmm1);
	}

	for(int j = 0, k = m_sel.fst ? 2 : 3; j < k; j++)
	{
		// GSVector4 ds = t.xxxx();
		// GSVector4 dt = t.yyyy();
		// GSVector4 dq = t.zzzz();

		vshufps(xmm1, xmm0, xmm0, (uint8)_MM_SHUFFLE(j, j, j, j));

		for(int i = 0; i < 4; i++)
		{
			// GSVector4 v = ds/dt * m_shift[i];

			vmulps(xmm2, xmm1, Xmm(4 + i));

			if(m_sel.fst)
			{
				// m_local.d[i].si/ti = GSVector4i(v);

				vcvttps2dq(xmm2, xmm2);

				switch(j)
				{
				case 0: vmovdqa(ptr[&m_local.d[i].si], xmm2); break;
				case 1: vmovdqa(ptr[&m_local.d[i].ti], xmm2); break;
				}
			}
			else
			{
				// m_local.d[i].s/t/q = v;

				switch(j)
				{
				case 0: vmovaps(ptr[&m_local.d[i].s], xmm2); break;
				case 1: vmovaps(ptr[&m_local.d[i].t], xmm2); break;
				case 2: vmovaps(ptr[&m_local.d[i].q], xmm2); break;
				}
			}
		}
	}
}

void GSSetupPrimCodeGenerator::Color()
{
	if(!m_en.c)
	{
		return;
	}

	if(m_sel.iip)
	{
		// GSVector4 c = dscan.c;

		vmovaps(xmm0, ptr[edx]);

		// m_local.d4.c = GSVector4i(c * 4.0f).xzyw().ps32();

		vmulps(xmm1, xmm0, xmm3);
		vcvttps2dq(xmm1, xmm1);
		vpshufd(xmm1, xmm1, _MM_SHUFFLE(3, 1, 2, 0));
		vpackssdw(xmm1, xmm1);
		vmovdqa(ptr[&m_local.d4.c], xmm1);

		// xmm3 is not needed anymore

		// GSVector4 dr = c.xxxx();
		// GSVector4 db = c.zzzz();

		vshufps(xmm2, xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
		vshufps(xmm3, xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

		for(int i = 0; i < 4; i++)
		{
			// GSVector4i r = GSVector4i(dr * m_shift[i]).ps32();

			vmulps(xmm0, xmm2, Xmm(4 + i));
			vcvttps2dq(xmm0, xmm0);
			vpackssdw(xmm0, xmm0);

			// GSVector4i b = GSVector4i(db * m_shift[i]).ps32();

			vmulps(xmm1, xmm3, Xmm(4 + i));
			vcvttps2dq(xmm1, xmm1);
			vpackssdw(xmm1, xmm1);

			// m_local.d[i].rb = r.upl16(b);

			vpunpcklwd(xmm0, xmm1);
			vmovdqa(ptr[&m_local.d[i].rb], xmm0);
		}

		// GSVector4 c = dscan.c;

		vmovaps(xmm0, ptr[edx]); // not enough regs, have to reload it

		// GSVector4 dg = c.yyyy();
		// GSVector4 da = c.wwww();

		vshufps(xmm2, xmm0, xmm0, _MM_SHUFFLE(1, 1, 1, 1));
		vshufps(xmm3, xmm0, xmm0, _MM_SHUFFLE(3, 3, 3, 3));

		for(int i = 0; i < 4; i++)
		{
			// GSVector4i g = GSVector4i(dg * m_shift[i]).ps32();

			vmulps(xmm0, xmm2, Xmm(4 + i));
			vcvttps2dq(xmm0, xmm0);
			vpackssdw(xmm0, xmm0);

			// GSVector4i a = GSVector4i(da * m_shift[i]).ps32();

			vmulps(xmm1, xmm3, Xmm(4 + i));
			vcvttps2dq(xmm1, xmm1);
			vpackssdw(xmm1, xmm1);

			// m_local.d[i].ga = g.upl16(a);

			vpunpcklwd(xmm0, xmm1);
			vmovdqa(ptr[&m_local.d[i].ga], xmm0);
		}
	}
	else
	{
		// GSVector4i c = GSVector4i(vertices[0].c);

		vcvttps2dq(xmm0, ptr[ecx]);

		// c = c.upl16(c.zwxy());

		vpshufd(xmm1, xmm0, _MM_SHUFFLE(1, 0, 3, 2));
		vpunpcklwd(xmm0, xmm1);

		// if(!tme) c = c.srl16(7);

		if(m_sel.tfx == TFX_NONE)
		{
			vpsrlw(xmm0, 7);
		}

		// m_local.c.rb = c.xxxx();
		// m_local.c.ga = c.zzzz();

		vpshufd(xmm1, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
		vpshufd(xmm2, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

		vmovdqa(ptr[&m_local.c.rb], xmm1);
		vmovdqa(ptr[&m_local.c.ga], xmm2);
	}
}

#endif