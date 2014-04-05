#pragma once
#include "sysutil_video.h"
#include "GSRender.h"

struct GSInfo
{
	struct
	{
		u8  resolutionId;
		u8  scanMode;
		u8  conversion;
		u8  aspect;
		u8  format;
		u16 refreshRates;
		u32 pitch;
	} mode;
	//CellVideoOutDisplayMode mode;

	GSInfo()
	{
	}

	void Init()
	{
		mode.resolutionId = Ini.GSResolution.GetValue();
		mode.scanMode = CELL_VIDEO_OUT_SCAN_MODE_INTERLACE;
		mode.conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
		mode.aspect = Ini.GSAspectRatio.GetValue();
		mode.refreshRates = CELL_VIDEO_OUT_REFRESH_RATE_50HZ;
		mode.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
		mode.pitch = 4;
	}
};

struct gcmBuffer
{
	u32 offset;
	u32 pitch;
	u32 width;
	u32 height;
};

class GSManager
{
	GSInfo m_info;
	GSRender* m_render;

public:
	GSManager();

	void Init();
	void Close();

	bool IsInited() const { return m_render != nullptr; }

	GSInfo& GetInfo() { return m_info; }
	GSRender& GetRender() { assert(m_render); return *m_render; }

	u8 GetState();
	u8 GetColorSpace();
};