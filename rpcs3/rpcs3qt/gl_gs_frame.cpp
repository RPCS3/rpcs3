#include "stdafx.h"
#include "Utilities/Config.h"
#include "gl_gs_frame.h"

#include <QOpenGLContext>
#include <QWindow>

extern cfg::bool_entry g_cfg_rsx_debug_output;

gl_gs_frame::gl_gs_frame(int w, int h, QIcon appIcon)
	: gs_frame("OpenGL", w, h, appIcon)
{
	setSurfaceType(QSurface::OpenGLSurface);

	m_format.setMajorVersion(4);
	m_format.setMinorVersion(3);
	m_format.setProfile(QSurfaceFormat::CoreProfile);
	m_format.setDepthBufferSize(16);
	m_format.setSwapBehavior(QSurfaceFormat::SwapBehavior::DoubleBuffer);
	if (g_cfg_rsx_debug_output)
		m_format.setOption(QSurfaceFormat::FormatOption::DebugContext);

	setFormat(m_format);
}

void* gl_gs_frame::make_context()
{
	auto context = new QOpenGLContext();
	context->setFormat(m_format);
	context->create();

	return context;
}

void gl_gs_frame::set_current(draw_context_t ctx)
{
	((QOpenGLContext*)ctx.get())->makeCurrent(this);
}

void gl_gs_frame::delete_context(void* ctx)
{
	delete (QOpenGLContext*)ctx;
}

void gl_gs_frame::flip(draw_context_t context)
{
	gs_frame::flip(context);

	((QOpenGLContext*)context.get())->makeCurrent(this);
	((QOpenGLContext*)context.get())->swapBuffers(this);
}
