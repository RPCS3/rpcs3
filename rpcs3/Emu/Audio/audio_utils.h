#pragma once

namespace audio
{
	struct audio_fxo
	{
		atomic_t<bool> audio_muted {false};
		atomic_t<bool> voice_chat_mic_muted {false};
	};

	f32 get_volume();

	void toggle_mute();
	void toggle_voice_chat_mic_mute();
	f32 get_voice_chat_mic_volume();
	void change_volume(s32 delta);
}
