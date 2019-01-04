#pragma once

#include "stdafx.h"

#include "keyboard_pad_handler.h"
#include "basic_keyboard_handler.h"
#include "basic_mouse_handler.h"

#include "Utilities/Config.h"
#include "Emu/VFS.h"
#include "Emu/RSX/GSRender.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/PadHandler.h"
#include "Emu/Io/MouseHandler.h"
#include "Emu/Audio/AudioThread.h"

#include "rpcs3qt/msg_dialog_frame.h"
#include "rpcs3qt/osk_dialog_frame.h"
#include "rpcs3qt/main_window.h"
#include "rpcs3qt/gui_settings.h"
#include "rpcs3qt/emu_settings.h"

#include <QApplication>
#include <QFontDatabase>

/** RPCS3 Application Class
 * The point of this class is to do application initialization and to hold onto the main window. The main thing I intend this class to do, for now, is to initialize callbacks and the main_window.
*/

class rpcs3_app : public QApplication
{
	Q_OBJECT
public:
	rpcs3_app(int& argc, char** argv);
	/** Call this method before calling app.exec
	*/
	void Init();

	/** Emu.Init() wrapper for user manager */
	static bool InitializeEmulator(const std::string& user, bool force_init);
Q_SIGNALS:
	void OnEmulatorRun();
	void OnEmulatorPause();
	void OnEmulatorResume();
	void OnEmulatorStop();
	void OnEmulatorReady();
	void RequestCallAfter(const std::function<void()>& func);
private Q_SLOTS:
	void OnChangeStyleSheetRequest(const QString& path);
	void HandleCallAfter(const std::function<void()>& func);
private:
	void InitializeCallbacks();
	void InitializeConnects();

	main_window* RPCS3MainWin;

	std::shared_ptr<gui_settings> guiSettings;
	std::shared_ptr<emu_settings> emuSettings;
	QWindow* gameWindow = nullptr; //! (Currently) only needed so that pad handlers have a valid target for event filtering.
};
