#include "rpcs3_app.h"

#include "rpcs3qt/welcome_dialog.h"

#include "Emu/System.h"
#include "rpcs3qt/gs_frame.h"
#include "rpcs3qt/gl_gs_frame.h"

#include "Emu/Cell/Modules/cellSaveData.h"
#include "rpcs3qt/save_data_dialog.h"

#include "rpcs3qt/msg_dialog_frame.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"

#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "basic_keyboard_handler.h"

#include "Emu/Io/Null/NullMouseHandler.h"
#include "basic_mouse_handler.h"

#include "Emu/Io/Null/NullPadHandler.h"
#include "keyboard_pad_handler.h"
#include "ds4_pad_handler.h"
#ifdef _MSC_VER
#include "xinput_pad_handler.h"
#endif
#ifdef _WIN32
#include "mm_joystick_handler.h"
#endif
#ifdef HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif


#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"
#include "Emu/Audio/Null/NullAudioThread.h"
#include "Emu/Audio/AL/OpenALThread.h"
#ifdef _MSC_VER
#include "Emu/RSX/D3D12/D3D12GSRender.h"
#endif
#if defined(_WIN32) || defined(__linux__)
#include "Emu/RSX/VK/VKGSRender.h"
#endif
#ifdef _WIN32
#include "Emu/Audio/XAudio2/XAudio2Thread.h"
#endif
#ifdef HAVE_ALSA
#include "Emu/Audio/ALSA/ALSAThread.h"
#endif

// For now, a trivial constructor/destructor.  May add command line usage later.
rpcs3_app::rpcs3_app(int& argc, char** argv) : QApplication(argc, argv)
{
}

void rpcs3_app::Init()
{
	Emu.Init();

	guiSettings.reset(new gui_settings());

	// Create the main window
	RPCS3MainWin = new main_window(guiSettings, nullptr);

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	RPCS3MainWin->Init();

	setApplicationName("RPCS3");
	RPCS3MainWin->show();

	// Create the thumbnail toolbar after the main_window is created
	RPCS3MainWin->CreateThumbnailToolbar();

	if (guiSettings->GetValue(GUI::ib_show_welcome).toBool())
	{
		welcome_dialog* welcome = new welcome_dialog();
		welcome->exec();
	}
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
		RequestCallAfter(std::move(func));
	};

	callbacks.process_events = [this]()
	{
		RPCS3MainWin->update();
		processEvents();
	};

	callbacks.get_kb_handler = [=]() -> std::shared_ptr<KeyboardHandlerBase>
	{
		switch (keyboard_handler type = g_cfg.io.keyboard)
		{
		case keyboard_handler::null: return std::make_shared<NullKeyboardHandler>();
		case keyboard_handler::basic:
		{
			basic_keyboard_handler* ret = new basic_keyboard_handler();
			ret->moveToThread(thread());
			ret->SetTargetWindow(gameWindow);
			return std::shared_ptr<KeyboardHandlerBase>(ret);
		}
		default: fmt::throw_exception("Invalid keyboard handler: %s", type);
		}
	};

	callbacks.get_mouse_handler = [=]() -> std::shared_ptr<MouseHandlerBase>
	{
		switch (mouse_handler type = g_cfg.io.mouse)
		{
		case mouse_handler::null: return std::make_shared<NullMouseHandler>();
		case mouse_handler::basic:
		{
			basic_mouse_handler* ret = new basic_mouse_handler();
			ret->moveToThread(thread());
			ret->SetTargetWindow(gameWindow);
			return std::shared_ptr<MouseHandlerBase>(ret);
		}
		default: fmt::throw_exception("Invalid mouse handler: %s", type);
		}
	};

	callbacks.get_pad_handler = [this]() -> std::shared_ptr<PadHandlerBase>
	{
		switch (pad_handler type = g_cfg.io.pad)
		{
		case pad_handler::null: return std::make_shared<NullPadHandler>();
		case pad_handler::keyboard:
		{
			keyboard_pad_handler* ret = new keyboard_pad_handler();
			ret->moveToThread(thread());
			ret->SetTargetWindow(gameWindow);
			return std::shared_ptr<PadHandlerBase>(ret);
		}
		case pad_handler::ds4: return std::make_shared<ds4_pad_handler>();
#ifdef _MSC_VER
		case pad_handler::xinput: return std::make_shared<xinput_pad_handler>();
#endif
#ifdef _WIN32
		case pad_handler::mm: return std::make_shared<mm_joystick_handler>();
#endif
#ifdef HAVE_LIBEVDEV
		case pad_handler::evdev: return std::make_shared<evdev_joystick_handler>();
#endif
		default: fmt::throw_exception("Invalid pad handler: %s", type);
		}
	};

	callbacks.get_gs_frame = [this]() -> std::unique_ptr<GSFrameBase>
	{
		extern const std::unordered_map<video_resolution, std::pair<int, int>, value_hash<video_resolution>> g_video_out_resolution_map;

		const auto size = g_video_out_resolution_map.at(g_cfg.video.resolution);
		int w = size.first;
		int h = size.second;

		if (guiSettings->GetValue(GUI::gs_resize).toBool())
		{
			w = guiSettings->GetValue(GUI::gs_width).toInt();
			h = guiSettings->GetValue(GUI::gs_height).toInt();
		}

		bool disableMouse = guiSettings->GetValue(GUI::gs_disableMouse).toBool();

		switch (video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null:
		{
			gs_frame* ret = new gs_frame("Null", w, h, RPCS3MainWin->GetAppIcon(), disableMouse);
			gameWindow = ret;
			return std::unique_ptr<gs_frame>(ret);
		}
		case video_renderer::opengl: 
		{
			gl_gs_frame* ret = new gl_gs_frame(w, h, RPCS3MainWin->GetAppIcon(), disableMouse);
			gameWindow = ret;
			return std::unique_ptr<gl_gs_frame>(ret);
		}
		case video_renderer::vulkan:
		{
			gs_frame* ret = new gs_frame("Vulkan", w, h, RPCS3MainWin->GetAppIcon(), disableMouse);
			gameWindow = ret;
			return std::unique_ptr<gs_frame>(ret);
		}
#ifdef _MSC_VER
		case video_renderer::dx12:
		{
			gs_frame* ret = new gs_frame("DirectX 12", w, h, RPCS3MainWin->GetAppIcon(), disableMouse);
			gameWindow = ret;
			return std::unique_ptr<gs_frame>(ret);
		}
#endif
		default: fmt::throw_exception("Invalid video renderer: %s" HERE, type);
		}
	};

	callbacks.get_gs_render = []() -> std::shared_ptr<GSRender>
	{
		switch (video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null: return std::make_shared<NullGSRender>();
		case video_renderer::opengl: return std::make_shared<GLGSRender>();
#if defined(_WIN32) || defined(__linux__)
		case video_renderer::vulkan: return std::make_shared<VKGSRender>();
#endif
#ifdef _MSC_VER
		case video_renderer::dx12: return std::make_shared<D3D12GSRender>();
#endif
		default: fmt::throw_exception("Invalid video renderer: %s" HERE, type);
		}
	};

	callbacks.get_audio = []() -> std::shared_ptr<AudioThread>
	{
		switch (audio_renderer type = g_cfg.audio.renderer)
		{
		case audio_renderer::null: return std::make_shared<NullAudioThread>();
#ifdef _WIN32
		case audio_renderer::xaudio: return std::make_shared<XAudio2Thread>();
#elif defined(HAVE_ALSA)
		case audio_renderer::alsa: return std::make_shared<ALSAThread>();
#endif
		case audio_renderer::openal: return std::make_shared<OpenALThread>();
		default: fmt::throw_exception("Invalid audio renderer: %s" HERE, type);
		}
	};

	callbacks.get_msg_dialog = [=]() -> std::shared_ptr<MsgDialogBase>
	{
		return std::make_shared<msg_dialog_frame>(RPCS3MainWin->windowHandle());
	};

	callbacks.get_save_dialog = [=]() -> std::unique_ptr<SaveDialogBase>
	{
		return std::make_unique<save_data_dialog>();
	};

	callbacks.on_run = [=]() { OnEmulatorRun(); };
	callbacks.on_pause = [=]() { OnEmulatorPause(); };
	callbacks.on_resume = [=]() { OnEmulatorResume(); };
	callbacks.on_stop = [=]() { OnEmulatorStop(); };
	callbacks.on_ready = [=]() { OnEmulatorReady(); };

	Emu.SetCallbacks(std::move(callbacks));
}

/*
 * Initialize connects here. These are used to connect things between UI elements that require the intervention of rpcs3_app.
 */
void rpcs3_app::InitializeConnects()
{
	connect(RPCS3MainWin, &main_window::RequestGlobalStylesheetChange, this, &rpcs3_app::OnChangeStyleSheetRequest);

	qRegisterMetaType <std::function<void()>>("std::function<void()>");
	connect(this, &rpcs3_app::RequestCallAfter, this, &rpcs3_app::HandleCallAfter);

	connect(this, &rpcs3_app::OnEmulatorRun, RPCS3MainWin, &main_window::OnEmuRun);
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
		// toolbar color stylesheet
		QColor tbc = GUI::mw_tool_bar_color;
		QString style_toolbar = QString(
			"QLineEdit#mw_searchbar { margin-left:14px; background-color: rgba(%1, %2, %3, %4); }"
			"QToolBar#mw_toolbar { background-color: rgba(%1, %2, %3, %4); }"
			"QToolBar#mw_toolbar QSlider { background-color: rgba(%1, %2, %3, %4); }"
			"QToolBar#mw_toolbar::separator {background-color: rgba(%5, %6, %7, %8); width: 1px; margin-top: 2px; margin-bottom: 2px;}")
			.arg(tbc.red()).arg(tbc.green()).arg(tbc.blue()).arg(tbc.alpha())
			.arg(tbc.red() - 20).arg(tbc.green() - 20).arg(tbc.blue() - 20).arg(tbc.alpha() - 20);

		// toolbar icon color stylesheet
		QColor tic = GUI::mw_tool_icon_color;
		QString style_toolbar_icons = QString(
			"QLabel#toolbar_icon_color { color: rgba(%1, %2, %3, %4); }")
			.arg(tic.red()).arg(tic.green()).arg(tic.blue()).arg(tic.alpha());

		// gamelist toolbar stylesheet
		QColor gltic = GUI::gl_tool_icon_color;
		QString style_gamelist_toolbar = QString(
			"QLineEdit#tb_searchbar { background: transparent; }"
			"QLabel#gamelist_toolbar_icon_color { color: rgba(%1, %2, %3, %4); }")
			.arg(gltic.red()).arg(gltic.green()).arg(gltic.blue()).arg(gltic.alpha());

		// gamelist icon color stylesheet
		QColor glic = GUI::gl_icon_color;
		QString style_gamelist_icons = QString(
			"QLabel#gamelist_icon_background_color { color: rgba(%1, %2, %3, %4); }")
			.arg(glic.red()).arg(glic.green()).arg(glic.blue()).arg(glic.alpha());

		// other objects' stylesheet
		QString style_rest = QString(
			"QWidget#header_section { background-color: #ffffff; }"
			"QLabel#gamegrid_font { font-weight: 600; font-size: 8pt; font-family: Lucida Grande; color: rgba(51, 51, 51, 255); }");

		setStyleSheet(style_toolbar + style_toolbar_icons + style_gamelist_toolbar + style_gamelist_icons  + style_rest);
	}
	else if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		setStyleSheet(file.readAll());
		file.close();
	}
	GUI::stylesheet = styleSheet();
}

/**
 * Using connects avoids timers being unable to be used in a non-qt thread.  So, even if this looks stupid to just call func, it's succinct.
*/
void rpcs3_app::HandleCallAfter(const std::function<void()>& func)
{
	func();
}
