#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Utilities/rFile.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "D3D9GSRender.h"
#include <d3d9.h>
#include <d3dx9.h>

#pragma comment(lib, "d3dx9.lib") 
#pragma comment(lib, "d3d9.lib")

D3D9GSRender::D3D9GSRender()
	: GSRender()
	, m_frame(nullptr)
	, m_context(nullptr)
{
	m_frame = GetGSFrame(1);
}

D3D9GSRender::~D3D9GSRender()
{
	m_frame->Close();
	m_frame->DeleteContext(m_context);
}

extern CellGcmContextData current_context;

void D3D9GSRender::Close()
{
	Stop();

	if(m_frame->IsShown()) m_frame->Hide();
	m_ctrl = nullptr;
}

void D3D9GSRender::OnInit()
{
	m_draw_frames = 1;
	m_skip_frames = 0;
	RSXThread::m_width = 720;
	RSXThread::m_height = 576;
}

thread_local LPDIRECT3D9       g_pD3D = nullptr;
thread_local LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;

void D3D9GSRender::OnInitThread()
{
	m_context = m_frame->GetNewContext();

	g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	D3DDISPLAYMODE d3ddm;

	if (FAILED(g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm)))
	{
		return;
	}

	HRESULT hr;

	if (FAILED(hr = g_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		d3ddm.Format, D3DUSAGE_DEPTHSTENCIL,
		D3DRTYPE_SURFACE, D3DFMT_D16)))
	{
		if (hr == D3DERR_NOTAVAILABLE)
			return;
	}

	D3DCAPS9 d3dCaps;

	if (FAILED(g_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL, &d3dCaps)))
	{
		return;
	}

	DWORD dwBehaviorFlags = 0;

	if (d3dCaps.VertexProcessingCaps != 0)
		dwBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		dwBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	D3DPRESENT_PARAMETERS d3dpp;
	memset(&d3dpp, 0, sizeof(d3dpp));

	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.Windowed = TRUE;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (HWND)m_context,
		dwBehaviorFlags, &d3dpp, &g_pd3dDevice)))
	{
	}

	m_frame->Show();
}

void D3D9GSRender::OnExitThread()
{
	if (g_pd3dDevice != nullptr)
	{
		g_pd3dDevice->Release();
		g_pd3dDevice = nullptr;
	}

	if (g_pD3D != nullptr)
	{
		g_pD3D->Release();
		g_pD3D = nullptr;
	}
}

class D3D9ShaderBuilder
{
	std::vector<u32> tokens;

	union version_token
	{
		struct
		{
			u16 version_minor : 8;
			u16 version_major : 8;
			u16 type : 16;
		};

		u32 _u32;
	};

	union instruction_token
	{
		struct
		{
			u16 opcode : 16;
			u16 specific_controls : 8;
			u16 size_of_instruction : 16;
		};

		u32 _u32;
	};

public:
	static u32 make_version_token(u16 version_major, u16 version_minor, bool is_vertex_shader)
	{
		version_token res;
		res.version_minor = version_minor;
		res.version_major = version_major;
		res.type = is_vertex_shader ? 0xFFFE : 0xFFFF;

		return res._u32;
	}

	static u32 make_end_token()
	{
		return 0x0000FFFF;
	}

	static u32 make_opcode(D3DSHADER_INSTRUCTION_OPCODE_TYPE op)
	{
	}

	void build()
	{
		tokens.push_back(make_version_token(9, 3, true));
		tokens.push_back(make_end_token());
	}
};
void D3D9GSRender::OnReset()
{
}

void D3D9GSRender::ExecCMD(u32 cmd)
{
	assert(cmd == NV4097_CLEAR_SURFACE);

	D3DVIEWPORT9 viewport;
	viewport.X = methodRegisters[NV4097_SET_VIEWPORT_HORIZONTAL] & 0xffff;
	viewport.Y = methodRegisters[NV4097_SET_VIEWPORT_VERTICAL] & 0xffff;
	viewport.Width = methodRegisters[NV4097_SET_VIEWPORT_HORIZONTAL] >> 16;
	viewport.Height = methodRegisters[NV4097_SET_VIEWPORT_VERTICAL] >> 16;
	viewport.MinZ = 0;
	viewport.MaxZ = 0;
	g_pd3dDevice->SetViewport(&viewport);

	int flags = 0;

	if (m_clear_surface_mask & 1)
		flags |= D3DCLEAR_ZBUFFER;

	if (m_clear_surface_mask & 2)
		flags |= D3DCLEAR_STENCIL;

	if (m_clear_surface_mask & 0xF0)
		flags |= D3DCLEAR_TARGET;

	g_pd3dDevice->Clear(0, nullptr, flags,
		methodRegisters[NV4097_SET_COLOR_CLEAR_VALUE],
		(methodRegisters[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8) / (float)0xffffff,
		methodRegisters[NV4097_SET_ZSTENCIL_CLEAR_VALUE] & 0xf);
}

void D3D9GSRender::ExecCMD()
{
	auto make_pixel_shader = [&]() -> std::string
	{
		/*
		RSXFragmentProgramDecompiler decompiler;
		u32* data = RSXThread::m_vertex_prog + methodRegisters[NV4097_SET_TRANSFORM_PROGRAM_START] * 4;

		std::string res =
			"vs.2.0\n"
			"dcl_position o0\n";

		static const std::string reg_table[] =
		{
			"gl_Position",
			"diff_color", "spec_color",
			"fogc",
			"tc0", "tc1", "tc2", "tc3", "tc4", "tc5", "tc6", "tc7", "tc8", "tc9",
			"ssa"
		};

		for (int start = methodRegisters[NV4097_SET_TRANSFORM_PROGRAM_START]; start < 512; ++start)
		{
			decompiler.dst.HEX  = data[start * 4 + 0] << 16 | data[start * 4 + 0] >> 16;
			decompiler.src0.HEX = data[start * 4 + 1] << 16 | data[start * 4 + 1] >> 16;
			decompiler.src1.HEX = data[start * 4 + 2] << 16 | data[start * 4 + 2] >> 16;
			decompiler.src2.HEX = data[start * 4 + 3] << 16 | data[start * 4 + 3] >> 16;

			if (decompiler.dst.end)
				break;
		}
		*/

		return "";
	};


}

void D3D9GSRender::Flip(int buffer)
{
	/*
	const char BasicVertexShader[] =
		"ps.3.0"\
		"dcl_normal      v0.xyz\n"
		"def                 c0, 0, 0, 0, 0\n"\
		"mov o0, c0\n"\
		"mov oC0, c0\n";
	LPD3DXBUFFER pVS, pErrors;
	D3DXAssembleShader(BasicVertexShader, sizeof(BasicVertexShader) - 1,
		0, NULL, 0, &pVS, &pErrors);
	u32 size;
	u32* data;

	if (pVS)
	{
		data = (u32*)pVS->GetBufferPointer();
		size = pVS->GetBufferSize();
	}
	else
	{
		data = (u32*)pErrors->GetBufferPointer();
		size = pErrors->GetBufferSize();
		LOG_ERROR(RSX, (char*)data);
	}

	D3D9ShaderBuilder b;
	b.build();
	*/
	
	g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
			D3DCOLOR_COLORVALUE(0.0f, 1.0f, 0.0f, 1.0f), 1.0f, 0);

	//g_pd3dDevice->BeginScene();
	//g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 0, 1);
	//g_pd3dDevice->EndScene();

	g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
}
