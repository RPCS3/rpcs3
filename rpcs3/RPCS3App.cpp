#include "RPCS3App.h"

#include "Emu/System.h"
#include "RPCS3Qt/GSFrame.h"

#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "BasicKeyboardHandler.h"

#include "Emu/Io/Null/NullMouseHandler.h"
#include "BasicMouseHandler.h"

#include "Emu/Io/Null/NullPadHandler.h"
#include "KeyboardPadHandler.h"
#ifdef _MSC_VER
#include "XInputPadHandler.h"
#include "MMJoystickHandler.h"
#endif


#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"
#include "Emu/Audio/Null/NullAudioThread.h"
#include "Emu/Audio/AL/OpenALThread.h"
#ifdef _MSC_VER
#include "Emu/RSX/D3D12/D3D12GSRender.h"
#endif
#ifdef _WIN32
#include "Emu/RSX/VK/VKGSRender.h"
#include "Emu/Audio/XAudio2/XAudio2Thread.h"
//#include <wx/msw/wrapwin.h>
#endif
#ifdef __linux__
#include "Emu/Audio/ALSA/ALSAThread.h"
#endif

#include <QDesktopWidget>

// For now, a trivial constructor/destructor.  May add command line usage later.
RPCS3App::RPCS3App(int argc, char* argv[]) : QApplication(argc, argv)
{
}

void RPCS3App::Init()
{
	// Create the main window
	RPCS3MainWin = new MainWindow(nullptr);

	// Create the handlers.
	InitializeHandlers();

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	QSize defaultSize = QDesktopWidget().availableGeometry().size() * 0.7;
	RPCS3MainWin->resize(defaultSize);

	//TheApp = this;
	setApplicationName("RPCS3");
	//wxInitAllImageHandlers();

	Emu.Init();

	RPCS3MainWin->show();
	//m_MainFrame->DoSettings(true);

	// Load default style sheet, if it exists.
	ChangeStyleSheetRequest("stylesheet.qss");
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here.
*/
void RPCS3App::InitializeCallbacks()
{
	EmuCallbacks callbacks;

	callbacks.exit = [this]()
	{
		quit();
	};
	callbacks.call_after = [](std::function<void()> func)
	{	
		QTimer::singleShot(1, std::move(func));
	};

	callbacks.process_events = [this]()
	{
		RPCS3MainWin->update();
		processEvents();
	};

	callbacks.get_kb_handler = [=] { return cfg_kb_handler->get()(); };

	callbacks.get_mouse_handler = [=] { return cfg_mouse_handler->get()(); };

	callbacks.get_pad_handler = [=] { return cfg_pad_handler->get()(); }; 

	callbacks.get_gs_render = [=] { return cfg_gs_render->get()(); };

	callbacks.get_gs_frame = [](frame_type type, int w, int h) -> std::unique_ptr<GSFrameBase>
	{
		switch (type)
		{
		//case frame_type::OpenGL: return std::make_unique<GLGSFrame>(w, h);
		case frame_type::DX12: return std::make_unique<GSFrame>("DirectX 12", w, h);
		case frame_type::Null: return std::make_unique<GSFrame>("Null", w, h);
		case frame_type::Vulkan: return std::make_unique<GSFrame>("Vulkan", w, h);
		}

		fmt::throw_exception("Invalid frame type (0x%x)" HERE, (int)type);
	};

	
	callbacks.get_audio = [=] { return cfg_audio_render->get()(); };
	/*
	callbacks.get_msg_dialog = []() -> std::shared_ptr<MsgDialogBase>
	{
		return std::make_shared<MsgDialogFrame>();
	};

	callbacks.get_save_dialog = []() -> std::unique_ptr<SaveDialogBase>
	{
		return std::make_unique<SaveDialogFrame>();
	};*/

	callbacks.on_run = [=]() { emit OnEmulatorRun(); };
	callbacks.on_pause = [=]() {emit OnEmulatorPause(); };
	callbacks.on_resume = [=]() {emit OnEmulatorResume(); };
	callbacks.on_stop = [=]() {emit OnEmulatorStop(); };
	callbacks.on_ready = [=]() {emit OnEmulatorReady(); };

	Emu.SetCallbacks(std::move(callbacks));
}

/** 
* Initialize semi-global variables here that are used by the emulator code in callbacks.
*/
void RPCS3App::InitializeHandlers()
{
	cfg_gs_render = new cfg::map_entry<std::function<std::shared_ptr<GSRender>()>>(cfg::root.video, "Renderer", "OpenGL",
	{
		{ "Null", &std::make_shared<NullGSRender> },
		{ "OpenGL", &std::make_shared<GLGSRender> },
#ifdef _MSC_VER
		{ "D3D12", &std::make_shared<D3D12GSRender> },
#endif
#ifdef _WIN32
		{ "Vulkan", &std::make_shared<VKGSRender> },
#endif
	});

	// Should I move these declarations inside of the lambdas??
	// Lambda to simulate the return value from making a shared pointer with constructor.
	BasicKeyboardHandler* boardHandler = new BasicKeyboardHandler(this, this);
	auto l_magicKeyboardHandler = [boardHandler]() {
		return std::shared_ptr<BasicKeyboardHandler>(boardHandler);
	};

	cfg_kb_handler = new cfg::map_entry<std::function<std::shared_ptr<KeyboardHandlerBase>()>>(cfg::root.io, "Keyboard",
	{
		{ "Null", &std::make_shared<NullKeyboardHandler> },
		{ "Basic", l_magicKeyboardHandler },
	});

	// Lambda to simulate the return value from making a shared pointer with constructor.
	BasicMouseHandler* mouseHandler = new BasicMouseHandler(this, this);
	auto l_magicMouseHandler = [mouseHandler]() {
		return std::shared_ptr<BasicMouseHandler>(mouseHandler);
	};

	cfg_mouse_handler = new cfg::map_entry<std::function<std::shared_ptr<MouseHandlerBase>()>>(cfg::root.io, "Mouse",
	{
		{ "Null", &std::make_shared<NullMouseHandler> },
		{ "Basic", l_magicMouseHandler },
	});

	// Lambda to simulate the return value from making a shared pointer with constructor.
	KeyboardPadHandler* keyboardPadHandler = new KeyboardPadHandler(this, this);
	auto l_magicPadHandler = [keyboardPadHandler]() {
		return std::shared_ptr<KeyboardPadHandler>(keyboardPadHandler);
	};

	cfg_pad_handler = new cfg::map_entry<std::function<std::shared_ptr<PadHandlerBase>()>>(cfg::root.io, "Pad", "Keyboard",
	{
		{ "Null", &std::make_shared<NullPadHandler> },
		{ "Keyboard", l_magicPadHandler },
		#ifdef _MSC_VER
				{ "XInput", &std::make_shared<XInputPadHandler> },
				{ "MMJoystick", &std::make_shared<MMJoystickHandler> },
		#endif
	});


	cfg_audio_render = new cfg::map_entry<std::function<std::shared_ptr<AudioThread>()>>(cfg::root.audio, "Renderer", 1,
	{
		{ "Null", &std::make_shared<NullAudioThread> },
#ifdef _WIN32
		{ "XAudio2", &std::make_shared<XAudio2Thread> },
#elif __linux__
		{ "ALSA", &std::make_shared<ALSAThread> },
#endif
		{ "OpenAL", &std::make_shared<OpenALThread> },
	});
}

/*
 * Initialize connects here. These are used to connect things between UI elements that require the intervention of RPCS3App.
 */
void RPCS3App::InitializeConnects()
{
	connect(RPCS3MainWin, &MainWindow::RequestGlobalStylesheetChange, this, &RPCS3App::ChangeStyleSheetRequest);
	connect(this, &RPCS3App::OnEmulatorRun, RPCS3MainWin, &MainWindow::OnEmuRun);
	connect(this, &RPCS3App::OnEmulatorStop, RPCS3MainWin, &MainWindow::OnEmuStop);
	connect(this, &RPCS3App::OnEmulatorPause, RPCS3MainWin, &MainWindow::OnEmuPause);
	connect(this, &RPCS3App::OnEmulatorResume, RPCS3MainWin, &MainWindow::OnEmuResume);
	connect(this, &RPCS3App::OnEmulatorReady, RPCS3MainWin, &MainWindow::OnEmuReady);
}

/*
* Handle a request to change the stylesheet. May consider adding reporting of errors in future.
*/
void RPCS3App::ChangeStyleSheetRequest(const QString& sheetFilePath)
{
	QFile file(sheetFilePath);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		setStyleSheet(file.readAll());
		file.close();
	}
}
