#include "main_application.h"

#include "util/types.hpp"
#include "util/logs.hpp"
#include "util/sysinfo.hpp"

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
#include "Emu/Audio/AL/OpenALBackend.h"
#ifdef _WIN32
#include "Emu/Audio/XAudio2/XAudio2Backend.h"
#endif
#ifdef HAVE_ALSA
#include "Emu/Audio/ALSA/ALSABackend.h"
#endif
#ifdef HAVE_PULSE
#include "Emu/Audio/Pulse/PulseBackend.h"
#endif
#ifdef HAVE_FAUDIO
#include "Emu/Audio/FAudio/FAudioBackend.h"
#endif

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
			g_fxo->init<KeyboardHandlerBase, NullKeyboardHandler>();
			break;
		}
		case keyboard_handler::basic:
		{
			basic_keyboard_handler* ret = g_fxo->init<KeyboardHandlerBase, basic_keyboard_handler>();
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
				basic_mouse_handler* ret = g_fxo->init<MouseHandlerBase, basic_mouse_handler>();
				ret->moveToThread(get_thread());
				ret->SetTargetWindow(m_game_window);
			}
			else
				g_fxo->init<MouseHandlerBase, NullMouseHandler>();

			break;
		}
		case mouse_handler::basic:
		{
			basic_mouse_handler* ret = g_fxo->init<MouseHandlerBase, basic_mouse_handler>();
			ret->moveToThread(get_thread());
			ret->SetTargetWindow(m_game_window);
			break;
		}
		}
	};

	callbacks.init_pad_handler = [this](std::string_view title_id)
	{
		g_fxo->init<pad_thread>(get_thread(), m_game_window, title_id);
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
#ifdef HAVE_ALSA
		case audio_renderer::alsa: result = std::make_shared<ALSABackend>(); break;
#endif
#ifdef HAVE_PULSE
		case audio_renderer::pulse: result = std::make_shared<PulseBackend>(); break;
#endif

		case audio_renderer::openal: result = std::make_shared<OpenALBackend>(); break;
#ifdef HAVE_FAUDIO
		case audio_renderer::faudio: result = std::make_shared<FAudioBackend>(); break;
#endif
		}

		if (!result->Initialized())
		{
			// Fall back to a null backend if something went wrong
			sys_log.error("Audio renderer %s could not be initialized, using a Null renderer instead", result->GetName());
			result = std::make_shared<NullAudioBackend>();
		}
		return result;
	};

	return callbacks;
}
