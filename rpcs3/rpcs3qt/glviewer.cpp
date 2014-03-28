#include "glviewer.h"
#include <QQuickWindow>
#include <QOpenGLContext>
#include <QSGSimpleTextureNode>
#include <QCoreApplication>

GLViewer::GLViewer(QQuickItem* parent)
	: QQuickItem(parent),
	  m_timerID(0),
	  m_fbo(0)
{
	this->setFlag(QQuickItem::ItemHasContents);
}

GLViewer::~GLViewer()
{
	this->cleanup();
}

void GLViewer::timerEvent(QTimerEvent* evt)
{
	if (evt && evt->timerId() == m_timerID)
		this->update();
}

QSGNode* GLViewer::updatePaintNode(QSGNode* node, UpdatePaintNodeData* data)
{
	QSGSimpleTextureNode* textureNode = static_cast<QSGSimpleTextureNode*>(node);
	if (!textureNode)
		textureNode = new QSGSimpleTextureNode();
	// Push Qt state.
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glShadeModel(GL_FLAT);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	delete m_fbo;
	m_fbo = 0;
	int width = this->width();
	int height = this->height();
	if (width && height) {
		m_fbo = new QOpenGLFramebufferObject(width, height);
		textureNode->setTexture(this->window()->createTextureFromId(m_fbo->texture(), m_fbo->size()));
	}
	else
	{
		textureNode->setTexture(this->window()->createTextureFromId(0, QSize(0,0)));
	}
	textureNode->setRect(this->boundingRect());

	if (m_fbo) {
		m_fbo->bind();
	}
	// Restore (pop) Qt state.
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
	glPopClientAttrib();

	if (!m_timerID)
		m_timerID = this->startTimer(16);

	return textureNode;
}

void GLViewer::cleanup() {
	this->killTimer(m_timerID);
	m_timerID = 0;
	delete m_fbo;
	m_fbo = nullptr;
}
