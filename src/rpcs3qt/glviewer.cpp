#include "glviewer.h"
#include <QQuickWindow>
#include <QOpenGLContext>
#include <QCoreApplication>

// This class hooks beforeRendering and allows us to draw a scene and reset GL state.
// In future, we will likely want to manually control the update rate.

void GLRenderer::paint() {
	// Do GL here
	glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

	glDisable(GL_DEPTH_TEST);

	// Draw blue to the window to show that we work
	glClearColor(0.2, 0, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	// Put the GL state back to how it was in case it makes SceneGraph angry
	((QQuickWindow*)sender())->resetOpenGLState();
}

GLViewer::GLViewer()
	: m_renderer(0)
{
	connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
}

void GLViewer::handleWindowChanged(QQuickWindow *win)
{
	if (win) {
		connect(win, SIGNAL(beforeSynchronizing()), this, SLOT(sync()), Qt::DirectConnection);
		connect(win, SIGNAL(sceneGraphInvalidated()), this, SLOT(cleanup()), Qt::DirectConnection);
		// We will take over from here
		win->setClearBeforeRendering(false);
	}
}

void GLViewer::sync()
{
	if (!m_renderer) {
		m_renderer = new GLRenderer();
		connect(window(), SIGNAL(beforeRendering()), m_renderer, SLOT(paint()), Qt::DirectConnection);
	}
	m_renderer->setViewportSize(window()->size() * window()->devicePixelRatio());
}

void GLViewer::cleanup() {
	if (m_renderer) {
		delete m_renderer;
		m_renderer = 0;
	}
}

