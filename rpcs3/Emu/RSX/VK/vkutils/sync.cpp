#include "barriers.h"
#include "buffer_object.h"
#include "commands.h"
#include "device.h"
#include "sync.h"
#include "shared.h"

#include "Emu/Cell/timers.hpp"

#include "util/sysinfo.hpp"
#include "util/asm.hpp"

// FIXME: namespace pollution
#include "../VKResourceManager.h"

namespace vk
{
	fence::fence(VkDevice dev)
	{
		owner                  = dev;
		VkFenceCreateInfo info = {};
		info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		CHECK_RESULT(vkCreateFence(dev, &info, nullptr, &handle));
	}

	fence::~fence()
	{
		if (handle)
		{
			vkDestroyFence(owner, handle, nullptr);
			handle = VK_NULL_HANDLE;
		}
	}

	void fence::reset()
	{
		vkResetFences(owner, 1, &handle);
		flushed.release(false);
	}

	void fence::signal_flushed()
	{
		flushed.release(true);
	}

	void fence::wait_flush()
	{
		while (!flushed)
		{
			utils::pause();
		}
	}

	fence::operator bool() const
	{
		return (handle != VK_NULL_HANDLE);
	}

	semaphore::semaphore(const render_device& dev)
		: m_device(dev)
	{
		VkSemaphoreCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		CHECK_RESULT(vkCreateSemaphore(m_device, &info, nullptr, &m_handle));
	}

	semaphore::~semaphore()
	{
		vkDestroySemaphore(m_device, m_handle, nullptr);
	}

	semaphore::operator VkSemaphore() const
	{
		return m_handle;
	}

	event::event(const render_device& dev, sync_domain domain)
		: m_device(dev)
	{
		const auto vendor = dev.gpu().get_driver_vendor();
		if (domain != sync_domain::gpu &&
			(vendor == vk::driver_vendor::AMD || vendor == vk::driver_vendor::INTEL))
		{
			// Work around AMD and INTEL broken event signal synchronization scope
			// Will be dropped after transitioning to VK1.3
			m_buffer = std::make_unique<buffer>
			(
				dev,
				4,
				dev.get_memory_mapping().host_visible_coherent,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				0,
				VMM_ALLOCATION_POOL_SYSTEM
			);

			m_value = reinterpret_cast<u32*>(m_buffer->map(0, 4));
			*m_value = 0xCAFEBABE;
		}
		else
		{
			VkEventCreateInfo info
			{
				.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0
			};
			vkCreateEvent(dev, &info, nullptr, &m_vk_event);
		}
	}

	event::~event()
	{
		if (m_vk_event) [[likely]]
		{
			vkDestroyEvent(m_device, m_vk_event, nullptr);
		}
		else
		{
			m_buffer->unmap();
			m_buffer.reset();
			m_value = nullptr;
		}
	}

	void event::signal(const command_buffer& cmd, VkPipelineStageFlags stages, VkAccessFlags access)
	{
		if (m_vk_event) [[likely]]
		{
			vkCmdSetEvent(cmd, m_vk_event, stages);
		}
		else
		{
			insert_global_memory_barrier(cmd, stages, VK_PIPELINE_STAGE_TRANSFER_BIT, access, VK_ACCESS_TRANSFER_WRITE_BIT);
			vkCmdFillBuffer(cmd, m_buffer->value, 0, 4, 0xDEADBEEF);
		}
	}

	void event::host_signal() const
	{
		ensure(m_vk_event);
		vkSetEvent(m_device, m_vk_event);
	}

	void event::gpu_wait(const command_buffer& cmd) const
	{
		ensure(m_vk_event);
		vkCmdWaitEvents(cmd, 1, &m_vk_event, 0, 0, 0, nullptr, 0, nullptr, 0, nullptr);
	}

	void event::reset() const
	{
		if (m_vk_event) [[likely]]
		{
			vkResetEvent(m_device, m_vk_event);
		}
		else
		{
			*m_value = 0xCAFEBABE;
		}
	}

	VkResult event::status() const
	{
		if (m_vk_event) [[likely]]
		{
			return vkGetEventStatus(m_device, m_vk_event);
		}
		else
		{
			return (*m_value == 0xCAFEBABE) ? VK_EVENT_RESET : VK_EVENT_SET;
		}
	}

	device_marker_pool::device_marker_pool(const vk::render_device& dev, u32 count)
		: pdev(&dev), m_count(count)
	{}

	std::tuple<VkBuffer, u64, volatile u32*> device_marker_pool::allocate()
	{
		if (!m_buffer || m_offset >= m_count)
		{
			create_impl();
		}

		const auto out_offset = m_offset;
		m_offset ++;
		return { m_buffer->value, out_offset * 4, m_mapped + out_offset };
	}

	void device_marker_pool::create_impl()
	{
		if (m_buffer)
		{
			m_buffer->unmap();
			vk::get_resource_manager()->dispose(m_buffer);
		}

		m_buffer = std::make_unique<buffer>
		(
			*pdev,
			m_count * 4,
			pdev->get_memory_mapping().host_visible_coherent,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			0,
			VMM_ALLOCATION_POOL_SYSTEM
		);

		m_mapped = reinterpret_cast<volatile u32*>(m_buffer->map(0, VK_WHOLE_SIZE));
		m_offset = 0;
	}

	device_debug_marker::device_debug_marker(device_marker_pool& pool, std::string message)
		: m_device(*pool.pdev), m_message(std::move(message))
	{
		std::tie(m_buffer, m_buffer_offset, m_value) = pool.allocate();
		*m_value = 0xCAFEBABE;
	}

	device_debug_marker::~device_debug_marker()
	{
		if (!m_printed)
		{
			dump();
		}

		m_value = nullptr;
	}

	void device_debug_marker::signal(const command_buffer& cmd, VkPipelineStageFlags stages, VkAccessFlags access)
	{
		insert_global_memory_barrier(cmd, stages, VK_PIPELINE_STAGE_TRANSFER_BIT, access, VK_ACCESS_TRANSFER_WRITE_BIT);
		vkCmdFillBuffer(cmd, m_buffer, m_buffer_offset, 4, 0xDEADBEEF);
	}

	void device_debug_marker::dump()
	{
		if (*m_value == 0xCAFEBABE)
		{
			rsx_log.error("DEBUG MARKER NOT REACHED: %s", m_message);
		}

		m_printed = true;
	}

	void device_debug_marker::dump() const
	{
		if (*m_value == 0xCAFEBABE)
		{
			rsx_log.error("DEBUG MARKER NOT REACHED: %s", m_message);
		}
		else
		{
			rsx_log.error("DEBUG MARKER: %s", m_message);
		}
	}

	// FIXME
	static std::unique_ptr<device_marker_pool> g_device_marker_pool;

	device_marker_pool& get_shared_marker_pool(const vk::render_device& dev)
	{
		if (!g_device_marker_pool)
		{
			g_device_marker_pool = std::make_unique<device_marker_pool>(dev, 65536);
		}
		return *g_device_marker_pool;
	}

	void device_debug_marker::insert(
		const vk::render_device& dev,
		const vk::command_buffer& cmd,
		std::string message,
		VkPipelineStageFlags stages,
		VkAccessFlags access)
	{
		auto result = std::make_unique<device_debug_marker>(get_shared_marker_pool(dev), message);
		result->signal(cmd, stages, access);
		vk::get_resource_manager()->dispose(result);
	}

	debug_marker_scope::debug_marker_scope(const vk::command_buffer& cmd, const std::string& message)
		: dev(&cmd.get_command_pool().get_owner()), cb(&cmd), message(message)
	{
		vk::device_debug_marker::insert(
			*dev,
			*cb,
			fmt::format("0x%x: Enter %s", rsx::get_shared_tag(), message)
		);
	}

	debug_marker_scope::~debug_marker_scope()
	{
		ensure(cb && cb->is_recording());

		vk::device_debug_marker::insert(
			*dev,
			*cb,
			fmt::format("0x%x: Exit %s", rsx::get_shared_tag(), message)
		);
	}

	VkResult wait_for_fence(fence* pFence, u64 timeout)
	{
		pFence->wait_flush();

		if (timeout)
		{
			return vkWaitForFences(*g_render_device, 1, &pFence->handle, VK_FALSE, timeout * 1000ull);
		}
		else
		{
			while (auto status = vkGetFenceStatus(*g_render_device, pFence->handle))
			{
				switch (status)
				{
				case VK_NOT_READY:
					utils::pause();
					continue;
				default:
					die_with_error(status);
					return status;
				}
			}

			return VK_SUCCESS;
		}
	}

	VkResult wait_for_event(event* pEvent, u64 timeout)
	{
		// Convert timeout to TSC cycles. Timeout accuracy isn't super-important, only fast response when event is signaled (within 10us if possible)
		const u64 freq = utils::get_tsc_freq();

		if (freq)
		{
			timeout *= (freq / 1'000'000);
		}

		u64 start = 0;

		while (true)
		{
			switch (const auto status = pEvent->status())
			{
			case VK_EVENT_SET:
				return VK_SUCCESS;
			case VK_EVENT_RESET:
				break;
			default:
				die_with_error(status);
				return status;
			}

			if (timeout)
			{
				const auto now = freq ? utils::get_tsc() : get_system_time();

				if (!start)
				{
					start = now;
					continue;
				}

				if ((now > start) &&
					(now - start) > timeout)
				{
					rsx_log.error("[vulkan] vk::wait_for_event has timed out!");
					return VK_TIMEOUT;
				}
			}

			utils::pause();
		}
	}
}
