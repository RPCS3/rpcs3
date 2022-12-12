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
	virtual void play(const std::string& path) = 0;
	virtual void fast_forward(const std::string& path) = 0;
	virtual void fast_reverse(const std::string& path) = 0;
	virtual void set_volume(f32 volume) = 0;
	virtual f32 get_volume() const = 0;

	s32 get_state() const
	{
		return m_state;
	}

	void set_status_callback(std::function<void(player_status)> status_callback)
	{
		m_status_callback = std::move(status_callback);
	}

protected:
	atomic_t<s32> m_state{0};
	std::function<void(player_status status)> m_status_callback;
};
