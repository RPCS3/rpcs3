﻿#include "main_application.h"

#include "pad_thread.h"
#include "Emu/Io/Null/NullPadHandler.h"
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/PadHandler.h"
#include "Emu/Io/MouseHandler.h"
#include "basic_keyboard_handler.h"
#include "basic_mouse_handler.h"
#include "keyboard_pad_handler.h"
#include "ds4_pad_handler.h"
#ifdef _WIN32
#include "xinput_pad_handler.h"
#include "mm_joystick_handler.h"
#endif
#ifdef HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif

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

#include "Emu/RSX/GSRender.h"
#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"

#if defined(_WIN32) || defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKGSRender.h"
#endif

/** Emu.Init() wrapper for user manager */
bool main_application::InitializeEmulator(const std::string& user, bool force_init, bool show_gui)
{
	Emu.SetHasGui(show_gui);

	// try to set a new user
	const bool user_was_set = Emu.SetUsr(user);

	// only init the emulation if forced or a user was set
	if (user_was_set || force_init)
	{
		Emu.Init();
	}

	return user_was_set;
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here. */
EmuCallbacks main_application::CreateCallbacks()
{
	EmuCallbacks callbacks;

	callbacks.reset_pads = [this](const std::string& title_id)
	{
		pad::get_current_handler()->Reset(title_id);
	};
	callbacks.enable_pads = [this](bool enable)
	{
		pad::get_current_handler()->SetEnabled(enable);
	};

	callbacks.init_kb_handler = [=]()
	{
		switch (keyboard_handler type = g_cfg.io.keyboard)
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
		default: fmt::throw_exception("Invalid keyboard handler: %s", type);
		}
	};

	callbacks.init_mouse_handler = [=]()
	{
		switch (mouse_handler type = g_cfg.io.mouse)
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
		default: fmt::throw_exception("Invalid mouse handler: %s", type);
		}
	};

	callbacks.init_pad_handler = [this](std::string_view title_id)
	{
		g_fxo->init<pad_thread>(get_thread(), m_game_window, title_id);
	};

	callbacks.init_gs_render = []()
	{
		switch (video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null:
		{
			g_fxo->init<rsx::thread, named_thread<NullGSRender>>();
			break;
		}
		case video_renderer::opengl:
		{
			g_fxo->init<rsx::thread, named_thread<GLGSRender>>();
			break;
		}
#if defined(_WIN32) || defined(HAVE_VULKAN)
		case video_renderer::vulkan:
		{
			g_fxo->init<rsx::thread, named_thread<VKGSRender>>();
			break;
		}
#endif
		default: fmt::throw_exception("Invalid video renderer: %s" HERE, type);
		}
	};

	callbacks.get_audio = []() -> std::shared_ptr<AudioBackend>
	{
		switch (audio_renderer type = g_cfg.audio.renderer)
		{
		case audio_renderer::null: return std::make_shared<NullAudioBackend>();
#ifdef _WIN32
		case audio_renderer::xaudio: return std::make_shared<XAudio2Backend>();
#endif
#ifdef HAVE_ALSA
		case audio_renderer::alsa: return std::make_shared<ALSABackend>();
#endif
#ifdef HAVE_PULSE
		case audio_renderer::pulse: return std::make_shared<PulseBackend>();
#endif

		case audio_renderer::openal: return std::make_shared<OpenALBackend>();
#ifdef HAVE_FAUDIO
		case audio_renderer::faudio: return std::make_shared<FAudioBackend>();
#endif
		default: fmt::throw_exception("Invalid audio renderer: %s" HERE, type);
		}
	};

	return callbacks;
}
