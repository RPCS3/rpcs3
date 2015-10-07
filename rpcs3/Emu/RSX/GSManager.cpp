#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules/cellVideoOut.h"

#include "GSManager.h"
#include "Null/NullGSRender.h"
#include "GL/GLGSRender.h"

void GSInfo::Init()
{
	mode.resolutionId = Ini.GSResolution.GetValue();
	mode.scanMode = CELL_VIDEO_OUT_SCAN_MODE_INTERLACE;
	mode.conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
	mode.aspect = Ini.GSAspectRatio.GetValue();
	mode.refreshRates = CELL_VIDEO_OUT_REFRESH_RATE_50HZ;
	mode.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
	mode.pitch = 4;
}

GSManager::GSManager() : m_render(nullptr)
{
}

extern GSRender * createGSRender(u8);

void GSManager::Init()
{
	if(m_render) return;

	m_info.Init();

	m_render = createGSRender(Ini.GSRenderMode.GetValue());
	//m_render->Init(GetInfo().outresolution.width, GetInfo().outresolution.height);
}

void GSManager::Close()
{
	if(m_render)
	{
		m_render->Close();
		delete m_render;
		m_render = nullptr;
	}
}

u8 GSManager::GetState()
{
	return CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED;
}

u8 GSManager::GetColorSpace()
{
	return CELL_VIDEO_OUT_COLOR_SPACE_RGB;
}
