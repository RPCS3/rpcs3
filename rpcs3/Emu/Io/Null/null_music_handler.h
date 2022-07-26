#pragma once

#include "Emu/Io/music_handler_base.h"

class null_music_handler final : public music_handler_base
{
public:
	null_music_handler() : music_handler_base() {}

	void stop() override { m_state = 0; } // CELL_MUSIC_PB_STATUS_STOP
	void pause() override { m_state = 2; } // CELL_MUSIC_PB_STATUS_PAUSE
	void play(const std::string& /*path*/) override { m_state = 1; } // CELL_MUSIC_PB_STATUS_PLAY
	void fast_forward(const std::string& /*path*/) override { m_state = 3; } // CELL_MUSIC_PB_STATUS_FASTFORWARD
	void fast_reverse(const std::string& /*path*/) override { m_state = 4; } // CELL_MUSIC_PB_STATUS_FASTREVERSE
	void set_volume(f32 volume) override { m_volume = volume; }
	f32 get_volume() const override { return m_volume; }

private:
	atomic_t<f32> m_volume{0.0f};
};
