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

#include "GSVector.h"
#include "GSThread.h"

#ifdef _WINDOWS
#include "GSCaptureDlg.h"
#endif

class GSCapture : protected GSCritSec
{
	bool m_capturing;
	GSVector2i m_size;

	#ifdef _WINDOWS

	CComPtr<IGraphBuilder> m_graph;
	CComPtr<IBaseFilter> m_src;

	#endif

public:
	GSCapture();
	virtual ~GSCapture();

	bool BeginCapture(float fps);
	bool DeliverFrame(const void* bits, int pitch, bool rgba);
	bool EndCapture();

	bool IsCapturing() {return m_capturing;}
	GSVector2i GetSize() {return m_size;}
};
