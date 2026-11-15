#pragma once

#include "util/types.hpp"

#include <span>

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

	u64 m_instance_id = 0;
};
