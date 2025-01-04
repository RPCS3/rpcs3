#pragma once

#include "gs_frame.h"

#include <memory>

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
	explicit gl_gs_frame(QScreen* screen, const QRect& geometry, const QIcon& appIcon, std::shared_ptr<gui_settings> gui_settings, bool force_fullscreen);

	void reset() override;
	draw_context_t make_context() override;
	void set_current(draw_context_t ctx) override;
	void delete_context(draw_context_t ctx) override;
	void flip(draw_context_t context, bool skip_frame = false) override;
};
