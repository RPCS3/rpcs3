/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2012 zeydlitz@gmail.com, arcum42@gmail.com, gregory.hainaut@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "Util.h"

#ifndef _PROFILE_GL3_H_
#define _PROFILE_GL3_H_

#define ENABLE_MARKER // Fire some marker for opengl Debugger (apitrace, gdebugger)

class GPU_Profile {
	struct ProfileInfo {
		u32 timer;
		u32 frame;
		u32 draw;

		ProfileInfo(u32 timer, u32 frame, u32 draw) : timer(timer), frame(frame), draw(draw) {}
		ProfileInfo(u32 timer) : timer(timer), frame(0), draw(0) {}
	};

	std::list<ProfileInfo> datas;
	u32 frame;
	u32 draw;


	public:
	GPU_Profile() : frame(0), draw(0) {
		datas.clear();
	}

	void inc_draw() { draw++;}
	void inc_frame() { frame++;}

	void create_timer();
	u32 read_diff_timers(u32 start_timer, u32 stop_timer);

	void dump(bool flush = false);
};
extern GPU_Profile GPU_Timer;

#endif
