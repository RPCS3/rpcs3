#pragma once

#include "stdafx.h"
#include "Emu/RSX/GSRender.h"

#include <QWidget>
#include <QWindow>

class gs_frame : public QWindow, public GSFrameBase
{
	Q_OBJECT
	u64 m_frames = 0;
	QString m_windowTitle;
	bool m_show_fps;
	bool m_disable_mouse;

	bool m_in_sizing_event = false; //a signal that the window is about to be resized was received
	bool m_user_interaction_active = false; //a signal indicating the window is being manually moved/resized was received
	bool m_interactive_resize = false; //resize signal received while dragging window
	bool m_minimized = false;

public:
	gs_frame(const QString& title, int w, int h, QIcon appIcon, bool disableMouse);

	draw_context_t make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(draw_context_t context) override;

	wm_event get_default_wm_event() const override;
protected:
	virtual void paintEvent(QPaintEvent *event);

	void keyPressEvent(QKeyEvent *keyEvent) override;
	void OnFullScreen();

	void close() override;

	bool shown() override;
	void hide() override;
	void show() override;
	void mouseDoubleClickEvent(QMouseEvent* ev) override;

	void* handle() const override;

	void flip(draw_context_t context, bool skip_frame=false) override;
	int client_width() override;
	int client_height() override;

	bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

	bool event(QEvent* ev) override;
private Q_SLOTS:
	void HandleCursor(QWindow::Visibility visibility);
};
