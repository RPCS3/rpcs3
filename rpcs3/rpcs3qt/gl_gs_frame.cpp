#include "stdafx.h"
#include "gl_gs_frame.h"
#include "Emu/System.h"

#include <QOpenGLContext>
#include <qoffscreensurface.h>
#include <QWindow>

gl_gs_frame::gl_gs_frame(const QRect& geometry, const QIcon& appIcon, const std::shared_ptr<gui_settings>& gui_settings)
	: gs_frame("OpenGL", geometry, appIcon, gui_settings)
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
	auto context = new GLContext();
	context->handle = new QOpenGLContext();

	if (m_primary_context)
	{
		auto surface = new QOffscreenSurface();
		surface->setFormat(m_format);
		surface->create();

		// Share resources with the first created context
		context->handle->setShareContext(m_primary_context->handle);
		context->surface = surface;
		context->owner = true;
	}
	else
	{
		// This is the first created context, all others will share resources with this one
		m_primary_context = context;
		context->surface = this;
		context->owner = false;
	}

	context->handle->setFormat(m_format);
	context->handle->create();

	return context;
}

void gl_gs_frame::set_current(draw_context_t ctx)
{
	if (!ctx)
	{
		fmt::throw_exception("Null context handle passed to set_current" HERE);
	}

	const auto context = static_cast<GLContext*>(ctx);

	if (!context->handle->makeCurrent(context->surface))
	{
		if (!context->owner)
		{
			create();
		}
		else if (!context->handle->isValid())
		{
			context->handle->create();
		}

		if (!context->handle->makeCurrent(context->surface))
		{
			fmt::throw_exception("Could not bind OpenGL context" HERE);
		}
	}
}

void gl_gs_frame::delete_context(draw_context_t ctx)
{
	const auto gl_ctx = static_cast<GLContext*>(ctx);

	gl_ctx->handle->doneCurrent();

#ifdef _MSC_VER
	//AMD driver crashes when executing wglDeleteContext
	//Catch with SEH
	__try
	{
		delete gl_ctx->handle;
	}
	__except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		rsx_log.fatal("Your graphics driver just crashed whilst cleaning up. All consumed VRAM should have been released, but you may want to restart the emulator just in case");
	}
#else
	delete gl_ctx->handle;
#endif

	if (gl_ctx->owner)
	{
		delete gl_ctx->surface;
	}

	delete gl_ctx;
}

void gl_gs_frame::flip(draw_context_t context, bool skip_frame)
{
	gs_frame::flip(context);

	//Do not swap buffers if frame skip is active
	if (skip_frame) return;

	const auto gl_ctx = static_cast<GLContext*>(context);

	gl_ctx->handle->swapBuffers(gl_ctx->surface);
}
