#include "barriers.h"
#include "buffer_object.h"
#include "commands.h"
#include "device.h"
#include "sync.h"
#include "shared.h"

#include "util/sysinfo.hpp"
#include "util/asm.hpp"

extern u64 get_system_time();

namespace vk
{
#ifdef _MSC_VER
	extern "C" void _mm_pause();
#endif

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
#ifdef _MSC_VER
			_mm_pause();
#else
			__builtin_ia32_pause();
#endif
		}
	}

	fence::operator bool() const
	{
		return (handle != VK_NULL_HANDLE);
	}

	event::event(const render_device& dev, sync_domain domain)
	{
		m_device = dev;
		if (domain == sync_domain::gpu || dev.gpu().get_driver_vendor() != driver_vendor::AMD)
		{
			VkEventCreateInfo info
			{
				.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0
			};
			vkCreateEvent(dev, &info, nullptr, &m_vk_event);
		}
		else
		{
			// Work around AMD's broken event signals
			m_buffer = std::make_unique<buffer>
			(
				dev,
				4,
				dev.get_memory_mapping().host_visible_coherent,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				0
			);

			m_value = reinterpret_cast<u32*>(m_buffer->map(0, 4));
			*m_value = 0xCAFEBABE;
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
		timeout *= (utils::get_tsc_freq() / 1'000'000);
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
				if (!start)
				{
					start = utils::get_tsc();
					continue;
				}

				if (const auto now = utils::get_tsc();
					(now > start) &&
					(now - start) > timeout)
				{
					rsx_log.error("[vulkan] vk::wait_for_event has timed out!");
					return VK_TIMEOUT;
				}
			}

#ifdef _MSC_VER
			_mm_pause();
#else
			__builtin_ia32_pause();
#endif
		}
	}
}
