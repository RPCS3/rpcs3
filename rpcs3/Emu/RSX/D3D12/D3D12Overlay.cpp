#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <d3d11on12.h>
#include <dxgi1_4.h>

// D2D
ComPtr<ID3D11Device> d3d11Device;
ComPtr<ID3D11DeviceContext> m_d3d11DeviceContext;
ComPtr<ID3D11On12Device> m_d3d11On12Device;
ComPtr<ID3D12Device> m_d3d12Device;
ComPtr<IDWriteFactory> m_dWriteFactory;
ComPtr<ID2D1Factory3> m_d2dFactory;
ComPtr<ID2D1Device2> m_d2dDevice;
ComPtr<ID2D1DeviceContext2> m_d2dDeviceContext;
ComPtr<ID3D11Resource> m_wrappedBackBuffers[2];
ComPtr<ID2D1Bitmap1> m_d2dRenderTargets[2];
ComPtr<IDWriteTextFormat> m_textFormat;
ComPtr<ID2D1SolidColorBrush> m_textBrush;

#pragma comment (lib, "d2d1.lib")
#pragma comment (lib, "dwrite.lib")
#pragma comment (lib, "d3d11.lib")

void D3D12GSRender::InitD2DStructures()
{
	D3D11On12CreateDevice(
		m_device.Get(),
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		nullptr,
		0,
		reinterpret_cast<IUnknown**>(m_commandQueueGraphic.GetAddressOf()),
		1,
		0,
		&d3d11Device,
		&m_d3d11DeviceContext,
		nullptr
		);

	d3d11Device.As(&m_d3d11On12Device);

	D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, &m_d2dFactory);
	Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
	m_d3d11On12Device.As(&dxgiDevice);
	m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice);
	m_d2dDevice->CreateDeviceContext(deviceOptions, &m_d2dDeviceContext);
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &m_dWriteFactory);

	float dpiX;
	float dpiY;
	m_d2dFactory->GetDesktopDpi(&dpiX, &dpiY);
	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
		dpiX,
		dpiY
		);

	for (unsigned i = 0; i < 2; i++)
	{
		D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
		m_d3d11On12Device->CreateWrappedResource(
			m_backBuffer[i].Get(),
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(&m_wrappedBackBuffers[i])
			);

		// Create a render target for D2D to draw directly to this back buffer.
		Microsoft::WRL::ComPtr<IDXGISurface> surface;
		m_wrappedBackBuffers[i].As(&surface);
		m_d2dDeviceContext->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&bitmapProperties,
			&m_d2dRenderTargets[i]
			);
		}

	m_d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkGreen), &m_textBrush);
	m_dWriteFactory->CreateTextFormat(
		L"Verdana",
		NULL,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		14,
		L"en-us",
		&m_textFormat
		);
	m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
}

void D3D12GSRender::ReleaseD2DStructures()
{
	d3d11Device.Reset();
	m_d3d11DeviceContext.Reset();
	m_d3d11On12Device.Reset();
	m_d3d12Device.Reset();
	m_dWriteFactory.Reset();
	m_d2dFactory.Reset();
	m_d2dDevice.Reset();
	m_d2dDeviceContext.Reset();
	m_wrappedBackBuffers[0].Reset();
	m_d2dRenderTargets[0].Reset();
	m_wrappedBackBuffers[1].Reset();
	m_d2dRenderTargets[1].Reset();
	m_textFormat.Reset();
	m_textBrush.Reset();
}

void D3D12GSRender::renderOverlay()
{
	D2D1_SIZE_F rtSize = m_d2dRenderTargets[m_swapChain->GetCurrentBackBufferIndex()]->GetSize();
	std::wstring duration = L"Draw duration : " + std::to_wstring(m_timers.m_drawCallDuration) + L" ms";
	std::wstring count = L"Draw count : " + std::to_wstring(m_timers.m_drawCallCount);

	// Acquire our wrapped render target resource for the current back buffer.
	m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[m_swapChain->GetCurrentBackBufferIndex()].GetAddressOf(), 1);

	// Render text directly to the back buffer.
	m_d2dDeviceContext->SetTarget(m_d2dRenderTargets[m_swapChain->GetCurrentBackBufferIndex()].Get());
	m_d2dDeviceContext->BeginDraw();
	m_d2dDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
	m_d2dDeviceContext->DrawTextW(
		duration.c_str(),
		duration.size(),
		m_textFormat.Get(),
		&D2D1::RectF(0, 0, rtSize.width, rtSize.height),
		m_textBrush.Get()
		);
	m_d2dDeviceContext->DrawTextW(
		count.c_str(),
		count.size(),
		m_textFormat.Get(),
		&D2D1::RectF(0, 14, rtSize.width, rtSize.height),
		m_textBrush.Get()
		);
	m_d2dDeviceContext->EndDraw();

	// Release our wrapped render target resource. Releasing 
	// transitions the back buffer resource to the state specified
	// as the OutState when the wrapped resource was created.
	m_d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[m_swapChain->GetCurrentBackBufferIndex()].GetAddressOf(), 1);

	// Flush to submit the 11 command list to the shared command queue.
	m_d3d11DeviceContext->Flush();
}

#endif