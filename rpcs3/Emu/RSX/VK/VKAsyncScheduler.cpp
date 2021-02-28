#include "VKAsyncScheduler.h"
#include "VKHelpers.h"
#include "VKResourceManager.h"

#include "Emu/IdManager.h"
#include "Utilities/lockless.h"
#include "Utilities/mutex.h"

#include <vector>

namespace vk
{
	void AsyncTaskScheduler::operator()()
	{
		add_ref();

		while (thread_ctrl::state() != thread_state::aborting)
		{
			for (auto&& job : m_event_queue.pop_all())
			{
				vk::wait_for_event(job->queue1_signal.get(), GENERAL_WAIT_TIMEOUT);
				job->queue2_signal->host_signal();
			}
		}

		release();
	}

	void AsyncTaskScheduler::delayed_init()
	{
		auto pdev = get_current_renderer();
		m_command_pool.create(*const_cast<render_device*>(pdev), pdev->get_transfer_queue_family());

		for (usz i = 0; i < events_pool_size; ++i)
		{
			auto ev1 = std::make_unique<event>(*get_current_renderer(), sync_domain::gpu);
			auto ev2 = std::make_unique<event>(*get_current_renderer(), sync_domain::gpu);
			m_events_pool.emplace_back(std::move(ev1), std::move(ev2), 0ull);
		}
	}

	void AsyncTaskScheduler::insert_sync_event()
	{
		ensure(m_current_cb);

		xqueue_event* sync_label;
		ensure(m_next_event_id < events_pool_size);
		sync_label = &m_events_pool[m_next_event_id];

		if (++m_next_event_id == events_pool_size)
		{
			// Wrap
			m_next_event_id = 0;
		}

		ensure(sync_label->completion_eid <= vk::last_completed_event_id());

		sync_label->queue1_signal->reset();
		sync_label->queue2_signal->reset();
		sync_label->completion_eid = vk::current_event_id();

		sync_label->queue1_signal->signal(*m_current_cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);

		m_event_queue.push(sync_label);
		m_sync_label = sync_label->queue2_signal.get();
	}

	AsyncTaskScheduler::~AsyncTaskScheduler()
	{
		if (!m_async_command_queue.empty())
		{
			// Driver resources should be destroyed before driver is detached or you get crashes. RAII won't save you here.
			rsx_log.error("Async task scheduler resources were not freed correctly!");
		}
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
		auto pdev = get_current_renderer();
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
				m_async_command_queue.push_back({});
				m_current_cb = &m_async_command_queue.back();
				m_current_cb->create(m_command_pool, true);
			}
		}

		m_next_cb_index++;
		return m_current_cb;
	}

	event* AsyncTaskScheduler::get_primary_sync_label()
	{
		std::lock_guard lock(m_submit_mutex);

		if (m_sync_required)
		{
			ensure(m_current_cb);
			insert_sync_event();
			m_sync_required = false;
		}

		return std::exchange(m_sync_label, nullptr);
	}

	void AsyncTaskScheduler::flush(VkSemaphore wait_semaphore, VkPipelineStageFlags wait_dst_stage_mask)
	{
		std::lock_guard lock(m_submit_mutex);

		if (!m_current_cb)
		{
			return;
		}

		if (m_sync_required)
		{
			insert_sync_event();
		}

		m_current_cb->end();
		m_current_cb->submit(get_current_renderer()->get_transfer_queue(), wait_semaphore, VK_NULL_HANDLE, nullptr, wait_dst_stage_mask, VK_FALSE);

		m_last_used_cb = m_current_cb;
		m_current_cb = nullptr;
		m_sync_required = false;
	}

	void AsyncTaskScheduler::kill()
	{
		*g_fxo->get<async_scheduler_thread>() = thread_state::aborting;
		while (has_refs()) _mm_pause();

		for (auto& cb : m_async_command_queue)
		{
			cb.destroy();
		}

		m_async_command_queue.clear();
		m_next_cb_index = 0;
		m_command_pool.destroy();
		m_events_pool.clear();
	}
}
