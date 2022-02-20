#pragma once

#include <QObject>
#include <QMediaPlayer>

class qt_music_error_handler : public QObject
{
	Q_OBJECT

public:
	qt_music_error_handler(std::shared_ptr<QMediaPlayer> media_player);
	virtual ~qt_music_error_handler();

private Q_SLOTS:
	void handle_media_status(QMediaPlayer::MediaStatus status);
	void handle_music_state(QMediaPlayer::State state);
	void handle_music_error(QMediaPlayer::Error error);

private:
	std::shared_ptr<QMediaPlayer> m_media_player;
};
