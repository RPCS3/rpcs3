/* 
 *	Copyright (C) 2003-2005 Gabest
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

class GSCapture
{
	CComPtr<IDirect3DSurface9> m_pRTSurf, m_pSysMemSurf;

	CComPtr<IGraphBuilder> m_pGB;
	CComPtr<IBaseFilter> m_pSrc;

public:
	GSCapture();

	bool BeginCapture(IDirect3DDevice9* pD3Dev, int fps);
	bool BeginFrame(int& w, int& h, IDirect3DSurface9** pRTSurf);
	bool EndFrame();
	bool EndCapture();

	bool IsCapturing() {return !!m_pRTSurf;}
};
