#pragma once

#include "stdafx.h"
#include "Emu/RSX/GSRender.h"

#include <QWidget>
#include <QWindow>

#ifdef _WIN32
#include <QWinTaskbarProgress>
#include <QWinTaskbarButton>
#include <QWinTHumbnailToolbar>
#include <QWinTHumbnailToolbutton>
#elif HAVE_QTDBUS
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#endif

class gs_frame : public QWindow, public GSFrameBase
{
	Q_OBJECT

private:
	// taskbar progress
	int m_gauge_max = 100;
#ifdef _WIN32
	QWinTaskbarButton* m_tb_button = nullptr;
	QWinTaskbarProgress* m_tb_progress = nullptr;
#elif HAVE_QTDBUS
	int m_progress_value = 0;
	void UpdateProgress(int progress, bool disable = false);
#endif

	u64 m_frames = 0;
	QString m_windowTitle;
	bool m_show_fps;
	bool m_disable_mouse;

	bool m_in_sizing_event = false; //a signal that the window is about to be resized was received
	bool m_user_interaction_active = false; //a signal indicating the window is being manually moved/resized was received
	bool m_interactive_resize = false; //resize signal received while dragging window
	bool m_minimized = false;

public:
	gs_frame(const QString& title, const QRect& geometry, QIcon appIcon, bool disableMouse);
	~gs_frame();

	draw_context_t make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(draw_context_t context) override;

	wm_event get_default_wm_event() const override;

	// taskbar progress
	void progress_reset(bool reset_limit = false);
	void progress_increment(int delta);
	void progress_set_limit(int limit);

protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void showEvent(QShowEvent *event) override;

	void keyPressEvent(QKeyEvent *keyEvent) override;
	void OnFullScreen();

	void close() override;

	bool shown() override;
	void hide() override;
	void show() override;
	void mouseDoubleClickEvent(QMouseEvent* ev) override;

	display_handle_t handle() const override;

	void flip(draw_context_t context, bool skip_frame=false) override;
	int client_width() override;
	int client_height() override;

	bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

	bool event(QEvent* ev) override;

private Q_SLOTS:
	void HandleCursor(QWindow::Visibility visibility);
};
