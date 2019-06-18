#pragma once

#include "stdafx.h"

#include "main_application.h"
#include "main_window.h"
#include "emu_settings.h"
#include "gui_settings.h"
#include "gs_frame.h"
#include "gl_gs_frame.h"

#include <QApplication>
#include <QFontDatabase>
#include <QIcon>

class gui_application : public QApplication, public main_application
{
	Q_OBJECT
public:
	gui_application(int& argc, char** argv);

	/** Call this method before calling app.exec */
	void Init() override;

	std::unique_ptr<gs_frame> get_gs_frame();

	main_window* m_main_window = nullptr;

private:
	QThread* get_thread() override
	{
		return thread();
	}

	void InitializeCallbacks();
	void InitializeConnects();

	std::shared_ptr<emu_settings> m_emu_settings;
	std::shared_ptr<gui_settings> m_gui_settings;

private Q_SLOTS:
	void OnChangeStyleSheetRequest(const QString& path);

Q_SIGNALS:
	void OnEmulatorRun();
	void OnEmulatorPause();
	void OnEmulatorResume();
	void OnEmulatorStop();
	void OnEmulatorReady();

	void RequestCallAfter(const std::function<void()>& func);

private Q_SLOTS:
	void HandleCallAfter(const std::function<void()>& func);
};
