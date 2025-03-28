#pragma once

#include "types.hpp"
#include <functional>

class video_source
{
public:
	video_source() {};
	virtual ~video_source() {};
	virtual void set_video_path(const std::string& video_path) = 0;
	virtual void set_active(bool active) = 0;
	virtual bool get_active() const = 0;
	virtual bool has_new() const = 0;
	virtual void get_image(std::vector<u8>& data, int& w, int& h, int& ch, int& bpp) = 0;

	void set_update_callback(std::function<void()> callback)
	{
		m_update_callback = callback;
	}

protected:
	void notify_update()
	{
		if (m_update_callback)
		{
			m_update_callback();
		}
	}

private:
	std::function<void()> m_update_callback;
};
