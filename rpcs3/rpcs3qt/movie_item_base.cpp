#include "stdafx.h"
#include "movie_item_base.h"

#include <QFile>

movie_item_base::movie_item_base()
{
	init_pointers();
}

movie_item_base::~movie_item_base()
{
	if (m_movie)
	{
		m_movie->stop();
	}

	if (m_media_player)
	{
		m_media_player->stop();
	}

	wait_for_icon_loading(true);
	wait_for_size_on_disk_loading(true);
}

void movie_item_base::init_pointers()
{
	m_icon_loading_aborted.reset(new atomic_t<bool>(false));
	m_size_on_disk_loading_aborted.reset(new atomic_t<bool>(false));
}

void movie_item_base::set_active(bool active)
{
	if (!std::exchange(m_active, active) && active)
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
	}
}

void movie_item_base::init_movie()
{
	if (m_movie || m_media_player)
	{
		// Already initialized
		return;
	}

	if (!m_icon_callback || m_movie_path.isEmpty() || !QFile::exists(m_movie_path))
	{
		m_movie_path.clear();
		return;
	}

	const QString lower = m_movie_path.toLower();

	if (lower.endsWith(".gif"))
	{
		m_movie.reset(new QMovie(m_movie_path));
		m_movie_path.clear();

		if (!m_movie->isValid())
		{
			m_movie.reset();
			return;
		}

		QObject::connect(m_movie.get(), &QMovie::frameChanged, m_movie.get(), [this](int)
		{
			m_icon_callback({});
		});
		return;
	}

	if (lower.endsWith(".pam"))
	{
		// We can't set PAM files as source of the video player, so we have to feed them as raw data.
		QFile file(m_movie_path);
		if (!file.open(QFile::OpenModeFlag::ReadOnly))
		{
			return;
		}

		// TODO: Decode the pam properly before pushing it to the player
		m_movie_data = file.readAll();
		if (m_movie_data.isEmpty())
		{
			return;
		}

		m_movie_buffer.reset(new QBuffer(&m_movie_data));
		m_movie_buffer->open(QIODevice::ReadOnly);
	}

	m_video_sink.reset(new QVideoSink());
	QObject::connect(m_video_sink.get(), &QVideoSink::videoFrameChanged, m_video_sink.get(), [this](const QVideoFrame& frame)
	{
		m_icon_callback(frame);
	});

	m_media_player.reset(new QMediaPlayer());
	m_media_player->setVideoSink(m_video_sink.get());
	m_media_player->setLoops(QMediaPlayer::Infinite);

	if (m_movie_buffer)
	{
		m_media_player->setSourceDevice(m_movie_buffer.get());
	}
	else
	{
		m_media_player->setSource(m_movie_path);
	}
}

void movie_item_base::stop_movie()
{
	if (m_movie)
	{
		m_movie->stop();
	}

	m_video_sink.reset();
	m_media_player.reset();
	m_movie_buffer.reset();
	m_movie_data.clear();
}

QPixmap movie_item_base::get_movie_image(const QVideoFrame& frame) const
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

void movie_item_base::call_icon_func() const
{
	if (m_icon_callback)
	{
		m_icon_callback({});
	}
}

void movie_item_base::set_icon_func(const icon_callback_t& func)
{
	m_icon_callback = func;
}

void movie_item_base::call_icon_load_func(int index)
{
	if (!m_icon_load_callback || m_icon_loading || m_icon_loading_aborted->load())
	{
		return;
	}

	wait_for_icon_loading(true);

	*m_icon_loading_aborted = false;
	m_icon_loading = true;
	m_icon_load_thread.reset(QThread::create([this, index]()
	{
		if (m_icon_load_callback)
		{
			m_icon_load_callback(index);
		}
	}));
	m_icon_load_thread->start();
}

void movie_item_base::set_icon_load_func(const icon_load_callback_t& func)
{
	wait_for_icon_loading(true);

	m_icon_loading = false;
	m_icon_load_callback = func;
	*m_icon_loading_aborted = false;
}

void movie_item_base::call_size_calc_func()
{
	if (!m_size_calc_callback || m_size_on_disk_loading || m_size_on_disk_loading_aborted->load())
	{
		return;
	}

	wait_for_size_on_disk_loading(true);

	*m_size_on_disk_loading_aborted = false;
	m_size_on_disk_loading = true;
	m_size_calc_thread.reset(QThread::create([this]()
	{
		if (m_size_calc_callback)
		{
			m_size_calc_callback();
		}
	}));
	m_size_calc_thread->start();
}

void movie_item_base::set_size_calc_func(const size_calc_callback_t& func)
{
	m_size_on_disk_loading = false;
	m_size_calc_callback = func;
	*m_size_on_disk_loading_aborted = false;
}

void movie_item_base::wait_for_icon_loading(bool abort)
{
	*m_icon_loading_aborted = abort;

	if (m_icon_load_thread && m_icon_load_thread->isRunning())
	{
		m_icon_load_thread->wait();
		m_icon_load_thread.reset();
	}
}

void movie_item_base::wait_for_size_on_disk_loading(bool abort)
{
	*m_size_on_disk_loading_aborted = abort;

	if (m_size_calc_thread && m_size_calc_thread->isRunning())
	{
		m_size_calc_thread->wait();
		m_size_calc_thread.reset();
	}
}
