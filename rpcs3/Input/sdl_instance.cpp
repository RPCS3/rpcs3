#ifdef HAVE_SDL3

#include "stdafx.h"
#include "sdl_instance.h"
#include "Emu/System.h"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "SDL3/SDL.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

LOG_CHANNEL(sdl_log, "SDL");

sdl_instance::~sdl_instance()
{
	// Only quit SDL once on exit. SDL uses a global state internally...
	if (m_initialized)
	{
		sdl_log.notice("Quitting SDL ...");
		SDL_Quit();
	}
}

sdl_instance& sdl_instance::get_instance()
{
	static sdl_instance instance{};
	return instance;
}

void sdl_instance::pump_events()
{
	std::lock_guard lock(m_instance_mutex);

	if (m_initialized)
	{
		SDL_PumpEvents();
	}
}

void sdl_instance::set_hint(const char* name, const char* value)
{
	if (!SDL_SetHint(name, value))
	{
		sdl_log.error("Could not set hint '%s' to '%s': %s", name, value, SDL_GetError());
	}
}

bool sdl_instance::initialize()
{
	std::lock_guard lock(m_instance_mutex);

	if (m_initialized)
	{
		return true;
	}

	bool instance_success = false;

	Emu.BlockingCallFromMainThread([this, &instance_success]()
	{
		instance_success = initialize_impl();
	}, false);

	return instance_success;
}

bool sdl_instance::initialize_impl()
{
	// Only init SDL once. SDL uses a global state internally...
	if (m_initialized)
	{
		return true;
	}

	sdl_log.notice("Initializing SDL ...");

	// Set non-dynamic hints before SDL_Init
	set_hint(SDL_HINT_JOYSTICK_THREAD, "1");

	// DS3 pressure sensitive buttons
#ifdef _WIN32
	set_hint(SDL_HINT_JOYSTICK_HIDAPI_PS3_SIXAXIS_DRIVER, "1");
#else
	set_hint(SDL_HINT_JOYSTICK_HIDAPI_PS3, "1");
#endif

	if (!SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC))
	{
		sdl_log.error("Could not initialize! SDL Error: %s", SDL_GetError());
		return false;
	}

	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
	SDL_SetLogOutputFunction([](void*, int category, SDL_LogPriority priority, const char* message)
		{
			std::string category_name;
			switch (category)
			{
			case SDL_LOG_CATEGORY_APPLICATION:
				category_name = "app";
				break;
			case SDL_LOG_CATEGORY_ERROR:
				category_name = "error";
				break;
			case SDL_LOG_CATEGORY_ASSERT:
				category_name = "assert";
				break;
			case SDL_LOG_CATEGORY_SYSTEM:
				category_name = "system";
				break;
			case SDL_LOG_CATEGORY_AUDIO:
				category_name = "audio";
				break;
			case SDL_LOG_CATEGORY_VIDEO:
				category_name = "video";
				break;
			case SDL_LOG_CATEGORY_RENDER:
				category_name = "render";
				break;
			case SDL_LOG_CATEGORY_INPUT:
				category_name = "input";
				break;
			case SDL_LOG_CATEGORY_TEST:
				category_name = "test";
				break;
			case SDL_LOG_CATEGORY_GPU:
				category_name = "gpu";
				break;
			default:
				category_name = fmt::format("unknown(%d)", category);
				break;
			}

			switch (priority)
			{
			case SDL_LOG_PRIORITY_VERBOSE:
			case SDL_LOG_PRIORITY_DEBUG:
				sdl_log.trace("%s: %s", category_name, message);
				break;
			case SDL_LOG_PRIORITY_INFO:
				sdl_log.notice("%s: %s", category_name, message);
				break;
			case SDL_LOG_PRIORITY_WARN:
				sdl_log.warning("%s: %s", category_name, message);
				break;
			case SDL_LOG_PRIORITY_ERROR:
				sdl_log.error("%s: %s", category_name, message);
				break;
			case SDL_LOG_PRIORITY_CRITICAL:
				sdl_log.error("%s: %s", category_name, message);
				break;
			default:
				break;
			}
		},
		nullptr);

	m_initialized = true;
	return true;
}

#endif
