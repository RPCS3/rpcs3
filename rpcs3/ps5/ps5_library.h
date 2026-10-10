#pragma once

// PS5: what the launcher keeps about each game beyond its files: whether it is
// a favourite, when it was last played and for how long in all, and how the
// Library is sorted. Kept in the config folder's library.txt, a line a game
// (written when something changes, and the play time every minute of play).

#include "util/types.hpp"

#include <string>

namespace ps5_library
{
	struct entry
	{
		bool favourite = false;
		u64 last_played = 0;  // seconds since 1970, UTC; 0 if never
		u64 play_seconds = 0;
	};

	// How the games are ordered, in the Library and on Home
	enum class order : u8
	{
		recent,     // last played first, then the rest by name
		name,       // A to Z
		most_played,
		favourites, // the favourites first, each part as recent
		count
	};

	entry get(const std::string& serial);
	void set_favourite(const std::string& serial, bool favourite);

	// A game started (the time it was last played: now), and how long it ran
	void started(const std::string& serial);
	void add_play(const std::string& serial, u32 seconds);

	order get_order();
	void set_order(order next);
	const char* order_text(order kind); // "Recently played"...

	// What changed, written now
	void flush();

	// "3 h 20 min", "45 min", "under a minute"
	std::string play_text(u64 seconds);

	// "10 Oct 2026", in the console's local time
	std::string date_text(u64 seconds);
}
