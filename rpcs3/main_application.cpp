#include "main_application.h"

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
#ifdef _MSC_VER
#include "Emu/RSX/D3D12/D3D12GSRender.h"
#endif
#if defined(_WIN32) || defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKGSRender.h"
#endif

/** Emu.Init() wrapper for user manager */
bool main_application::InitializeEmulator(const std::string& user, bool force_init)
{
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

	callbacks.reset_pads = [this](const std::string& title_id = "")
	{
		pad::get_current_handler()->Reset(title_id);
	};
	callbacks.enable_pads = [this](bool enable)
	{
		pad::get_current_handler()->SetEnabled(enable);
	};

	callbacks.get_kb_handler = [=]() -> std::shared_ptr<KeyboardHandlerBase>
	{
		switch (keyboard_handler type = g_cfg.io.keyboard)
		{
		case keyboard_handler::null: return std::make_shared<NullKeyboardHandler>();
		case keyboard_handler::basic:
		{
			basic_keyboard_handler* ret = new basic_keyboard_handler();
			ret->moveToThread(get_thread());
			ret->SetTargetWindow(m_game_window);
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
			ret->moveToThread(get_thread());
			ret->SetTargetWindow(m_game_window);
			return std::shared_ptr<MouseHandlerBase>(ret);
		}
		default: fmt::throw_exception("Invalid mouse handler: %s", type);
		}
	};

	callbacks.get_pad_handler = [this](const std::string& title_id) -> std::shared_ptr<pad_thread>
	{
		return std::make_shared<pad_thread>(get_thread(), m_game_window, title_id);
	};

	callbacks.get_gs_render = []() -> std::shared_ptr<GSRender>
	{
		switch (video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null: return std::make_shared<named_thread<NullGSRender>>("rsx::thread");
		case video_renderer::opengl: return std::make_shared<named_thread<GLGSRender>>("rsx::thread");
#if defined(_WIN32) || defined(HAVE_VULKAN)
		case video_renderer::vulkan: return std::make_shared<named_thread<VKGSRender>>("rsx::thread");
#endif
#ifdef _MSC_VER
		case video_renderer::dx12: return std::make_shared<named_thread<D3D12GSRender>>("rsx::thread");
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
