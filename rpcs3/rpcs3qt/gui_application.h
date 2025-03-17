#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

#include <QApplication>
#include <QAbstractNativeEventFilter>
#include <QElapsedTimer>
#include <QTimer>
#include <QTranslator>
#include <QSoundEffect>

#include "main_application.h"

#include "Emu/System.h"
#include "Input/raw_mouse_handler.h"

#include <memory>
#include <functional>
#include <deque>

#ifdef _WIN32
#include "Windows.h"
#endif

class gs_frame;
class main_window;
class gui_settings;
class emu_settings;
class persistent_settings;

extern std::unique_ptr<raw_mouse_handler> g_raw_mouse_handler; // Only used for GUI

/** RPCS3 GUI Application Class
 * The main point of this class is to do application initialization, to hold the main and game windows and to initialize callbacks.
 */
class gui_application : public QApplication, public main_application
{
	Q_OBJECT
public:
	gui_application(int& argc, char** argv);
	~gui_application();

	void SetShowGui(bool show_gui = true)
	{
		m_show_gui = show_gui;
	}

	void SetUseCliStyle(bool use_cli_style = false)
	{
		m_use_cli_style = use_cli_style;
	}

	void SetWithCliBoot(bool with_cli_boot = false)
	{
		m_with_cli_boot = with_cli_boot;
	}

	void SetStartGamesFullscreen(bool start_games_fullscreen = false)
	{
		m_start_games_fullscreen = start_games_fullscreen;
	}

	void SetGameScreenIndex(int screen_index = -1)
	{
		m_game_screen_index = screen_index;
	}

	/** Call this method before calling app.exec */
	bool Init() override;

	static s32 get_language_id();

	std::unique_ptr<gs_frame> get_gs_frame();

	main_window* m_main_window = nullptr;

private:
	QThread* get_thread() override
	{
		return thread();
	}

	void SwitchTranslator(QTranslator& translator, const QString& filename, const QString& language_code);
	void LoadLanguage(const QString& language_code);
	static QStringList GetAvailableLanguageCodes();

	void InitializeCallbacks();
	void InitializeConnects();

	void StartPlaytime(bool start_playtime);
	void UpdatePlaytime();
	void StopPlaytime();

	void set_language_code(QString language_code);

	class native_event_filter : public QAbstractNativeEventFilter
	{
	public:
		bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

	} m_native_event_filter;

	QTranslator m_translator;
	QString m_language_code;
	static s32 m_language_id;

	QTimer m_timer;
	QElapsedTimer m_timer_playtime;

	std::deque<std::unique_ptr<QSoundEffect>> m_sound_effects{};

	std::shared_ptr<emu_settings> m_emu_settings;
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;

	QString m_default_style;

	bool m_show_gui = true;
	bool m_use_cli_style = false;
	bool m_with_cli_boot = false;
	bool m_start_games_fullscreen = false;
	int m_game_screen_index = -1;

	u64 m_pause_amend_time_on_focus_loss = umax;
	u64 m_pause_delayed_tag = 0;
	typename Emulator::stop_counter_t m_emu_focus_out_emulation_id{};
	bool m_is_pause_on_focus_loss_active = false;

#ifdef _WIN32
	void register_device_notification(WId window_id);
	void unregister_device_notification();
	HDEVNOTIFY m_device_notification_handle {};
#endif

private Q_SLOTS:
	void OnChangeStyleSheetRequest();
	void OnShortcutChange();
	void OnAppStateChanged(Qt::ApplicationState state);

Q_SIGNALS:
	void OnEmulatorRun(bool start_playtime);
	void OnEmulatorPause();
	void OnEmulatorResume(bool start_playtime);
	void OnEmulatorStop();
	void OnEmulatorReady();
	void OnEnableDiscEject(bool enabled);
	void OnEnableDiscInsert(bool enabled);

	void RequestCallFromMainThread(std::function<void()> func, atomic_t<u32>* wake_up);

private Q_SLOTS:
	static void CallFromMainThread(const std::function<void()>& func, atomic_t<u32>* wake_up);
};
