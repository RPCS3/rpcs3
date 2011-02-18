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
#include "GSdx.h"
#include "GSDeviceDX.h"

GSDeviceDX::GSDeviceDX()
{
	m_msaa = theApp.GetConfig("msaa", 0);

	m_msaa_desc.Count = 1;
	m_msaa_desc.Quality = 0;
}

GSDeviceDX::~GSDeviceDX()
{
}

GSTexture* GSDeviceDX::Fetch(int type, int w, int h, bool msaa, int format)
{
	if(m_msaa < 2)
	{
		msaa = false;
	}

	return __super::Fetch(type, w, h, msaa, format);
}

bool GSDeviceDX::SetFeatureLevel(D3D_FEATURE_LEVEL level, bool compat_mode)
{
	m_shader.level = level;

	switch(level)
	{
	case D3D_FEATURE_LEVEL_9_1:
	case D3D_FEATURE_LEVEL_9_2:
		m_shader.model = "0x200";
		m_shader.vs = compat_mode ? "vs_4_0_level_9_1" : "vs_2_0";
		m_shader.ps = compat_mode ? "ps_4_0_level_9_1" : "ps_2_0";
		break;
	case D3D_FEATURE_LEVEL_9_3:
		m_shader.model = "0x300";
		m_shader.vs = compat_mode ? "vs_4_0_level_9_3" : "vs_3_0";
		m_shader.ps = compat_mode ? "ps_4_0_level_9_3" : "ps_3_0";
		break;
	case D3D_FEATURE_LEVEL_10_0:
		m_shader.model = "0x400";
		m_shader.vs = "vs_4_0";
		m_shader.gs = "gs_4_0";
		m_shader.ps = "ps_4_0";
		break;
	case D3D_FEATURE_LEVEL_10_1:
		m_shader.model = "0x401";
		m_shader.vs = "vs_4_1";
		m_shader.gs = "gs_4_1";
		m_shader.ps = "ps_4_1";
		break;
	case D3D_FEATURE_LEVEL_11_0:
		m_shader.model = "0x500";
		m_shader.vs = "vs_5_0";
		m_shader.gs = "gs_5_0";
		m_shader.ps = "ps_5_0";
		break;
	default:
		ASSERT(0);
		return false;
	}

	return true;
}

