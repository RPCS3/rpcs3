#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

class music_handler_base
{
public:
	enum class player_status
	{
		end_of_media
	};

	virtual ~music_handler_base() = default;

	virtual void stop() = 0;
	virtual void pause() = 0;
	virtual void play(const std::string& path, bool automatic = false) = 0;
	virtual void fast_forward(const std::string& path) = 0;
	virtual void fast_reverse(const std::string& path) = 0;
	virtual void set_volume(f32 volume) = 0;
	virtual f32 get_volume() const = 0;

	void set_state(u32 state, bool automatic = false)
	{
		const bool changed = m_state.exchange(state) != state;

		if (m_event_status_callback && (changed || !automatic))
		{
			m_event_status_callback(state);
		}
	}

	u32 get_state() const
	{
		return m_state;
	}

	void set_event_status_callback(std::function<void(u32)> status_callback)
	{
		m_event_status_callback = std::move(status_callback);
	}

	void set_playback_status_callback(std::function<void(player_status)> status_callback)
	{
		m_playback_status_callback = std::move(status_callback);
	}

private:
	atomic_t<u32> m_state{0};
	std::function<void(u32)> m_event_status_callback;

protected:
	std::function<void(player_status)> m_playback_status_callback;
};
