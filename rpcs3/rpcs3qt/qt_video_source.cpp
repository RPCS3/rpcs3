#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Audio/audio_utils.h"
#include "qt_video_source.h"
#include "gui_settings.h"

#include "Loader/ISO.h"

#include <QAudioOutput>
#include <QPropertyAnimation>
#include <QFile>

struct qt_audio_instance
{
	static constexpr u32 gui_index = 0;
	static constexpr u32 emu_index = 1;

	video_source* source = nullptr;
	std::unique_ptr<QMediaPlayer> player;
	std::unique_ptr<QAudioOutput> output;
	std::unique_ptr<QBuffer> buffer;
	std::unique_ptr<QByteArray> data;
};

static std::array<qt_audio_instance, 2> s_audio_instance = {};

static constexpr int emu_timeout_start_ms = 0;
static constexpr int gui_timeout_start_ms = 1000;
static constexpr int gui_fade_in_ms = 2000;
static constexpr int gui_fade_out_ms = 1000;

static_assert(gui_fade_out_ms <= gui_timeout_start_ms);

qt_video_source::qt_video_source(bool is_emulation)
	: video_source()
	, m_audio_instance_index(is_emulation ? qt_audio_instance::emu_index : qt_audio_instance::gui_index)
	, m_video_timer_timeout_ms(is_emulation ? emu_timeout_start_ms : gui_timeout_start_ms)
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
		start_movie_timer();
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
			const auto movie_size = movie_file->size();
			if (movie_size == 0) return;

			m_video_data = QByteArray(movie_size, 0);
			movie_file->read(m_video_data.data(), movie_size);

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
			const auto movie_size = movie_file->size();
			if (movie_size == 0)
			{
				return;
			}

			m_video_data = QByteArray(movie_size, 0);
			movie_file->read(m_video_data.data(), movie_size);
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

void qt_video_source::start_movie_timer()
{
	if (m_video_timer_timeout_ms == 0)
	{
		start_movie();
		return;
	}

	if (!m_video_timer)
	{
		m_video_timer = std::make_unique<QTimer>();
		m_video_timer->setSingleShot(true);
		QObject::connect(m_video_timer.get(), &QTimer::timeout, m_video_timer.get(), [this]()
		{
			if (!m_active) return;
			start_movie();
		});
	}

	m_video_timer->start(m_video_timer_timeout_ms);
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
	m_video_timer.reset();

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
	if (m_audio_path.isEmpty()) return;

	qt_audio_instance& audio = ::at32(s_audio_instance, m_audio_instance_index);
	if (audio.source == this) return;

	if (!audio.player)
	{
		audio.output = std::make_unique<QAudioOutput>();
		audio.player = std::make_unique<QMediaPlayer>();
		audio.player->setAudioOutput(audio.output.get());
		audio.player->setLoops(QMediaPlayer::Infinite);
	}

	if (m_iso_path.empty())
	{
		audio.player->setSource(QUrl::fromLocalFile(m_audio_path));
	}
	else
	{
		iso_archive archive(m_iso_path);
		auto audio_file = archive.open(m_audio_path.toStdString());
		const auto audio_size = audio_file->size();
		if (audio_size == 0) return;

		std::unique_ptr<QByteArray> old_audio_data = std::move(audio.data);
		audio.data = std::make_unique<QByteArray>(audio_size, 0);
		audio_file->read(audio.data->data(), audio_size);

		if (!audio.buffer)
		{
			audio.buffer = std::make_unique<QBuffer>();
		}

		audio.buffer->setBuffer(audio.data.get());
		audio.buffer->open(QIODevice::ReadOnly);
		audio.player->setSourceDevice(audio.buffer.get());

		if (old_audio_data)
		{
			old_audio_data.reset();
		}
	}

	f32 volume = gui::volume;

	if (m_audio_instance_index == qt_audio_instance::emu_index)
	{
		volume = audio::get_volume();
	}

	QPropertyAnimation* fade_in = new QPropertyAnimation(audio.output.get(), "volume", audio.output.get());
	fade_in->setDuration(gui_fade_in_ms);
	fade_in->setStartValue(0.0);
	fade_in->setEndValue(std::clamp(volume, 0.0f, 1.0f));
	fade_in->setEasingCurve(QEasingCurve::InSine);
	fade_in->start(QAbstractAnimation::DeleteWhenStopped);

	audio.player->play();
	audio.source = this;
}

void qt_video_source::stop_audio()
{
	qt_audio_instance& audio = ::at32(s_audio_instance, m_audio_instance_index);
	if (audio.source != this) return;

	audio.source = nullptr;

	QMediaPlayer* player = audio.player.release();
	QAudioOutput* output = audio.output.release();
	QBuffer* buffer = audio.buffer.release();
	QByteArray* data = audio.data.release();

	const auto reset_player = [=]()
	{
		if (player)
		{
			player->stop();
			delete player;
		}

		if (output) delete output;
		if (buffer) delete buffer;
		if (data) delete data;
	};

	if (output)
	{
		QPropertyAnimation* fade_out = new QPropertyAnimation(output, "volume", output);
		fade_out->setDuration(gui_fade_out_ms);
		fade_out->setEasingCurve(QEasingCurve::OutSine);
		fade_out->setStartValue(output->volume());
		fade_out->setEndValue(0.0);

		QObject::connect(fade_out, &QPropertyAnimation::finished, [reset_player]()
		{
			reset_player();
		});

		fade_out->start(QAbstractAnimation::DeleteWhenStopped);
		return;
	}

	reset_player();
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

void qt_video_source_wrapper::init_video_source()
{
	if (!m_qt_video_source)
	{
		m_qt_video_source = std::make_unique<qt_video_source>(true);
	}
}

void qt_video_source_wrapper::set_video_path(const std::string& video_path)
{
	Emu.CallFromMainThread([this, path = video_path]()
	{
		init_video_source();

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
		init_video_source();

		m_qt_video_source->set_audio_path(path);
	});
}

void qt_video_source_wrapper::set_active(bool active)
{
	Emu.CallFromMainThread([this, active]()
	{
		ensure(m_qt_video_source);
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
