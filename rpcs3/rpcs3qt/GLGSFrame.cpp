#include "stdafx.h"
#include "Utilities/Config.h"
#include "GLGSFrame.h"
#include "config.h"

#include <QOpenGLContext>
#include <QWindow>

extern cfg::bool_entry g_cfg_rsx_debug_output;

GLGSFrame::GLGSFrame(int w, int h, QIcon appIcon)
	: GSFrame("OpenGL", w, h, appIcon)
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

void* GLGSFrame::make_context()
{
	auto context = new QOpenGLContext();
	context->setFormat(m_format);
	context->create();

	return context;
}

void GLGSFrame::set_current(draw_context_t ctx)
{
	((QOpenGLContext*)ctx.get())->makeCurrent(this);
}

void GLGSFrame::delete_context(void* ctx)
{
	delete (QOpenGLContext*)ctx;
}

void GLGSFrame::flip(draw_context_t context)
{
	GSFrame::flip(context);

	((QOpenGLContext*)context.get())->makeCurrent(this);
	((QOpenGLContext*)context.get())->swapBuffers(this);
}
