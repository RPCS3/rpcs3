#pragma once

#include <QQuickItem>
#include <QOpenGLFramebufferObject>

class GLViewer : public QQuickItem
{
    Q_OBJECT
public:
    GLViewer(QQuickItem* parent = 0);
    virtual ~GLViewer();

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data);
    void timerEvent(QTimerEvent* evt);

private slots:
    void cleanup();

private:
    int m_timerID;
    QOpenGLFramebufferObject* m_fbo;
};
