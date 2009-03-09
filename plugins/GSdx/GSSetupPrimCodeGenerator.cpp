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

// TODO: x64

#include "StdAfx.h"
#include "GSSetupPrimCodeGenerator.h"

GSSetupPrimCodeGenerator::GSSetupPrimCodeGenerator(GSScanlineEnvironment& env, UINT64 key, void* ptr, size_t maxsize)
	: CodeGenerator(maxsize, ptr)
	, m_env(env)
{
	m_sel.key = key;

	m_en.z = m_sel.zb ? 1 : 0;
	m_en.f = m_sel.fb && m_sel.fge ? 1 : 0;
	m_en.t = m_sel.fb && m_sel.tfx != TFX_NONE ? 1 : 0;
	m_en.c = m_sel.fb && m_sel.tfx != TFX_DECAL ? 1 : 0;

	#if _M_AMD64
	#error TODO
	#endif

	Generate();
}

void GSSetupPrimCodeGenerator::Generate()
{
	const int params = 0;

	if((m_en.z || m_en.f) && !m_sel.sprite || m_en.t || m_en.c && m_sel.iip)
	{
		for(int i = 0; i < 5; i++)
		{
			movaps(Xmm(3 + i), xmmword[&m_shift[i]]);
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
		// GSVector4 t = dscan.p;

		movaps(xmm0, xmmword[edx + 16]);

		if(m_en.f)
		{
			// GSVector4 df = p.wwww();

			movaps(xmm1, xmm0);
			shufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));

			// m_env.d4.f = GSVector4i(df * 4.0f).xxzzlh();

			movaps(xmm2, xmm1);
			mulps(xmm2, xmm3);
			cvttps2dq(xmm2, xmm2);
			pshuflw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
			pshufhw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
			movdqa(xmmword[&m_env.d4.f], xmm2);

			for(int i = 0; i < 4; i++)
			{
				// m_env.d[i].f = GSVector4i(df * m_shift[i]).xxzzlh();

				movaps(xmm2, xmm1);
				mulps(xmm2, Xmm(4 + i));
				cvttps2dq(xmm2, xmm2);
				pshuflw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
				pshufhw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
				movdqa(xmmword[&m_env.d[i].f], xmm2);
			}
		}

		if(m_en.z)
		{
			// GSVector4 dz = p.zzzz();

			shufps(xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

			// m_env.d4.z = dz * 4.0f;

			movaps(xmm1, xmm0);
			mulps(xmm1, xmm3);
			movdqa(xmmword[&m_env.d4.z], xmm1);

			for(int i = 0; i < 4; i++)
			{
				// m_env.d[i].z = dz * m_shift[i];

				movaps(xmm1, xmm0);
				mulps(xmm1, Xmm(4 + i));
				movdqa(xmmword[&m_env.d[i].z], xmm1);
			}
		}
	}
	else
	{
		// GSVector4 p = vertices[0].p;

		movaps(xmm0, xmmword[ecx + 16]);

		if(m_en.f)
		{
			// m_env.p.f = GSVector4i(p).zzzzh().zzzz();

			movaps(xmm1, xmm0);
			cvttps2dq(xmm1, xmm1);
			pshufhw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
			pshufd(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
			movdqa(xmmword[&m_env.p.f], xmm1);
		}

		if(m_en.z)
		{
			// GSVector4 z = p.zzzz();

			shufps(xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

			if(m_sel.zoverflow)
			{
				// m_env.p.z = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001());

				static const float half = 0.5f;

				movss(xmm1, dword[&half]);
				shufps(xmm1, xmm1, _MM_SHUFFLE(0, 0, 0, 0));
				mulps(xmm1, xmm0);
				cvttps2dq(xmm1, xmm1);
				pslld(xmm1, 1);

				cvttps2dq(xmm0, xmm0);
				pcmpeqd(xmm2, xmm2);
				psrld(xmm2, 31);
				pand(xmm0, xmm2);
				
				por(xmm0, xmm1);
			}
			else
			{
				// m_env.p.z = GSVector4i(z);

				cvttps2dq(xmm0, xmm0);
			}

			movdqa(xmmword[&m_env.p.z], xmm0);
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

	movaps(xmm0, xmmword[edx + 32]);

	movaps(xmm1, xmm0);
	mulps(xmm1, xmm3);

	if(m_sel.fst)
	{
		// m_env.d4.st = GSVector4i(t * 4.0f);

		cvttps2dq(xmm1, xmm1);
		movdqa(xmmword[&m_env.d4.st], xmm1);
	}
	else
	{
		// m_env.d4.stq = t * 4.0f;

		movaps(xmmword[&m_env.d4.stq], xmm1);
	}

	for(int j = 0, k = m_sel.fst ? 2 : 3; j < k; j++)
	{
		// GSVector4 ds = t.xxxx();
		// GSVector4 dt = t.yyyy();
		// GSVector4 dq = t.zzzz();

		movaps(xmm1, xmm0);
		shufps(xmm1, xmm1, (uint8)_MM_SHUFFLE(j, j, j, j));

		for(int i = 0; i < 4; i++)
		{
			// GSVector4 v = ds/dt * m_shift[i];

			movaps(xmm2, xmm1);
			mulps(xmm2, Xmm(4 + i));

			if(m_sel.fst)
			{
				// m_env.d[i].si/ti = GSVector4i(v);

				cvttps2dq(xmm2, xmm2);

				switch(j)
				{
				case 0: movdqa(xmmword[&m_env.d[i].si], xmm2); break;
				case 1: movdqa(xmmword[&m_env.d[i].ti], xmm2); break;
				}
			}
			else
			{
				// m_env.d[i].s/t/q = v;

				switch(j)
				{
				case 0: movaps(xmmword[&m_env.d[i].s], xmm2); break;
				case 1: movaps(xmmword[&m_env.d[i].t], xmm2); break;
				case 2: movaps(xmmword[&m_env.d[i].q], xmm2); break;
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

		movaps(xmm0, xmmword[edx]);
		movaps(xmm1, xmm0);

		// m_env.d4.c = GSVector4i(c * 4.0f).xzyw().ps32();

		movaps(xmm2, xmm0);
		mulps(xmm2, xmm3);
		cvttps2dq(xmm2, xmm2);
		pshufd(xmm2, xmm2, _MM_SHUFFLE(3, 1, 2, 0));
		packssdw(xmm2, xmm2);
		movdqa(xmmword[&m_env.d4.c], xmm2);

		// xmm3 is not needed anymore

		// GSVector4 dr = c.xxxx();
		// GSVector4 db = c.zzzz();

		shufps(xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
		shufps(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));

		for(int i = 0; i < 4; i++)
		{
			// GSVector4i r = GSVector4i(dr * m_shift[i]).ps32();

			movaps(xmm2, xmm0);
			mulps(xmm2, Xmm(4 + i));
			cvttps2dq(xmm2, xmm2);
			packssdw(xmm2, xmm2);

			// GSVector4i b = GSVector4i(db * m_shift[i]).ps32();

			movaps(xmm3, xmm1);
			mulps(xmm3, Xmm(4 + i));
			cvttps2dq(xmm3, xmm3);
			packssdw(xmm3, xmm3);

			// m_env.d[i].rb = r.upl16(b);

			punpcklwd(xmm2, xmm3);
			movdqa(xmmword[&m_env.d[i].rb], xmm2);
		}

		// GSVector4 c = dscan.c;

		movaps(xmm0, xmmword[edx]); // not enough regs, have to reload it
		movaps(xmm1, xmm0);

		// GSVector4 dg = c.yyyy();
		// GSVector4 da = c.wwww();

		shufps(xmm0, xmm0, _MM_SHUFFLE(1, 1, 1, 1));
		shufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));

		for(int i = 0; i < 4; i++)
		{
			// GSVector4i g = GSVector4i(dg * m_shift[i]).ps32();

			movaps(xmm2, xmm0);
			mulps(xmm2, Xmm(4 + i));
			cvttps2dq(xmm2, xmm2);
			packssdw(xmm2, xmm2);

			// GSVector4i a = GSVector4i(da * m_shift[i]).ps32();

			movaps(xmm3, xmm1);
			mulps(xmm3, Xmm(4 + i));
			cvttps2dq(xmm3, xmm3);
			packssdw(xmm3, xmm3);

			// m_env.d[i].ga = g.upl16(a);

			punpcklwd(xmm2, xmm3);
			movdqa(xmmword[&m_env.d[i].ga], xmm2);
		}
	}
	else
	{
		// GSVector4i c = GSVector4i(vertices[0].c);

		movaps(xmm0, xmmword[ecx]);
		cvttps2dq(xmm0, xmm0);

		// c = c.upl16(c.zwxy());

		movdqa(xmm1, xmm0);
		pshufd(xmm1, xmm1, _MM_SHUFFLE(1, 0, 3, 2));
		punpcklwd(xmm0, xmm1);

		// if(!tme) c = c.srl16(7);

		if(m_sel.tfx == TFX_NONE)
		{
			psrlw(xmm0, 7);
		}

		// m_env.c.rb = c.xxxx();
		// m_env.c.ga = c.zzzz();

		movdqa(xmm1, xmm0);
		pshufd(xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
		pshufd(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
		movdqa(xmmword[&m_env.c.rb], xmm0);
		movdqa(xmmword[&m_env.c.ga], xmm1);
	}
}

const GSVector4 GSSetupPrimCodeGenerator::m_shift[5] = 
{
	GSVector4(4.0f, 4.0f, 4.0f, 4.0f),
	GSVector4(0.0f, 1.0f, 2.0f, 3.0f),
	GSVector4(-1.0f, 0.0f, 1.0f, 2.0f),
	GSVector4(-2.0f, -1.0f, 0.0f, 1.0f),
	GSVector4(-3.0f, -2.0f, -1.0f, 0.0f),
};