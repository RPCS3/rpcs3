#pragma once

#include <QObject>
#include <QMediaPlayer>

class qt_music_error_handler : public QObject
{
	Q_OBJECT

public:
	qt_music_error_handler(std::shared_ptr<QMediaPlayer> media_player, std::function<void(QMediaPlayer::MediaStatus)> status_callback);
	virtual ~qt_music_error_handler();

private Q_SLOTS:
	void handle_media_status(QMediaPlayer::MediaStatus status);
	void handle_music_state(QMediaPlayer::PlaybackState state);
	void handle_music_error(QMediaPlayer::Error error, const QString& errorString);

private:
	std::shared_ptr<QMediaPlayer> m_media_player;
	std::function<void(QMediaPlayer::MediaStatus)> m_status_callback = nullptr;
};
