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

#include "Profile_gl3.h"
GPU_Profile GPU_Timer;

void GPU_Profile::dump(bool flush)
{
	u32 high_limit;
	if (flush) high_limit = 1;
	else	   high_limit = 1000;

    while (datas.size() > high_limit) {
        ProfileInfo data_start = datas.front();
        datas.pop_front();
        ProfileInfo data_stop = datas.front();
        datas.pop_front();

        u32 gpu_time = read_diff_timers(data_start.timer, data_stop.timer);

#ifdef ENABLE_MARKER
		if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, format("Time %6dus", gpu_time).c_str());
#else
        fprintf(stderr, "Frame %d (%d): %6dus\n", data_start.frame, data_start.draw, gpu_time);
#endif
    }
}

void GPU_Profile::create_timer()
{
	u32 timer = 0;
#ifdef GLSL4_API
	glGenQueries(1, &timer);
	glQueryCounter(timer, GL_TIMESTAMP);
#endif
	datas.push_back(ProfileInfo(timer, frame, draw));

#ifdef ENABLE_MARKER
	dump(true);
#endif
}

u32 GPU_Profile::read_diff_timers(u32 start_timer, u32 stop_timer)
{
#ifdef GLSL4_API
	if(!start_timer || !stop_timer) return -1;

	int stopTimerAvailable = 0;
	while (!stopTimerAvailable)
		glGetQueryObjectiv(stop_timer, GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);

	u64 start, stop = 0;
	// Note: timers have a precision of the ns, so you need 64 bits value to avoid overflow!
	glGetQueryObjectui64v(start_timer, GL_QUERY_RESULT, &start);
	glGetQueryObjectui64v(stop_timer, GL_QUERY_RESULT, &stop);

	// delete timer
	glDeleteQueries(1, &start_timer);
	glDeleteQueries(1, &stop_timer);

	return (stop-start)/1000;
#else
	return 0;
#endif
}

