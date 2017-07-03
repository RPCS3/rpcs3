#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"
#include "D3D12GSRender.h"
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <d3d11on12.h>
#include <dxgi1_4.h>


namespace
{
// D2D
ComPtr<ID3D11Device> g_d3d11_device;
ComPtr<ID3D11DeviceContext> g_d3d11_device_context;
ComPtr<ID3D11On12Device> g_d3d11on12_device;
ComPtr<IDWriteFactory> g_dwrite_factory;
ComPtr<ID2D1Factory3> g_d2d_factory;
ComPtr<ID2D1Device2> g_d2d_device;
ComPtr<ID2D1DeviceContext2> g_d2d_device_context;
ComPtr<ID3D11Resource> g_wrapped_backbuffers[2];
ComPtr<ID2D1Bitmap1> g_d2d_render_targets[2];
ComPtr<IDWriteTextFormat> g_text_format;
ComPtr<ID2D1SolidColorBrush> g_text_brush;

void draw_strings(const D2D1_SIZE_F &rtSize, size_t backbuffer_id, const std::vector<std::wstring> &strings)
{
	// Acquire our wrapped render target resource for the current back buffer.
	g_d3d11on12_device->AcquireWrappedResources(g_wrapped_backbuffers[backbuffer_id ].GetAddressOf(), 1);

	// Render text directly to the back buffer.
	g_d2d_device_context->SetTarget(g_d2d_render_targets[backbuffer_id].Get());
	g_d2d_device_context->BeginDraw();
	g_d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
	float xpos = 0.f;
	for (const std::wstring &str : strings)
	{
		g_d2d_device_context->DrawTextW(
			str.c_str(),
			(UINT32)str.size(),
			g_text_format.Get(),
			&D2D1::RectF(0, xpos, rtSize.width, rtSize.height),
			g_text_brush.Get()
			);
		xpos += 14.f;
	}
	g_d2d_device_context->EndDraw();

	// Release our wrapped render target resource. Releasing 
	// transitions the back buffer resource to the state specified
	// as the OutState when the wrapped resource was created.
	g_d3d11on12_device->ReleaseWrappedResources(g_wrapped_backbuffers[backbuffer_id].GetAddressOf(), 1);

	// Flush to submit the 11 command list to the shared command queue.
	g_d3d11_device_context->Flush();
}
}

extern PFN_D3D11ON12_CREATE_DEVICE wrapD3D11On12CreateDevice;

void D3D12GSRender::init_d2d_structures()
{
	wrapD3D11On12CreateDevice(
		m_device.Get(),
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		nullptr,
		0,
		reinterpret_cast<IUnknown**>(m_command_queue.GetAddressOf()),
		1,
		0,
		&g_d3d11_device,
		&g_d3d11_device_context,
		nullptr
		);

	g_d3d11_device.As(&g_d3d11on12_device);

	D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, &g_d2d_factory);
	Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
	g_d3d11on12_device.As(&dxgiDevice);
	g_d2d_factory->CreateDevice(dxgiDevice.Get(), &g_d2d_device);
	g_d2d_device->CreateDeviceContext(deviceOptions, &g_d2d_device_context);
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &g_dwrite_factory);

	float dpiX;
	float dpiY;
	g_d2d_factory->GetDesktopDpi(&dpiX, &dpiY);
	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
		dpiX,
		dpiY
		);

	for (unsigned i = 0; i < 2; i++)
	{
		D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
		g_d3d11on12_device->CreateWrappedResource(
			m_backbuffer[i].Get(),
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(&g_wrapped_backbuffers[i])
			);

		// Create a render target for D2D to draw directly to this back buffer.
		Microsoft::WRL::ComPtr<IDXGISurface> surface;
		g_wrapped_backbuffers[i].As(&surface);
		g_d2d_device_context->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&bitmapProperties,
			&g_d2d_render_targets[i]
			);
		}

	g_d2d_device_context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkGreen), &g_text_brush);
	g_dwrite_factory->CreateTextFormat(
		L"Verdana",
		NULL,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		14,
		L"en-us",
		&g_text_format
		);
	g_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	g_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
}

void D3D12GSRender::release_d2d_structures()
{
	g_d3d11_device.Reset();
	g_d3d11_device_context.Reset();
	g_d3d11on12_device.Reset();
	g_dwrite_factory.Reset();
	g_d2d_factory.Reset();
	g_d2d_device.Reset();
	g_d2d_device_context.Reset();
	g_wrapped_backbuffers[0].Reset();
	g_d2d_render_targets[0].Reset();
	g_wrapped_backbuffers[1].Reset();
	g_d2d_render_targets[1].Reset();
	g_text_format.Reset();
	g_text_brush.Reset();
}

void D3D12GSRender::render_overlay()
{
	D2D1_SIZE_F rtSize = g_d2d_render_targets[m_swap_chain->GetCurrentBackBufferIndex()]->GetSize();
	std::wstring duration = L"Draw duration : " + std::to_wstring(m_timers.draw_calls_duration) + L" us";
	float vtxIdxPercent = (float)m_timers.vertex_index_duration / (float)m_timers.draw_calls_duration;
	std::wstring vertexIndexDuration = L"Vtx/Idx upload : " + std::to_wstring(m_timers.vertex_index_duration) + L" us (" + std::to_wstring(100.f * vtxIdxPercent) + L" %)";
	std::wstring size = L"Upload size : " + std::to_wstring(m_timers.buffer_upload_size) + L" Bytes";
	float texPercent = (float)m_timers.texture_duration / (float)m_timers.draw_calls_duration;
	std::wstring texDuration = L"Textures : " + std::to_wstring(m_timers.texture_duration) + L" us (" + std::to_wstring(100.f * texPercent) + L" %)";
	float programPercent = (float)m_timers.program_load_duration / (float)m_timers.draw_calls_duration;
	std::wstring programDuration = L"Program : " + std::to_wstring(m_timers.program_load_duration) + L" us (" + std::to_wstring(100.f * programPercent) + L" %)";
	float constantsPercent = (float)m_timers.constants_duration / (float)m_timers.draw_calls_duration;
	std::wstring constantDuration = L"Constants : " + std::to_wstring(m_timers.constants_duration) + L" us (" + std::to_wstring(100.f * constantsPercent) + L" %)";
	float rttPercent = (float)m_timers.prepare_rtt_duration / (float)m_timers.draw_calls_duration;
	std::wstring rttDuration = L"RTT : " + std::to_wstring(m_timers.prepare_rtt_duration) + L" us (" + std::to_wstring(100.f * rttPercent) + L" %)";
	std::wstring flipDuration = L"Flip : " + std::to_wstring(m_timers.flip_duration) + L" us";

	std::wstring count = L"Draw count : " + std::to_wstring(m_timers.draw_calls_count);
	draw_strings(rtSize, m_swap_chain->GetCurrentBackBufferIndex(),
		{
			duration,
			count,
			rttDuration,
			vertexIndexDuration,
			size,
			programDuration,
			constantDuration,
			texDuration,
			flipDuration
		});
}
#endif
