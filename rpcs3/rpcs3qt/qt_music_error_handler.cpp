#include "qt_music_error_handler.h"
#include "util/logs.hpp"

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

qt_music_error_handler::qt_music_error_handler(std::shared_ptr<QMediaPlayer> media_player, std::function<void(QMediaPlayer::MediaStatus)> status_callback)
	: m_media_player(std::move(media_player))
	, m_status_callback(std::move(status_callback))
{
	if (m_media_player)
	{
		connect(m_media_player.get(), &QMediaPlayer::mediaStatusChanged, this, &qt_music_error_handler::handle_media_status);
		connect(m_media_player.get(), &QMediaPlayer::playbackStateChanged, this, &qt_music_error_handler::handle_music_state);
		connect(m_media_player.get(), &QMediaPlayer::errorOccurred, this, &qt_music_error_handler::handle_music_error);
	}
}

qt_music_error_handler::~qt_music_error_handler()
{
}

void qt_music_error_handler::handle_media_status(QMediaPlayer::MediaStatus status)
{
	music_log.notice("New media status: %s (status=%d)", status, static_cast<int>(status));

	if (m_status_callback)
	{
		m_status_callback(status);
	}
}

void qt_music_error_handler::handle_music_state(QMediaPlayer::PlaybackState state)
{
	music_log.notice("New playback state: %s (state=%d)", state, static_cast<int>(state));
}

void qt_music_error_handler::handle_music_error(QMediaPlayer::Error error, const QString& errorString)
{
	music_log.error("Error event: \"%s\" (error=%s)", errorString, error);
}
