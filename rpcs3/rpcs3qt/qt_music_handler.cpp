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

	m_error_handler = std::make_unique<qt_music_error_handler>(m_media_player,
		[this](QMediaPlayer::MediaStatus status)
		{
			if (!m_status_callback)
			{
				return;
			}

			switch (status)
			{
			case QMediaPlayer::MediaStatus::UnknownMediaStatus:
			case QMediaPlayer::MediaStatus::NoMedia:
			case QMediaPlayer::MediaStatus::LoadingMedia:
			case QMediaPlayer::MediaStatus::LoadedMedia:
			case QMediaPlayer::MediaStatus::StalledMedia:
			case QMediaPlayer::MediaStatus::BufferingMedia:
			case QMediaPlayer::MediaStatus::BufferedMedia:
			case QMediaPlayer::MediaStatus::InvalidMedia:
				break;
			case QMediaPlayer::MediaStatus::EndOfMedia:
				m_status_callback(player_status::end_of_media);
				break;
			default:
				music_log.error("Ignoring unknown status %d", static_cast<int>(status));
				break;
			}
		});
}

qt_music_handler::~qt_music_handler()
{
	Emu.BlockingCallFromMainThread([this]()
	{
		music_log.notice("Destroying Qt music handler...");
		m_media_player->stop();
		m_media_player.reset();
		m_error_handler.reset();
	});
}

void qt_music_handler::stop()
{
	std::lock_guard lock(m_mutex);

	Emu.BlockingCallFromMainThread([this]()
	{
		music_log.notice("Stopping music...");
		m_media_player->stop();
	});

	m_state = CELL_MUSIC_PB_STATUS_STOP;
}

void qt_music_handler::pause()
{
	std::lock_guard lock(m_mutex);

	Emu.BlockingCallFromMainThread([this]()
	{
		music_log.notice("Pausing music...");
		m_media_player->pause();
	});

	m_state = CELL_MUSIC_PB_STATUS_PAUSE;
}

void qt_music_handler::play(const std::string& path)
{
	std::lock_guard lock(m_mutex);

	Emu.BlockingCallFromMainThread([&path, this]()
	{
		if (m_path != path)
		{
			m_path = path;
			m_media_player->setMedia(QUrl(QString::fromStdString(path)));
		}

		music_log.notice("Playing music: %s", path);
		m_media_player->setPlaybackRate(1.0);
		m_media_player->play();
	});

	m_state = CELL_MUSIC_PB_STATUS_PLAY;
}

void qt_music_handler::fast_forward(const std::string& path)
{
	std::lock_guard lock(m_mutex);

	Emu.BlockingCallFromMainThread([&path, this]()
	{
		if (m_path != path)
		{
			m_path = path;
			m_media_player->setMedia(QUrl(QString::fromStdString(path)));
		}

		music_log.notice("Fast-forwarding music...");
		m_media_player->setPlaybackRate(2.0);
		m_media_player->play();
	});

	m_state = CELL_MUSIC_PB_STATUS_FASTFORWARD;
}

void qt_music_handler::fast_reverse(const std::string& path)
{
	std::lock_guard lock(m_mutex);

	Emu.BlockingCallFromMainThread([&path, this]()
	{
		if (m_path != path)
		{
			m_path = path;
			m_media_player->setMedia(QUrl(QString::fromStdString(path)));
		}

		music_log.notice("Fast-reversing music...");
		m_media_player->setPlaybackRate(-2.0);
		m_media_player->play();
	});

	m_state = CELL_MUSIC_PB_STATUS_FASTREVERSE;
}

void qt_music_handler::set_volume(f32 volume)
{
	std::lock_guard lock(m_mutex);

	Emu.BlockingCallFromMainThread([&volume, this]()
	{
		const int new_volume = std::max<int>(0, std::min<int>(volume * 100, 100));
		music_log.notice("Setting volume to %d%%", new_volume);
		m_media_player->setVolume(new_volume);
	});
}

f32 qt_music_handler::get_volume() const
{
	std::lock_guard lock(m_mutex);
	f32 volume = 0.0f;

	Emu.BlockingCallFromMainThread([&volume, this]()
	{
		const int current_volume = std::max(0, std::min(m_media_player->volume(), 100));
		music_log.notice("Getting volume: %d%%", current_volume);
		volume = current_volume / 100.0f;
	});

	return volume;
}
