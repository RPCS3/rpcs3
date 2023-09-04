#include "stdafx.h"
#include "qt_camera_video_sink.h"

#include "Emu/Cell/Modules/cellCamera.h"
#include "Emu/system_config.h"

#include <QtConcurrent>

LOG_CHANNEL(camera_log, "Camera");

qt_camera_video_sink::qt_camera_video_sink(bool front_facing, QObject *parent)
	: QVideoSink(parent), m_front_facing(front_facing)
{
	connect(this, &QVideoSink::videoFrameChanged, this, &qt_camera_video_sink::present);
}

qt_camera_video_sink::~qt_camera_video_sink()
{
	std::lock_guard lock(m_mutex);

	// Free memory
	for (auto& image_buffer : m_image_buffer)
	{
		if (image_buffer.data)
		{
			delete[] image_buffer.data;
			image_buffer.data = nullptr;
		}
	}
}

bool qt_camera_video_sink::present(const QVideoFrame& frame)
{
	if (!frame.isValid())
	{
		camera_log.error("Received invalid video frame");
		return false;
	}

	// Get video image. Map frame for faster read operations.
	QVideoFrame tmp(frame);
	if (!tmp.map(QVideoFrame::ReadOnly))
	{
		camera_log.error("Failed to map video frame");
		return false;
	}

	// Get image. This usually also converts the image to ARGB32.
	QImage image = frame.toImage();

	if (image.isNull())
	{
		camera_log.warning("Image is invalid: pixel_format=%s, format=%d", tmp.pixelFormat(), static_cast<int>(QVideoFrameFormat::imageFormatFromPixelFormat(tmp.pixelFormat())));
	}
	else
	{
		// Scale image if necessary
		if (m_width > 0 && m_height > 0 && m_width != static_cast<u32>(image.width()) && m_height != static_cast<u32>(image.height()))
		{
			image = image.scaled(m_width, m_height, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::SmoothTransformation);
		}

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
		if (flip_horizontally || flip_vertically)
		{
			image = image.mirrored(flip_horizontally, flip_vertically);
		}
	}

	const u64 new_size = m_bytesize;
	image_buffer& image_buffer = m_image_buffer[m_write_index];

	// Reset buffer if necessary
	if (image_buffer.size != new_size)
	{
		image_buffer.size = 0;
		if (image_buffer.data)
		{
			delete[] image_buffer.data;
			image_buffer.data = nullptr;
		}
	}

	// Create buffer if necessary
	if (!image_buffer.data && new_size > 0)
	{
		image_buffer.data = new u8[new_size];
		image_buffer.size = new_size;
		image_buffer.width = m_width;
		image_buffer.height = m_height;
	}

	if (image_buffer.size > 0 && !image.isNull())
	{
		// Convert image to proper layout
		// TODO: check if pixel format and bytes per pixel match and convert if necessary
		// TODO: implement or improve more conversions

		switch (m_format)
		{
		case CELL_CAMERA_JPG:
			break;
		case CELL_CAMERA_RGBA:
			break;
		case CELL_CAMERA_RAW8: // The game seems to expect BGGR
		{
			// Let's use a very simple algorithm to convert the image to raw BGGR
			const auto convert_to_bggr = [&image_buffer, &image](u32 y_begin, u32 y_end)
			{
				for (u32 y = y_begin; y < std::min<u32>(image_buffer.height, image.height()) && y < y_end; y++)
				{
					for (u32 x = 0; x < std::min<u32>(image_buffer.width, image.width()); x++)
					{
						u8& pixel = image_buffer.data[image_buffer.width * y + x];
						const bool is_left_pixel = (x % 2) == 0;
						const bool is_top_pixel = (y % 2) == 0;

						if (is_left_pixel && is_top_pixel)
						{
							pixel = qBlue(image.pixel(x, y));
						}
						else if (is_left_pixel || is_top_pixel)
						{
							pixel = qGreen(image.pixel(x, y));
						}
						else
						{
							pixel = qRed(image.pixel(x, y));
						}
					}
				}
			};

			// Use a multithreaded workload. The faster we get this done, the better.
			constexpr u32 thread_count = 4;
			const u32 lines_per_thread = std::ceil(image_buffer.height / static_cast<double>(thread_count));
			u32 y_begin = 0;
			u32 y_end = lines_per_thread;

			QFutureSynchronizer<void> synchronizer;
			for (u32 i = 0; i < thread_count; i++)
			{
				synchronizer.addFuture(QtConcurrent::run(convert_to_bggr, y_begin, y_end));
				y_begin = y_end;
				y_end += lines_per_thread;
			}
			synchronizer.waitForFinished();
			break;
		}
		//case CELL_CAMERA_YUV422:
		case CELL_CAMERA_Y0_U_Y1_V:
		case CELL_CAMERA_V_Y1_U_Y0:
		{
			// Simple RGB to Y0_U_Y1_V conversion from stackoverflow.
			const auto convert_to_yuv422 = [&image_buffer, &image, format = m_format](u32 y_begin, u32 y_end)
			{
				constexpr int yuv_bytes_per_pixel = 2;
				const int yuv_pitch = image_buffer.width * yuv_bytes_per_pixel;

				const int y0_offset = (format == CELL_CAMERA_Y0_U_Y1_V) ? 0 : 3;
				const int u_offset  = (format == CELL_CAMERA_Y0_U_Y1_V) ? 1 : 2;
				const int y1_offset = (format == CELL_CAMERA_Y0_U_Y1_V) ? 2 : 1;
				const int v_offset  = (format == CELL_CAMERA_Y0_U_Y1_V) ? 3 : 0;

				for (u32 y = y_begin; y < std::min<u32>(image_buffer.height, image.height()) && y < y_end; y++)
				{
					uint8_t* yuv_row_ptr = &image_buffer.data[y * yuv_pitch];

					for (u32 x = 0; x < std::min<u32>(image_buffer.width, image.width()) - 1; x += 2)
					{
						const QRgb pixel_1 = image.pixel(x, y);
						const QRgb pixel_2 = image.pixel(x + 1, y);

						const double r1 = qRed(pixel_1);
						const double g1 = qGreen(pixel_1);
						const double b1 = qBlue(pixel_1);
						const double r2 = qRed(pixel_2);
						const double g2 = qGreen(pixel_2);
						const double b2 = qBlue(pixel_2);

						const int y0 =  (0.257 * r1) + (0.504 * g1) + (0.098 * b1) +  16.0;
						const int u  = -(0.148 * r1) - (0.291 * g1) + (0.439 * b1) + 128.0;
						const int v  =  (0.439 * r1) - (0.368 * g1) - (0.071 * b1) + 128.0;
						const int y1 =  (0.257 * r2) + (0.504 * g2) + (0.098 * b2) +  16.0;

						const int yuv_index = x * yuv_bytes_per_pixel;
						yuv_row_ptr[yuv_index + y0_offset] = std::max<u8>(0, std::min<u8>(y0, 255));
						yuv_row_ptr[yuv_index + u_offset]  = std::max<u8>(0, std::min<u8>( u, 255));
						yuv_row_ptr[yuv_index + y1_offset] = std::max<u8>(0, std::min<u8>(y1, 255));
						yuv_row_ptr[yuv_index + v_offset]  = std::max<u8>(0, std::min<u8>( v, 255));
					}
				}
			};

			// Use a multithreaded workload. The faster we get this done, the better.
			constexpr u32 thread_count = 4;
			const u32 lines_per_thread = std::ceil(image_buffer.height / static_cast<double>(thread_count));
			u32 y_begin = 0;
			u32 y_end = lines_per_thread;

			QFutureSynchronizer<void> synchronizer;
			for (u32 i = 0; i < thread_count; i++)
			{
				synchronizer.addFuture(QtConcurrent::run(convert_to_yuv422, y_begin, y_end));
				y_begin = y_end;
				y_end += lines_per_thread;
			}
			synchronizer.waitForFinished();
			break;
		}
		case CELL_CAMERA_RAW10:
		case CELL_CAMERA_YUV420:
		case CELL_CAMERA_FORMAT_UNKNOWN:
		default:
			std::memcpy(image_buffer.data, image.constBits(), std::min<usz>(image_buffer.size, image.height() * image.bytesPerLine()));
			break;
		}
	}

	// Unmap frame memory
	tmp.unmap();

	camera_log.trace("Wrote image to video surface. index=%d, m_frame_number=%d, width=%d, height=%d, bytesize=%d",
		m_write_index, m_frame_number.load(), m_width, m_height, m_bytesize);

	// Toggle write/read index
	std::lock_guard lock(m_mutex);
	image_buffer.frame_number = m_frame_number++;
	m_write_index = read_index();

	return true;
}

void qt_camera_video_sink::set_format(s32 format, u32 bytesize)
{
	camera_log.notice("Setting format: format=%d, bytesize=%d", format, bytesize);

	m_format = format;
	m_bytesize = bytesize;
}

void qt_camera_video_sink::set_resolution(u32 width, u32 height)
{
	camera_log.notice("Setting resolution: width=%d, height=%d", width, height);

	m_width = width;
	m_height = height;
}

void qt_camera_video_sink::set_mirrored(bool mirrored)
{
	camera_log.notice("Setting mirrored: mirrored=%d", mirrored);

	m_mirrored = mirrored;
}

u64 qt_camera_video_sink::frame_number() const
{
	return m_frame_number.load();
}

void qt_camera_video_sink::get_image(u8* buf, u64 size, u32& width, u32& height, u64& frame_number, u64& bytes_read)
{
	// Lock read buffer
	std::lock_guard lock(m_mutex);
	const image_buffer& image_buffer = m_image_buffer[read_index()];

	width = image_buffer.width;
	height = image_buffer.height;
	frame_number = image_buffer.frame_number;

	// Copy to out buffer
	if (buf && image_buffer.data)
	{
		bytes_read = std::min<u64>(image_buffer.size, size);
		std::memcpy(buf, image_buffer.data, bytes_read);

		if (image_buffer.size != size)
		{
			camera_log.error("Buffer size mismatch: in=%d, out=%d. Cropping to incoming size. Please contact a developer.", size, image_buffer.size);
		}
	}
	else
	{
		bytes_read = 0;
	}
}

u32 qt_camera_video_sink::read_index() const
{
	// The read buffer index cannot be the same as the write index
	return (m_write_index + 1u) % ::narrow<u32>(m_image_buffer.size());
}
