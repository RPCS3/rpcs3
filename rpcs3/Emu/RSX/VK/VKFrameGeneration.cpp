#include "stdafx.h"
#include "VKFrameGeneration.h"
#include "vkutils/device.h"

#include "lsfg/lsfg_chain.hpp"
#include "lsfg/lsfg_pacer.hpp"
#include "lsfg/lsfg_shaders.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <mutex>

namespace vk
{
	namespace
	{
		std::mutex g_frame_generation_lock;
		std::string g_frame_generation_cache;
		frame_generation_settings g_frame_generation_settings;
		frame_generation_status g_frame_generation_status;

		std::atomic<u64> g_frame_generation_presented{0};
		std::atomic<u64> g_frame_generation_generated{0};
		std::atomic<u64> g_frame_generation_revision{0};
		std::atomic<s32> g_frame_generation_refresh_mhz{0};

		constexpr u64 required_history_frames = 2;
		constexpr u32 recurrence_frames = 2;
		constexpr u64 telemetry_interval = 120;

		constexpr u64 acquire_timeout_min_ns = 3'000'000;
		constexpr u64 acquire_timeout_max_ns = 12'000'000;

		constexpr float flow_scale_min = 0.25f;
		constexpr float flow_scale_max = 1.f;
		constexpr float flow_scale_steps = 20.f;

		VkImageMemoryBarrier make_barrier(VkImage image, VkAccessFlags src_access, VkAccessFlags dst_access,
			VkImageLayout old_layout, VkImageLayout new_layout)
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcAccessMask = src_access;
			barrier.dstAccessMask = dst_access;
			barrier.oldLayout = old_layout;
			barrier.newLayout = new_layout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			return barrier;
		}
	}

	void set_frame_generation_shader_cache(std::string path)
	{
		std::lock_guard lock(g_frame_generation_lock);

		if (g_frame_generation_cache != path)
		{
			g_frame_generation_cache = std::move(path);
			g_frame_generation_revision.fetch_add(1, std::memory_order_relaxed);
		}
	}

	std::string get_frame_generation_shader_cache()
	{
		std::lock_guard lock(g_frame_generation_lock);
		return g_frame_generation_cache;
	}

	u64 frame_generation_shader_revision()
	{
		return g_frame_generation_revision.load(std::memory_order_relaxed);
	}

	void invalidate_frame_generation_shaders()
	{
		g_frame_generation_revision.fetch_add(1, std::memory_order_relaxed);
	}

	void set_frame_generation_settings(const frame_generation_settings& settings)
	{
		std::lock_guard lock(g_frame_generation_lock);
		g_frame_generation_settings = settings;
		g_frame_generation_settings.multiplier = std::clamp<u32>(settings.multiplier, 2, max_generated_frames + 1);
		g_frame_generation_settings.flow_scale_percent = std::clamp<u32>(settings.flow_scale_percent, 25, 100);
	}

	frame_generation_settings get_frame_generation_settings()
	{
		std::lock_guard lock(g_frame_generation_lock);
		return g_frame_generation_settings;
	}

	void set_frame_generation_refresh_rate(float hz)
	{
		const s32 mhz = hz > 0.f ? static_cast<s32>(hz * 1000.f + 0.5f) : 0;
		g_frame_generation_refresh_mhz.store(mhz, std::memory_order_relaxed);
	}

	float frame_generation_refresh_rate()
	{
		const s32 mhz = g_frame_generation_refresh_mhz.load(std::memory_order_relaxed);
		return mhz > 0 ? static_cast<float>(mhz) / 1000.f : 0.f;
	}

	u64 frame_generation_acquire_timeout()
	{
		const float refresh = frame_generation_refresh_rate();

		if (refresh <= 1.f)
		{
			return acquire_timeout_min_ns;
		}

		const u64 period = static_cast<u64>(1'000'000'000.f / refresh);
		return std::clamp(period, acquire_timeout_min_ns, acquire_timeout_max_ns);
	}

	u32 frame_generation_reserved_images()
	{
		const auto settings = get_frame_generation_settings();

		if (!settings.enabled || get_frame_generation_shader_cache().empty())
		{
			return 0;
		}

		if (settings.target_rate != 0)
		{
			return max_generated_frames;
		}

		return std::min<u32>(settings.multiplier - 1, max_generated_frames);
	}

	u64 frame_generation_presented_count()
	{
		return g_frame_generation_presented.load(std::memory_order_relaxed);
	}

	u64 frame_generation_generated_count()
	{
		return g_frame_generation_generated.load(std::memory_order_relaxed);
	}

	void frame_generation_count_presented(u32 frames)
	{
		g_frame_generation_presented.fetch_add(frames, std::memory_order_relaxed);
	}

	void frame_generation_count_generated(u32 frames)
	{
		g_frame_generation_generated.fetch_add(frames, std::memory_order_relaxed);
	}

	frame_generation_status get_frame_generation_status()
	{
		std::lock_guard lock(g_frame_generation_lock);
		return g_frame_generation_status;
	}

	void set_frame_generation_status(const frame_generation_status& status)
	{
		std::lock_guard lock(g_frame_generation_lock);
		g_frame_generation_status = status;
	}

	struct frame_generator::impl
	{
		lsfg::Device device;
		std::unique_ptr<lsfg::LsfgShaders> shaders;
		std::unique_ptr<lsfg::LsfgChain> chain;
		std::array<lsfg::LsfgImage, max_generated_frames> targets;
		lsfg::LsfgPacer pacer;
		lsfg::LsfgPlan plan{};

		VkExtent2D built_extent{};
		VkExtent2D peak_guest_extent{};
		VkFormat built_format = VK_FORMAT_UNDEFINED;
		float built_flow_scale = 0.f;

		u64 frame_count = 0;
		u64 last_frame = 0;
		u64 plan_calls = 0;
		usz last_generations = 0;
		u32 warm_streak = 0;
		bool warm = false;
		bool generating = false;

		float effective_flow_scale(const frame_generation_settings& settings, u32 output_width) const
		{
			const float preset = std::clamp(settings.flow_scale_percent / 100.f, flow_scale_min, flow_scale_max);

			if (!peak_guest_extent.width || !output_width)
			{
				return preset;
			}

			const float ratio = static_cast<float>(peak_guest_extent.width) / static_cast<float>(output_width);
			const float stepped = std::ceil(ratio * flow_scale_steps) / flow_scale_steps;
			return std::clamp(std::min(stepped, preset), flow_scale_min, flow_scale_max);
		}
	};

	frame_generator::frame_generator(const vk::render_device& dev)
		: m_device(dev)
	{
		const std::string cache = get_frame_generation_shader_cache();

		if (cache.empty())
		{
			m_unavailable = true;
			return;
		}

		m_impl = std::make_unique<impl>();
		m_impl->device = lsfg::Device(dev, dev.gpu());
		m_impl->shaders = std::make_unique<lsfg::LsfgShaders>(m_impl->device, cache);

		if (!m_impl->shaders->IsValid())
		{
			rsx_log.warning("Frame generation: shader cache at '%s' did not yield a complete chain", cache);
			m_impl.reset();
			m_unavailable = true;
			return;
		}

		m_shaders_ready = true;
		rsx_log.notice("Frame generation: shaders ready");
	}

	frame_generator::~frame_generator()
	{
		for (VkSemaphore sema : m_acquire_semaphores)
		{
			VK_GET_SYMBOL(vkDestroySemaphore)(m_device, sema, nullptr);
		}

		for (VkSemaphore sema : m_present_semaphores)
		{
			if (sema != VK_NULL_HANDLE)
			{
				VK_GET_SYMBOL(vkDestroySemaphore)(m_device, sema, nullptr);
			}
		}

		m_impl.reset();
	}

	VkSemaphore frame_generator::create_semaphore() const
	{
		VkSemaphoreCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkSemaphore result = VK_NULL_HANDLE;
		CHECK_RESULT(VK_GET_SYMBOL(vkCreateSemaphore)(m_device, &info, nullptr, &result));
		return result;
	}

	VkSemaphore frame_generator::next_acquire_semaphore()
	{
		constexpr usz ring_size = max_generated_frames * 3;

		if (m_acquire_semaphores.size() < ring_size)
		{
			m_acquire_semaphores.push_back(create_semaphore());
		}

		const VkSemaphore result = m_acquire_semaphores[m_acquire_cursor % m_acquire_semaphores.size()];
		m_acquire_cursor++;
		return result;
	}

	VkSemaphore frame_generator::present_semaphore(u32 image_index)
	{
		if (image_index >= m_present_semaphores.size())
		{
			m_present_semaphores.resize(image_index + 1, VK_NULL_HANDLE);
		}

		if (m_present_semaphores[image_index] == VK_NULL_HANDLE)
		{
			m_present_semaphores[image_index] = create_semaphore();
		}

		return m_present_semaphores[image_index];
	}

	void frame_generator::set_guest_extent(u32 width, u32 height)
	{
		if (!m_impl || !width || !height)
		{
			return;
		}

		m_impl->peak_guest_extent.width = std::max(m_impl->peak_guest_extent.width, width);
		m_impl->peak_guest_extent.height = std::max(m_impl->peak_guest_extent.height, height);
	}

	bool frame_generator::prepare(u32 width, u32 height)
	{
		constexpr VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

		if (!is_usable() || !width || !height)
		{
			return false;
		}

		const auto settings = get_frame_generation_settings();
		const float flow_scale = m_impl->effective_flow_scale(settings, width);

		lsfg::LsfgPacerConfig pacer_config;
		pacer_config.multiplier = settings.multiplier;
		pacer_config.target_rate = settings.target_rate;
		pacer_config.refresh_rate = frame_generation_refresh_rate();
		m_impl->pacer.SetConfig(pacer_config);

		if (m_impl->chain &&
			m_impl->built_extent.width == width &&
			m_impl->built_extent.height == height &&
			m_impl->built_format == format &&
			m_impl->built_flow_scale == flow_scale)
		{
			return m_impl->chain->Valid();
		}

		m_impl->chain.reset();

		for (auto& target : m_impl->targets)
		{
			target = lsfg::LsfgImage();
		}

		m_impl->chain = std::make_unique<lsfg::LsfgChain>(m_impl->device, *m_impl->shaders,
			VkExtent2D{ width, height }, format, flow_scale);

		if (!m_impl->chain->Valid())
		{
			rsx_log.error("Frame generation: chain build failed at %ux%u; disabling", width, height);
			m_impl->chain.reset();
			m_unavailable = true;
			set_frame_generation_status({ .ready = false, .unsupported = true, .width = width, .height = height });
			return false;
		}

		for (auto& target : m_impl->targets)
		{
			target = lsfg::LsfgImage(m_impl->device, VkExtent2D{ width, height }, format);

			if (!target.Valid())
			{
				rsx_log.error("Frame generation: could not allocate an interpolation target at %ux%u", width, height);
				m_impl->chain.reset();
				m_unavailable = true;
				set_frame_generation_status({ .ready = false, .unsupported = true, .width = width, .height = height });
				return false;
			}
		}

		m_impl->built_extent = VkExtent2D{ width, height };
		m_impl->built_format = format;
		m_impl->built_flow_scale = flow_scale;
		m_impl->frame_count = 0;
		m_impl->plan_calls = 0;
		m_impl->warm_streak = 0;
		m_impl->warm = false;
		m_impl->generating = false;
		m_impl->pacer.Reset();

		const u32 flow_width = static_cast<u32>(width * flow_scale);
		const u32 flow_height = static_cast<u32>(height * flow_scale);

		set_frame_generation_status({ .ready = true, .unsupported = false, .width = width, .height = height,
			.flow_width = flow_width, .flow_height = flow_height,
			.guest_width = m_impl->peak_guest_extent.width, .guest_height = m_impl->peak_guest_extent.height });
		rsx_log.notice("Frame generation: chain built at %ux%u, motion at %ux%u scale %.2f (preset %.2f, game outputs %ux%u)",
			width, height, flow_width, flow_height, flow_scale, settings.flow_scale_percent / 100.f,
			m_impl->peak_guest_extent.width, m_impl->peak_guest_extent.height);
		return true;
	}

	u32 frame_generator::plan(u32 capacity)
	{
		if (!is_usable() || !m_impl->chain)
		{
			return 0;
		}

		const float refresh_rate = frame_generation_refresh_rate();

		if (lsfg::LsfgPacerConfig config = m_impl->pacer.Config(); config.refresh_rate != refresh_rate)
		{
			config.refresh_rate = refresh_rate;
			m_impl->pacer.SetConfig(config);
		}

		m_impl->plan = m_impl->pacer.Plan(std::min<usz>(capacity, max_generated_frames), m_impl->frame_count);

		m_impl->warm = m_impl->plan.warm && (m_impl->frame_count + 1) >= required_history_frames;
		m_impl->warm_streak = m_impl->warm ? m_impl->warm_streak + 1 : 0;
		m_impl->generating = m_impl->warm && m_impl->warm_streak >= recurrence_frames && m_impl->plan.generations > 0;

		if ((m_impl->plan_calls++ % telemetry_interval) == 0)
		{
			const lsfg::LsfgPacerStats stats = m_impl->pacer.Stats();
			const float wanted = stats.source_rate * static_cast<float>(m_impl->plan.generations + 1);
			rsx_log.notice("Frame generation: gen=%zu max=%zu cap=%u guest=%.1f loop=%.1f refresh=%.1f target=%.0f "
				"slots=%.2f needs=%.1fHz%s%s",
				m_impl->plan.generations, m_impl->pacer.MaxGenerations(), capacity, stats.source_rate, stats.loop_rate,
				stats.refresh_rate, stats.target_rate, stats.slots, wanted,
				(stats.refresh_rate > 0.f && wanted > stats.refresh_rate + 1.f) ? " PANEL-BOUND" : "",
				stats.rates_settled ? (m_impl->warm ? "" : " cold") : " sampling");
		}

		return m_impl->generating ? static_cast<u32>(m_impl->plan.generations) : 0;
	}

	void frame_generator::process(VkCommandBuffer cmd, VkImage source, VkImageLayout source_layout, u32 width, u32 height, u32 generations)
	{
		if (!is_usable() || !m_impl->chain || !m_impl->chain->Valid())
		{
			return;
		}

		const u64 count = m_impl->frame_count++;
		m_impl->last_frame = count;
		m_impl->last_generations = generations;

		auto& destination = m_impl->chain->Input(count);

		const VkImageMemoryBarrier before[] =
		{
			make_barrier(source, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT, source_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
			make_barrier(destination.Handle(), VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
				destination.Layout(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		};

		VK_GET_SYMBOL(vkCmdPipelineBarrier)(cmd,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, before);

		VkImageCopy region{};
		region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.extent = { width, height, 1 };

		VK_GET_SYMBOL(vkCmdCopyImage)(cmd, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			destination.Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		const VkImageMemoryBarrier after[] =
		{
			make_barrier(source, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, source_layout),
			make_barrier(destination.Handle(), VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL)
		};

		VK_GET_SYMBOL(vkCmdPipelineBarrier)(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 2, after);

		destination.SetLayout(VK_IMAGE_LAYOUT_GENERAL);

		if (m_impl->warm)
		{
			m_impl->chain->DispatchShared(cmd, count);
		}
	}

	void frame_generator::emit(VkCommandBuffer cmd, u32 index, VkImage target, VkImageLayout present_layout, u32 width, u32 height)
	{
		if (!is_usable() || !m_impl->chain || !m_impl->chain->Valid() || index >= m_impl->last_generations)
		{
			return;
		}

		auto& surface = m_impl->targets[index];

		m_impl->chain->SetTarget(m_impl->device, m_impl->last_generations, index, index, surface.View());
		m_impl->chain->DispatchGeneration(cmd, m_impl->last_frame, m_impl->last_generations, index, index,
			surface.Handle(), VkExtent2D{ width, height });
		surface.SetLayout(VK_IMAGE_LAYOUT_GENERAL);

		const VkImageMemoryBarrier before = make_barrier(target, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VK_GET_SYMBOL(vkCmdPipelineBarrier)(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &before);

		VkImageCopy region{};
		region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.extent = { width, height, 1 };

		VK_GET_SYMBOL(vkCmdCopyImage)(cmd, surface.Handle(), VK_IMAGE_LAYOUT_GENERAL,
			target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		const VkImageMemoryBarrier after = make_barrier(target, VK_ACCESS_TRANSFER_WRITE_BIT, 0,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, present_layout);

		VK_GET_SYMBOL(vkCmdPipelineBarrier)(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, nullptr, 0, nullptr, 1, &after);
	}

	void frame_generator::reset()
	{
		if (!m_impl)
		{
			return;
		}

		m_impl->pacer.Reset();
		m_impl->peak_guest_extent = VkExtent2D{};
		m_impl->warm_streak = 0;
		m_impl->warm = false;
		m_impl->generating = false;
		m_impl->plan = {};

		if (m_impl->chain)
		{
			m_impl->chain->ForgetTargets();
		}
	}
}
