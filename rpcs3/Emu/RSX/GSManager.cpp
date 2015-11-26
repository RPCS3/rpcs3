#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules/cellVideoOut.h"
#include "Emu/state.h"

#include "GSManager.h"
#include "Null/NullGSRender.h"
#include "GL/GLGSRender.h"
#ifdef _MSC_VER
#include "Emu/RSX/D3D12/D3D12GSRender.h"
#endif

void GSInfo::Init()
{
	mode.resolutionId = (u8)rpcs3::state.config.rsx.resolution.value();
	mode.scanMode = CELL_VIDEO_OUT_SCAN_MODE_INTERLACE;
	mode.conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
	mode.aspect = (u8)rpcs3::state.config.rsx.aspect_ratio.value();
	mode.refreshRates = CELL_VIDEO_OUT_REFRESH_RATE_50HZ;
	mode.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
	mode.pitch = 4;
}

void GSManager::Init()
{
	if (m_render) return;

	m_info.Init();

	switch (rpcs3::state.config.rsx.renderer.value())
	{
	default:
	case rsx_renderer_type::Null : m_render = std::make_shared<NullGSRender>(); break;
	case rsx_renderer_type::OpenGL: m_render = std::make_shared<GLGSRender>(); break;
#ifdef _MSC_VER
	case rsx_renderer_type::DX12: m_render = std::make_shared<D3D12GSRender>(); break;
#endif
	}

	//m_render->Init(GetInfo().outresolution.width, GetInfo().outresolution.height);
}

void GSManager::Close()
{
	m_render.reset();
}

u8 GSManager::GetState()
{
	return CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED;
}

u8 GSManager::GetColorSpace()
{
	return CELL_VIDEO_OUT_COLOR_SPACE_RGB;
}
