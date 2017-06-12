#ifndef RPCS3_APP_H
#define RPCS3_APP_H

#include "stdafx.h"

#include "keyboard_pad_handler.h"
#include "basic_keyboard_handler.h"
#include "basic_mouse_handler.h"

#include "Utilities/Config.h"
#include "Emu/RSX/GSRender.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/PadHandler.h"
#include "Emu/Io/MouseHandler.h"
#include "Emu/Audio/AudioThread.h"

#include "rpcs3qt/msg_dialog_frame.h"
#include "rpcs3qt/main_window.h"

#include <QApplication>

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
signals:
	void OnEmulatorRun();
	void OnEmulatorPause();
	void OnEmulatorResume();
	void OnEmulatorStop();
	void OnEmulatorReady();
	void RequestCallAfter(const std::function<void()>& func);
private slots:
	void OnChangeStyleSheetRequest(const QString& path);
	void HandleCallAfter(const std::function<void()>& func);
	void ResetPads();
private:
	void InitializeCallbacks();
	void InitializeConnects();

	// See ResetPads() for why these shared pointers exist.
	std::shared_ptr<keyboard_pad_handler> m_keyboardPadHandler;
	std::shared_ptr<basic_keyboard_handler> m_basicKeyboardHandler;
	std::shared_ptr<basic_mouse_handler> m_basicMouseHandler;

	main_window* RPCS3MainWin;
};
#endif
