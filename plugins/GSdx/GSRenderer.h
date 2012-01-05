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

#include "GSdx.h"
#include "GSWnd.h"
#include "GSState.h"
#include "GSCapture.h"

class GSRenderer : public GSState
{
	GSCapture m_capture;
	string m_snapshot;
	bool m_snapdump;
	int m_shader;

	bool Merge(int field);

protected:
	int m_interlace;
	int m_aspectratio;
	int m_filter;
	bool m_vsync;
	bool m_aa1;
	bool m_mipmap;
	bool m_framelimit;
	bool m_fxaa;

	virtual GSTexture* GetOutput(int i) = 0;

public:
	GSWnd m_wnd;
	GSDevice* m_dev;

	int s_n;
	bool s_dump;
	bool s_save;
	bool s_savez;
	int s_saven;

public:
	GSRenderer(GSVertexTrace* vt, size_t vertex_stride);
	virtual ~GSRenderer();

	virtual bool CreateWnd(const string& title, int w, int h);
	virtual bool CreateDevice(GSDevice* dev);
	virtual void ResetDevice();
	virtual void VSync(int field);
	virtual bool MakeSnapshot(const string& path);
	virtual void KeyEvent(GSKeyEventData* e);
	virtual bool CanUpscale() {return false;}
	virtual int GetUpscaleMultiplier() {return 1;}
	void SetAspectRatio(int aspect) {m_aspectratio = aspect;}
	void SetVSync(bool enabled);
	void SetFrameLimit(bool limit);
	virtual void SetExclusive(bool isExcl) {}

	virtual bool BeginCapture();
	virtual void EndCapture();

public:
	GSCritSec m_pGSsetTitle_Crit;

	char m_GStitleInfoBuffer[128];
};