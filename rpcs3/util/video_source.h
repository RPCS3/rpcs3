#pragma once

#include "types.hpp"

class video_source
{
public:
	video_source() {};
	virtual ~video_source() {};
	virtual void set_video_path(const std::string& path) { static_cast<void>(path); }
	virtual bool has_new() const { return false; };
	virtual void get_image(std::vector<u8>& data, int& w, int& h, int& ch, int& bpp)
	{
		static_cast<void>(data);
		static_cast<void>(w);
		static_cast<void>(h);
		static_cast<void>(ch);
		static_cast<void>(bpp);
	}
};
