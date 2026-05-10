#pragma once

#ifdef HAVE_SDL3

#include "Input/camera_video_sink.h"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "SDL3/SDL.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include <thread>

class sdl_camera_video_sink final : public camera_video_sink
{
public:
	sdl_camera_video_sink(bool front_facing, SDL_Camera* camera);
	virtual ~sdl_camera_video_sink();

private:
	void present(SDL_Surface* frame);
	void run();

	std::vector<u8> m_buffer;
	atomic_t<bool> m_terminate = false;
	SDL_Camera* m_camera = nullptr;
	std::unique_ptr<std::thread> m_thread;
};

#endif
