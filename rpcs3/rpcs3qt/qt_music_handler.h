#pragma once

#include "Emu/Io/music_handler_base.h"

#include <QMediaPlayer>
#include <QObject>
#include <mutex>

class qt_music_handler final : public QObject, public music_handler_base
{
	Q_OBJECT

public:
	qt_music_handler();
	virtual ~qt_music_handler();

	void stop() override;
	void pause() override;
	void play(const std::string& path) override;
	void fast_forward(const std::string& path) override;
	void fast_reverse(const std::string& path) override;
	void set_volume(f32 volume) override;
	f32 get_volume() const override;

private Q_SLOTS:
	void handle_media_status(QMediaPlayer::MediaStatus status);
	void handle_music_state(QMediaPlayer::PlaybackState state);
	void handle_music_error(QMediaPlayer::Error error, const QString& errorString);
	void handle_volume_change(float volume) const;

private:
	mutable std::mutex m_mutex;
	std::unique_ptr<QMediaPlayer> m_media_player;
	std::string m_path;
};
