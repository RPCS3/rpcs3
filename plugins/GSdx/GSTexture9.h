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

#include "GSTexture.h"

class GSTexture9 : public GSTexture
{
	CComPtr<IDirect3DDevice9> m_dev;
	CComPtr<IDirect3DSurface9> m_surface;
	CComPtr<IDirect3DTexture9> m_texture; 
	D3DSURFACE_DESC m_desc;

public:
	GSTexture9();
	explicit GSTexture9(IDirect3DSurface9* surface);
	explicit GSTexture9(IDirect3DTexture9* texture);
	virtual ~GSTexture9();

	operator bool();

	int GetType() const;
	int GetWidth() const;
	int GetHeight() const;
	int GetFormat() const;
	bool Update(const CRect& r, const void* data, int pitch);
	bool Map(BYTE** bits, int& pitch, const RECT* r = NULL);
	void Unmap();
	bool Save(CString fn, bool dds = false);

	IDirect3DTexture9* operator->(); // TODO: remove direct access

	operator IDirect3DSurface9*();
	operator IDirect3DTexture9*();
};
