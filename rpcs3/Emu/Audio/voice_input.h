#pragma once

#include "util/shared_ptr.hpp"
#include <string>

namespace voice_input
{
	// Written by the GUI, read by the voice worker. A generation also records a
	// release followed by another press between two audio ticks.
	struct talk_state
	{
		std::string key;
		u64 generation = 0;
	};

	inline stx::atomic_ptr<talk_state> talk{talk_state{}};

	inline void set_talk_key(std::string key)
	{
		const auto previous = talk.load();
		if (previous->key != key)
			talk = talk_state{std::move(key), previous->generation + 1};
	}
}
