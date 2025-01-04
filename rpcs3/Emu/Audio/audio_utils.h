#pragma once

namespace audio
{
	struct audio_fxo
	{
		atomic_t<bool> audio_muted {false};
	};

	f32 get_volume();

	void toggle_mute();
	void change_volume(s32 delta);
}
