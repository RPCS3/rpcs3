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

#include "stdafx.h"
#include "GSCapture.h"

#ifdef _WINDOWS

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
	STDMETHOD(DeliverFrame)(const void* bits, int pitch, bool rgba) PURE;
	STDMETHOD(DeliverEOS)() PURE;
};

#ifdef __INTEL_COMPILER
class __declspec(uuid("F8BB6F4F-0965-4ED4-BA74-C6A01E6E6C77"))
#else
[uuid("F8BB6F4F-0965-4ED4-BA74-C6A01E6E6C77")] class
#endif
GSSource : public CBaseFilter, private CCritSec, public IGSSource
{
	GSVector2i m_size;
	REFERENCE_TIME m_atpf;
	REFERENCE_TIME m_now;

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		return
			riid == __uuidof(IGSSource) ? GetInterface((IGSSource*)this, ppv) :
			__super::NonDelegatingQueryInterface(riid, ppv);
	}

	class GSSourceOutputPin : public CBaseOutputPin
	{
		GSVector2i m_size;
		vector<CMediaType> m_mts;

	public:
		GSSourceOutputPin(const GSVector2i& size, REFERENCE_TIME atpf, CBaseFilter* pFilter, CCritSec* pLock, HRESULT& hr, int colorspace)
			: CBaseOutputPin("GSSourceOutputPin", pFilter, pLock, &hr, L"Output")
			, m_size(size)
		{
			CMediaType mt;
			mt.majortype = MEDIATYPE_Video;
			mt.formattype = FORMAT_VideoInfo;

			VIDEOINFOHEADER vih;
			memset(&vih, 0, sizeof(vih));
			vih.AvgTimePerFrame = atpf;
			vih.bmiHeader.biSize = sizeof(vih.bmiHeader);
			vih.bmiHeader.biWidth = m_size.x;
			vih.bmiHeader.biHeight = m_size.y;

			// YUY2

			mt.subtype = MEDIASUBTYPE_YUY2;
			mt.lSampleSize = m_size.x * m_size.y * 2;

			vih.bmiHeader.biCompression = '2YUY';
			vih.bmiHeader.biPlanes = 1;
			vih.bmiHeader.biBitCount = 16;
			vih.bmiHeader.biSizeImage = m_size.x * m_size.y * 2;
			mt.SetFormat((uint8*)&vih, sizeof(vih));

			m_mts.push_back(mt);

			// RGB32

			mt.subtype = MEDIASUBTYPE_RGB32;
			mt.lSampleSize = m_size.x * m_size.y * 4;

			vih.bmiHeader.biCompression = BI_RGB;
			vih.bmiHeader.biPlanes = 1;
			vih.bmiHeader.biBitCount = 32;
			vih.bmiHeader.biSizeImage = m_size.x * m_size.y * 4;
			mt.SetFormat((uint8*)&vih, sizeof(vih));

			if(colorspace == 1) m_mts.insert(m_mts.begin(), mt);
			else m_mts.push_back(mt);
		}

		HRESULT GSSourceOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
		{
			ASSERT(pAlloc && pProperties);

			HRESULT hr;

			pProperties->cBuffers = 1;
			pProperties->cbBuffer = m_mt.lSampleSize;

			ALLOCATOR_PROPERTIES Actual;

			if(FAILED(hr = pAlloc->SetProperties(pProperties, &Actual)))
			{
				return hr;
			}

			if(Actual.cbBuffer < pProperties->cbBuffer)
			{
				return E_FAIL;
			}

			ASSERT(Actual.cBuffers == pProperties->cBuffers);

			return S_OK;
		}

	    HRESULT CheckMediaType(const CMediaType* pmt)
		{
			for(vector<CMediaType>::iterator i = m_mts.begin(); i != m_mts.end(); i++)
			{
				if(i->majortype == pmt->majortype && i->subtype == pmt->subtype)
				{
					return S_OK;
				}
			}

			return E_FAIL;
		}

	    HRESULT GetMediaType(int i, CMediaType* pmt)
		{
			CheckPointer(pmt, E_POINTER);

			if(i < 0) return E_INVALIDARG;
			if(i > 1) return VFW_S_NO_MORE_ITEMS;

			*pmt = m_mts[i];

			return S_OK;
		}

		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q)
		{
			return E_NOTIMPL;
		}

		const CMediaType& CurrentMediaType()
		{
			return m_mt;
		}
	};

	GSSourceOutputPin* m_output;

public:

	GSSource(int w, int h, float fps, IUnknown* pUnk, HRESULT& hr, int colorspace)
		: CBaseFilter(NAME("GSSource"), pUnk, this, __uuidof(this), &hr)
		, m_output(NULL)
		, m_size(w, h)
		, m_atpf((REFERENCE_TIME)(10000000.0f / fps))
		, m_now(0)
	{
		m_output = new GSSourceOutputPin(m_size, m_atpf, this, this, hr, colorspace);
	}

	virtual ~GSSource()
	{
		delete m_output;
	}

	DECLARE_IUNKNOWN;

	int GetPinCount()
	{
		return 1;
	}

	CBasePin* GetPin(int n)
	{
		return n == 0 ? m_output : NULL;
	}

	// IGSSource

	STDMETHODIMP DeliverNewSegment()
	{
		m_now = 0;

		return m_output->DeliverNewSegment(0, _I64_MAX, 1.0);
	}

	STDMETHODIMP DeliverFrame(const void* bits, int pitch, bool rgba)
	{
		if(!m_output || !m_output->IsConnected())
		{
			return E_UNEXPECTED;
		}

		CComPtr<IMediaSample> sample;

		if(FAILED(m_output->GetDeliveryBuffer(&sample, NULL, NULL, 0)))
		{
			return E_FAIL;
		}

		REFERENCE_TIME start = m_now;
		REFERENCE_TIME stop = m_now + m_atpf;

		sample->SetTime(&start, &stop);
		sample->SetSyncPoint(TRUE);

		const CMediaType& mt = m_output->CurrentMediaType();

		uint8* src = (uint8*)bits;
		uint8* dst = NULL;

		sample->GetPointer(&dst);

		int w = m_size.x;
		int h = m_size.y;
		int srcpitch = pitch;

		if(mt.subtype == MEDIASUBTYPE_YUY2)
		{
			int dstpitch = ((VIDEOINFOHEADER*)mt.Format())->bmiHeader.biWidth * 2;

			GSVector4 ys(0.257f, 0.504f, 0.098f, 0.0f);
			GSVector4 us(-0.148f / 2, -0.291f / 2, 0.439f / 2, 0.0f);
			GSVector4 vs(0.439f / 2, -0.368f / 2, -0.071f / 2, 0.0f);

			if(!rgba)
			{
				ys = ys.zyxw();
				us = us.zyxw();
				vs = vs.zyxw();
			}

			const GSVector4 offset(16, 128, 16, 128);

			for(int j = 0; j < h; j++, dst += dstpitch, src += srcpitch)
			{
				uint32* s = (uint32*)src;
				uint16* d = (uint16*)dst;

				for(int i = 0; i < w; i += 2)
				{
					GSVector4 c0 = GSVector4::rgba32(s[i + 0]);
					GSVector4 c1 = GSVector4::rgba32(s[i + 1]);
					GSVector4 c2 = c0 + c1;

					GSVector4 lo = (c0 * ys).hadd(c2 * us);
					GSVector4 hi = (c1 * ys).hadd(c2 * vs);

					GSVector4 c = lo.hadd(hi) + offset;

					*((uint32*)&d[i]) = GSVector4i(c).rgba32();
				}
			}
		}
		else if(mt.subtype == MEDIASUBTYPE_RGB32)
		{
			int dstpitch = ((VIDEOINFOHEADER*)mt.Format())->bmiHeader.biWidth * 4;

			dst += dstpitch * (h - 1);
			dstpitch = -dstpitch;

			for(int j = 0; j < h; j++, dst += dstpitch, src += srcpitch)
			{
				if(rgba)
				{
					#if _M_SSE >= 0x301

					GSVector4i* s = (GSVector4i*)src;
					GSVector4i* d = (GSVector4i*)dst;

					GSVector4i mask(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15);

					for(int i = 0, w4 = w >> 2; i < w4; i++)
					{
						d[i] = s[i].shuffle8(mask);
					}

					#else

					GSVector4i* s = (GSVector4i*)src;
					GSVector4i* d = (GSVector4i*)dst;

					for(int i = 0, w4 = w >> 2; i < w4; i++)
					{
						d[i] = ((s[i] & 0x00ff0000) >> 16) | ((s[i] & 0x000000ff) << 16) | (s[i] & 0x0000ff00);
					}

					#endif
				}
				else
				{
					memcpy(dst, src, w * 4);
				}
			}
		}
		else
		{
			return E_FAIL;
		}

		if(FAILED(m_output->Deliver(sample)))
		{
			return E_FAIL;
		}

		m_now = stop;

		return S_OK;
	}

	STDMETHODIMP DeliverEOS()
	{
		return m_output->DeliverEndOfStream();
	}
};

#define BeginEnumPins(pBaseFilter, pEnumPins, pPin) \
	{CComPtr<IEnumPins> pEnumPins; \
	if(pBaseFilter && SUCCEEDED(pBaseFilter->EnumPins(&pEnumPins))) \
	{ \
		for(CComPtr<IPin> pPin; S_OK == pEnumPins->Next(1, &pPin, 0); pPin = NULL) \
		{ \

#define EndEnumPins }}}

static IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir)
{
	if(!pBF) return(NULL);

	BeginEnumPins(pBF, pEP, pPin)
	{
		PIN_DIRECTION dir2;
		pPin->QueryDirection(&dir2);
		if(dir == dir2)
		{
			IPin* pRet = pPin.Detach();
			pRet->Release();
			return(pRet);
		}
	}
	EndEnumPins

	return(NULL);
}

#endif

//
// GSCapture
//

GSCapture::GSCapture()
	: m_capturing(false)
{
}

GSCapture::~GSCapture()
{
	EndCapture();
}

bool GSCapture::BeginCapture(float fps)
{
	GSAutoLock lock(this);

	ASSERT(fps != 0);

	EndCapture();

#ifdef _WINDOWS

	GSCaptureDlg dlg;

	if(IDOK != dlg.DoModal()) return false;

	m_size.x = (dlg.m_width + 7) & ~7;
	m_size.y = (dlg.m_height + 7) & ~7;

	wstring fn(dlg.m_filename.begin(), dlg.m_filename.end());

	//

	HRESULT hr;

	CComPtr<ICaptureGraphBuilder2> cgb;
	CComPtr<IBaseFilter> mux;

	if(FAILED(hr = m_graph.CoCreateInstance(CLSID_FilterGraph))
	|| FAILED(hr = cgb.CoCreateInstance(CLSID_CaptureGraphBuilder2))
	|| FAILED(hr = cgb->SetFiltergraph(m_graph))
	|| FAILED(hr = cgb->SetOutputFileName(&MEDIASUBTYPE_Avi, fn.c_str(), &mux, NULL)))
	{
		return false;
	}

	m_src = new GSSource(m_size.x, m_size.y, fps, NULL, hr, dlg.m_colorspace);

	if (dlg.m_enc==0)
	{
		if (FAILED(hr = m_graph->AddFilter(m_src, L"Source")))
			return false;
		if (FAILED(hr = m_graph->ConnectDirect(GetFirstPin(m_src, PINDIR_OUTPUT), GetFirstPin(mux, PINDIR_INPUT), NULL)))
			return false;
	}
	else
	{
		if(FAILED(hr = m_graph->AddFilter(m_src, L"Source"))
		|| FAILED(hr = m_graph->AddFilter(dlg.m_enc, L"Encoder")))
		{
			return false;
		}

		if(FAILED(hr = m_graph->ConnectDirect(GetFirstPin(m_src, PINDIR_OUTPUT), GetFirstPin(dlg.m_enc, PINDIR_INPUT), NULL))
		|| FAILED(hr = m_graph->ConnectDirect(GetFirstPin(dlg.m_enc, PINDIR_OUTPUT), GetFirstPin(mux, PINDIR_INPUT), NULL)))
		{
			return false;
		}
	}

	BeginEnumFilters(m_graph, pEF, pBF)
	{
		CFilterInfo fi;
		pBF->QueryFilterInfo(&fi);
		wstring s(fi.achName);
		printf("Filter [%p]: %s\n", pBF.p, string(s.begin(), s.end()).c_str());

		BeginEnumPins(pBF, pEP, pPin)
		{
			CComPtr<IPin> pPinTo;
			pPin->ConnectedTo(&pPinTo);

			CPinInfo pi;
			pPin->QueryPinInfo(&pi);
			wstring s(pi.achName);
			printf("- Pin [%p - %p]: %s (%s)\n", pPin.p, pPinTo.p, string(s.begin(), s.end()).c_str(), pi.dir ? "out" : "in");

			BeginEnumMediaTypes(pPin, pEMT, pmt)
			{
			}
			EndEnumMediaTypes(pmt)
		}
		EndEnumPins
	}
	EndEnumFilters

	hr = CComQIPtr<IMediaControl>(m_graph)->Run();

	CComQIPtr<IGSSource>(m_src)->DeliverNewSegment();

#endif

	m_capturing = true;

	return true;
}

bool GSCapture::DeliverFrame(const void* bits, int pitch, bool rgba)
{
	GSAutoLock lock(this);

	if(bits == NULL || pitch == 0)
	{
		ASSERT(0);

		return false;
	}

#ifdef _WINDOWS

	if(m_src)
	{
		CComQIPtr<IGSSource>(m_src)->DeliverFrame(bits, pitch, rgba);

		return true;
	}

#endif

	return false;
}

bool GSCapture::EndCapture()
{
	GSAutoLock lock(this);

#ifdef _WINDOWS

	if(m_src)
	{
		CComQIPtr<IGSSource>(m_src)->DeliverEOS();

		m_src = NULL;
	}

	if(m_graph)
	{
		CComQIPtr<IMediaControl>(m_graph)->Stop();

		m_graph = NULL;
	}

#endif

	m_capturing = false;

	return true;
}
