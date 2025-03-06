#pragma once

#include "shortcut_handler.h"
#include "progress_indicator.h"
#include "util/types.hpp"
#include "util/atomic.hpp"
#include "util/media_utils.h"
#include "Emu/RSX/GSFrameBase.h"

#include <QWindow>
#include <QPaintEvent>
#include <QTimer>

#include <memory>
#include <vector>

class gui_settings;
enum class video_renderer;

class gs_frame : public QWindow, public GSFrameBase
{
	Q_OBJECT

private:
	// taskbar progress
	std::unique_ptr<progress_indicator> m_progress_indicator;

	shortcut_handler* m_shortcut_handler = nullptr;

	QRect m_initial_geometry;

	std::shared_ptr<gui_settings> m_gui_settings;
	QTimer m_mousehide_timer;

	u64 m_frames = 0;
	std::string m_window_title;
	QWindow::Visibility m_last_visibility = Visibility::Windowed;
	atomic_t<bool> m_is_closing = false;
	atomic_t<bool> m_show_mouse = true;
	bool m_disable_mouse = false;
	bool m_disable_kb_hotkeys = false;
	bool m_mouse_hide_and_lock = false;
	bool m_show_mouse_in_fullscreen = false;
	bool m_lock_mouse_in_fullscreen = true;
	bool m_hide_mouse_after_idletime = false;
	u32 m_hide_mouse_idletime = 2000; // ms
	bool m_flip_showed_frame = false;
	bool m_start_games_fullscreen = false;
	bool m_ignore_stop_events = false;

	std::shared_ptr<utils::video_encoder> m_video_encoder{};

public:
	explicit gs_frame(QScreen* screen, const QRect& geometry, const QIcon& appIcon, std::shared_ptr<gui_settings> gui_settings, bool force_fullscreen);
	~gs_frame();

	video_renderer renderer() const { return m_renderer; };

	void ignore_stop_events() { m_ignore_stop_events = true; }

	draw_context_t make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(draw_context_t context) override;
	void toggle_fullscreen() override;

	void update_shortcuts();

	// taskbar progress
	void progress_reset(bool reset_limit = false);
	void progress_set_value(int value);
	void progress_increment(int delta);
	void progress_set_limit(int limit);

	/*
		Returns true if the mouse is locked inside the game window.
		Also conveniently updates the cursor visibility, because using it from a mouse handler indicates mouse emulation.
	*/
	bool get_mouse_lock_state();

	bool can_consume_frame() const override;
	void present_frame(std::vector<u8>& data, u32 pitch, u32 width, u32 height, bool is_bgra) const override;
	void take_screenshot(std::vector<u8>&& data, u32 sshot_width, u32 sshot_height, bool is_bgra) override;

protected:
	video_renderer m_renderer;

	void paintEvent(QPaintEvent *event) override;
	void showEvent(QShowEvent *event) override;

	void close() override;
	void reset() override;

	bool shown() override;
	void hide() override;
	void show() override;
	void mouseDoubleClickEvent(QMouseEvent* ev) override;

	display_handle_t handle() const override;

	void flip(draw_context_t context, bool skip_frame = false) override;
	int client_width() override;
	int client_height() override;
	f64 client_display_rate() override;
	bool has_alpha() override;

	bool event(QEvent* ev) override;

private:
	void load_gui_settings();
	void hide_on_close();
	void toggle_recording();
	void toggle_mouselock();
	void update_cursor();
	void handle_cursor(QWindow::Visibility visibility, bool visibility_changed, bool active_changed, bool start_idle_timer);

private Q_SLOTS:
	void mouse_hide_timeout();
	void handle_shortcut(gui::shortcuts::shortcut shortcut_key, const QKeySequence& key_sequence);
};
