#include "stdafx.h"
#include "qt_camera_video_surface.h"

#include "Emu/Cell/Modules/cellCamera.h"
#include "Emu/system_config.h"

#include <QtConcurrent>

LOG_CHANNEL(camera_log, "Camera");

qt_camera_video_surface::qt_camera_video_surface(bool front_facing, QObject *parent)
	: QAbstractVideoSurface(parent), m_front_facing(front_facing)
{
}

qt_camera_video_surface::~qt_camera_video_surface()
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

QList<QVideoFrame::PixelFormat> qt_camera_video_surface::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const
{
	Q_UNUSED(type)

	// Let's only allow RGB formats for now
	QList<QVideoFrame::PixelFormat> result;
	result
		<< QVideoFrame::Format_RGB32
		<< QVideoFrame::Format_RGB24;
	return result;
}

bool qt_camera_video_surface::present(const QVideoFrame& frame)
{
	if (!frame.isValid())
	{
		camera_log.error("Received invalid video frame");
		return false;
	}

	// Get video image. Map frame for faster read operations.
	QVideoFrame tmp(frame);
	if (!tmp.map(QAbstractVideoBuffer::ReadOnly))
	{
		camera_log.error("Failed to map video frame");
		return false;
	}

	// Create shallow copy
	QImage image(tmp.bits(), tmp.width(), tmp.height(), tmp.bytesPerLine(), QVideoFrame::imageFormatFromPixelFormat(tmp.pixelFormat()));

	if (!image.isNull())
	{
		// Scale image if necessary
		if (m_width > 0 && m_height > 0 && m_width != image.width() && m_height != image.height())
		{
			image = image.scaled(m_width, m_height, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::SmoothTransformation);
		}

		// Determine image flip
		const camera_flip flip_setting = g_cfg.io.camera_flip;

		bool flip_horizontally = m_front_facing; // Front facing cameras are flipped already
		if (flip_setting == camera_flip::horizontal || flip_setting == camera_flip::both)
		{
			flip_horizontally = !flip_horizontally;
		}
		if (m_mirrored) // Set by the game
		{
			flip_horizontally = !flip_horizontally;
		}

		bool flip_vertically = true; // It appears games expect this. Might be camera specific.
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

	const u64 new_size = m_width * m_height * m_bytes_per_pixel;
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
			const auto convert_to_bggr = [&image_buffer, &image](int y_begin, int y_end)
			{
				for (int y = y_begin; y < std::min<int>(image_buffer.height, image.height()) && y < y_end; y++)
				{
					for (int x = 0; x < std::min<int>(image_buffer.width, image.width()); x++)
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
			const int lines_per_thread = std::ceil(image_buffer.height / static_cast<double>(thread_count));
			int y_begin = 0;
			int y_end = lines_per_thread;

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
		//case CELL_CAMERA_Y0_U_Y1_V:
		case CELL_CAMERA_YUV422:
		{
			// Simple conversion from stackoverflow.
			const int rgb_bytes_per_pixel = 4;
			const int yuv_bytes_per_pixel = 2;
			const int yuv_pitch = image_buffer.width * yuv_bytes_per_pixel;

			for (u32 y = 0; y < image_buffer.height; y++)
			{
				const uint8_t* rgb_row_ptr = image.constScanLine(y);
				uint8_t* yuv_row_ptr       = &image_buffer.data[y * yuv_pitch];

				for (u32 x = 0; x < image_buffer.width - 1; x += 2)
				{
					const int rgb_index = x * rgb_bytes_per_pixel;
					const int yuv_index = x * yuv_bytes_per_pixel;

					const uint8_t R1 =  rgb_row_ptr[rgb_index + 0];
					const uint8_t G1 =  rgb_row_ptr[rgb_index + 1];
					const uint8_t B1 =  rgb_row_ptr[rgb_index + 2];
					//const uint8_t A1 = rgb_row_ptr[rgb_index + 3];
					const uint8_t R2 = rgb_row_ptr[rgb_index + 4];
					const uint8_t G2 = rgb_row_ptr[rgb_index + 5];
					const uint8_t B2 = rgb_row_ptr[rgb_index + 6];
					//const uint8_t A2 = rgb_row_ptr[rgb_index + 7];

					const int Y  = (0.257f * R1) + (0.504f * G1) + (0.098f * B1) + 16.0f;
					const int U  = -(0.148f * R1) - (0.291f * G1) + (0.439f * B1) + 128.0f;
					const int V  = (0.439f * R1) - (0.368f * G1) - (0.071f * B1) + 128.0f;
					const int Y2 = (0.257f * R2) + (0.504f * G2) + (0.098f * B2) + 16.0f;

					yuv_row_ptr[yuv_index + 0] = std::max<u8>(0, std::min<u8>(Y, 255));
					yuv_row_ptr[yuv_index + 1] = std::max<u8>(0, std::min<u8>(U, 255));
					yuv_row_ptr[yuv_index + 2] = std::max<u8>(0, std::min<u8>(Y2, 255));
					yuv_row_ptr[yuv_index + 3] = std::max<u8>(0, std::min<u8>(V, 255));
				}
			}
			break;
		}
		case CELL_CAMERA_RAW10:
		case CELL_CAMERA_YUV420:
		case CELL_CAMERA_V_Y1_U_Y0:
		case CELL_CAMERA_FORMAT_UNKNOWN:
		default:
			std::memcpy(image_buffer.data, image.constBits(), std::min<usz>(image_buffer.size, image.height() * image.bytesPerLine()));
			break;
		}
	}

	// Unmap frame memory
	tmp.unmap();

	camera_log.trace("Wrote image to video surface. index=%d, m_frame_number=%d, width=%d, height=%d, bytes_per_pixel=%d",
		m_write_index, m_frame_number.load(), m_width, m_height, m_bytes_per_pixel);

	// Toggle write/read index
	std::lock_guard lock(m_mutex);
	image_buffer.frame_number = m_frame_number++;
	m_write_index = read_index();

	return true;
}

void qt_camera_video_surface::set_format(s32 format, u32 bytes_per_pixel)
{
	camera_log.notice("Setting format: format=%d, bytes_per_pixel=%d", format, bytes_per_pixel);

	m_format = format;
	m_bytes_per_pixel = bytes_per_pixel;
}

void qt_camera_video_surface::set_resolution(u32 width, u32 height)
{
	camera_log.notice("Setting resolution: width=%d, height=%d", width, height);

	m_width = width;
	m_height = height;
}

void qt_camera_video_surface::set_mirrored(bool mirrored)
{
	camera_log.notice("Setting mirrored: mirrored=%d", mirrored);

	m_mirrored = mirrored;
}

u64 qt_camera_video_surface::frame_number() const
{
	return m_frame_number.load();
}

void qt_camera_video_surface::get_image(u8* buf, u64 size, u32& width, u32& height, u64& frame_number, u64& bytes_read)
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

u32 qt_camera_video_surface::read_index() const
{
	// The read buffer index cannot be the same as the write index
	return (m_write_index + 1u) % ::narrow<u32>(m_image_buffer.size());
}
