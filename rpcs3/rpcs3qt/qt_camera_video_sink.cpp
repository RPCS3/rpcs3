#include "stdafx.h"
#include "qt_camera_video_sink.h"

#include "Emu/system_config.h"

#include <QtConcurrent>

LOG_CHANNEL(camera_log, "Camera");

qt_camera_video_sink::qt_camera_video_sink(bool front_facing, QObject *parent)
	: camera_video_sink(front_facing), QVideoSink(parent)
{
	connect(this, &QVideoSink::videoFrameChanged, this, &qt_camera_video_sink::present);
}

qt_camera_video_sink::~qt_camera_video_sink()
{
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
	QImage image = tmp.toImage();
	u32 width = image.isNull() ? 0 : static_cast<u32>(image.width());
	u32 height = image.isNull() ? 0 : static_cast<u32>(image.height());

	if (image.isNull())
	{
		camera_log.warning("Image is invalid: pixel_format=%s, format=%d", tmp.pixelFormat(), static_cast<s32>(QVideoFrameFormat::imageFormatFromPixelFormat(tmp.pixelFormat())));
	}
	else
	{
		// Scale image if necessary
		if (m_width > 0 && m_height > 0 && m_width != width && m_height != height)
		{
			image = image.scaled(m_width, m_height, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::SmoothTransformation);
			width = static_cast<u32>(image.width());
			height = static_cast<u32>(image.height());
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
			Qt::Orientations orientation{};
			orientation.setFlag(Qt::Orientation::Horizontal, flip_horizontally);
			orientation.setFlag(Qt::Orientation::Vertical, flip_vertically);
			image.flip(orientation);
#else
			image.mirror(flip_horizontally, flip_vertically);
#endif
		}

		if (image.format() != QImage::Format_RGBA8888)
		{
			image.convertTo(QImage::Format_RGBA8888);
		}
	}

	camera_video_sink::present(width, height, image.bytesPerLine(), 4, [&image](u32 y){ return image.constScanLine(y); });

	// Unmap frame memory
	tmp.unmap();

	return true;
}
