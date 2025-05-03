#ifdef HAVE_SDL3

#include "stdafx.h"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "SDL3/SDL.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include "util/logs.hpp"

#include "sdl_instance.h"

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
	const std::lock_guard<std::mutex> lock(m_instance_mutex);
	if (m_initialized)
		SDL_PumpEvents();
}

bool sdl_instance::initialize()
{
	const std::lock_guard<std::mutex> lock(m_instance_mutex);
	// Only init SDL once. SDL uses a global state internally...
	if (m_initialized)
	{
		return true;
	}

	sdl_log.notice("Initializing SDL ...");

	// Set non-dynamic hints before SDL_Init
	if (!SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1"))
	{
		sdl_log.error("Could not set SDL_HINT_JOYSTICK_THREAD: %s", SDL_GetError());
	}

	if (!SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD))
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
