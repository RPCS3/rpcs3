#pragma once

#include "util/atomic.hpp"
#include "util/types.hpp"

#include <mutex>

class camera_handler_base
{
public:
	enum class camera_handler_state
	{
		closed,
		open,
		running
	};

	virtual ~camera_handler_base() = default;

	virtual void open_camera() = 0;
	virtual void close_camera() = 0;
	virtual void start_camera() = 0;
	virtual void stop_camera() = 0;

	virtual void set_format(s32 format, u32 bytes_per_pixel) = 0;
	virtual void set_frame_rate(u32 frame_rate) = 0;
	virtual void set_resolution(u32 width, u32 height) = 0;
	virtual void set_mirrored(bool mirrored) = 0;

	virtual u64 frame_number() const = 0; // Convenience function to check if there's a new frame.
	virtual camera_handler_state get_image(u8* buf, u64 size, u32& width, u32& height, u64& frame_number, u64& bytes_read) = 0;

	camera_handler_state get_state() const { return m_state.load(); };

protected:
	std::mutex m_mutex;
	atomic_t<camera_handler_state> m_state = camera_handler_state::closed;
	bool m_mirrored = false;
	s32 m_format = 2; // CELL_CAMERA_RAW8
	u32 m_bytesize = 0;
	u32 m_width = 640;
	u32 m_height = 480;
	u32 m_frame_rate = 30;
};
