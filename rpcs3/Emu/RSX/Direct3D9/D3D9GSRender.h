#pragma once
#include "Emu/RSX/GSRender.h"

class D3D9GSRender : public GSRender
{
private:
	void* m_context;

public:
	GSFrameBase* m_frame;
	u32 m_draw_frames;
	u32 m_skip_frames;

	D3D9GSRender();
	virtual ~D3D9GSRender();

	virtual void Close();

protected:
	virtual void OnInit();
	virtual void OnInitThread();
	virtual void OnExitThread();
	virtual void OnReset();
	virtual void ExecCMD(u32 cmd);
	virtual void ExecCMD();
	virtual void Flip(int buffer);
};
