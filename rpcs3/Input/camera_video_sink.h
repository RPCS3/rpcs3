#pragma once

#include <mutex>

class camera_video_sink
{
public:
	camera_video_sink(bool front_facing);
	virtual ~camera_video_sink();

	void set_format(s32 format, u32 bytesize);
	void set_resolution(u32 width, u32 height);
	void set_mirrored(bool mirrored);

	u64 frame_number() const;

	bool present(u32 src_width, u32 src_height, u32 src_pitch, u32 src_bytes_per_pixel, std::function<const u8*(u32)> src_line_ptr);

	void get_image(u8* buf, u64 size, u32& width, u32& height, u64& frame_number, u64& bytes_read);

protected:
	u32 read_index() const;

	bool m_front_facing = false;
	bool m_mirrored = false; // Set by cellCamera
	s32 m_format = 2; // CELL_CAMERA_RAW8, set by cellCamera
	u32 m_bytesize = 0;
	u32 m_width = 640;
	u32 m_height = 480;

	std::mutex m_mutex;
	atomic_t<u64> m_frame_number{0};
	u32 m_write_index{0};

	struct image_buffer
	{
		u64 frame_number = 0;
		u32 width = 0;
		u32 height = 0;
		std::vector<u8> data;
	};
	std::array<image_buffer, 2> m_image_buffer;
};
