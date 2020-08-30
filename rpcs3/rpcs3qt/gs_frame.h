#pragma once

#include "stdafx.h"
#include "Emu/RSX/GSRender.h"

#include <QWindow>
#include <QPaintEvent>
#include <QTimer>

#ifdef _WIN32
#include <QWinTaskbarProgress>
#include <QWinTaskbarButton>
#endif

class gui_settings;

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

	std::shared_ptr<gui_settings> m_gui_settings;
	QTimer m_mousehide_timer;

	u64 m_frames = 0;
	QString m_window_title;
	bool m_disable_mouse = false;
	bool m_disable_kb_hotkeys = false;
	bool m_mouse_hide_and_lock = false;
	bool m_show_mouse_in_fullscreen = false;
	bool m_hide_mouse_after_idletime = false;
	u32 m_hide_mouse_idletime = 2000; // ms

public:
	gs_frame(const QRect& geometry, const QIcon& appIcon, const std::shared_ptr<gui_settings>& gui_settings);
	~gs_frame();

	draw_context_t make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(draw_context_t context) override;
	void toggle_fullscreen() override;

	// taskbar progress
	void progress_reset(bool reset_limit = false);
	void progress_increment(int delta);
	void progress_set_limit(int limit);

	bool get_mouse_lock_state();

	void take_screenshot(const std::vector<u8> sshot_data, const u32 sshot_width, const u32 sshot_height, bool is_bgra) override;

protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void showEvent(QShowEvent *event) override;

	void keyPressEvent(QKeyEvent *keyEvent) override;

	void close() override;

	bool shown() override;
	void hide() override;
	void show() override;
	void mouseDoubleClickEvent(QMouseEvent* ev) override;

	display_handle_t handle() const override;

	void flip(draw_context_t context, bool skip_frame=false) override;
	int client_width() override;
	int client_height() override;

	bool event(QEvent* ev) override;

private:
	void toggle_mouselock();

private Q_SLOTS:
	void HandleCursor(QWindow::Visibility visibility);
	void MouseHideTimeout();
};
