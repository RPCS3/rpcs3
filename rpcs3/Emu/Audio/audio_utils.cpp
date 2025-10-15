#include "stdafx.h"
#include "audio_utils.h"
#include "Emu/system_config.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include <cmath>

namespace audio
{
	f32 get_volume()
	{
		return g_fxo->get<audio_fxo>().audio_muted ? 0.0f : g_cfg.audio.volume / 100.0f;
	}

	void toggle_mute()
	{
		audio_fxo& fxo = g_fxo->get<audio_fxo>();
		fxo.audio_muted = !fxo.audio_muted;
		Emu.GetCallbacks().update_emu_settings();

		rsx::overlays::queue_message(fxo.audio_muted ? localized_string_id::AUDIO_MUTED : localized_string_id::AUDIO_UNMUTED, 3'000'000);
	}

	void change_volume(s32 delta)
	{
		// Ignore if muted
		if (g_fxo->get<audio_fxo>().audio_muted)
			return;

		const s32 old_volume = g_cfg.audio.volume;

		// Apply non-linear volume scaling for better perceived volume control
		// Use smaller steps at lower volumes for finer control
		s32 adjusted_delta = delta;
		if (old_volume < 25 && abs(delta) > 1)
		{
			// Smaller steps at low volume for better control
			adjusted_delta = delta > 0 ? 1 : -1;
		}
		else if (old_volume > 75 && abs(delta) < 5)
		{
			// Larger steps at high volume for faster adjustment
			adjusted_delta = delta > 0 ? std::min(delta * 2, 5) : std::max(delta * 2, -5);
		}

		const s32 new_volume = old_volume + adjusted_delta;

		if (old_volume == new_volume)
			return;

		g_cfg.audio.volume.set(std::clamp<s32>(new_volume, g_cfg.audio.volume.min, g_cfg.audio.volume.max));
		Emu.GetCallbacks().update_emu_settings();

		rsx::overlays::queue_message(localized_string(localized_string_id::AUDIO_CHANGED, "%d%%", g_cfg.audio.volume.get()), 3'000'000, {}, rsx::overlays::message_pin_location::top_left, {}, true, true);
	}
} // namespace audio
