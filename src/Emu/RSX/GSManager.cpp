#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules/cellVideoOut.h"
#include "Emu/state.h"

#include "GSManager.h"

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

	m_render = Emu.GetCallbacks().get_gs_render();

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
