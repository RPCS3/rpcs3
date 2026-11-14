#pragma once

#include "util/types.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <span>
#include <string>

class AudioBackend;

class buzzer_player
{
public:
	buzzer_player();
	~buzzer_player();

	buzzer_player(const buzzer_player&) = delete;
	buzzer_player& operator=(const buzzer_player&) = delete;

	void play_beep_1();
	void play_beep_3();
	void play_scale();
	void stop();

	bool is_playing() const;
	void check_device();

private:
	void play(std::span<const f32> wave);
	bool ensure_backend();
	void close_backend();

	std::mutex m_init_mutex;
	std::shared_ptr<AudioBackend> m_backend;
	std::string m_opened_device;
	std::atomic<bool> m_device_changed{false};

	std::mutex m_playback_mutex;
	std::span<const f32> m_wave{};
	usz m_wave_pos = 0;
	std::atomic<bool> m_is_playing{false};
	steady_clock::time_point m_play_start_time{};
};
