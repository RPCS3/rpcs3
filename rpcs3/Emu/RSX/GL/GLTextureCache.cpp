#pragma once

#include "stdafx.h"

#include "GLGSRender.h"
#include "GLTextureCache.h"

namespace gl
{
	bool texture_cache::flush_section(u32 address)
	{
		if (address < rtt_cache_range.first ||
			address >= rtt_cache_range.second)
			return false;

		bool post_task = false;
		cached_rtt_section* section_to_post = nullptr;

		{
			std::lock_guard<std::mutex> lock(m_section_mutex);

			for (cached_rtt_section &rtt : m_rtt_cache)
			{
				if (rtt.is_dirty()) continue;

				if (rtt.is_locked() && rtt.overlaps(address))
				{
					if (rtt.is_flushed())
					{
						LOG_WARNING(RSX, "Section matches range, but marked as already flushed!, 0x%X+0x%X", rtt.get_section_base(), rtt.get_section_size());
						continue;
					}

					//LOG_WARNING(RSX, "Cell needs GPU data synced here, address=0x%X", address);

					if (std::this_thread::get_id() != m_renderer_thread)
					{
						post_task = true;
						section_to_post = &rtt;
						break;
					}

					rtt.flush();
					return true;
				}
			}
		}

		if (post_task)
		{
			//LOG_WARNING(RSX, "Cache access not from worker thread! address = 0x%X", address);
			work_item &task = m_renderer->post_flush_request(address, section_to_post);

			{
				std::unique_lock<std::mutex> lock(task.guard_mutex);
				task.cv.wait(lock, [&task] { return task.processed; });
			}

			task.received = true;
			return task.result;
		}

		return false;
	}
}