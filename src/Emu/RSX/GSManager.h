#pragma once

class GSRender;

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

	GSInfo()
	{
	}

	void Init();
};

class GSManager
{
	GSInfo m_info;
	std::shared_ptr<GSRender> m_render;

public:
	void Init();
	void Close();

	bool IsInited() const { return m_render != nullptr; }

	GSInfo& GetInfo() { return m_info; }
	GSRender& GetRender() { return *m_render; }

	u8 GetState();
	u8 GetColorSpace();
};
