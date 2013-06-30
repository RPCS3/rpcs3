#pragma once
#include "GCM.h"

class ExecRSXCMDdata
{
protected:
	bool m_set_color_mask;
	bool m_color_mask_r;
	bool m_color_mask_g;
	bool m_color_mask_b;
	bool m_color_mask_a;

	bool m_set_clip;
	float m_clip_min;
	float m_clip_max;

	bool m_set_depth_func;
	int m_depth_func;

	bool m_set_alpha_test;
	bool m_set_blend;
	bool m_set_depth_bounds_test;
	bool m_depth_test_enable;

	bool m_set_viewport_horizontal;
	bool m_set_viewport_vertical;
	u16 m_viewport_x;
	u16 m_viewport_y;
	u16 m_viewport_w;
	u16 m_viewport_h;

	bool m_set_scissor_horizontal;
	bool m_set_scissor_vertical;
	u16 m_scissor_x;
	u16 m_scissor_y;
	u16 m_scissor_w;
	u16 m_scissor_h;

public:
	ExecRSXCMDdata()
	{
		Reset();
	}

	virtual void Reset()
	{
		m_set_color_mask = false;
		m_set_alpha_test = false;
		m_set_blend = false;
		m_set_depth_bounds_test = false;
		m_set_clip = false;
		m_set_depth_func = false;
		m_depth_test_enable = false;
		m_set_viewport_horizontal = false;
		m_set_viewport_vertical = false;
		m_set_scissor_horizontal = false;
		m_set_scissor_vertical = false;
	}

	virtual void ExecCMD()=0;
};

class RSXThread : public ThreadBase
{
	Array<u32> call_stack;
	CellGcmControl& m_ctrl;
	u32 m_ioAddress;

protected:
	RSXThread(CellGcmControl* ctrl, u32 ioAddress);

private:
	virtual void Task();
	virtual void OnExit();
};