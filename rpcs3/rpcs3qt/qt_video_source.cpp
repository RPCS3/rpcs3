#include "stdafx.h"
#include "Emu/System.h"
#include "qt_video_source.h"

#include <QFile>

qt_video_source::qt_video_source()
	: video_source()
{
}

qt_video_source::~qt_video_source()
{
	stop_movie();
}

void qt_video_source::set_video_path(const std::string& video_path)
{
	m_video_path = QString::fromStdString(video_path);
}

void qt_video_source::set_active(bool active)
{
	if (m_active.exchange(active) == active) return;

	if (active)
	{
		start_movie();
	}
	else
	{
		stop_movie();
		image_change_callback();
	}
}

void qt_video_source::image_change_callback(const QVideoFrame& frame) const
{
	if (m_image_change_callback)
	{
		m_image_change_callback(frame);
	}
}

void qt_video_source::set_image_change_callback(const std::function<void(const QVideoFrame&)>& func)
{
	m_image_change_callback = func;
}

void qt_video_source::init_movie()
{
	if (m_movie || m_media_player)
	{
		// Already initialized
		return;
	}

	if (!m_image_change_callback || m_video_path.isEmpty() || !QFile::exists(m_video_path))
	{
		m_video_path.clear();
		return;
	}

	const QString lower = m_video_path.toLower();

	if (lower.endsWith(".gif"))
	{
		m_movie = std::make_unique<QMovie>(m_video_path);
		m_video_path.clear();

		if (!m_movie->isValid())
		{
			m_movie.reset();
			return;
		}

		QObject::connect(m_movie.get(), &QMovie::frameChanged, m_movie.get(), [this](int)
		{
			image_change_callback();
			m_has_new = true;
		});
		return;
	}

	if (lower.endsWith(".pam"))
	{
		// We can't set PAM files as source of the video player, so we have to feed them as raw data.
		QFile file(m_video_path);
		if (!file.open(QFile::OpenModeFlag::ReadOnly))
		{
			return;
		}

		// TODO: Decode the pam properly before pushing it to the player
		m_video_data = file.readAll();
		if (m_video_data.isEmpty())
		{
			return;
		}

		m_video_buffer = std::make_unique<QBuffer>(&m_video_data);
		m_video_buffer->open(QIODevice::ReadOnly);
	}

	m_video_sink = std::make_unique<QVideoSink>();
	QObject::connect(m_video_sink.get(), &QVideoSink::videoFrameChanged, m_video_sink.get(), [this](const QVideoFrame& frame)
	{
		image_change_callback(frame);
		m_has_new = true;
	});

	m_media_player = std::make_unique<QMediaPlayer>();
	m_media_player->setVideoSink(m_video_sink.get());
	m_media_player->setLoops(QMediaPlayer::Infinite);

	if (m_video_buffer)
	{
		m_media_player->setSourceDevice(m_video_buffer.get());
	}
	else
	{
		m_media_player->setSource(m_video_path);
	}
}

void qt_video_source::start_movie()
{
	init_movie();

	if (m_movie)
	{
		m_movie->jumpToFrame(1);
		m_movie->start();
	}

	if (m_media_player)
	{
		m_media_player->play();
	}

	m_active = true;
}

void qt_video_source::stop_movie()
{
	m_active = false;

	if (m_movie)
	{
		m_movie->stop();
	}

	m_video_sink.reset();
	m_media_player.reset();
	m_video_buffer.reset();
	m_video_data.clear();
}

QPixmap qt_video_source::get_movie_image(const QVideoFrame& frame) const
{
	if (!m_active)
	{
		return {};
	}

	if (m_movie)
	{
		return m_movie->currentPixmap();
	}

	if (!frame.isValid())
	{
		return {};
	}

	// Get image. This usually also converts the image to ARGB32.
	return QPixmap::fromImage(frame.toImage());
}

void qt_video_source::get_image(std::vector<u8>& data, int& w, int& h, int& ch, int& bpp)
{
	if (!m_has_new.exchange(false))
	{
		return;
	}

	std::lock_guard lock(m_image_mutex);

	if (m_image.isNull())
	{
		w = h = ch = bpp = 0;
		data.clear();
		return;
	}

	w = m_image.width();
	h = m_image.height();
	ch = m_image.colorCount();
	bpp = m_image.depth();

	data.resize(m_image.height() * m_image.bytesPerLine());
	std::memcpy(data.data(), m_image.constBits(), data.size());
}

qt_video_source_wrapper::~qt_video_source_wrapper()
{
	Emu.BlockingCallFromMainThread([this]()
	{
		m_qt_video_source.reset();
	});
}

void qt_video_source_wrapper::set_video_path(const std::string& video_path)
{
	Emu.CallFromMainThread([this, path = video_path]()
	{
		m_qt_video_source = std::make_unique<qt_video_source>();
		m_qt_video_source->m_image_change_callback = [this](const QVideoFrame& frame)
		{
			std::unique_lock lock(m_qt_video_source->m_image_mutex);

			if (m_qt_video_source->m_movie)
			{
				m_qt_video_source->m_image = m_qt_video_source->m_movie->currentImage();
			}
			else if (frame.isValid())
			{
				// Get image. This usually also converts the image to ARGB32.
				m_qt_video_source->m_image = frame.toImage();
			}
			else
			{
				return;
			}

			if (m_qt_video_source->m_image.format() != QImage::Format_RGBA8888)
			{
				m_qt_video_source->m_image.convertTo(QImage::Format_RGBA8888);
			}

			lock.unlock();

			notify_update();
		};
		m_qt_video_source->set_video_path(path);
	});
}

void qt_video_source_wrapper::set_active(bool active)
{
	Emu.CallFromMainThread([this, active]()
	{
		m_qt_video_source->set_active(true);
	});
}

bool qt_video_source_wrapper::get_active() const
{
	ensure(m_qt_video_source);

	return m_qt_video_source->get_active();
}

void qt_video_source_wrapper::get_image(std::vector<u8>& data, int& w, int& h, int& ch, int& bpp)
{
	ensure(m_qt_video_source);

	m_qt_video_source->get_image(data, w, h, ch, bpp);
}
