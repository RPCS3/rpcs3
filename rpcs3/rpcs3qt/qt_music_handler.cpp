#include "qt_music_handler.h"
#include "Emu/Cell/Modules/cellMusic.h"
#include "Emu/System.h"
#include "Utilities/Thread.h"
#include "util/logs.hpp"

#include <QUrl>

LOG_CHANNEL(music_log, "Music");

qt_music_handler::qt_music_handler()
{
	music_log.notice("Constructing Qt music handler...");

	m_media_player = std::make_shared<QMediaPlayer>();
	m_media_player->setAudioRole(QAudio::Role::MusicRole);

	m_error_handler = std::make_unique<qt_music_error_handler>(m_media_player);
}

qt_music_handler::~qt_music_handler()
{
	atomic_t<bool> wake_up = false;

	Emu.CallFromMainThread([&wake_up, this]()
	{
		music_log.notice("Destroying Qt music handler...");
		m_media_player->stop();
		m_media_player.reset();
		m_error_handler.reset();
	});

	while (!wake_up && !Emu.IsStopped())
	{
		thread_ctrl::wait_on(wake_up, false);
	}
}

void qt_music_handler::play(const std::string& path)
{
	std::lock_guard lock(m_mutex);
	atomic_t<bool> wake_up = false;

	Emu.CallFromMainThread([&wake_up, &path, this]()
	{
		if (m_state == CELL_MUSIC_PB_STATUS_PAUSE)
		{
			music_log.notice("Resuming music: %s", path);
		}
		else
		{
			music_log.notice("Playing music: %s", path);
			m_media_player->setPlaybackRate(1.0);
			m_media_player->setMedia(QUrl(QString::fromStdString(path)));
		}

		m_media_player->play();
		wake_up = true;
		wake_up.notify_one();
	});

	while (!wake_up && !Emu.IsStopped())
	{
		thread_ctrl::wait_on(wake_up, false);
	}

	m_state = CELL_MUSIC_PB_STATUS_PLAY;
}

void qt_music_handler::stop()
{
	std::lock_guard lock(m_mutex);
	atomic_t<bool> wake_up = false;

	Emu.CallFromMainThread([&wake_up, this]()
	{
		music_log.notice("Stopping music...");
		m_media_player->stop();
		wake_up = true;
		wake_up.notify_one();
	});

	while (!wake_up && !Emu.IsStopped())
	{
		thread_ctrl::wait_on(wake_up, false);
	}

	m_state = CELL_MUSIC_PB_STATUS_STOP;
}

void qt_music_handler::pause()
{
	std::lock_guard lock(m_mutex);
	atomic_t<bool> wake_up = false;

	Emu.CallFromMainThread([&wake_up, this]()
	{
		music_log.notice("Pausing music...");
		m_media_player->pause();
		wake_up = true;
		wake_up.notify_one();
	});

	while (!wake_up && !Emu.IsStopped())
	{
		thread_ctrl::wait_on(wake_up, false);
	}

	m_state = CELL_MUSIC_PB_STATUS_PAUSE;
}

void qt_music_handler::fast_forward()
{
	std::lock_guard lock(m_mutex);
	atomic_t<bool> wake_up = false;

	Emu.CallFromMainThread([&wake_up, this]()
	{
		music_log.notice("Fast-forwarding music...");
		m_media_player->setPlaybackRate(2.0);
		wake_up = true;
		wake_up.notify_one();
	});

	while (!wake_up && !Emu.IsStopped())
	{
		thread_ctrl::wait_on(wake_up, false);
	}

	m_state = CELL_MUSIC_PB_STATUS_FASTFORWARD;
}

void qt_music_handler::fast_reverse()
{
	std::lock_guard lock(m_mutex);
	atomic_t<bool> wake_up = false;

	Emu.CallFromMainThread([&wake_up, this]()
	{
		music_log.notice("Fast-reversing music...");
		m_media_player->setPlaybackRate(-2.0);
		wake_up = true;
		wake_up.notify_one();
	});

	while (!wake_up && !Emu.IsStopped())
	{
		thread_ctrl::wait_on(wake_up, false);
	}

	m_state = CELL_MUSIC_PB_STATUS_FASTREVERSE;
}

void qt_music_handler::set_volume(f32 volume)
{
	std::lock_guard lock(m_mutex);
	atomic_t<bool> wake_up = false;

	Emu.CallFromMainThread([&wake_up, &volume, this]()
	{
		const int new_volume = std::max<int>(0, std::min<int>(volume * 100, 100));
		music_log.notice("Setting volume to %d%%", new_volume);
		m_media_player->setVolume(new_volume);
		wake_up = true;
		wake_up.notify_one();
	});

	while (!wake_up && !Emu.IsStopped())
	{
		thread_ctrl::wait_on(wake_up, false);
	}
}

f32 qt_music_handler::get_volume() const
{
	std::lock_guard lock(m_mutex);
	atomic_t<bool> wake_up = false;
	f32 volume = 0.0f;

	Emu.CallFromMainThread([&wake_up, &volume, this]()
	{
		const int current_volume = std::max(0, std::min(m_media_player->volume(), 100));
		music_log.notice("Getting volume: %d%%", current_volume);
		volume = current_volume / 100.0f;
		wake_up = true;
		wake_up.notify_one();
	});

	while (!wake_up && !Emu.IsStopped())
	{
		thread_ctrl::wait_on(wake_up, false);
	}

	return volume;
}
