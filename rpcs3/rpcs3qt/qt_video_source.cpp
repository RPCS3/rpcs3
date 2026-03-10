#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "qt_video_source.h"

#include "Loader/ISO.h"

#include <QAudioOutput>
#include <QFile>

static video_source* s_audio_source = nullptr;
static std::unique_ptr<QMediaPlayer> s_audio_player = nullptr;
static std::unique_ptr<QAudioOutput> s_audio_output = nullptr;
static std::unique_ptr<QBuffer> s_audio_buffer = nullptr;
static std::unique_ptr<QByteArray> s_audio_data = nullptr;

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

void qt_video_source::set_audio_path(const std::string& audio_path)
{
	m_audio_path = QString::fromStdString(audio_path);
}

void qt_video_source::set_iso_path(const std::string& iso_path)
{
	m_iso_path = iso_path;
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

	if (!m_image_change_callback || m_video_path.isEmpty() || (m_iso_path.empty() && !QFile::exists(m_video_path)))
	{
		m_video_path.clear();
		return;
	}

	const QString lower = m_video_path.toLower();

	if (lower.endsWith(".gif"))
	{
		if (m_iso_path.empty())
		{
			m_movie = std::make_unique<QMovie>(m_video_path);
		}
		else
		{
			iso_archive archive(m_iso_path);
			auto movie_file = archive.open(m_video_path.toStdString());
			const auto movie_size = movie_file.size();
			if (movie_size == 0) return;

			m_video_data = QByteArray(movie_size, 0);
			movie_file.read(m_video_data.data(), movie_size);

			m_video_buffer = std::make_unique<QBuffer>(&m_video_data);
			m_video_buffer->open(QIODevice::ReadOnly);
			m_movie = std::make_unique<QMovie>(m_video_buffer.get());
		}

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
		if (m_iso_path.empty())
		{
			QFile file(m_video_path);
			if (!file.open(QFile::OpenModeFlag::ReadOnly))
			{
				return;
			}

			// TODO: Decode the pam properly before pushing it to the player
			m_video_data = file.readAll();
		}
		else
		{
			iso_archive archive(m_iso_path);
			auto movie_file = archive.open(m_video_path.toStdString());
			const auto movie_size = movie_file.size();
			if (movie_size == 0)
			{
				return;
			}

			m_video_data = QByteArray(movie_size, 0);
			movie_file.read(m_video_data.data(), movie_size);
		}

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

	start_audio();

	m_active = true;
}

void qt_video_source::stop_movie()
{
	m_active = false;

	if (m_movie)
	{
		m_movie->stop();
		m_movie.reset();
	}

	m_video_sink.reset();
	m_media_player.reset();
	m_video_buffer.reset();
	m_video_data.clear();

	stop_audio();
}

void qt_video_source::start_audio()
{
	if (m_audio_path.isEmpty() || s_audio_source == this) return;

	if (!s_audio_player)
	{
		s_audio_output = std::make_unique<QAudioOutput>();
		s_audio_player = std::make_unique<QMediaPlayer>();
		s_audio_player->setAudioOutput(s_audio_output.get());
	}

	if (m_iso_path.empty())
	{
		s_audio_player->setSource(QUrl::fromLocalFile(m_audio_path));
	}
	else
	{
		iso_archive archive(m_iso_path);
		auto audio_file = archive.open(m_audio_path.toStdString());
		const auto audio_size = audio_file.size();
		if (audio_size == 0) return;

		std::unique_ptr<QByteArray> old_audio_data = std::move(s_audio_data);
		s_audio_data = std::make_unique<QByteArray>(audio_size, 0);
		audio_file.read(s_audio_data->data(), audio_size);

		if (!s_audio_buffer)
		{
			s_audio_buffer = std::make_unique<QBuffer>();
		}

		s_audio_buffer->setBuffer(s_audio_data.get());
		s_audio_buffer->open(QIODevice::ReadOnly);
		s_audio_player->setSourceDevice(s_audio_buffer.get());

		if (old_audio_data)
		{
			old_audio_data.reset();
		}
	}

	s_audio_output->setVolume(g_cfg.audio.volume.get() / 100.0f);
	s_audio_player->play();
	s_audio_source = this;
}

void qt_video_source::stop_audio()
{
	if (s_audio_source != this) return;

	s_audio_source = nullptr;

	if (s_audio_player)
	{
		s_audio_player->stop();
		s_audio_player.reset();
	}

	s_audio_output.reset();
	s_audio_buffer.reset();
	s_audio_data.reset();
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

void qt_video_source_wrapper::set_audio_path(const std::string& audio_path)
{
	Emu.CallFromMainThread([this, path = audio_path]()
	{
		// TODO
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
