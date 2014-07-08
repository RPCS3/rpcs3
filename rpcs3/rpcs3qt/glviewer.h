#pragma once

#include <QQuickItem>

class GLRenderer : public QObject {
    Q_OBJECT
public:
    GLRenderer() { }

    void setViewportSize(const QSize &size) { m_viewportSize = size; }

public slots:
    void paint();

private:
    QSize m_viewportSize;
};

class GLViewer : public QQuickItem
{
	Q_OBJECT
public:
	GLViewer();
	~GLViewer() { cleanup(); }

public slots:
	void sync();
	void cleanup();

private slots:
	void handleWindowChanged(QQuickWindow *win);

private:
	GLRenderer *m_renderer;
};
