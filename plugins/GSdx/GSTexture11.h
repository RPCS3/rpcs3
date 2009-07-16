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

class GSTexture11 : public GSTexture
{
	CComPtr<ID3D11Device> m_dev;
	CComPtr<ID3D11DeviceContext> m_ctx;
	CComPtr<ID3D11Texture2D> m_texture;
	D3D11_TEXTURE2D_DESC m_desc;
	CComPtr<ID3D11ShaderResourceView> m_srv;
	CComPtr<ID3D11RenderTargetView> m_rtv;
	CComPtr<ID3D11DepthStencilView> m_dsv;

public:
	explicit GSTexture11(ID3D11Texture2D* texture);

	int GetType() const;
	int GetFormat() const;

	bool Update(const GSVector4i& r, const void* data, int pitch);
	bool Map(GSMap& m, const GSVector4i* r);
	void Unmap();
	bool Save(const string& fn, bool dds = false);

	operator ID3D11Texture2D*();
	operator ID3D11ShaderResourceView*();
	operator ID3D11RenderTargetView*();
	operator ID3D11DepthStencilView*();
};
