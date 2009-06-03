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

#include "StdAfx.h"
#include "GSRenderer.h"

GSRenderer::GSRenderer(uint8* base, bool mt, void (*irq)(), GSDevice* dev, bool psrr)
	: GSState(base, mt, irq)
	, m_dev(dev)
	, m_shader(0)
	, m_psrr(psrr)
{
	m_interlace = theApp.GetConfig("interlace", 0);
	m_aspectratio = theApp.GetConfig("aspectratio", 1);
	m_filter = theApp.GetConfig("filter", 1);
	m_vsync = !!theApp.GetConfig("vsync", 0);
	m_nativeres = !!theApp.GetConfig("nativeres", 0);
	m_aa1 = !!theApp.GetConfig("aa1", 0);
	m_blur = !!theApp.GetConfig("blur", 0);

	s_n = 0;
	s_dump = !!theApp.GetConfig("dump", 0);
	s_save = !!theApp.GetConfig("save", 0);
	s_savez = !!theApp.GetConfig("savez", 0);
}

GSRenderer::~GSRenderer()
{
	delete m_dev;
}

bool GSRenderer::Create(const string& title)
{
	if(!m_wnd.Create(title.c_str()))
	{
		return false;
	}

	ASSERT(m_dev);

	if(!m_dev->Create(&m_wnd, m_vsync))
	{
		return false;
	}

	Reset();

	return true;
}

bool GSRenderer::Merge(int field)
{
	bool en[2];

	GSVector4i fr[2];
	GSVector4i dr[2];

	int baseline = INT_MAX;

	for(int i = 0; i < 2; i++)
	{
		en[i] = IsEnabled(i);

		if(en[i])
		{
			fr[i] = GetFrameRect(i);
			dr[i] = GetDisplayRect(i);

			baseline = min(dr[i].top, baseline);

			// printf("[%d]: %d %d %d %d, %d %d %d %d\n", i, fr[i], dr[i]); 
		}
	}

	if(!en[0] && !en[1])
	{
		return false;
	}

	// try to avoid fullscreen blur, could be nice on tv but on a monitor it's like double vision, hurts my eyes (persona 4, guitar hero)
	//
	// NOTE: probably the technique explained in graphtip.pdf (Antialiasing by Supersampling / 4. Reading Odd/Even Scan Lines Separately with the PCRTC then Blending)

	bool samesrc = 
		en[0] && en[1] && 
		m_regs->DISP[0].DISPFB.FBP == m_regs->DISP[1].DISPFB.FBP && 
		m_regs->DISP[0].DISPFB.FBW == m_regs->DISP[1].DISPFB.FBW && 
		m_regs->DISP[0].DISPFB.PSM == m_regs->DISP[1].DISPFB.PSM;

	bool blurdetected = false;

	if(samesrc && m_regs->PMODE.SLBG == 0 && m_regs->PMODE.MMOD == 1 && m_regs->PMODE.ALP == 0x80)
	{
		if(fr[0].eq(fr[1] + GSVector4i(0, -1, 0, 0)) && dr[0].eq(dr[1] + GSVector4i(0, 0, 0, 1))
		|| fr[1].eq(fr[0] + GSVector4i(0, -1, 0, 0)) && dr[1].eq(dr[0] + GSVector4i(0, 0, 0, 1)))
		{
			// persona 4:
			//
			// fr[0] = 0 0 640 448
			// fr[1] = 0 1 640 448
			// dr[0] = 159 50 779 498
			// dr[1] = 159 50 779 497
			//
			// second image shifted up by 1 pixel and blended over itself
			//
			// god of war:
			//
			// fr[0] = 0 1 512 448
			// fr[1] = 0 0 512 448
			// dr[0] = 127 50 639 497
			// dr[1] = 127 50 639 498
			//
			// same just the first image shifted

			int top = min(fr[0].top, fr[1].top);
			int bottom = max(dr[0].bottom, dr[1].bottom);

			fr[0].top = top;
			fr[1].top = top;
			dr[0].bottom = bottom;
			dr[1].bottom = bottom;

			blurdetected = true;
		}
		else if(dr[0].eq(dr[1]) && (fr[0].eq(fr[1] + GSVector4i(0, 1, 0, 1)) || fr[1].eq(fr[0] + GSVector4i(0, 1, 0, 1))))
		{
			// dq5:
			//
			// fr[0] = 0 1 512 445
			// fr[1] = 0 0 512 444
			// dr[0] = 127 50 639 494
			// dr[1] = 127 50 639 494

			int top = min(fr[0].top, fr[1].top);
			int bottom = min(fr[0].bottom, fr[1].bottom);

			fr[0].top = fr[1].top = top;
			fr[0].bottom = fr[1].bottom = bottom;

			blurdetected = true;
		}
	}

	GSVector2i fs(0, 0);
	GSVector2i ds(0, 0);

	GSTexture* tex[2] = {NULL, NULL};

	if(samesrc && fr[0].bottom == fr[1].bottom)
	{
		tex[0] = GetOutput(0);
		tex[1] = tex[0]; // saves one texture fetch
	}
	else
	{
		if(en[0]) tex[0] = GetOutput(0);
		if(en[1]) tex[1] = GetOutput(1);
	}

	GSVector4 src[2];
	GSVector4 dst[2];

	for(int i = 0; i < 2; i++)
	{
		if(!en[i] || !tex[i]) continue;

		GSVector4i r = fr[i];

		// overscan hack

		if(dr[i].height() > 512) // hmm
		{
			int y = GetDeviceSize(i).y;
			if(m_regs->SMODE2.INT && m_regs->SMODE2.FFMD) y /= 2;
			r.bottom = r.top + y;
		}

		//

		if(m_blur && blurdetected && i == 1)
		{
			r += GSVector4i(0, 1).xyxy();
		}

		GSVector4 scale = GSVector4(tex[i]->m_scale).xyxy();

		src[i] = GSVector4(r) * scale / GSVector4(tex[i]->GetSize()).xyxy();

		GSVector2 o(0, 0);
		
		if(dr[i].top - baseline >= 4) // 2?
		{
			o.y = tex[i]->m_scale.y * (dr[i].top - baseline);

			if(m_regs->SMODE2.INT && m_regs->SMODE2.FFMD)
			{
				o.y /= 2;
			}
		}

		dst[i] = GSVector4(o).xyxy() + scale * GSVector4(r.rsize());

		fs.x = max(fs.x, (int)(dst[i].z + 0.5f));
		fs.y = max(fs.y, (int)(dst[i].w + 0.5f));
	}

	ds = fs;

	if(m_regs->SMODE2.INT && m_regs->SMODE2.FFMD)
	{
		ds.y *= 2;
	}

	bool slbg = m_regs->PMODE.SLBG;
	bool mmod = m_regs->PMODE.MMOD;

	if(tex[0] || tex[1])
	{
		GSVector4 c = GSVector4((int)m_regs->BGCOLOR.R, (int)m_regs->BGCOLOR.G, (int)m_regs->BGCOLOR.B, (int)m_regs->PMODE.ALP) / 255;

		m_dev->Merge(tex, src, dst, fs, slbg, mmod, c);

		if(m_regs->SMODE2.INT && m_interlace > 0)
		{
			int field2 = 1 - ((m_interlace - 1) & 1);
			int mode = (m_interlace - 1) >> 1;

			m_dev->Interlace(ds, field ^ field2, mode, tex[1]->m_scale.y);
		}
	}

	return true;
}


void GSRenderer::VSync(int field)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	m_perfmon.Put(GSPerfMon::Frame);

	Flush();

	field = field ? 1 : 0;

	if(!Merge(field)) return;

	// osd 

	if((m_perfmon.GetFrame() & 0x1f) == 0)
	{
		m_perfmon.Update();

		double fps = 1000.0f / m_perfmon.Get(GSPerfMon::Frame);

		string s2 = m_regs->SMODE2.INT ? (string("Interlaced ") + (m_regs->SMODE2.FFMD ? "(frame)" : "(field)")) : "Progressive";

		GSVector4i r = GetDisplayRect();
		
		string s = format(
			"%I64d | %d x %d | %.2f fps (%d%%) | %s - %s | %s | %d/%d/%d | %d%% CPU | %.2f | %.2f", 
			m_perfmon.GetFrame(), r.width(), r.height(), fps, (int)(100.0 * fps / GetFPS()),
			s2.c_str(),
			GSSettingsDlg::g_interlace[m_interlace].name,
			GSSettingsDlg::g_aspectratio[m_aspectratio].name,
			(int)m_perfmon.Get(GSPerfMon::Quad),
			(int)m_perfmon.Get(GSPerfMon::Prim),
			(int)m_perfmon.Get(GSPerfMon::Draw),
			m_perfmon.CPU(),
			m_perfmon.Get(GSPerfMon::Swizzle) / 1024,
			m_perfmon.Get(GSPerfMon::Unswizzle) / 1024
		);

		double fillrate = m_perfmon.Get(GSPerfMon::Fillrate);

		if(fillrate > 0)
		{
			s += format(" | %.2f mpps", fps * fillrate / (1024 * 1024));
		}

		if(m_capture.IsCapturing())
		{
			s += " | Recording...";
		}

		m_wnd.SetWindowText(s.c_str());
	}

	if(m_frameskip)
	{
		return;
	}

	// present

	if(m_dev->IsLost())
	{
		ResetDevice();
	}

	m_dev->Present(m_wnd.GetClientRect().fit(m_aspectratio), m_shader);

	// snapshot

	if(!m_snapshot.empty())
	{
		if(!m_dump && (::GetAsyncKeyState(VK_SHIFT) & 0x8000))
		{
			GSFreezeData fd;
			fd.size = 0;
			fd.data = NULL;
			Freeze(&fd, true);
			fd.data = new uint8[fd.size];
			Freeze(&fd, false);

			m_dump.Open(m_snapshot, m_crc, fd, m_regs);

			delete [] fd.data;
		}

		if(GSTexture* t = m_dev->GetCurrent())
		{
			t->Save(m_snapshot + ".bmp");
		}

		m_snapshot.clear();
	}
	else
	{
		if(m_dump)
		{
			m_dump.VSync(field, !(::GetAsyncKeyState(VK_CONTROL) & 0x8000), m_regs);
		}
	}

	// capture

	if(m_capture.IsCapturing())
	{
		if(GSTexture* current = m_dev->GetCurrent())
		{
			GSVector2i size = m_capture.GetSize();

			if(GSTexture* offscreen = m_dev->CopyOffscreen(current, GSVector4(0, 0, 1, 1), size.x, size.y))
			{
				uint8* bits = NULL;
				int pitch = 0;

				if(offscreen->Map(&bits, pitch))
				{
					m_capture.DeliverFrame(bits, pitch, m_dev->IsCurrentRGBA());

					offscreen->Unmap();
				}

				m_dev->Recycle(offscreen);
			}
		}
	}
}

bool GSRenderer::MakeSnapshot(const string& path)
{
	if(m_snapshot.empty())
	{
		time_t t = time(NULL);

		char buff[16];

		if(strftime(buff, sizeof(buff), "%Y%m%d%H%M%S", localtime(&t)))
		{
			m_snapshot = format("%s_%s", path.c_str(), buff);
		}
	}

	return true;
}

void GSRenderer::KeyEvent(GSKeyEventData* e)
{
	if(e->type == KEYPRESS)
	{
		// TODO: linux

		int step = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1;

		switch(e->key)
		{
		case VK_F5:
			m_interlace = (m_interlace + 7 + step) % 7;
			return;
		case VK_F6:
			m_aspectratio = (m_aspectratio + 3 + step) % 3;
			return;
		case VK_F7:
			m_shader = (m_shader + 3 + step) % 3;
			return;
		case VK_F12:
			if(m_capture.IsCapturing()) m_capture.EndCapture();
			else m_capture.BeginCapture(GetFPS());
			return;
		case VK_DELETE:
			m_aa1 = !m_aa1;
			return;
		case VK_END:
			m_blur = !m_blur;
			return;
		}
	}
}

