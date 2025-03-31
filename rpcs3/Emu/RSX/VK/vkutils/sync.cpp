#include "barriers.h"
#include "buffer_object.h"
#include "commands.h"
#include "device.h"
#include "garbage_collector.h"
#include "sync.h"
#include "shared.h"

#include "Emu/Cell/timers.hpp"

#include "util/sysinfo.hpp"
#include "util/asm.hpp"

namespace vk
{
	namespace globals
	{
		static std::unique_ptr<gpu_debug_marker_pool> g_gpu_debug_marker_pool;
		static std::unique_ptr<gpu_label_pool> g_gpu_label_pool;

		gpu_debug_marker_pool& get_shared_marker_pool(const vk::render_device& dev)
		{
			if (!g_gpu_debug_marker_pool)
			{
				g_gpu_debug_marker_pool = std::make_unique<gpu_debug_marker_pool>(dev, 65536);
				vk::get_gc()->add_exit_callback([]()
				{
					g_gpu_debug_marker_pool.reset();
				});
			}

			return *g_gpu_debug_marker_pool;
		}

		gpu_label_pool& get_shared_label_pool(const vk::render_device& dev)
		{
			if (!g_gpu_label_pool)
			{
				g_gpu_label_pool = std::make_unique<gpu_label_pool>(dev, 65536);
				vk::get_gc()->add_exit_callback([]()
				{
					g_gpu_label_pool.reset();
				});
			}

			return *g_gpu_label_pool;
		}
	}

	// Util
	namespace v1_utils
	{
		VkPipelineStageFlags gather_src_stages(const VkDependencyInfoKHR& dependency)
		{
			VkPipelineStageFlags stages = VK_PIPELINE_STAGE_NONE;
			for (u32 i = 0; i < dependency.bufferMemoryBarrierCount; ++i)
			{
				stages |= dependency.pBufferMemoryBarriers[i].srcStageMask;
			}
			for (u32 i = 0; i < dependency.imageMemoryBarrierCount; ++i)
			{
				stages |= dependency.pImageMemoryBarriers[i].srcStageMask;
			}
			for (u32 i = 0; i < dependency.memoryBarrierCount; ++i)
			{
				stages |= dependency.pMemoryBarriers[i].srcStageMask;
			}
			return stages;
		}

		VkPipelineStageFlags gather_dst_stages(const VkDependencyInfoKHR& dependency)
		{
			VkPipelineStageFlags stages = VK_PIPELINE_STAGE_NONE;
			for (u32 i = 0; i < dependency.bufferMemoryBarrierCount; ++i)
			{
				stages |= dependency.pBufferMemoryBarriers[i].dstStageMask;
			}
			for (u32 i = 0; i < dependency.imageMemoryBarrierCount; ++i)
			{
				stages |= dependency.pImageMemoryBarriers[i].dstStageMask;
			}
			for (u32 i = 0; i < dependency.memoryBarrierCount; ++i)
			{
				stages |= dependency.pMemoryBarriers[i].dstStageMask;
			}
			return stages;
		}

		auto get_memory_barriers(const VkDependencyInfoKHR& dependency)
		{
			std::vector<VkMemoryBarrier> result;
			for (u32 i = 0; i < dependency.memoryBarrierCount; ++i)
			{
				result.push_back
				({
					VK_STRUCTURE_TYPE_MEMORY_BARRIER,
					nullptr,
					static_cast<VkAccessFlags>(dependency.pMemoryBarriers[i].srcAccessMask),
					static_cast<VkAccessFlags>(dependency.pMemoryBarriers[i].dstAccessMask)
				});
			}
			return result;
		}

		auto get_image_memory_barriers(const VkDependencyInfoKHR& dependency)
		{
			std::vector<VkImageMemoryBarrier> result;
			for (u32 i = 0; i < dependency.imageMemoryBarrierCount; ++i)
			{
				result.push_back
				({
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					static_cast<VkAccessFlags>(dependency.pImageMemoryBarriers[i].srcAccessMask),
					static_cast<VkAccessFlags>(dependency.pImageMemoryBarriers[i].dstAccessMask),
					dependency.pImageMemoryBarriers[i].oldLayout,
					dependency.pImageMemoryBarriers[i].newLayout,
					dependency.pImageMemoryBarriers[i].srcQueueFamilyIndex,
					dependency.pImageMemoryBarriers[i].dstQueueFamilyIndex,
					dependency.pImageMemoryBarriers[i].image,
					dependency.pImageMemoryBarriers[i].subresourceRange
				});
			}
			return result;
		}

		auto get_buffer_memory_barriers(const VkDependencyInfoKHR& dependency)
		{
			std::vector<VkBufferMemoryBarrier> result;
			for (u32 i = 0; i < dependency.bufferMemoryBarrierCount; ++i)
			{
				result.push_back
				({
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
					nullptr,
					static_cast<VkAccessFlags>(dependency.pBufferMemoryBarriers[i].srcAccessMask),
					static_cast<VkAccessFlags>(dependency.pBufferMemoryBarriers[i].dstAccessMask),
					dependency.pBufferMemoryBarriers[i].srcQueueFamilyIndex,
					dependency.pBufferMemoryBarriers[i].dstQueueFamilyIndex,
					dependency.pBufferMemoryBarriers[i].buffer,
					dependency.pBufferMemoryBarriers[i].offset,
					dependency.pBufferMemoryBarriers[i].size
				});
			}
			return result;
		}
	}

	// Objects
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
		: m_device(&dev), m_domain(domain)
	{
		m_backend = dev.get_synchronization2_support()
			? sync_backend::events_v2
			: sync_backend::events_v1;

		if (domain == sync_domain::host &&
			vk::get_driver_vendor() == vk::driver_vendor::AMD &&
			vk::get_chip_family() < vk::chip_class::AMD_navi1x)
		{
			// Events don't work quite right on AMD drivers
			m_backend = sync_backend::gpu_label;

			m_label = std::make_unique<vk::gpu_label>(globals::get_shared_label_pool(dev));
			return;
		}

		VkEventCreateInfo info
		{
			.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		if (domain == sync_domain::gpu && m_backend == sync_backend::events_v2)
		{
			info.flags = VK_EVENT_CREATE_DEVICE_ONLY_BIT_KHR;
		}

		CHECK_RESULT(vkCreateEvent(dev, &info, nullptr, &m_vk_event));
	}

	event::~event()
	{
		if (m_vk_event) [[likely]]
		{
			vkDestroyEvent(*m_device, m_vk_event, nullptr);
		}
	}

	void event::resolve_dependencies(const command_buffer& cmd, const VkDependencyInfoKHR& dependency)
	{
		ensure(m_backend != sync_backend::gpu_label);

		if (m_backend == sync_backend::events_v2)
		{
			_vkCmdPipelineBarrier2KHR(cmd, &dependency);
			return;
		}

		const auto src_stages = v1_utils::gather_src_stages(dependency);
		const auto dst_stages = v1_utils::gather_dst_stages(dependency);
		const auto memory_barriers = v1_utils::get_memory_barriers(dependency);
		const auto image_memory_barriers = v1_utils::get_image_memory_barriers(dependency);
		const auto buffer_memory_barriers = v1_utils::get_buffer_memory_barriers(dependency);

		vkCmdPipelineBarrier(cmd, src_stages, dst_stages, dependency.dependencyFlags,
			::size32(memory_barriers), memory_barriers.data(),
			::size32(buffer_memory_barriers), buffer_memory_barriers.data(),
			::size32(image_memory_barriers), image_memory_barriers.data());
	}

	void event::signal(const command_buffer& cmd, const VkDependencyInfoKHR& dependency)
	{
		if (m_backend == sync_backend::gpu_label)
		{
			// Fallback path
			m_label->signal(cmd, dependency);
			return;
		}

		if (m_domain != sync_domain::host)
		{
			// As long as host is not involved, keep things consistent.
			// The expectation is that this will be awaited using the gpu_wait function.
			if (m_backend == sync_backend::events_v2) [[ likely ]]
			{
				_vkCmdSetEvent2KHR(cmd, m_vk_event, &dependency);
			}
			else
			{
				const auto dst_stages = v1_utils::gather_dst_stages(dependency);
				vkCmdSetEvent(cmd, m_vk_event, dst_stages);
			}

			return;
		}

		// Host sync doesn't behave intuitively with events, so we use some workarounds.
		// 1. Resolve the actual dependencies on a pipeline barrier.
		resolve_dependencies(cmd, dependency);

		// 2. Signalling won't wait. The caller is responsible for setting up the dependencies correctly.
		if (m_backend != sync_backend::events_v2)
		{
			vkCmdSetEvent(cmd, m_vk_event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
			return;
		}

		// We need a memory barrier to keep AMDVLK from hanging
		VkMemoryBarrier2KHR mem_barrier =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
			.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR,
			.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT
		};

		// Empty dependency that does nothing
		VkDependencyInfoKHR empty_dependency
		{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
			.memoryBarrierCount = 1,
			.pMemoryBarriers = &mem_barrier
		};

		_vkCmdSetEvent2KHR(cmd, m_vk_event, &empty_dependency);
	}

	void event::host_signal() const
	{
		if (m_backend != sync_backend::gpu_label) [[ likely ]]
		{
			vkSetEvent(*m_device, m_vk_event);
			return;
		}

		m_label->set();
	}

	void event::gpu_wait(const command_buffer& cmd, const VkDependencyInfoKHR& dependency) const
	{
		ensure(m_domain != sync_domain::host);

		if (m_backend == sync_backend::events_v2) [[ likely ]]
		{
			_vkCmdWaitEvents2KHR(cmd, 1, &m_vk_event, &dependency);
			return;
		}

		const auto src_stages = v1_utils::gather_src_stages(dependency);
		const auto dst_stages = v1_utils::gather_dst_stages(dependency);
		const auto memory_barriers = v1_utils::get_memory_barriers(dependency);
		const auto image_memory_barriers = v1_utils::get_image_memory_barriers(dependency);
		const auto buffer_memory_barriers = v1_utils::get_buffer_memory_barriers(dependency);

		vkCmdWaitEvents(cmd,
			1, &m_vk_event,
			src_stages, dst_stages,
			::size32(memory_barriers), memory_barriers.data(),
			::size32(buffer_memory_barriers), buffer_memory_barriers.data(),
			::size32(image_memory_barriers), image_memory_barriers.data());
	}

	void event::reset() const
	{
		if (m_backend != sync_backend::gpu_label) [[ likely ]]
		{
			vkResetEvent(*m_device, m_vk_event);
			return;
		}

		m_label->reset();
	}

	VkResult event::status() const
	{
		if (m_backend != sync_backend::gpu_label) [[ likely ]]
		{
			return vkGetEventStatus(*m_device, m_vk_event);
		}

		return m_label->signaled() ? VK_EVENT_SET : VK_EVENT_RESET;
	}

	gpu_label_pool::gpu_label_pool(const vk::render_device& dev, u32 count)
		: pdev(&dev), m_count(count)
	{}

	gpu_label_pool::~gpu_label_pool()
	{
		if (m_mapped)
		{
			ensure(m_buffer);
			m_buffer->unmap();
		}
	}

	std::tuple<VkBuffer, u64, volatile u32*> gpu_label_pool::allocate()
	{
		if (!m_buffer || m_offset >= m_count)
		{
			create_impl();
		}

		const auto out_offset = m_offset;
		m_offset ++;
		return { m_buffer->value, out_offset * 4, m_mapped + out_offset };
	}

	void gpu_label_pool::create_impl()
	{
		if (m_buffer)
		{
			m_buffer->unmap();
			vk::get_gc()->dispose(m_buffer);
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

	gpu_label::gpu_label(gpu_label_pool& pool)
	{
		std::tie(m_buffer_handle, m_buffer_offset, m_ptr) = pool.allocate();
		reset();
	}

	gpu_label::~gpu_label()
	{
		m_ptr = nullptr;
		m_buffer_offset = 0;
		m_buffer_handle = VK_NULL_HANDLE;
	}

	void gpu_label::signal(const vk::command_buffer& cmd, const VkDependencyInfoKHR& dependency)
	{
		const auto src_stages = v1_utils::gather_src_stages(dependency);
		auto dst_stages = v1_utils::gather_dst_stages(dependency);
		auto memory_barriers = v1_utils::get_memory_barriers(dependency);
		const auto image_memory_barriers = v1_utils::get_image_memory_barriers(dependency);
		const auto buffer_memory_barriers = v1_utils::get_buffer_memory_barriers(dependency);

		// Ensure wait before filling the label
		dst_stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		if (memory_barriers.empty())
		{
			const VkMemoryBarrier signal_barrier =
			{
				.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT
			};
			memory_barriers.push_back(signal_barrier);
		}
		else
		{
			auto& barrier = memory_barriers.front();
			barrier.dstAccessMask |= VK_ACCESS_TRANSFER_WRITE_BIT;
		}

		vkCmdPipelineBarrier(cmd, src_stages, dst_stages, dependency.dependencyFlags,
			::size32(memory_barriers), memory_barriers.data(),
			::size32(buffer_memory_barriers), buffer_memory_barriers.data(),
			::size32(image_memory_barriers), image_memory_barriers.data());

		vkCmdFillBuffer(cmd, m_buffer_handle, m_buffer_offset, 4, label_constants::set_);
	}

	gpu_debug_marker::gpu_debug_marker(gpu_debug_marker_pool& pool, std::string message)
		: gpu_label(pool), m_message(std::move(message))
	{}

	gpu_debug_marker::~gpu_debug_marker()
	{
		if (!m_printed)
		{
			dump();
		}
	}

	void gpu_debug_marker::dump()
	{
		if (*m_ptr == gpu_label::label_constants::reset_)
		{
			rsx_log.error("DEBUG MARKER NOT REACHED: %s", m_message);
		}

		m_printed = true;
	}

	void gpu_debug_marker::dump() const
	{
		if (*m_ptr == gpu_label::label_constants::reset_)
		{
			rsx_log.error("DEBUG MARKER NOT REACHED: %s", m_message);
		}
		else
		{
			rsx_log.error("DEBUG MARKER: %s", m_message);
		}
	}

	void gpu_debug_marker::insert(
		const vk::render_device& dev,
		const vk::command_buffer& cmd,
		std::string message,
		VkPipelineStageFlags stages,
		VkAccessFlags access)
	{
		VkMemoryBarrier2KHR barrier =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
			.srcStageMask = stages,
			.srcAccessMask = access,
			.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT
		};

		VkDependencyInfoKHR dependency =
		{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
			.memoryBarrierCount = 1,
			.pMemoryBarriers = &barrier
		};

		auto result = std::make_unique<gpu_debug_marker>(globals::get_shared_marker_pool(dev), message);
		result->signal(cmd, dependency);
		vk::get_gc()->dispose(result);
	}

	debug_marker_scope::debug_marker_scope(const vk::command_buffer& cmd, const std::string& message)
		: m_device(&cmd.get_command_pool().get_owner()), m_cb(&cmd), m_message(message), m_tag(rsx::get_shared_tag())
	{
		vk::gpu_debug_marker::insert(
			*m_device,
			*m_cb,
			fmt::format("0x%llx: Enter %s", m_tag, m_message)
		);
	}

	debug_marker_scope::~debug_marker_scope()
	{
		ensure(m_cb && m_cb->is_recording());

		vk::gpu_debug_marker::insert(
			*m_device,
			*m_cb,
			fmt::format("0x%x: Exit %s", m_tag, m_message)
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
