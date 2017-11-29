#pragma once

#include "stdafx.h"
#include "gs_frame.h"

class gl_gs_frame : public gs_frame
{
private:
	QSurfaceFormat m_format;

public:
	gl_gs_frame(int w, int h, QIcon appIcon, bool disableMouse);

	draw_context_t make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(draw_context_t context) override;
	void flip(draw_context_t context, bool skip_frame=false) override;
};
