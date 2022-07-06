#include "main_application.h"

#include "util/types.hpp"
#include "util/logs.hpp"
#include "util/sysinfo.hpp"

#include "Utilities/Thread.h"
#include "Input/pad_thread.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/IdManager.h"
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/MouseHandler.h"
#include "Input/basic_keyboard_handler.h"
#include "Input/basic_mouse_handler.h"

#include "Emu/Audio/AudioBackend.h"
#include "Emu/Audio/Null/NullAudioBackend.h"
#include "Emu/Audio/Null/null_enumerator.h"
#include "Emu/Audio/Cubeb/CubebBackend.h"
#include "Emu/Audio/Cubeb/cubeb_enumerator.h"
#ifdef _WIN32
#include "Emu/Audio/XAudio2/XAudio2Backend.h"
#include "Emu/Audio/XAudio2/xaudio2_enumerator.h"
#endif
#ifdef HAVE_FAUDIO
#include "Emu/Audio/FAudio/FAudioBackend.h"
#include "Emu/Audio/FAudio/faudio_enumerator.h"
#endif

#include <QFileInfo> // This shouldn't be outside rpcs3qt...
#include <thread>

LOG_CHANNEL(sys_log, "SYS");

/** Emu.Init() wrapper for user management */
void main_application::InitializeEmulator(const std::string& user, bool show_gui)
{
	Emu.SetHasGui(show_gui);
	Emu.SetUsr(user);
	Emu.Init();

	// Log Firmware Version after Emu was initialized
	const std::string firmware_version = utils::get_firmware_version();
	const std::string firmware_string  = firmware_version.empty() ? "Missing Firmware" : ("Firmware version: " + firmware_version);
	sys_log.always()("%s", firmware_string);
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here. */
EmuCallbacks main_application::CreateCallbacks()
{
	EmuCallbacks callbacks;

	callbacks.init_kb_handler = [this]()
	{
		switch (g_cfg.io.keyboard.get())
		{
		case keyboard_handler::null:
		{
			g_fxo->init<KeyboardHandlerBase, NullKeyboardHandler>(Emu.DeserialManager());
			break;
		}
		case keyboard_handler::basic:
		{
			basic_keyboard_handler* ret = g_fxo->init<KeyboardHandlerBase, basic_keyboard_handler>(Emu.DeserialManager());
			ret->moveToThread(get_thread());
			ret->SetTargetWindow(m_game_window);
			break;
		}
		}
	};

	callbacks.init_mouse_handler = [this]()
	{
		switch (g_cfg.io.mouse.get())
		{
		case mouse_handler::null:
		{
			if (g_cfg.io.move == move_handler::mouse)
			{
				basic_mouse_handler* ret = g_fxo->init<MouseHandlerBase, basic_mouse_handler>(Emu.DeserialManager());
				ret->moveToThread(get_thread());
				ret->SetTargetWindow(m_game_window);
			}
			else
				g_fxo->init<MouseHandlerBase, NullMouseHandler>(Emu.DeserialManager());

			break;
		}
		case mouse_handler::basic:
		{
			basic_mouse_handler* ret = g_fxo->init<MouseHandlerBase, basic_mouse_handler>(Emu.DeserialManager());
			ret->moveToThread(get_thread());
			ret->SetTargetWindow(m_game_window);
			break;
		}
		}
	};

	callbacks.init_pad_handler = [this](std::string_view title_id)
	{
		ensure(g_fxo->init<named_thread<pad_thread>>(get_thread(), m_game_window, title_id));
		while (pad::g_reset) std::this_thread::yield();
	};

	callbacks.get_audio = []() -> std::shared_ptr<AudioBackend>
	{
		std::shared_ptr<AudioBackend> result;
		switch (g_cfg.audio.renderer.get())
		{
		case audio_renderer::null: result = std::make_shared<NullAudioBackend>(); break;
#ifdef _WIN32
		case audio_renderer::xaudio: result = std::make_shared<XAudio2Backend>(); break;
#endif
		case audio_renderer::cubeb: result = std::make_shared<CubebBackend>(); break;
#ifdef HAVE_FAUDIO
		case audio_renderer::faudio: result = std::make_shared<FAudioBackend>(); break;
#endif
		}

		if (!result->Initialized())
		{
			// Fall back to a null backend if something went wrong
			sys_log.error("Audio renderer %s could not be initialized, using a Null renderer instead. Make sure that no other application is running that might block audio access (e.g. Netflix).", result->GetName());
			result = std::make_shared<NullAudioBackend>();
		}
		return result;
	};

	callbacks.get_audio_enumerator = [](u64 renderer) -> std::shared_ptr<audio_device_enumerator>
	{
		switch (static_cast<audio_renderer>(renderer))
		{
		case audio_renderer::null: return std::make_shared<null_enumerator>();
#ifdef _WIN32
		case audio_renderer::xaudio: return std::make_shared<xaudio2_enumerator>();
#endif
		case audio_renderer::cubeb: return std::make_shared<cubeb_enumerator>();
#ifdef HAVE_FAUDIO
		case audio_renderer::faudio: return std::make_shared<faudio_enumerator>();
#endif
		default: fmt::throw_exception("Invalid renderer index %u", renderer);
		}
	};

	callbacks.resolve_path = [](std::string_view sv)
	{
		return QFileInfo(QString::fromUtf8(sv.data(), static_cast<int>(sv.size()))).canonicalFilePath().toStdString();
	};

	return callbacks;
}
