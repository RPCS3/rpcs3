#pragma once

#include "Emu/Io/music_handler_base.h"
#include "qt_music_error_handler.h"

#include <QMediaPlayer>
#include <mutex>

class qt_music_handler final : public music_handler_base
{
public:
	qt_music_handler();
	virtual ~qt_music_handler();

	void stop() override;
	void play(const std::string& path) override;
	void pause() override;
	void fast_forward() override;
	void fast_reverse() override;
	void set_volume(f32 volume) override;
	f32 get_volume() const override;

private:
	mutable std::mutex m_mutex;
	std::unique_ptr<qt_music_error_handler> m_error_handler;
	std::shared_ptr<QMediaPlayer> m_media_player;
};
