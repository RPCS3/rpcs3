#include "qt_music_handler.h"
#include "Emu/Cell/Modules/cellMusic.h"
#include "Emu/System.h"
#include "util/logs.hpp"
#include "Utilities/File.h"

#include <QAudioOutput>
#include <QUrl>

LOG_CHANNEL(music_log, "Music");

template <>
void fmt_class_string<QMediaPlayer::Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](QMediaPlayer::Error value)
	{
		switch (value)
		{
		case QMediaPlayer::Error::NoError: return "NoError";
		case QMediaPlayer::Error::ResourceError: return "ResourceError";
		case QMediaPlayer::Error::FormatError: return "FormatError";
		case QMediaPlayer::Error::NetworkError: return "NetworkError";
		case QMediaPlayer::Error::AccessDeniedError: return "AccessDeniedError";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<QMediaPlayer::MediaStatus>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](QMediaPlayer::MediaStatus value)
	{
		switch (value)
		{
		case QMediaPlayer::MediaStatus::NoMedia: return "NoMedia";
		case QMediaPlayer::MediaStatus::LoadingMedia: return "LoadingMedia";
		case QMediaPlayer::MediaStatus::LoadedMedia: return "LoadedMedia";
		case QMediaPlayer::MediaStatus::StalledMedia: return "StalledMedia";
		case QMediaPlayer::MediaStatus::BufferingMedia: return "BufferingMedia";
		case QMediaPlayer::MediaStatus::BufferedMedia: return "BufferedMedia";
		case QMediaPlayer::MediaStatus::EndOfMedia: return "EndOfMedia";
		case QMediaPlayer::MediaStatus::InvalidMedia: return "InvalidMedia";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<QMediaPlayer::PlaybackState>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](QMediaPlayer::PlaybackState value)
	{
		switch (value)
		{
		case QMediaPlayer::PlaybackState::StoppedState: return "StoppedState";
		case QMediaPlayer::PlaybackState::PlayingState: return "PlayingState";
		case QMediaPlayer::PlaybackState::PausedState: return "PausedState";
		}

		return unknown;
	});
}

qt_music_handler::qt_music_handler()
{
	music_log.notice("Constructing Qt music handler...");

	m_media_player = std::make_unique<QMediaPlayer>();
	m_media_player->setAudioOutput(new QAudioOutput(m_media_player.get()));

	connect(m_media_player.get(), &QMediaPlayer::mediaStatusChanged, this, &qt_music_handler::handle_media_status);
	connect(m_media_player.get(), &QMediaPlayer::playbackStateChanged, this, &qt_music_handler::handle_music_state);
	connect(m_media_player.get(), &QMediaPlayer::errorOccurred, this, &qt_music_handler::handle_music_error);
	connect(m_media_player->audioOutput(), &QAudioOutput::volumeChanged, this, &qt_music_handler::handle_volume_change);
}

qt_music_handler::~qt_music_handler()
{
	Emu.BlockingCallFromMainThread([this]()
	{
		music_log.notice("Destroying Qt music handler...");
		m_media_player->stop();
		m_media_player.reset();
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
		if (!fs::is_file(path))
		{
			music_log.error("play: File does not exist: '%s'", path);
		}

		if (m_path != path)
		{
			m_path = path;
			m_media_player->setSource(QUrl::fromLocalFile(QString::fromStdString(path)));
		}

		music_log.notice("Playing music: '%s'", path);
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
		if (!fs::is_file(path))
		{
			music_log.error("fast_forward: File does not exist: '%s'", path);
		}

		if (m_path != path)
		{
			m_path = path;
			m_media_player->setSource(QUrl::fromLocalFile(QString::fromStdString(path)));
		}

		music_log.notice("Fast-forwarding music: '%s'", path);
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
		if (!fs::is_file(path))
		{
			music_log.error("fast_reverse: File does not exist: '%s'", path);
		}

		if (m_path != path)
		{
			m_path = path;
			m_media_player->setSource(QUrl::fromLocalFile(QString::fromStdString(path)));
		}

		music_log.notice("Fast-reversing music: '%s'", path);
		m_media_player->setPlaybackRate(-2.0); // NOTE: This doesn't work on the current Qt version
		m_media_player->play();
	});

	m_state = CELL_MUSIC_PB_STATUS_FASTREVERSE;
}

void qt_music_handler::set_volume(f32 volume)
{
	std::lock_guard lock(m_mutex);

	Emu.BlockingCallFromMainThread([&volume, this]()
	{
		const f32 new_volume = std::clamp(volume, 0.0f, 1.0f);
		music_log.notice("Setting volume to %f", new_volume);
		m_media_player->audioOutput()->setVolume(new_volume);
	});
}

f32 qt_music_handler::get_volume() const
{
	std::lock_guard lock(m_mutex);
	f32 volume = 0.0f;

	Emu.BlockingCallFromMainThread([&volume, this]()
	{
		volume = std::clamp(m_media_player->audioOutput()->volume(), 0.0f, 1.0f);
		music_log.notice("Getting volume: %f", volume);
	});

	return volume;
}

void qt_music_handler::handle_media_status(QMediaPlayer::MediaStatus status)
{
	music_log.notice("New media status: %s (status=%d)", status, static_cast<int>(status));

	if (!m_status_callback)
	{
		return;
	}

	switch (status)
	{
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
}

void qt_music_handler::handle_music_state(QMediaPlayer::PlaybackState state)
{
	music_log.notice("New playback state: %s (state=%d)", state, static_cast<int>(state));
}

void qt_music_handler::handle_music_error(QMediaPlayer::Error error, const QString& errorString)
{
	music_log.error("Error event: \"%s\" (error=%s)", errorString, error);
}

void qt_music_handler::handle_volume_change(float volume) const
{
	music_log.notice("Volume changed: %f", volume);
}
