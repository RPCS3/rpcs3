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

#include "stdafx.h"
#include "GSdx9.h"
#include "GSState.h"

BOOL IsDepthFormatOk(IDirect3D9* pD3D, D3DFORMAT DepthFormat, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat)
{
    // Verify that the depth format exists.
	HRESULT hr = pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, CGSdx9App::D3DDEVTYPE_X, AdapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, DepthFormat);
    if(FAILED(hr)) return FALSE;

    // Verify that the depth format is compatible.
    hr = pD3D->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, CGSdx9App::D3DDEVTYPE_X, AdapterFormat, BackBufferFormat, DepthFormat);
    return SUCCEEDED(hr);
}

HRESULT CompileShaderFromResource(IDirect3DDevice9* pD3DDev, UINT id, CString entry, CString target, UINT flags, IDirect3DPixelShader9** ppPixelShader)
{
	CheckPointer(pD3DDev, E_POINTER);
	CheckPointer(ppPixelShader, E_POINTER);

	CComPtr<ID3DXBuffer> pShader, pErrorMsgs;
	HRESULT hr = D3DXCompileShaderFromResource(
		AfxGetResourceHandle(), MAKEINTRESOURCE(id),
		NULL, NULL, 
		entry, target, flags, 
		&pShader, &pErrorMsgs, NULL);
	ASSERT(SUCCEEDED(hr));

	if(SUCCEEDED(hr))
	{
		hr = pD3DDev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), ppPixelShader);
		ASSERT(SUCCEEDED(hr));
	}

	return hr;
}

HRESULT AssembleShaderFromResource(IDirect3DDevice9* pD3DDev, UINT id, UINT flags, IDirect3DPixelShader9** ppPixelShader)
{
	CheckPointer(pD3DDev, E_POINTER);
	CheckPointer(ppPixelShader, E_POINTER);

	CComPtr<ID3DXBuffer> pShader, pErrorMsgs;
	HRESULT hr = D3DXAssembleShaderFromResource(
		AfxGetResourceHandle(), MAKEINTRESOURCE(id),
		NULL, NULL, 
		flags, 
		&pShader, &pErrorMsgs);
	ASSERT(SUCCEEDED(hr));

	if(SUCCEEDED(hr))
	{
		hr = pD3DDev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), ppPixelShader);
		ASSERT(SUCCEEDED(hr));
	}

	return hr;
}
