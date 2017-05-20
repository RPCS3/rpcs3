#include "rpcs3_app.h"

#include "Emu/System.h"
#include "RPCS3Qt/gs_frame.h"
#include "RPCS3Qt/gl_gs_frame.h"

#include "Emu/Cell/Modules/cellSaveData.h"
#include "RPCS3Qt/save_data_dialog.h"

#include "rpcs3qt/msg_dialog_Frame.h"
#include "Emu/Cell/Modules/CellMsgDialog.h"

#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "basic_keyboard_handler.h"

#include "Emu/Io/Null/NullMouseHandler.h"
#include "basic_mouse_handler.h"

#include "Emu/Io/Null/NullPadHandler.h"
#include "keyboard_pad_handler.h"
#include "ds4_pad_handler.h"
#ifdef _MSC_VER
#include "xinput_pad_handler.h"
#include "mm_joystick_handler.h"
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

// For now, a trivial constructor/destructor.  May add command line usage later.
rpcs3_app::rpcs3_app(int argc, char* argv[]) : QApplication(argc, argv)
{
}

void rpcs3_app::Init()
{
	// Create the main window
	RPCS3MainWin = new main_window(nullptr);

	// Create the handlers.
	InitializeHandlers();

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	//TheApp = this;
	setApplicationName("RPCS3");
	//wxInitAllImageHandlers();

	Emu.Init();

	RPCS3MainWin->show();
	//m_MainFrame->DoSettings(true);

	// Create the thumbnail toolbar after the main_window is created
	RPCS3MainWin->CreateThumbnailToolbar();

}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here.
*/
void rpcs3_app::InitializeCallbacks()
{
	EmuCallbacks callbacks;

	callbacks.exit = [this]()
	{
		quit();
	};
	callbacks.call_after = [=](std::function<void()> func)
	{	
		emit RequestCallAfter(std::move(func));
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

	callbacks.get_gs_frame = [=](frame_type type, int w, int h) -> std::unique_ptr<GSFrameBase>
	{
		switch (type)
		{
		case frame_type::OpenGL: return std::make_unique<gl_gs_frame>(w, h, RPCS3MainWin->GetAppIcon());
		case frame_type::DX12: return std::make_unique<gs_frame>("DirectX 12", w, h, RPCS3MainWin->GetAppIcon());
		case frame_type::Null: return std::make_unique<gs_frame>("Null", w, h, RPCS3MainWin->GetAppIcon());
		case frame_type::Vulkan: return std::make_unique<gs_frame>("Vulkan", w, h, RPCS3MainWin->GetAppIcon());
		}

		fmt::throw_exception("Invalid frame type (0x%x)" HERE, (int)type);
	};

	
	callbacks.get_audio = [=] { return cfg_audio_render->get()(); };
	
	callbacks.get_msg_dialog = [=]() -> std::shared_ptr<MsgDialogBase>
	{
		return std::make_shared<msg_dialog_frame>();
	};

	callbacks.get_save_dialog = [=]() -> std::unique_ptr<SaveDialogBase>
	{
		return std::make_unique<save_data_dialog>();
	};

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
void rpcs3_app::InitializeHandlers()
{
	ResetPads();

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

	cfg_kb_handler = new cfg::map_entry<std::function<std::shared_ptr<KeyboardHandlerBase>()>>(cfg::root.io, "Keyboard",
	{
		{ "Null", &std::make_shared<NullKeyboardHandler> },
		{ "Basic", [=]() {return m_basicKeyboardHandler; }},
	});

	cfg_mouse_handler = new cfg::map_entry<std::function<std::shared_ptr<MouseHandlerBase>()>>(cfg::root.io, "Mouse",
	{
		{ "Null", &std::make_shared<NullMouseHandler> },
		{ "Basic", [=]() {return m_basicMouseHandler; } },
	});

	cfg_pad_handler = new cfg::map_entry<std::function<std::shared_ptr<PadHandlerBase>()>>(cfg::root.io, "Pad", "Keyboard",
	{
		{ "Null", &std::make_shared<NullPadHandler> },
		{ "Keyboard", [=]() {return m_keyboardPadHandler; } },
		{ "DualShock 4", &std::make_shared<ds4_pad_handler> },
		#ifdef _MSC_VER
				{ "XInput", &std::make_shared<xinput_pad_handler> },
				{ "MMJoystick", &std::make_shared<mm_joystick_handler> },
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
 * Initialize connects here. These are used to connect things between UI elements that require the intervention of rpcs3_app.
 */
void rpcs3_app::InitializeConnects()
{
	connect(RPCS3MainWin, &main_window::RequestGlobalStylesheetChange, this, &rpcs3_app::OnChangeStyleSheetRequest);

	qRegisterMetaType <std::function<void()>>("std::function<void()>");
	connect(this, &rpcs3_app::RequestCallAfter, this, &rpcs3_app::HandleCallAfter); // might need blocking, might not. Idk

	connect(this, &rpcs3_app::OnEmulatorRun, RPCS3MainWin, &main_window::OnEmuRun);
	connect(this, &rpcs3_app::OnEmulatorStop, this, &rpcs3_app::ResetPads);
	connect(this, &rpcs3_app::OnEmulatorStop, RPCS3MainWin, &main_window::OnEmuStop);
	connect(this, &rpcs3_app::OnEmulatorPause, RPCS3MainWin, &main_window::OnEmuPause);
	connect(this, &rpcs3_app::OnEmulatorResume, RPCS3MainWin, &main_window::OnEmuResume);
	connect(this, &rpcs3_app::OnEmulatorReady, RPCS3MainWin, &main_window::OnEmuReady);
}

/*
* Handle a request to change the stylesheet. May consider adding reporting of errors in future.
* Empty string means default.
*/
void rpcs3_app::OnChangeStyleSheetRequest(const QString& sheetFilePath)
{
	QFile file(sheetFilePath);
	if (sheetFilePath == "")
	{
		setStyleSheet("");
	}
	else if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		setStyleSheet(file.readAll());
		file.close();
	}
}

/**
 * Using connects avoids timers being unable to be used in a non-qt thread.  So, even if this looks stupid to just call func, it's succinct.
*/
void rpcs3_app::HandleCallAfter(const std::function<void()>& func)
{
	func();
}

/**
 * We need to make this in the main thread to receive events from the main thread.
 * This leads to the tricky situation.  Creating it while booting leads to deadlock with a blocking connection.
 * So, I need to make them before, but when?
 * I opted to reset them when the Emu stops and on first init. Potentially a race condition on restart? Never encountered issues.
 * The other tricky issue is that I don't want Init to be called twice on the same object. Reseting the pointer on emu stop should handle this as well!
*/
void rpcs3_app::ResetPads()
{
	m_basicKeyboardHandler.reset(new basic_keyboard_handler(this, this));
	m_basicMouseHandler.reset(new basic_mouse_handler(this, this));
	m_keyboardPadHandler.reset(new keyboard_pad_handler(this, this));
}
