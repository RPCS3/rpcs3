#pragma once
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

	void Init();
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
