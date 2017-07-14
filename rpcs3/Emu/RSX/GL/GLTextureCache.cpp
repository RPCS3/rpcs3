#include "stdafx.h"
#include "GLGSRender.h"
#include "GLTextureCache.h"

namespace gl
{
	bool texture_cache::flush_section(u32 address)
	{
		if (address < no_access_range.first ||
			address >= no_access_range.second)
			return false;

		bool post_task = false;
		cached_texture_section* section_to_post = nullptr;

		{
			std::lock_guard<std::mutex> lock(m_section_mutex);

			for (cached_texture_section &tex : no_access_memory_sections)
			{
				if (tex.is_dirty()) continue;

				if (tex.is_locked() && tex.overlaps(address))
				{
					if (tex.is_flushed())
					{
						LOG_WARNING(RSX, "Section matches range, but marked as already flushed!, 0x%X+0x%X", tex.get_section_base(), tex.get_section_size());
						continue;
					}

					//LOG_WARNING(RSX, "Cell needs GPU data synced here, address=0x%X", address);

					if (std::this_thread::get_id() != m_renderer_thread)
					{
						post_task = true;
						section_to_post = &tex;
						break;
					}

					tex.flush();
					return true;
				}
			}
		}

		if (post_task)
		{
			//LOG_WARNING(RSX, "Cache access not from worker thread! address = 0x%X", address);
			work_item &task = m_renderer->post_flush_request(address, section_to_post);

			vm::temporary_unlock();
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
