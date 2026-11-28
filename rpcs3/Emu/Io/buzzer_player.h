#pragma once

#include "util/types.hpp"

class buzzer_player
{
public:
	buzzer_player();
	~buzzer_player();

	buzzer_player(const buzzer_player&) = delete;
	buzzer_player& operator=(const buzzer_player&) = delete;

	void play_beep_1();
	void play_beep_3();
	void play_periodic_burst();
	void play_short_long_alert();
	void play_descending_alert();
	void play_chromatic_siren();
	void play_arpeggio();
	void play_scale();
	void play_click();
	void play_count(u32 count);
	void play_high_scale();
	void stop();

	bool is_playing() const;
	void check_device();

	static void notify_settings_changed();

private:
	enum class wave_id : u8
	{
		beep_1,
		beep_3,
		periodic_burst,
		short_long_alert,
		descending_alert,
		chromatic_siren,
		arpeggio,
		scale,
		click,
		count,
		high_scale,
	};

	void play(wave_id id, u32 count = 0);

	const u64 m_instance_id = 0;
};
