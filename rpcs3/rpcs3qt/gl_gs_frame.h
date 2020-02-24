#pragma once

#include "stdafx.h"
#include "gs_frame.h"

struct GLContext
{
	QSurface *surface = nullptr;
	QOpenGLContext *handle = nullptr;
	bool owner = false;
};

class gl_gs_frame : public gs_frame
{
private:
	QSurfaceFormat m_format;
	GLContext *m_primary_context = nullptr;

public:
	gl_gs_frame(const QRect& geometry, const QIcon& appIcon, const std::shared_ptr<gui_settings>& gui_settings);

	draw_context_t make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(draw_context_t context) override;
	void flip(draw_context_t context, bool skip_frame=false) override;
};
