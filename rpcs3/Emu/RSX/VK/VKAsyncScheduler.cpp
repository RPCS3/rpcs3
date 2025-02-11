#include "VKAsyncScheduler.h"
#include "VKHelpers.h"

#include <vector>

namespace vk
{
	AsyncTaskScheduler::AsyncTaskScheduler(vk_gpu_scheduler_mode mode, const VkDependencyInfoKHR& queue_dependency)
	{
		if (g_cfg.video.renderer != video_renderer::vulkan || !g_cfg.video.vk.asynchronous_texture_streaming)
		{
			// Invalid renderer combination, do not proceed. This should never happen.
			// NOTE: If managed by fxo, this object may be created automatically on boot.
			rsx_log.notice("Vulkan async streaming is disabled. This thread will now exit.");
			return;
		}

		init_config_options(mode, queue_dependency);
	}

	AsyncTaskScheduler::~AsyncTaskScheduler()
	{
		if (!m_async_command_queue.empty())
		{
			// Driver resources should be destroyed before driver is detached or you get crashes. RAII won't save you here.
			rsx_log.error("Async task scheduler resources were not freed correctly!");
		}
	}

	void AsyncTaskScheduler::init_config_options(vk_gpu_scheduler_mode mode, const VkDependencyInfoKHR& queue_dependency)
	{
		std::lock_guard lock(m_config_mutex);
		if (std::exchange(m_options_initialized, true))
		{
			// Nothing to do
			return;
		}

		m_use_host_scheduler = (mode == vk_gpu_scheduler_mode::safe) || g_cfg.video.strict_rendering_mode;
		rsx_log.notice("Asynchronous task scheduler is active running in %s mode", m_use_host_scheduler? "'Safe'" : "'Fast'");

		m_dependency_info = queue_dependency;
	}

	void AsyncTaskScheduler::delayed_init()
	{
		ensure(m_options_initialized);

		auto pdev = get_current_renderer();
		m_command_pool.create(*const_cast<render_device*>(pdev), pdev->get_transfer_queue_family());

		if (m_use_host_scheduler)
		{
			for (usz i = 0; i < events_pool_size; ++i)
			{
				auto sema = std::make_unique<semaphore>(*pdev);
				m_semaphore_pool.emplace_back(std::move(sema));
			}

			return;
		}

		for (usz i = 0; i < events_pool_size; ++i)
		{
			auto ev = std::make_unique<vk::event>(*pdev, sync_domain::gpu);
			m_events_pool.emplace_back(std::move(ev));
		}
	}

	void AsyncTaskScheduler::insert_sync_event()
	{
		ensure(m_current_cb);
		auto& sync_label = m_events_pool[m_next_event_id++ % events_pool_size];

		sync_label->reset();
		sync_label->signal(*m_current_cb, m_dependency_info);
		m_sync_label = sync_label.get();
	}

	command_buffer* AsyncTaskScheduler::get_current()
	{
		std::lock_guard lock(m_submit_mutex);
		m_sync_required = true;

		// 0. Anything still active?
		if (m_current_cb)
		{
			return m_current_cb;
		}

		// 1. Check if there is a 'next' entry
		if (m_async_command_queue.empty())
		{
			delayed_init();
		}
		else if (m_next_cb_index < m_async_command_queue.size())
		{
			m_current_cb = &m_async_command_queue[m_next_cb_index];
		}

		// 2. Create entry
		if (!m_current_cb)
		{
			if (m_next_cb_index == VK_MAX_ASYNC_COMPUTE_QUEUES)
			{
				m_next_cb_index = 0;
				m_current_cb = &m_async_command_queue[m_next_cb_index];
			}
			else
			{
				m_async_command_queue.emplace_back();
				m_current_cb = &m_async_command_queue.back();
				m_current_cb->create(m_command_pool);
			}
		}

		m_next_cb_index++;
		return m_current_cb;
	}

	event* AsyncTaskScheduler::get_primary_sync_label()
	{
		ensure(!m_use_host_scheduler);

		if (m_sync_required) [[unlikely]]
		{
			std::lock_guard lock(m_submit_mutex); // For some reason this is inexplicably expensive. WTF!
			ensure(m_current_cb);
			insert_sync_event();
			m_sync_required = false;
		}

		return std::exchange(m_sync_label, nullptr);
	}

	semaphore* AsyncTaskScheduler::get_sema()
	{
		if (m_semaphore_pool.empty())
		{
			delayed_init();
			ensure(!m_semaphore_pool.empty());
		}

		const u32 sema_id = static_cast<u32>(m_next_semaphore_id++ % m_semaphore_pool.size());
		return m_semaphore_pool[sema_id].get();
	}

	void AsyncTaskScheduler::flush(queue_submit_t& submit_info, VkBool32 force_flush)
	{
		if (!m_current_cb)
		{
			return;
		}

		submit_info.queue = get_current_renderer()->get_transfer_queue();

		std::lock_guard lock(m_submit_mutex);
		if (m_sync_required && !m_use_host_scheduler)
		{
			insert_sync_event();
		}

		m_current_cb->end();
		m_current_cb->submit(submit_info, force_flush);

		m_submit_count++;

		m_last_used_cb = m_current_cb;
		m_current_cb = nullptr;
		m_sync_required = false;
	}

	void AsyncTaskScheduler::destroy()
	{
		for (auto& cb : m_async_command_queue)
		{
			cb.destroy();
		}

		m_async_command_queue.clear();
		m_next_cb_index = 0;
		m_command_pool.destroy();
		m_events_pool.clear();
		m_semaphore_pool.clear();
	}
}
