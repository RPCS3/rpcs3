#ifndef RPCS3_APP_H
#define RPCS3_APP_H

#include "stdafx.h"

#include "KeyboardPadHandler.h"
#include "BasicKeyboardHandler.h"
#include "BasicMouseHandler.h"

#include "Utilities/Config.h"
#include "Emu/RSX/GSRender.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/PadHandler.h"
#include "Emu/Io/MouseHandler.h"
#include "Emu/Audio/AudioThread.h"

#include "rpcs3qt/MsgDialog.h"
#include "RPCS3Qt/mainwindow.h"

#include <QApplication>

/** RPCS3 Application Class
 * The point of this class is to do application initialization and to hold onto the main window. The main thing I intend this class to do, for now, is to initialize callbacks and the mainwindow.
*/

class RPCS3App : public QApplication
{
	Q_OBJECT
public:
	RPCS3App(int argc, char* argv[]);
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
	void InitializeHandlers();
	void InitializeConnects();

	// The mapss. Yay!
	cfg::map_entry<std::function<std::shared_ptr<GSRender>()>>* cfg_gs_render;
	cfg::map_entry<std::function<std::shared_ptr<KeyboardHandlerBase>()>>* cfg_kb_handler;
	cfg::map_entry<std::function<std::shared_ptr<PadHandlerBase>()>>* cfg_pad_handler;
	cfg::map_entry<std::function<std::shared_ptr<MouseHandlerBase>()>>* cfg_mouse_handler;
	cfg::map_entry<std::function<std::shared_ptr<AudioThread>()>>* cfg_audio_render;

	// See ResetPads() for why these shared pointers exist.
	std::shared_ptr<KeyboardPadHandler> m_keyboardPadHandler;
	std::shared_ptr<BasicKeyboardHandler> m_basicKeyboardHandler;
	std::shared_ptr<BasicMouseHandler> m_basicMouseHandler;

	MainWindow* RPCS3MainWin;
};
#endif
