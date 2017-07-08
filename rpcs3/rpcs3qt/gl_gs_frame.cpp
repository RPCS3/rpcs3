#include "stdafx.h"
#include "gl_gs_frame.h"
#include "Emu/System.h"

#include <QOpenGLContext>
#include <QWindow>

gl_gs_frame::gl_gs_frame(int w, int h, QIcon appIcon)
	: gs_frame("OpenGL", w, h, appIcon)
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

void* gl_gs_frame::make_context()
{
	auto context = new QOpenGLContext();
	context->setFormat(m_format);
	context->create();

	return context;
}

void gl_gs_frame::set_current(draw_context_t ctx)
{
	if (!((QOpenGLContext*)ctx.get())->makeCurrent(this))
	{
		create();
		((QOpenGLContext*)ctx.get())->makeCurrent(this);
	}
}

void gl_gs_frame::delete_context(void* ctx)
{
	delete (QOpenGLContext*)ctx;
}

void gl_gs_frame::flip(draw_context_t context, bool skip_frame)
{
	gs_frame::flip(context);

	//Do not swap buffers if frame skip is active
	if (skip_frame) return;

	((QOpenGLContext*)context.get())->makeCurrent(this);
	((QOpenGLContext*)context.get())->swapBuffers(this);
}
