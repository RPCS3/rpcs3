#pragma once

#include "stdafx.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QTranslator>

#include "main_application.h"

class gs_frame;
class main_window;
class gui_settings;
class emu_settings;
class persistent_settings;

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

	/** Call this method before calling app.exec */
	void Init() override;

	std::unique_ptr<gs_frame> get_gs_frame();

	main_window* m_main_window = nullptr;

private:
	QThread* get_thread() override
	{
		return thread();
	}

	void SwitchTranslator(QTranslator& translator, const QString& filename, const QString& language_code);
	void LoadLanguage(const QString& language_code);
	QStringList GetAvailableLanguageCodes();

	void InitializeCallbacks();
	void InitializeConnects();

	void StartPlaytime(bool start_playtime);
	void StopPlaytime();

	QTranslator m_translator;
	QTranslator m_translator_qt;
	QString m_language_code;

	QElapsedTimer m_timer_playtime;

	std::shared_ptr<emu_settings> m_emu_settings;
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;

	bool m_show_gui = true;
	bool m_use_cli_style = false;

private Q_SLOTS:
	void OnChangeStyleSheetRequest(const QString& path);
	void OnEmuSettingsChange();

Q_SIGNALS:
	void OnEmulatorRun(bool start_playtime);
	void OnEmulatorPause();
	void OnEmulatorResume(bool start_playtime);
	void OnEmulatorStop();
	void OnEmulatorReady();

	void RequestCallAfter(const std::function<void()>& func);

private Q_SLOTS:
	void HandleCallAfter(const std::function<void()>& func);
};
