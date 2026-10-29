#pragma once

#include "util/shared_ptr.hpp"

namespace voice_input
{
	// Published by the pad thread. The generation records a release/repress
	// between audio ticks so already encoded speech cannot escape after muting.
	struct talk_state
	{
		bool transmitting = true;
		u64 generation = 0;
	};

	inline stx::atomic_ptr<talk_state> talk{talk_state{}};

	inline void set_transmitting(bool transmitting)
	{
		const auto previous = talk.load();
		if (previous->transmitting != transmitting)
			talk = talk_state{transmitting, previous->generation + 1};
	}

	struct microphone_control
	{
		bool muted = false;
		bool toggle_pressed = false;

		bool update(bool push_to_talk, bool held, bool toggle, bool input_allowed)
		{
			if (input_allowed && toggle && !toggle_pressed) muted = !muted;
			toggle_pressed = toggle;
			return input_allowed && !muted && (!push_to_talk || held);
		}
	};
}
