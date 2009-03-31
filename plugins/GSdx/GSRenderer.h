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

#include "GSWnd.h"
#include "GSState.h"
#include "GSVertexList.h"
#include "GSSettingsDlg.h"
#include "GSCapture.h"

struct GSRendererSettings
{
	int m_interlace;
	int m_aspectratio;
	int m_filter;
	bool m_vsync;
	bool m_nativeres;
	bool m_aa1;
	bool m_blur;
};

class GSRendererBase : public GSState, protected GSRendererSettings
{
protected:
	bool m_osd;

	void ProcessWindowMessages()
	{
		MSG msg;

		memset(&msg, 0, sizeof(msg));

		while(msg.message != WM_QUIT && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(OnMessage(msg))
			{
				continue;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	virtual bool OnMessage(const MSG& msg)
	{
		if(msg.message == WM_KEYDOWN)
		{
			int step = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1;

			if(msg.wParam == VK_F5)
			{
				m_interlace = (m_interlace + 7 + step) % 7;
				return true;
			}

			if(msg.wParam == VK_F6)
			{
				m_aspectratio = (m_aspectratio + 3 + step) % 3;
				return true;
			}			

			if(msg.wParam == VK_F7)
			{
				m_wnd.SetWindowText(_T("PCSX2"));
				m_osd = !m_osd;
				return true;
			}

			if(msg.wParam == VK_DELETE)
			{
				m_aa1 = !m_aa1;
				return true;
			}			

			if(msg.wParam == VK_END)
			{
				m_blur = !m_blur;
				return true;
			}			
		}

		return false;
	}

public:
	GSWnd m_wnd;

public:
	GSRendererBase(BYTE* base, bool mt, void (*irq)(), int nloophack, const GSRendererSettings& rs)
		: GSState(base, mt, irq, nloophack)
		, m_osd(true)
	{
		m_interlace = rs.m_interlace;
		m_aspectratio = rs.m_aspectratio;
		m_filter = rs.m_filter;
		m_vsync = rs.m_vsync;
		m_nativeres = rs.m_nativeres;
		m_aa1 = rs.m_aa1;
		m_blur = rs.m_blur;
	};

	virtual bool Create(LPCTSTR title) = 0;
	virtual void VSync(int field) = 0;
	virtual bool MakeSnapshot(LPCTSTR path) = 0;
};

template<class Device> class GSRenderer : public GSRendererBase
{
protected:
	typedef typename Device::Texture Texture;

	virtual void ResetDevice() {}
	virtual bool GetOutput(int i, Texture& t) = 0;

	bool Merge(int field)
	{
		bool en[2];

		CRect fr[2];
		CRect dr[2];

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
			if(fr[0] == fr[1] + CRect(0, 1, 0, 0) && dr[0] == dr[1] + CRect(0, 0, 0, 1)
			|| fr[1] == fr[0] + CRect(0, 1, 0, 0) && dr[1] == dr[0] + CRect(0, 0, 0, 1))
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
			else if(dr[0] == dr[1] && (fr[0] == fr[1] + CPoint(0, 1) || fr[1] == fr[0] + CPoint(0, 1)))
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

		CSize fs(0, 0);
		CSize ds(0, 0);

		Texture tex[2];

		if(samesrc && fr[0].bottom == fr[1].bottom)
		{
			GetOutput(0, tex[0]);

			tex[1] = tex[0]; // saves one texture fetch
		}
		else
		{
			if(en[0]) GetOutput(0, tex[0]);
			if(en[1]) GetOutput(1, tex[1]);
		}

		GSVector4 src[2];
		GSVector4 dst[2];

		for(int i = 0; i < 2; i++)
		{
			if(!en[i] || !tex[i]) continue;

			CRect r = fr[i];

			// overscan hack

			if(dr[i].Height() > 512) // hmm
			{
				int y = GetDeviceSize(i).cy;
				if(m_regs->SMODE2.INT && m_regs->SMODE2.FFMD) y /= 2;
				r.bottom = r.top + y;
			}

			//

			if(m_blur && blurdetected && i == 1)
			{
				src[i].x = tex[i].m_scale.x * r.left / tex[i].GetWidth();
				src[i].y = (tex[i].m_scale.y * r.top + 1) / tex[i].GetHeight();
				src[i].z = tex[i].m_scale.x * r.right / tex[i].GetWidth();
				src[i].w = (tex[i].m_scale.y * r.bottom + 1) / tex[i].GetHeight();
			}
			else
			{
				src[i].x = tex[i].m_scale.x * r.left / tex[i].GetWidth();
				src[i].y = tex[i].m_scale.y * r.top / tex[i].GetHeight();
				src[i].z = tex[i].m_scale.x * r.right / tex[i].GetWidth();
				src[i].w = tex[i].m_scale.y * r.bottom / tex[i].GetHeight();
			}

			GSVector2 o;

			o.x = 0;
			o.y = 0;
			
			if(dr[i].top - baseline >= 4) // 2?
			{
				o.y = tex[i].m_scale.y * (dr[i].top - baseline);
			}

			if(m_regs->SMODE2.INT && m_regs->SMODE2.FFMD) o.y /= 2;

			dst[i].x = o.x;
			dst[i].y = o.y;
			dst[i].z = o.x + tex[i].m_scale.x * r.Width();
			dst[i].w = o.y + tex[i].m_scale.y * r.Height();

			fs.cx = max(fs.cx, (int)(dst[i].z + 0.5f));
			fs.cy = max(fs.cy, (int)(dst[i].w + 0.5f));
		}

		ds.cx = fs.cx;
		ds.cy = fs.cy;

		if(m_regs->SMODE2.INT && m_regs->SMODE2.FFMD) ds.cy *= 2;

		bool slbg = m_regs->PMODE.SLBG;
		bool mmod = m_regs->PMODE.MMOD;

		if(tex[0] || tex[1])
		{
			GSVector4 c;

			c.r = (float)m_regs->BGCOLOR.R / 255;
			c.g = (float)m_regs->BGCOLOR.G / 255;
			c.b = (float)m_regs->BGCOLOR.B / 255;
			c.a = (float)m_regs->PMODE.ALP / 255;

			m_dev.Merge(tex, src, dst, fs, slbg, mmod, c);

			if(m_regs->SMODE2.INT && m_interlace > 0)
			{
				int field2 = 1 - ((m_interlace - 1) & 1);
				int mode = (m_interlace - 1) >> 1;

				if(!m_dev.Interlace(ds, field ^ field2, mode, tex[1].m_scale.y))
				{
					return false;
				}
			}
		}

		return true;
	}

	void DoSnapshot(int field)
	{
		if(!m_snapshot.IsEmpty())
		{
			if(!m_dump && (::GetAsyncKeyState(VK_SHIFT) & 0x8000))
			{
				GSFreezeData fd;
				fd.size = 0;
				fd.data = NULL;
				Freeze(&fd, true);
				fd.data = new BYTE[fd.size];
				Freeze(&fd, false);

				m_dump.Open(m_snapshot, m_crc, fd, m_regs);

				delete [] fd.data;
			}

			m_dev.SaveCurrent(m_snapshot + _T(".bmp"));

			m_snapshot.Empty();
		}
		else
		{
			if(m_dump)
			{
				m_dump.VSync(field, !(::GetAsyncKeyState(VK_CONTROL) & 0x8000), m_regs);
			}
		}
	}

	void DoCapture()
	{
		if(!m_capture.IsCapturing())
		{
			return;
		}

		CSize size = m_capture.GetSize();

		Texture current;

		m_dev.GetCurrent(current);

		Texture offscreen;

		if(m_dev.CopyOffscreen(current, GSVector4(0, 0, 1, 1), offscreen, size.cx, size.cy))
		{
			BYTE* bits = NULL;
			int pitch = 0;

			if(offscreen.Map(&bits, pitch))
			{
				m_capture.DeliverFrame(bits, pitch, m_dev.IsCurrentRGBA());

				offscreen.Unmap();
			}

			m_dev.Recycle(offscreen);
		}
	}

	virtual bool OnMessage(const MSG& msg)
	{
		if(msg.message == WM_KEYDOWN)
		{
			if(msg.wParam == VK_F12)
			{
				if(m_capture.IsCapturing())
				{
					m_capture.EndCapture();
				}
				else
				{
					m_capture.BeginCapture(GetFPS());
				}

				return true;
			}
		}

		return __super::OnMessage(msg);
	}

public:
	Device m_dev;
	bool m_psrr;

	int s_n;
	bool s_dump;
	bool s_save;
	bool s_savez;

	CString m_snapshot;
	GSCapture m_capture;

public:
	GSRenderer(BYTE* base, bool mt, void (*irq)(), int nloophack, const GSRendererSettings& rs, bool psrr)
		: GSRendererBase(base, mt, irq, nloophack, rs)
		, m_psrr(psrr)
	{
		s_n = 0;
		s_dump = !!AfxGetApp()->GetProfileInt(_T("Debug"), _T("dump"), 0);
		s_save = !!AfxGetApp()->GetProfileInt(_T("Debug"), _T("save"), 0);
		s_savez = !!AfxGetApp()->GetProfileInt(_T("Debug"), _T("savez"), 0);
	}

	bool Create(LPCTSTR title)
	{
		if(!m_wnd.Create(title))
		{
			return false;
		}

		if(!m_dev.Create(m_wnd, m_vsync))
		{
			return false;
		}

		Reset();

		return true;
	}

	void VSync(int field)
	{
		GSPerfMonAutoTimer pmat(m_perfmon);

		Flush();

		m_perfmon.Put(GSPerfMon::Frame);

		ProcessWindowMessages();

		field = field ? 1 : 0;

		if(!Merge(field)) return;

		// osd 

		static UINT64 s_frame = 0;
		static CString s_stats;

		if(m_perfmon.GetFrame() - s_frame >= 30)
		{
			m_perfmon.Update();

			s_frame = m_perfmon.GetFrame();

			double fps = 1000.0f / m_perfmon.Get(GSPerfMon::Frame);
			
			s_stats.Format(
				_T("%I64d | %d x %d | %.2f fps (%d%%) | %s - %s | %s | %d/%d/%d | %d%% CPU | %.2f | %.2f"), 
				m_perfmon.GetFrame(), GetDisplaySize().cx, GetDisplaySize().cy, fps, (int)(100.0 * fps / GetFPS()),
				m_regs->SMODE2.INT ? (CString(_T("Interlaced ")) + (m_regs->SMODE2.FFMD ? _T("(frame)") : _T("(field)"))) : _T("Progressive"),
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
				s_stats.Format(_T("%s | %.2f mpps"), CString(s_stats), fps * fillrate / (1024 * 1024));
			}

			if(m_capture.IsCapturing())
			{
				s_stats += _T(" | Recording...");
			}

			if(m_perfmon.Get(GSPerfMon::COLCLAMP)) _tprintf(_T("*** NOT SUPPORTED: color wrap ***\n"));
			if(m_perfmon.Get(GSPerfMon::PABE)) _tprintf(_T("*** NOT SUPPORTED: per pixel alpha blend ***\n"));
			if(m_perfmon.Get(GSPerfMon::DATE)) _tprintf(_T("*** PERFORMANCE WARNING: destination alpha test used ***\n"));
			if(m_perfmon.Get(GSPerfMon::ABE)) _tprintf(_T("*** NOT SUPPORTED: alpha blending mode ***\n"));
			if(m_perfmon.Get(GSPerfMon::DepthTexture)) _tprintf(_T("*** NOT SUPPORTED: depth texture ***\n"));		

			m_wnd.SetWindowText(s_stats);
		}

		if(m_osd)
		{
			m_dev.Draw(s_stats + _T("\n\nF5: interlace mode\nF6: aspect ratio\nF7: OSD"));
		}

		if(m_frameskip)
		{
			return;
		}

		//

		if(m_dev.IsLost())
		{
			ResetDevice();
		}

		//

		CRect r;
		
		m_wnd.GetClientRect(&r);

		GSUtil::FitRect(r, m_aspectratio);

		m_dev.Present(r);

		//

		DoSnapshot(field);

		DoCapture();
	}

	bool MakeSnapshot(LPCTSTR path)
	{
		if(m_snapshot.IsEmpty())
		{
			m_snapshot.Format(_T("%s_%s"), path, CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S")));
		}

		return true;
	}

	virtual void MinMaxUV(int w, int h, CRect& r) {r = CRect(0, 0, w, h);}
	virtual bool CanUpscale() {return !m_nativeres;}
};

template<class Device, class Vertex> class GSRendererT : public GSRenderer<Device>
{
protected:
	Vertex* m_vertices;
	int m_count;
	int m_maxcount;
	GSVertexList<Vertex> m_vl;

	void Reset()
	{
		m_count = 0;
		m_vl.RemoveAll();

		__super::Reset();
	}

	void ResetPrim()
	{
		m_vl.RemoveAll();
	}

	void FlushPrim() 
	{
		if(m_count > 0)
		{
			/*
			TRACE(_T("[%d] Draw f %05x (%d) z %05x (%d %d %d %d) t %05x %05x (%d)\n"), 
				  (int)m_perfmon.GetFrame(), 
				  (int)m_context->FRAME.Block(), 
				  (int)m_context->FRAME.PSM, 
				  (int)m_context->ZBUF.Block(), 
				  (int)m_context->ZBUF.PSM, 
				  m_context->TEST.ZTE, 
				  m_context->TEST.ZTST, 
				  m_context->ZBUF.ZMSK, 
				  PRIM->TME ? (int)m_context->TEX0.TBP0 : 0xfffff, 
				  PRIM->TME && m_context->TEX0.PSM > PSM_PSMCT16S ? (int)m_context->TEX0.CBP : 0xfffff, 
				  PRIM->TME ? (int)m_context->TEX0.PSM : 0xff);
			*/

			if(GSUtil::EncodePSM(m_context->FRAME.PSM) != 3 && GSUtil::EncodePSM(m_context->ZBUF.PSM) != 3)
			{
				// FIXME: berserk fpsm = 27 (8H)

				Draw();
			}

			m_count = 0;
		}
	}

	void GrowVertexBuffer()
	{
		m_maxcount = max(10000, m_maxcount * 3/2);
		m_vertices = (Vertex*)_aligned_realloc(m_vertices, sizeof(Vertex) * m_maxcount, 16);
		m_maxcount -= 100;
	}

	template<DWORD prim> __forceinline Vertex* DrawingKick(bool skip, DWORD& count)
	{
		switch(prim)
		{
		case GS_POINTLIST: count = 1; break;
		case GS_LINELIST: count = 2; break;
		case GS_LINESTRIP: count = 2; break;
		case GS_TRIANGLELIST: count = 3; break;
		case GS_TRIANGLESTRIP: count = 3; break;
		case GS_TRIANGLEFAN: count = 3; break;
		case GS_SPRITE: count = 2; break;
		case GS_INVALID: count = 1; break;
		default: __assume(0);
		}

		if(m_vl.GetCount() < count)
		{
			return NULL;
		}

		if(m_count >= m_maxcount)
		{
			GrowVertexBuffer();
		}

		Vertex* v = &m_vertices[m_count];

		switch(prim)
		{
		case GS_POINTLIST:
			m_vl.GetAt(0, v[0]);
			m_vl.RemoveAll();
			break;
		case GS_LINELIST:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.RemoveAll();
			break;
		case GS_LINESTRIP:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.RemoveAt(0, 1);
			break;
		case GS_TRIANGLELIST:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.GetAt(2, v[2]);
			m_vl.RemoveAll();
			break;
		case GS_TRIANGLESTRIP:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.GetAt(2, v[2]);
			m_vl.RemoveAt(0, 2);
			break;
		case GS_TRIANGLEFAN:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.GetAt(2, v[2]);
			m_vl.RemoveAt(1, 1);
			break;
		case GS_SPRITE:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.RemoveAll();
			break;
		case GS_INVALID:
			ASSERT(0);
			m_vl.RemoveAll();
			return NULL;
		default:
			__assume(0);
		}

		return !skip ? v : NULL;
	}

	virtual void Draw() = 0;

public:
	GSRendererT(BYTE* base, bool mt, void (*irq)(), int nloophack, const GSRendererSettings& rs, bool psrr = true)
		: GSRenderer<Device>(base, mt, irq, nloophack, rs, psrr)
		, m_count(0)
		, m_maxcount(0)
		, m_vertices(NULL)
	{
	}

	~GSRendererT()
	{
		if(m_vertices) _aligned_free(m_vertices);
	}
};
