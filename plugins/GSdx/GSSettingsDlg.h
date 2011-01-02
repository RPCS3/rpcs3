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

#include "GSDialog.h"
#include "GSSetting.h"

class GSSettingsDlg : public GSDialog
{
	list<D3DDISPLAYMODE> m_modes;
	bool m_IsOpen2;

	void UpdateControls();

protected:
	void OnInit();
	bool OnCommand(HWND hWnd, UINT id, UINT code);

	uint m_lastValidMsaa; //used to revert to previous dialog value if the user changed to invalid one, or lesser one and canceled

public:
	GSSettingsDlg( bool isOpen2 );

	static GSSetting g_renderers[];
	static GSSetting g_interlace[];
	static GSSetting g_aspectratio[];
	static GSSetting g_upscale_multiplier[];
};
