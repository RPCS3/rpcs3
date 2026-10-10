#pragma once

#ifdef HAVE_SDL3

#include "Emu/Io/camera_handler_base.h"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "SDL3/SDL.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include <map>

class sdl_camera_video_sink;

class sdl_camera_handler : public camera_handler_base
{
public:
	sdl_camera_handler();
	virtual ~sdl_camera_handler();

	void open_camera() override;
	void close_camera() override;
	void start_camera() override;
	void stop_camera() override;
	void set_format(s32 format, u32 bytesize) override;
	void set_frame_rate(u32 frame_rate) override;
	void set_resolution(u32 width, u32 height) override;
	void set_mirrored(bool mirrored) override;
	u64 frame_number() const override;
	camera_handler_state get_image(u8* buf, u64 size, u32& width, u32& height, u64& frame_number, u64& bytes_read) override;

	static std::vector<std::string> get_drivers();
	static std::map<SDL_CameraID, std::string> get_cameras();

private:
	void reset();

	std::string m_camera_id;
	SDL_CameraID m_sdl_camera_id = 0;
	SDL_Camera* m_camera = nullptr;
	std::unique_ptr<sdl_camera_video_sink> m_video_sink;
};

#endif
