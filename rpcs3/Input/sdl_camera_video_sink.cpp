#ifdef HAVE_SDL3

#include "stdafx.h"
#include "sdl_camera_video_sink.h"
#include "Utilities/Thread.h"
#include "Emu/system_config.h"

LOG_CHANNEL(camera_log, "Camera");

sdl_camera_video_sink::sdl_camera_video_sink(bool front_facing, SDL_Camera* camera)
	: camera_video_sink(front_facing), m_camera(camera)
{
	ensure(m_camera);

	m_thread = std::make_unique<std::thread>(&sdl_camera_video_sink::run, this);
}

sdl_camera_video_sink::~sdl_camera_video_sink()
{
	m_terminate = true;

	if (m_thread && m_thread->joinable())
	{
		m_thread->join();
		m_thread.reset();
	}
}

void sdl_camera_video_sink::present(SDL_Surface* frame)
{
	const int bytes_per_pixel = SDL_BYTESPERPIXEL(frame->format);
	const u32 src_width_in_bytes = std::max(0, frame->w * bytes_per_pixel);
	const u32 dst_width_in_bytes = std::max<u32>(0, m_width * bytes_per_pixel);
	const u8* pixels = reinterpret_cast<const u8*>(frame->pixels);

	bool use_buffer = false;

	// Scale image if necessary
	const bool scale_image = m_width > 0 && m_height > 0 && m_width != static_cast<u32>(frame->w) && m_height != static_cast<u32>(frame->h);

	// Determine image flip
	const camera_flip flip_setting = g_cfg.io.camera_flip_option;

	bool flip_horizontally = m_front_facing; // Front facing cameras are flipped already
	if (flip_setting == camera_flip::horizontal || flip_setting == camera_flip::both)
	{
		flip_horizontally = !flip_horizontally;
	}
	if (m_mirrored) // Set by the game
	{
		flip_horizontally = !flip_horizontally;
	}

	bool flip_vertically = false;
	if (flip_setting == camera_flip::vertical || flip_setting == camera_flip::both)
	{
		flip_vertically = !flip_vertically;
	}

	// Flip image if necessary
	if (flip_horizontally || flip_vertically || scale_image)
	{
		m_buffer.resize(m_height * dst_width_in_bytes);
		use_buffer = true;

		if (m_width > 0 && m_height > 0 && frame->w > 0 && frame->h > 0)
		{
			const f32 scale_x = frame->w / static_cast<f32>(m_width);
			const f32 scale_y = frame->h / static_cast<f32>(m_height);

			if (flip_horizontally && flip_vertically)
			{
				for (u32 y = 0; y < m_height; y++)
				{
					const u32 src_y = frame->h - static_cast<u32>(scale_y * y) - 1;
					const u8* src = pixels + src_y * src_width_in_bytes;
					u8* dst = &m_buffer[y * dst_width_in_bytes];

					for (u32 x = 0; x < m_width; x++)
					{
						const u32 src_x = frame->w - static_cast<u32>(scale_x * x) - 1;
						std::memcpy(dst + x * bytes_per_pixel, src + src_x * bytes_per_pixel, bytes_per_pixel);
					}
				}
			}
			else if (flip_horizontally)
			{
				for (u32 y = 0; y < m_height; y++)
				{
					const u32 src_y = static_cast<u32>(scale_y * y);
					const u8* src = pixels + src_y * src_width_in_bytes;
					u8* dst = &m_buffer[y * dst_width_in_bytes];

					for (u32 x = 0; x < m_width; x++)
					{
						const u32 src_x = frame->w - static_cast<u32>(scale_x * x) - 1;
						std::memcpy(dst + x * bytes_per_pixel, src + src_x * bytes_per_pixel, bytes_per_pixel);
					}
				}
			}
			else if (flip_vertically)
			{
				for (u32 y = 0; y < m_height; y++)
				{
					const u32 src_y = frame->h - static_cast<u32>(scale_y * y) - 1;
					const u8* src = pixels + src_y * src_width_in_bytes;
					u8* dst = &m_buffer[y * dst_width_in_bytes];

					for (u32 x = 0; x < m_width; x++)
					{
						const u32 src_x = static_cast<u32>(scale_x * x);
						std::memcpy(dst + x * bytes_per_pixel, src + src_x * bytes_per_pixel, bytes_per_pixel);
					}
				}
			}
			else
			{
				for (u32 y = 0; y < m_height; y++)
				{
					const u32 src_y = static_cast<u32>(scale_y * y);
					const u8* src = pixels + src_y * src_width_in_bytes;
					u8* dst = &m_buffer[y * dst_width_in_bytes];

					for (u32 x = 0; x < m_width; x++)
					{
						const u32 src_x = static_cast<u32>(scale_x * x);
						std::memcpy(dst + x * bytes_per_pixel, src + src_x * bytes_per_pixel, bytes_per_pixel);
					}
				}
			}
		}
	}

	if (use_buffer)
	{
		camera_video_sink::present(m_width, m_height, dst_width_in_bytes, bytes_per_pixel, [src = m_buffer.data(), dst_width_in_bytes](u32 y){ return src + y * dst_width_in_bytes; });
	}
	else
	{
		camera_video_sink::present(frame->w, frame->h, frame->pitch, bytes_per_pixel, [pixels, pitch = frame->pitch](u32 y){ return pixels + y * pitch; });
	}
}

void sdl_camera_video_sink::run()
{
	thread_base::set_name("SDL Capture Thread");

	camera_log.notice("SDL Capture Thread started");

	while (!m_terminate)
	{
		// Copy latest image into out buffer.
		u64 timestamp_ns = 0;
		SDL_Surface* frame = SDL_AcquireCameraFrame(m_camera, &timestamp_ns);
		if (!frame)
		{
			// No new frame
			std::this_thread::sleep_for(100us);
			continue;
		}

		present(frame);

		SDL_ReleaseCameraFrame(m_camera, frame);
	}
}

#endif
