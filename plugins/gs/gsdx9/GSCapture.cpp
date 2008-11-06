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

#include "StdAfx.h"
#include "GSCapture.h"
#include "GSCaptureDlg.h"
#include <initguid.h>
#include <uuids.h>
//
// GSSource
//

#ifdef __INTEL_COMPILER
interface __declspec(uuid("59C193BB-C520-41F3-BC1D-E245B80A86FA")) 
#else
[uuid("59C193BB-C520-41F3-BC1D-E245B80A86FA")] interface
#endif
IGSSource : public IUnknown
{
	STDMETHOD(DeliverNewSegment)() PURE;
	STDMETHOD(DeliverFrame)(D3DLOCKED_RECT& r) PURE;
	STDMETHOD(DeliverEOS)() PURE;
};

#ifdef __INTEL_COMPILER
class __declspec(uuid("F8BB6F4F-0965-4ED4-BA74-C6A01E6E6C77"))
#else
[uuid("F8BB6F4F-0965-4ED4-BA74-C6A01E6E6C77")] class 
#endif
GSSource : public CBaseFilter, public IGSSource
{
	CCritSec m_csLock;
	CAutoPtr<CBaseOutputPin> m_pOutput;
	CSize m_size;
	REFERENCE_TIME m_rtAvgTimePerFrame, m_rtNow;

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		return 
			QI(IGSSource)
			__super::NonDelegatingQueryInterface(riid, ppv);
	}

	class GSSourceOutputPin : public CBaseOutputPin
	{
		CMediaType m_mt;

	public:
		GSSourceOutputPin(CMediaType& mt, CBaseFilter* pFilter, CCritSec* pLock, HRESULT& hr)
			: CBaseOutputPin("GSSourceOutputPin", pFilter, pLock, &hr, L"Output")
			, m_mt(mt)
		{
		}

		HRESULT GSSourceOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
		{
			ASSERT(pAlloc && pProperties);

			HRESULT hr;

			pProperties->cBuffers = 1;
			pProperties->cbBuffer = m_mt.lSampleSize;

			ALLOCATOR_PROPERTIES Actual;
			if(FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) return hr;

			if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;
			ASSERT(Actual.cBuffers == pProperties->cBuffers);

			return S_OK;
		}

	    HRESULT CheckMediaType(const CMediaType* pmt)
		{
			return pmt->majortype == MEDIATYPE_Video && pmt->subtype == MEDIASUBTYPE_RGB32 ? S_OK : E_FAIL;
		}

	    HRESULT GetMediaType(int iPosition, CMediaType* pmt)
		{
			CheckPointer(pmt, E_POINTER);
			if(iPosition < 0) return E_INVALIDARG;
			if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;
			*pmt = m_mt;
			return S_OK;
		}

		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q)
		{
			return E_NOTIMPL;
		}
	};

public:
	GSSource(int w, int h, int fps, IUnknown* pUnk, HRESULT& hr)
		: CBaseFilter(NAME("GSSource"), pUnk, &m_csLock, __uuidof(this), &hr)
		, m_pOutput(NULL)
		, m_size(w, h)
		, m_rtAvgTimePerFrame(10000000i64/fps)
		, m_rtNow(0)
	{
		CMediaType mt;

		mt.majortype = MEDIATYPE_Video;
		mt.subtype = MEDIASUBTYPE_RGB32;
		mt.formattype = FORMAT_VideoInfo;
		mt.lSampleSize = w*h*4;

		VIDEOINFOHEADER vih;
		memset(&vih, 0, sizeof(vih));
		vih.AvgTimePerFrame = m_rtAvgTimePerFrame;
		vih.bmiHeader.biSize = sizeof(vih.bmiHeader);
		vih.bmiHeader.biCompression = BI_RGB;
		vih.bmiHeader.biPlanes = 1;
		vih.bmiHeader.biWidth = w;
		vih.bmiHeader.biHeight = -h;
		vih.bmiHeader.biBitCount = 32;
		vih.bmiHeader.biSizeImage = w*h*4;
		mt.SetFormat((BYTE*)&vih, sizeof(vih));

		m_pOutput.Attach(new GSSourceOutputPin(mt, this, &m_csLock, hr));
	}

	DECLARE_IUNKNOWN;

	int GetPinCount() {return 1;}
	CBasePin* GetPin(int n) {return n == 0 ? m_pOutput.m_p : NULL;}

	// IGSSource

	STDMETHODIMP DeliverNewSegment()
	{
		return m_pOutput->DeliverNewSegment(m_rtNow = 0, _I64_MAX, 1.0);
	}

	STDMETHODIMP DeliverFrame(D3DLOCKED_RECT& r)
	{
		HRESULT hr;

		if(!m_pOutput || !m_pOutput->IsConnected()) return E_UNEXPECTED;

		CComPtr<IMediaSample> pSample;
		if(FAILED(hr = m_pOutput->GetDeliveryBuffer(&pSample, NULL, NULL, 0)))
			return hr;

		REFERENCE_TIME rtStart = m_rtNow, rtStop = m_rtNow + m_rtAvgTimePerFrame;

		pSample->SetTime(&rtStart, &rtStop);
		pSample->SetSyncPoint(TRUE);

		BYTE* pSrc = (BYTE*)r.pBits;
		int srcpitch = r.Pitch;

		BYTE* pDst = NULL;
		pSample->GetPointer(&pDst);
		CMediaType mt;
		m_pOutput->GetMediaType(0, &mt);
		int dstpitch = ((VIDEOINFOHEADER*)mt.Format())->bmiHeader.biWidth*4;

		int minpitch = min(srcpitch, dstpitch);

		for(int y = 0; y < m_size.cy; y++, pDst += dstpitch, pSrc += srcpitch)
			memcpy(pDst, pSrc, minpitch);

		if(FAILED(hr = m_pOutput->Deliver(pSample)))
			return hr;

		m_rtNow = rtStop;

		return S_OK;
	}

	STDMETHODIMP DeliverEOS()
	{
		return m_pOutput->DeliverEndOfStream();
	}
};

//
// GSCapture
//

GSCapture::GSCapture()
{
}

bool GSCapture::BeginCapture(IDirect3DDevice9* pD3Dev, int fps)
{
	ASSERT(pD3Dev != NULL && fps != 0);

	EndCapture();

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	GSCaptureDlg dlg;
	dlg.DoModal();
	//if(IDOK != dlg.DoModal())
	//	return false;

	HRESULT hr;

	int w, h;
	w = 640, h = 480; // TODO

	CComPtr<IDirect3DSurface9> pRTSurf;
	if(FAILED(hr = pD3Dev->CreateRenderTarget(w, h, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pRTSurf, NULL)))
		return false;

	if(FAILED(hr = pD3Dev->CreateOffscreenPlainSurface(w, h, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_pSysMemSurf, NULL)))
		return false;

	//

	m_pGB.CoCreateInstance(CLSID_FilterGraph);
	if(!m_pGB) return false;

	CComPtr<ICaptureGraphBuilder2> pCGB;
	pCGB.CoCreateInstance(CLSID_CaptureGraphBuilder2);
	if(!pCGB) return false;

	pCGB->SetFiltergraph(m_pGB);

	CComPtr<IBaseFilter> pMux;
	if(FAILED(hr = pCGB->SetOutputFileName(&MEDIASUBTYPE_Avi, CStringW(dlg.m_filename), &pMux, NULL)))
		return false;

	m_pSrc = new GSSource(w, h, fps, NULL, hr);
	if(FAILED(hr = m_pGB->AddFilter(m_pSrc, L"Source")))
		return false;

	if(FAILED(hr = m_pGB->AddFilter(dlg.m_pVidEnc, L"Encoder")))
		return false;

	if(FAILED(hr = pCGB->RenderStream(NULL, NULL, m_pSrc, dlg.m_pVidEnc, pMux)))
		return false;

	hr = CComQIPtr<IMediaControl>(m_pGB)->Run();

	hr = CComQIPtr<IGSSource>(m_pSrc)->DeliverNewSegment();

	m_pRTSurf = pRTSurf;

	return true;
}

bool GSCapture::BeginFrame(int& w, int& h, IDirect3DSurface9** pRTSurf)
{
	if(!m_pRTSurf || !pRTSurf) return false;

	// FIXME: remember w, h from BeginCapture
	D3DSURFACE_DESC desc;
	if(FAILED(m_pRTSurf->GetDesc(&desc))) return false;
	w = desc.Width;
	h = desc.Height;

	(*pRTSurf = m_pRTSurf)->AddRef();

	return true;
}

bool GSCapture::EndFrame()
{
	if(m_pRTSurf == NULL || m_pSysMemSurf == NULL) {ASSERT(0); return false;}

	CComPtr<IDirect3DDevice9> pD3DDev;
	D3DLOCKED_RECT r;

	if(FAILED(m_pRTSurf->GetDevice(&pD3DDev))
	|| FAILED(pD3DDev->GetRenderTargetData(m_pRTSurf, m_pSysMemSurf))
	|| FAILED(m_pSysMemSurf->LockRect(&r, NULL, D3DLOCK_READONLY)))
		return false;

	CComQIPtr<IGSSource>(m_pSrc)->DeliverFrame(r);

	m_pSysMemSurf->UnlockRect();

	return true;
}

bool GSCapture::EndCapture()
{
	m_pRTSurf = NULL;
	m_pSysMemSurf = NULL;

	if(m_pSrc) CComQIPtr<IGSSource>(m_pSrc)->DeliverEOS();
	if(m_pGB) CComQIPtr<IMediaControl>(m_pGB)->Stop();

	m_pSrc = NULL;
	m_pGB = NULL;

	return true;
}
