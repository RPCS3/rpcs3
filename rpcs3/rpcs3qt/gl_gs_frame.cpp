#include "stdafx.h"
#include "gl_gs_frame.h"
#include "Emu/System.h"

#include <QOpenGLContext>
#include <QWindow>

gl_gs_frame::gl_gs_frame(int w, int h, QIcon appIcon, bool disableMouse)
	: gs_frame("OpenGL", w, h, appIcon, disableMouse)
{
	setSurfaceType(QSurface::OpenGLSurface);

	m_format.setMajorVersion(4);
	m_format.setMinorVersion(3);
	m_format.setProfile(QSurfaceFormat::CoreProfile);
	m_format.setDepthBufferSize(16);
	m_format.setSwapBehavior(QSurfaceFormat::SwapBehavior::DoubleBuffer);
	if (g_cfg.video.debug_output)
	{
		m_format.setOption(QSurfaceFormat::FormatOption::DebugContext);
	}
	setFormat(m_format);
}

draw_context_t gl_gs_frame::make_context()
{
	auto context = new QOpenGLContext();
	context->setFormat(m_format);
	context->create();

	return context;
}

void gl_gs_frame::set_current(draw_context_t ctx)
{
	if (!((QOpenGLContext*)ctx)->makeCurrent(this))
	{
		create();
		((QOpenGLContext*)ctx)->makeCurrent(this);
	}
}

void gl_gs_frame::delete_context(draw_context_t ctx)
{
	auto gl_ctx = (QOpenGLContext*)ctx;
	gl_ctx->doneCurrent();
	delete gl_ctx;
}

void gl_gs_frame::flip(draw_context_t context, bool skip_frame)
{
	gs_frame::flip(context);

	//Do not swap buffers if frame skip is active
	if (skip_frame) return;

	((QOpenGLContext*)context)->makeCurrent(this);
	((QOpenGLContext*)context)->swapBuffers(this);
}
