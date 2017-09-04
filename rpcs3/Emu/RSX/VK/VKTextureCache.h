#pragma once
#include "stdafx.h"
#include "VKRenderTargets.h"
#include "VKGSRender.h"
#include "Emu/System.h"
#include "../Common/TextureUtils.h"
#include "../rsx_utils.h"
#include "Utilities/mutex.h"

extern u64 get_system_time();

namespace vk
{
	class cached_texture_section : public rsx::buffered_section
	{
		u16 pitch;
		u16 width;
		u16 height;
		u16 depth;
		u16 mipmaps;

		std::unique_ptr<vk::image_view> uploaded_image_view;
		std::unique_ptr<vk::image> managed_texture = nullptr;

		//DMA relevant data
		u16 native_pitch;
		VkFence dma_fence = VK_NULL_HANDLE;
		bool synchronized = false;
		u64 sync_timestamp = 0;
		u64 last_use_timestamp = 0;
		vk::render_device* m_device = nullptr;
		vk::image *vram_texture = nullptr;
		std::unique_ptr<vk::buffer> dma_buffer;

	public:
	
		cached_texture_section() {}

		void reset(u32 base, u32 length)
		{
			if (length > cpu_address_range)
				release_dma_resources();

			rsx::protection_policy policy = g_cfg.video.strict_rendering_mode ? rsx::protection_policy::protect_policy_full_range : rsx::protection_policy::protect_policy_one_page;
			rsx::buffered_section::reset(base, length, policy);
		}

		void create(const u16 w, const u16 h, const u16 depth, const u16 mipmaps, vk::image_view *view, vk::image *image, const u32 native_pitch = 0, bool managed=true)
		{
			width = w;
			height = h;
			this->depth = depth;
			this->mipmaps = mipmaps;

			uploaded_image_view.reset(view);
			vram_texture = image;

			if (managed) managed_texture.reset(image);

			//TODO: Properly compute these values
			this->native_pitch = native_pitch;
			pitch = cpu_address_range / height;

			//Even if we are managing the same vram section, we cannot guarantee contents are static
			//The create method is only invoked when a new mangaged session is required
			synchronized = false;
			sync_timestamp = 0ull;
			last_use_timestamp = get_system_time();
		}

		void release_dma_resources()
		{
			if (dma_buffer.get() != nullptr)
			{
				dma_buffer.reset();

				if (dma_fence != nullptr)
				{
					vkDestroyFence(*m_device, dma_fence, nullptr);
					dma_fence = VK_NULL_HANDLE;
				}
			}
		}

		bool matches(u32 rsx_address, u32 rsx_size) const
		{
			return rsx::buffered_section::matches(rsx_address, rsx_size);
		}

		bool matches(u32 rsx_address, u32 width, u32 height, u32 mipmaps) const
		{
			if (rsx_address == cpu_address_base)
			{
				if (!width && !height && !mipmaps)
					return true;

				if (width && width != this->width)
					return false;

				if (height && height != this->height)
					return false;

				if (mipmaps && mipmaps != this->mipmaps)
					return false;

				return true;
			}

			return false;
		}

		bool exists() const
		{
			return (vram_texture != nullptr);
		}

		u16 get_width() const
		{
			return width;
		}

		u16 get_height() const
		{
			return height;
		}

		std::unique_ptr<vk::image_view>& get_view()
		{
			return uploaded_image_view;
		}

		std::unique_ptr<vk::image>& get_texture()
		{
			return managed_texture;
		}

		VkFormat get_format()
		{
			return vram_texture->info.format;
		}

		bool is_flushable() const
		{
			//This section is active and can be flushed to cpu
			return (protection == utils::protection::no);
		}

		bool is_flushed() const
		{
			//This memory section was flushable, but a flush has already removed protection
			return (protection == utils::protection::rw && uploaded_image_view.get() == nullptr && managed_texture.get() == nullptr);
		}

		void copy_texture(vk::command_buffer& cmd, u32 heap_index, VkQueue submit_queue, bool manage_cb_lifetime = false)
		{
			if (m_device == nullptr)
			{
				m_device = &cmd.get_command_pool().get_owner();
			}

			if (dma_fence == VK_NULL_HANDLE)
			{
				VkFenceCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				vkCreateFence(*m_device, &createInfo, nullptr, &dma_fence);
			}

			if (dma_buffer.get() == nullptr)
			{
				dma_buffer.reset(new vk::buffer(*m_device, align(cpu_address_range, 256), heap_index, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0));
			}

			if (manage_cb_lifetime)
			{
				//cb has to be guaranteed to be in a closed state
				//This function can be called asynchronously
				VkCommandBufferInheritanceInfo inheritance_info = {};
				inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

				VkCommandBufferBeginInfo begin_infos = {};
				begin_infos.pInheritanceInfo = &inheritance_info;
				begin_infos.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin_infos.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				CHECK_RESULT(vkBeginCommandBuffer(cmd, &begin_infos));
			}

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = width;
			copyRegion.bufferImageHeight = height;
			copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
			copyRegion.imageOffset = {};
			copyRegion.imageExtent = {width, height, 1};

			VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			
			VkImageLayout layout = vram_texture->current_layout;
			change_image_layout(cmd, vram_texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);
			vkCmdCopyImageToBuffer(cmd, vram_texture->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dma_buffer->value, 1, &copyRegion);
			change_image_layout(cmd, vram_texture, layout, subresource_range);

			if (manage_cb_lifetime)
			{
				CHECK_RESULT(vkEndCommandBuffer(cmd));

				VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				VkCommandBuffer command_buffer = cmd;

				VkSubmitInfo infos = {};
				infos.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				infos.commandBufferCount = 1;
				infos.pCommandBuffers = &command_buffer;
				infos.pWaitDstStageMask = &pipe_stage_flags;
				infos.pWaitSemaphores = nullptr;
				infos.waitSemaphoreCount = 0;

				CHECK_RESULT(vkQueueSubmit(submit_queue, 1, &infos, dma_fence));

				//Now we need to restart the command-buffer to restore it to the way it was before...
				CHECK_RESULT(vkWaitForFences(*m_device, 1, &dma_fence, VK_TRUE, UINT64_MAX));
				CHECK_RESULT(vkResetCommandBuffer(cmd, 0));
				CHECK_RESULT(vkResetFences(*m_device, 1, &dma_fence));
			}

			synchronized = true;
			sync_timestamp = get_system_time();
		}

		template<typename T>
		void do_memory_transfer(void *pixels_dst, const void *pixels_src)
		{
			if (sizeof(T) == 1)
				memcpy(pixels_dst, pixels_src, cpu_address_range);
			else
			{
				const u32 block_size = width * height;
					
				auto typed_dst = (be_t<T> *)pixels_dst;
				auto typed_src = (T *)pixels_src;

				for (u32 px = 0; px < block_size; ++px)
					typed_dst[px] = typed_src[px];
			}
		}

		bool flush(vk::render_device& dev, vk::command_buffer& cmd, u32 heap_index, VkQueue submit_queue)
		{
			if (m_device == nullptr)
				m_device = &dev;

			// Return false if a flush occured 'late', i.e we had a miss
			bool result = true;

			if (!synchronized)
			{
				LOG_WARNING(RSX, "Cache miss at address 0x%X. This is gonna hurt...", cpu_address_base);
				copy_texture(cmd, heap_index, submit_queue, true);
				result = false;
			}

			protect(utils::protection::rw);

			void* pixels_src = dma_buffer->map(0, cpu_address_range);
			void* pixels_dst = vm::base(cpu_address_base);

			const u8 bpp = native_pitch / width;

			if (pitch == native_pitch)
			{
				//We have to do our own byte swapping since the driver doesnt do it for us
				switch (bpp)
				{
				default:
					LOG_ERROR(RSX, "Invalid bpp %d", bpp);
				case 1:
					do_memory_transfer<u8>(pixels_dst, pixels_src);
					break;
				case 2:
					do_memory_transfer<u16>(pixels_dst, pixels_src);
					break;
				case 4:
					do_memory_transfer<u32>(pixels_dst, pixels_src);
					break;
				case 8:
					do_memory_transfer<u64>(pixels_dst, pixels_src);
					break;
				}
			}
			else
			{
				//Scale image to fit
				//usually we can just get away with nearest filtering
				const u8 samples = pitch / native_pitch;

				rsx::scale_image_nearest(pixels_dst, pixels_src, width, height, pitch, native_pitch, bpp, samples, true);
			}

			dma_buffer->unmap();
			//Its highly likely that this surface will be reused, so we just leave resources in place

			return result;
		}

		bool is_synchronized() const
		{
			return synchronized;
		}

		bool sync_valid() const
		{
			return (sync_timestamp > last_use_timestamp);
		}

		u64 get_sync_timestamp() const
		{
			return sync_timestamp;
		}
	};

	class texture_cache
	{
		struct ranged_storage
		{
			std::vector<cached_texture_section> data;  //Stored data
			std::atomic_int valid_count = { 0 };  //Number of usable (non-dirty) blocks
			u32 max_range = 0;  //Largest stored block

			void notify(u32 data_size)
			{
				max_range = std::max(data_size, max_range);
				valid_count++;
			}

			void add(cached_texture_section& section, u32 data_size)
			{
				max_range = std::max(data_size, max_range);
				valid_count++;

				data.push_back(std::move(section));
			}
		};

	private:
		std::atomic_bool in_access_violation_handler = { false };
		shared_mutex m_cache_mutex;
		std::unordered_map<u32, ranged_storage> m_cache;

		std::pair<u32, u32> read_only_range = std::make_pair(0xFFFFFFFF, 0);
		std::pair<u32, u32> no_access_range = std::make_pair(0xFFFFFFFF, 0);
		
		//Stuff that has been dereferenced goes into these
		std::vector<std::unique_ptr<vk::image_view> > m_temporary_image_view;
		std::vector<std::unique_ptr<vk::image>> m_dirty_textures;

		//Stuff that has been dereferenced twice goes here. Contents are evicted before new ones are added
		std::vector<std::unique_ptr<vk::image_view>> m_image_views_to_purge;
		std::vector<std::unique_ptr<vk::image>> m_images_to_purge;

		// Keep track of cache misses to pre-emptively flush some addresses
		struct framebuffer_memory_characteristics
		{
			u32 misses;
			u32 block_size;
			VkFormat format;
		};

		std::unordered_map<u32, framebuffer_memory_characteristics> m_cache_miss_statistics_table;

		//Memory usage
		const s32 m_max_zombie_objects = 32; //Limit on how many texture objects to keep around for reuse after they are invalidated
		s32 m_unreleased_texture_objects = 0; //Number of invalidated objects not yet freed from memory

		cached_texture_section *find_texture_from_range(u32 rsx_address, u32 range)
		{
			auto test = std::make_pair(rsx_address, range);
			for (auto &address_range : m_cache)
			{
				auto &range_data = address_range.second;
				for (auto &tex : range_data.data)
				{
					if (!tex.is_dirty() && tex.overlaps(test, true))
						return &tex;
				}
			}

			return nullptr;
		}

		cached_texture_section *find_texture_from_dimensions(u32 rsx_address, u32 rsx_size, u16 width = 0, u16 height = 0, u16 mipmaps = 0)
		{
			auto found = m_cache.find(rsx_address);
			if (found != m_cache.end())
			{
				auto &range_data = found->second;
				for (auto &tex : range_data.data)
				{
					if (tex.matches(rsx_address, width, height, mipmaps) && !tex.is_dirty())
					{
						return &tex;
					}
				}
			}

			return nullptr;
		}

		cached_texture_section& find_cached_texture(u32 rsx_address, u32 rsx_size, bool confirm_dimensions = false, u16 width = 0, u16 height = 0, u16 mipmaps = 0)
		{
			{
				reader_lock lock(m_cache_mutex);

				auto found = m_cache.find(rsx_address);
				if (found != m_cache.end())
				{
					auto &range_data = found->second;

					for (auto &tex : range_data.data)
					{
						if (tex.matches(rsx_address, rsx_size) && !tex.is_dirty())
						{
							if (!confirm_dimensions) return tex;

							if (tex.matches(rsx_address, width, height, mipmaps))
								return tex;
							else
							{
								LOG_ERROR(RSX, "Cached object for address 0x%X was found, but it does not match stored parameters.", rsx_address);
								LOG_ERROR(RSX, "%d x %d vs %d x %d", width, height, tex.get_width(), tex.get_height());
							}
						}
					}

					for (auto &tex : range_data.data)
					{
						if (tex.is_dirty())
						{
							if (tex.exists())
							{
								m_unreleased_texture_objects--;

								m_dirty_textures.push_back(std::move(tex.get_texture()));
								m_temporary_image_view.push_back(std::move(tex.get_view()));
							}

							tex.release_dma_resources();
							range_data.notify(rsx_size);
							return tex;
						}
					}
				}
			}

			writer_lock lock(m_cache_mutex);

			cached_texture_section tmp;
			m_cache[rsx_address].add(tmp, rsx_size);
			return m_cache[rsx_address].data.back();
		}

		cached_texture_section* find_flushable_section(const u32 address, const u32 range)
		{
			reader_lock lock(m_cache_mutex);

			auto found = m_cache.find(address);
			if (found != m_cache.end())
			{
				auto &range_data = found->second;
				for (auto &tex : range_data.data)
				{
					if (tex.is_dirty()) continue;
					if (!tex.is_flushable() && !tex.is_flushed()) continue;

					if (tex.matches(address, range))
						return &tex;
				}
			}

			return nullptr;
		}

		void purge_cache()
		{
			for (auto &address_range : m_cache)
			{
				auto &range_data = address_range.second;
				for (auto &tex : range_data.data)
				{
					if (tex.exists())
					{
						m_dirty_textures.push_back(std::move(tex.get_texture()));
						m_temporary_image_view.push_back(std::move(tex.get_view()));
					}

					if (tex.is_locked())
						tex.unprotect();

					tex.release_dma_resources();
				}

				range_data.data.resize(0);
			}

			m_temporary_image_view.clear();
			m_dirty_textures.clear();

			m_image_views_to_purge.clear();
			m_images_to_purge.clear();

			m_unreleased_texture_objects = 0;
		}

		//Helpers
		VkComponentMapping get_component_map(rsx::fragment_texture &tex, u32 gcm_format)
		{
			//Decoded remap returns 2 arrays; a redirection table and a lookup reference
			auto decoded_remap = tex.decoded_remap();

			//NOTE: Returns mapping in A-R-G-B
			auto native_mapping = vk::get_component_mapping(gcm_format);
			VkComponentSwizzle final_mapping[4] = {};

			for (u8 channel = 0; channel < 4; ++channel)
			{
				switch (decoded_remap.second[channel])
				{
				case CELL_GCM_TEXTURE_REMAP_ONE:
					final_mapping[channel] = VK_COMPONENT_SWIZZLE_ONE;
					break;
				case CELL_GCM_TEXTURE_REMAP_ZERO:
					final_mapping[channel] = VK_COMPONENT_SWIZZLE_ZERO;
					break;
				default:
					LOG_ERROR(RSX, "Unknown remap lookup value %d", decoded_remap.second[channel]);
				case CELL_GCM_TEXTURE_REMAP_REMAP:
					final_mapping[channel] = native_mapping[decoded_remap.first[channel]];
					break;
				}
			}

			return { final_mapping[1], final_mapping[2], final_mapping[3], final_mapping[0] };
		}

		VkComponentMapping get_component_map(rsx::vertex_texture&, u32 gcm_format)
		{
			auto mapping = vk::get_component_mapping(gcm_format);
			return { mapping[1], mapping[2], mapping[3], mapping[0] };
		}

		vk::image_view* create_temporary_subresource(vk::command_buffer& cmd, vk::image* source, u32 x, u32 y, u32 w, u32 h, const vk::memory_type_mapping &memory_type_mapping)
		{
			VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

			switch (source->info.format)
			{
			case VK_FORMAT_D16_UNORM:
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			}

			VkImageSubresourceRange subresource_range = { aspect, 0, 1, 0, 1 };

			std::unique_ptr<vk::image> image;
			std::unique_ptr<vk::image_view> view;

			image.reset(new vk::image(*vk::get_current_renderer(), memory_type_mapping.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				source->info.imageType,
				source->info.format,
				source->width(), source->height(), source->depth(), 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, source->info.flags));

			VkImageSubresourceRange view_range = { aspect & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 1, 0, 1 };
			view.reset(new vk::image_view(*vk::get_current_renderer(), image->value, VK_IMAGE_VIEW_TYPE_2D, source->info.format, source->native_component_map, view_range));

			VkImageLayout old_src_layout = source->current_layout;

			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_GENERAL, subresource_range);
			vk::change_image_layout(cmd, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);

			VkImageCopy copy_rgn;
			copy_rgn.srcOffset = { (s32)x, (s32)y, 0 };
			copy_rgn.dstOffset = { (s32)x, (s32)y, 0 };
			copy_rgn.dstSubresource = { aspect, 0, 0, 1 };
			copy_rgn.srcSubresource = { aspect, 0, 0, 1 };
			copy_rgn.extent = { w, h, 1 };

			vkCmdCopyImage(cmd, source->value, source->current_layout, image->value, image->current_layout, 1, &copy_rgn);
			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);
			vk::change_image_layout(cmd, source, old_src_layout, subresource_range);

			m_dirty_textures.push_back(std::move(image));
			m_temporary_image_view.push_back(std::move(view));

			return m_temporary_image_view.back().get();
		}

	public:

		texture_cache() {}
		~texture_cache() {}

		void destroy()
		{
			purge_cache();
		}

		bool is_depth_texture(const u32 texaddr, rsx::vk_render_targets &m_rtts)
		{
			if (m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
				return true;

			reader_lock lock(m_cache_mutex);

			auto found = m_cache.find(texaddr);
			if (found == m_cache.end())
				return false;

			if (found->second.valid_count == 0)
				return false;

			for (auto& tex : found->second.data)
			{
				if (tex.is_dirty())
					continue;

				switch (tex.get_format())
				{
				case VK_FORMAT_D16_UNORM:
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
					return true;
				default:
					return false;
				}
			}

			//Unreachable; silence compiler warning anyway
			return false;
		}

		template <typename RsxTextureType>
		vk::image_view* upload_texture(command_buffer &cmd, RsxTextureType &tex, rsx::vk_render_targets &m_rtts, const vk::memory_type_mapping &memory_type_mapping, vk_data_heap& upload_heap, vk::buffer* upload_buffer)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 range = (u32)get_texture_size(tex);

			if (!texaddr || !range)
			{
				LOG_ERROR(RSX, "Texture upload requested but texture not found, (address=0x%X, size=0x%X)", texaddr, range);
				return nullptr;
			}

			//First check if it exists as an rtt...
			vk::render_target *rtt_texture = nullptr;
			if ((rtt_texture = m_rtts.get_texture_from_render_target_if_applicable(texaddr)))
			{
				if (g_cfg.video.strict_rendering_mode)
				{
					for (const auto& tex : m_rtts.m_bound_render_targets)
					{
						if (std::get<0>(tex) == texaddr)
						{
							LOG_WARNING(RSX, "Attempting to sample a currently bound render target @ 0x%x", texaddr);
							return create_temporary_subresource(cmd, rtt_texture, 0, 0, rtt_texture->width(), rtt_texture->height(), memory_type_mapping);
						}
					}
				}

				return rtt_texture->get_view();
			}

			if ((rtt_texture = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr)))
			{
				if (g_cfg.video.strict_rendering_mode)
				{
					if (std::get<0>(m_rtts.m_bound_depth_stencil) == texaddr)
					{
						LOG_WARNING(RSX, "Attempting to sample a currently bound depth surface @ 0x%x", texaddr);
						return create_temporary_subresource(cmd, rtt_texture, 0, 0, rtt_texture->width(), rtt_texture->height(), memory_type_mapping);
					}
				}

				return rtt_texture->get_view();
			}

			VkImageType image_type;
			VkImageViewType image_view_type;
			u16 height = 0;
			u16 depth = 0;
			u8 layer = 0;

			switch (tex.get_extended_texture_dimension())
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				image_type = VK_IMAGE_TYPE_1D;
				image_view_type = VK_IMAGE_VIEW_TYPE_1D;
				height = 1;
				depth = 1;
				layer = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_2d:
				image_type = VK_IMAGE_TYPE_2D;
				image_view_type = VK_IMAGE_VIEW_TYPE_2D;
				height = tex.height();
				depth = 1;
				layer = 1;
				break;
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				image_type = VK_IMAGE_TYPE_2D;
				image_view_type = VK_IMAGE_VIEW_TYPE_CUBE;
				height = tex.height();
				depth = 1;
				layer = 6;
				break;
			case rsx::texture_dimension_extended::texture_dimension_3d:
				image_type = VK_IMAGE_TYPE_3D;
				image_view_type = VK_IMAGE_VIEW_TYPE_3D;
				height = tex.height();
				depth = tex.depth();
				layer = 1;
				break;
			}

			//Ignoring the mipmaps count is intentional - its common for games to pass in incorrect values as mipmap count
			cached_texture_section& region = find_cached_texture(texaddr, range, true, tex.width(), height, 0);
			if (region.exists() && !region.is_dirty())
			{
				return region.get_view().get();
			}

			bool is_cubemap = tex.get_extended_texture_dimension() == rsx::texture_dimension_extended::texture_dimension_cubemap;
			VkImageSubresourceRange subresource_range = vk::get_image_subresource_range(0, 0, is_cubemap ? 6 : 1, tex.get_exact_mipmap_count(), VK_IMAGE_ASPECT_COLOR_BIT);

			//If for some reason invalid dimensions are requested, fail
			if (!height || !depth || !layer || !tex.width())
			{
				LOG_ERROR(RSX, "Texture upload requested but invalid texture dimensions passed");
				return nullptr;
			}

			u32 raw_format = tex.format();
			u32 format = raw_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

			VkComponentMapping mapping = get_component_map(tex, format);
			VkFormat vk_format = get_compatible_sampler_format(format);

			vk::image *image = new vk::image(*vk::get_current_renderer(), memory_type_mapping.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image_type,
				vk_format,
				tex.width(), height, depth, tex.get_exact_mipmap_count(), layer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, is_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0);
			change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

			vk::image_view *view = new vk::image_view(*vk::get_current_renderer(), image->value, image_view_type, vk_format,
				mapping,
				subresource_range);

			//We cannot split mipmap uploads across multiple command buffers (must explicitly open and close operations on the same cb)
			vk::enter_uninterruptible();

			copy_mipmaped_image_using_buffer(cmd, image->value, get_subresources_layout(tex), format, !(tex.format() & CELL_GCM_TEXTURE_LN), tex.get_exact_mipmap_count(),
				upload_heap, upload_buffer);

			change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);

			vk::leave_uninterruptible();
			writer_lock lock(m_cache_mutex);

			region.reset(texaddr, range);
			region.create(tex.width(), height, depth, tex.get_exact_mipmap_count(), view, image);
			region.protect(utils::protection::ro);
			region.set_dirty(false);

			read_only_range = region.get_min_max(read_only_range);
			return view;
		}

		void lock_memory_region(vk::render_target* image, const u32 memory_address, const u32 memory_size, const u32 width, const u32 height)
		{
			cached_texture_section& region = find_cached_texture(memory_address, memory_size, true, width, height, 1);
			
			writer_lock lock(m_cache_mutex);

			if (!region.is_locked())
			{
				region.reset(memory_address, memory_size);
				region.set_dirty(false);
				no_access_range = region.get_min_max(no_access_range);
			}

			region.protect(utils::protection::no);
			region.create(width, height, 1, 1, nullptr, image, image->native_pitch, false);
		}

		bool flush_memory_to_cache(const u32 memory_address, const u32 memory_size, vk::command_buffer&cmd, vk::memory_type_mapping& memory_types, VkQueue submit_queue, bool skip_synchronized = false)
		{
			cached_texture_section* region = find_flushable_section(memory_address, memory_size);
			
			//TODO: Make this an assertion
			if (region == nullptr)
			{
				LOG_ERROR(RSX, "Failed to find section for render target 0x%X + 0x%X", memory_address, memory_size);
				return false;
			}

			if (skip_synchronized && region->is_synchronized())
				return false;

			region->copy_texture(cmd, memory_types.host_visible_coherent, submit_queue);
			return true;
		}

		std::tuple<bool, bool, u64> address_is_flushable(u32 address)
		{
			if (address < no_access_range.first ||
				address > no_access_range.second)
				return std::make_tuple(false, false, 0ull);

			reader_lock lock(m_cache_mutex);

			auto found = m_cache.find(address);
			if (found != m_cache.end())
			{
				auto &range_data = found->second;
				for (auto &tex : range_data.data)
				{
					if (tex.is_dirty()) continue;
					if (!tex.is_flushable()) continue;

					if (tex.overlaps(address))
						return std::make_tuple(true, tex.is_synchronized(), tex.get_sync_timestamp());
				}
			}

			for (auto &address_range : m_cache)
			{
				if (address_range.first == address)
					continue;

				auto &range_data = address_range.second;
				
				//Quickly discard range
				const u32 lock_base = address_range.first & ~0xfff;
				const u32 lock_limit = align(range_data.max_range + address_range.first, 4096);

				if (address < lock_base || address >= lock_limit)
					continue;

				for (auto &tex : range_data.data)
				{
					if (tex.is_dirty()) continue;
					if (!tex.is_flushable()) continue;

					if (tex.overlaps(address))
						return std::make_tuple(true, tex.is_synchronized(), tex.get_sync_timestamp());
				}
			}

			return std::make_tuple(false, false, 0ull);
		}

		bool flush_address(u32 address, vk::render_device& dev, vk::command_buffer& cmd, vk::memory_type_mapping& memory_types, VkQueue submit_queue)
		{
			if (address < no_access_range.first ||
				address > no_access_range.second)
				return false;

			bool response = false;
			std::pair<u32, u32> trampled_range = std::make_pair(0xffffffff, 0x0);
			std::unordered_map<u32, bool> processed_ranges;

			rsx::conditional_lock<shared_mutex> lock(in_access_violation_handler, m_cache_mutex);

			for (auto It = m_cache.begin(); It != m_cache.end(); It++)
			{
				auto &range_data = It->second;
				const u32 base = It->first;
				bool range_reset = false;

				if (processed_ranges[base] || range_data.valid_count == 0)
					continue;

				//Quickly discard range
				const u32 lock_base = base & ~0xfff;
				const u32 lock_limit = align(range_data.max_range + base, 4096);
				
				if ((trampled_range.first >= lock_limit || lock_base >= trampled_range.second) &&
					(lock_base > address || lock_limit <= address))
				{
					processed_ranges[base] = true;
					continue;
				}

				for (int i = 0; i < range_data.data.size(); i++)
				{
					auto &tex = range_data.data[i];

					if (tex.is_dirty()) continue;
					if (!tex.is_flushable()) continue;

					auto overlapped = tex.overlaps_page(trampled_range, address);
					if (std::get<0>(overlapped))
					{
						auto &new_range = std::get<1>(overlapped);

						if (new_range.first != trampled_range.first ||
							new_range.second != trampled_range.second)
						{
							i = 0;
							trampled_range = new_range;
							range_reset = true;
						}

						//TODO: Map basic host_visible memory without coherent constraint
						if (!tex.flush(dev, cmd, memory_types.host_visible_coherent, submit_queue))
						{
							//Missed address, note this
							//TODO: Lower severity when successful to keep the cache from overworking
							record_cache_miss(tex);
						}

						response = true;
					}
				}

				if (range_reset)
				{
					processed_ranges.clear();
					It = m_cache.begin();
				}

				processed_ranges[base] = true;
			}

			return response;
		}

		bool invalidate_address(u32 address)
		{
			return invalidate_range(address, 4096 - (address & 4095));
		}

		bool invalidate_range(u32 address, u32 range, bool unprotect=true)
		{
			std::pair<u32, u32> trampled_range = std::make_pair(address, address + range);

			if (trampled_range.second < read_only_range.first ||
				trampled_range.first > read_only_range.second)
			{
				//Doesnt fall in the read_only textures range; check render targets
				if (trampled_range.second < no_access_range.first ||
					trampled_range.first > no_access_range.second)
					return false;
			}

			bool response = false;
			std::unordered_map<u32, bool> processed_ranges;

			rsx::conditional_lock<shared_mutex> lock(in_access_violation_handler, m_cache_mutex);

			for (auto It = m_cache.begin(); It != m_cache.end(); It++)
			{
				auto &range_data = It->second;
				const u32 base = It->first;
				bool range_reset = false;

				if (processed_ranges[base] || range_data.valid_count == 0)
					continue;

				//Quickly discard range
				const u32 lock_base = base & ~0xfff;
				const u32 lock_limit = align(range_data.max_range + base, 4096);

				if (trampled_range.first >= lock_limit || lock_base >= trampled_range.second)
				{
					processed_ranges[base] = true;
					continue;
				}

				for (int i = 0; i < range_data.data.size(); i++)
				{
					auto &tex = range_data.data[i];

					if (tex.is_dirty()) continue;
					if (!tex.is_locked()) continue;	//flushable sections can be 'clean' but unlocked. TODO: Handle this better

					auto overlapped = tex.overlaps_page(trampled_range, address);
					if (std::get<0>(overlapped))
					{
						auto &new_range = std::get<1>(overlapped);

						if (new_range.first != trampled_range.first ||
							new_range.second != trampled_range.second)
						{
							i = 0;
							trampled_range = new_range;
							range_reset = true;
						}

						if (unprotect)
						{
							m_unreleased_texture_objects++;

							tex.set_dirty(true);
							tex.unprotect();
						}
						else
						{
							tex.discard();
						}

						range_data.valid_count--;
						response = true;
					}
				}

				if (range_reset)
				{
					processed_ranges.clear();
					It = m_cache.begin();
				}

				processed_ranges[base] = true;
			}

			return response;
		}

		void flush(bool purge_dirty=false)
		{
			if (purge_dirty || m_unreleased_texture_objects >= m_max_zombie_objects)
			{
				//Reclaims all graphics memory consumed by dirty textures
				std::vector<u32> empty_addresses;
				empty_addresses.resize(32);

				for (auto &address_range : m_cache)
				{
					auto &range_data = address_range.second;

					if (range_data.valid_count == 0)
						empty_addresses.push_back(address_range.first);

					for (auto &tex : range_data.data)
					{
						if (!tex.is_dirty())
							continue;

						if (tex.exists())
						{
							m_dirty_textures.push_back(std::move(tex.get_texture()));
							m_temporary_image_view.push_back(std::move(tex.get_view()));
						}

						tex.release_dma_resources();
					}
				}

				//Free descriptor objects as well
				for (const auto &address : empty_addresses)
				{
					m_cache.erase(address);
				}
			}

			m_image_views_to_purge.clear();
			m_images_to_purge.clear();

			m_image_views_to_purge = std::move(m_temporary_image_view);
			m_images_to_purge = std::move(m_dirty_textures);
		}

		void record_cache_miss(cached_texture_section &tex)
		{
			const u32 memory_address = tex.get_section_base();
			const u32 memory_size = tex.get_section_size();
			const VkFormat fmt = tex.get_format();

			auto It = m_cache_miss_statistics_table.find(memory_address);
			if (It == m_cache_miss_statistics_table.end())
			{
				m_cache_miss_statistics_table[memory_address] = { 1, memory_size, fmt };
				return;
			}

			auto &value = It->second;
			if (value.format != fmt || value.block_size != memory_size)
			{
				m_cache_miss_statistics_table[memory_address] = { 1, memory_size, fmt };
				return;
			}

			value.misses++;
		}

		void flush_if_cache_miss_likely(const VkFormat fmt, const u32 memory_address, const u32 memory_size, vk::command_buffer& cmd, vk::memory_type_mapping& memory_types, VkQueue submit_queue)
		{
			auto It = m_cache_miss_statistics_table.find(memory_address);
			if (It == m_cache_miss_statistics_table.end())
			{
				m_cache_miss_statistics_table[memory_address] = { 0, memory_size, fmt };
				return;
			}

			auto &value = It->second;

			if (value.format != fmt || value.block_size != memory_size)
			{
				//Reset since the data has changed
				//TODO: Keep track of all this information together
				m_cache_miss_statistics_table[memory_address] = { 0, memory_size, fmt };
				return;
			}

			//Properly synchronized - no miss
			if (!value.misses) return;

			//Auto flush if this address keeps missing (not properly synchronized)
			if (value.misses > 16)
			{
				//TODO: Determine better way of setting threshold
				if (!flush_memory_to_cache(memory_address, memory_size, cmd, memory_types, submit_queue, true))
					value.misses --;
			}
		}

		bool upload_scaled_image(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate,
				vk::render_device& dev, vk::command_buffer& cmd, vk::memory_type_mapping& memory_types, VkQueue submit_queue,
				rsx::vk_render_targets &m_rtts, vk_data_heap &upload_heap, vk::buffer* upload_buffer)
		{
			//Since we will have dst in vram, we can 'safely' ignore the swizzle flag
			//TODO: Verify correct behavior

			bool src_is_render_target = false;
			bool dst_is_render_target = false;
			bool dst_is_argb8 = (dst.format == rsx::blit_engine::transfer_destination_format::a8r8g8b8);
			bool src_is_argb8 = (src.format == rsx::blit_engine::transfer_source_format::a8r8g8b8);

			const VkComponentMapping rgba_map = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			const VkComponentMapping bgra_map = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };

			auto dest_mapping = (!dst_is_argb8 || dst.swizzled) ? bgra_map : rgba_map;

			vk::image* vram_texture = nullptr;
			vk::image* dest_texture = nullptr;
			cached_texture_section* cached_dest = nullptr;

			const u32 src_address = (u32)((u64)src.pixels - (u64)vm::base(0));
			const u32 dst_address = (u32)((u64)dst.pixels - (u64)vm::base(0));

			//Check if src/dst are parts of render targets
			auto dst_subres = m_rtts.get_surface_subresource_if_applicable(dst_address, dst.width, dst.clip_height, dst.pitch, true, true, false);
			dst_is_render_target = dst_subres.surface != nullptr;

			u16 max_dst_width = dst.width;
			u16 max_dst_height = dst.height;

			//Prepare areas and offsets
			//Copy from [src.offset_x, src.offset_y] a region of [clip.width, clip.height]
			//Stretch onto [dst.offset_x, y] with clipping performed on the source region
			//The implementation here adds the inverse scaled clip dimensions onto the source to completely bypass final clipping step

			float scale_x = (f32)dst.width / src.width;
			float scale_y = (f32)dst.height / src.height;

			//Clip offset is unused if the clip offsets are reprojected onto the source
			position2i clip_offset = { 0, 0 };//{ dst.clip_x, dst.clip_y };
			position2i dst_offset = { dst.offset_x, dst.offset_y };

			size2i clip_dimensions = { dst.clip_width, dst.clip_height };
			//Dimensions passed are restricted to powers of 2; get real height from clip_height and width from pitch
			const size2i dst_dimensions = { dst.pitch / (dst_is_argb8 ? 4 : 2), dst.clip_height };

			//Offset in x and y for src is 0 (it is already accounted for when getting pixels_src)
			//Reproject final clip onto source...
			const u16 src_w = (const u16)((f32)clip_dimensions.width / scale_x);
			const u16 src_h = (const u16)((f32)clip_dimensions.height / scale_y);

			areai src_area = { 0, 0, src_w, src_h };
			areai dst_area = { 0, 0, dst.clip_width, dst.clip_height };

			//If destination is neither a render target nor an existing texture in VRAM
			//its possible that this method is being used to perform a memcpy into RSX memory, so we check
			//parameters. Whenever a simple memcpy can get the job done, use it instead.
			//Dai-3-ji Super Robot Taisen for example uses this to copy program code to GPU RAM

			bool is_memcpy = false;
			u32 memcpy_bytes_length = 0;
			if (dst_is_argb8 == src_is_argb8 && !dst.swizzled)
			{
				if ((src.slice_h == 1 && dst.clip_height == 1) ||
					(dst.clip_width == src.width && dst.clip_height == src.slice_h && src.pitch == dst.pitch))
				{
					const u8 bpp = dst_is_argb8 ? 4 : 2;
					is_memcpy = true;
					memcpy_bytes_length = dst.clip_width * bpp * dst.clip_height;
				}
			}

			if (!dst_is_render_target)
			{
				//First check if this surface exists in VRAM with exact dimensions
				//Since scaled GPU resources are not invalidated by the CPU, we need to reuse older surfaces if possible
				cached_dest = find_texture_from_dimensions(dst.rsx_address, dst.pitch * dst.clip_height, dst_dimensions.width, dst_dimensions.height);

				//Check for any available region that will fit this one
				if (!cached_dest) cached_dest = find_texture_from_range(dst.rsx_address, dst.pitch * dst.clip_height);

				if (cached_dest)
				{
					//TODO: Verify that the new surface will fit
					dest_texture = cached_dest->get_texture().get();

					//TODO: Move this code into utils since it is used alot
					const u32 address_offset = dst.rsx_address - cached_dest->get_section_base();

					const u16 bpp = dst_is_argb8 ? 4 : 2;
					const u16 offset_y = address_offset / dst.pitch;
					const u16 offset_x = address_offset % dst.pitch;

					dst_offset.x += offset_x / bpp;
					dst_offset.y += offset_y;

					max_dst_width = cached_dest->get_width();
					max_dst_height = cached_dest->get_height();

					//If dest has a component swizzle (usually caused by ARGB->BGRA compatibility when uploading from cpu) remove it
					auto& image_view = cached_dest->get_view();

					if (image_view->info.components.a != dest_mapping.a ||
						image_view->info.components.r != dest_mapping.r ||
						image_view->info.components.g != dest_mapping.g ||
						image_view->info.components.b != dest_mapping.b)
					{
						auto create_info = image_view->info;
						create_info.components = dest_mapping;

						m_temporary_image_view.push_back(std::move(image_view));
						image_view.reset(new vk::image_view(dev, create_info));
					}
				}
				else if (is_memcpy)
				{
					memcpy(dst.pixels, src.pixels, memcpy_bytes_length);
					return true;
				}
			}
			else
			{
				dst_offset.x = dst_subres.x;
				dst_offset.y = dst_subres.y;

				dest_texture = dst_subres.surface;

				max_dst_width = dst_subres.surface->width();
				max_dst_height = dst_subres.surface->height();

				if (is_memcpy)
				{
					//Some render target descriptions are actually invalid
					//Confirm this is a flushable RTT
					const auto rsx_pitch = dst_subres.surface->rsx_pitch;
					const auto native_pitch = dst_subres.surface->native_pitch;

					if (rsx_pitch <= 64 && native_pitch != rsx_pitch)
					{
						memcpy(dst.pixels, src.pixels, memcpy_bytes_length);
						return true;
					}
				}
			}

			//TODO: Handle cases where src or dst can be a depth texture while the other is a color texture - requires a render pass to emulate
			auto src_subres = m_rtts.get_surface_subresource_if_applicable(src_address, src.width, src.height, src.pitch, true, true, false);
			src_is_render_target = src_subres.surface != nullptr;

			//Create source texture if does not exist
			if (!src_is_render_target)
			{
				auto preloaded_texture = find_texture_from_dimensions(src_address, src.pitch * src.slice_h, src.width, src.slice_h);

				if (preloaded_texture != nullptr)
				{
					vram_texture = preloaded_texture->get_texture().get();
				}
				else
				{
					flush_address(src_address, dev, cmd, memory_types, submit_queue);

					const VkFormat src_vk_format = src_is_argb8 ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R5G6B5_UNORM_PACK16;
					const VkComponentMapping component_mapping = (!src_is_argb8 || dst.swizzled) ? bgra_map : rgba_map;

					//Upload texture from CPU
					vk::image *image = new vk::image(*vk::get_current_renderer(), memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						VK_IMAGE_TYPE_2D,
						src_vk_format,
						src.width, src.slice_h, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);

					vk::image_view *view = new vk::image_view(*vk::get_current_renderer(), image->value, VK_IMAGE_VIEW_TYPE_2D, src_vk_format,
						component_mapping, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

					change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

					cached_texture_section& region = find_cached_texture(src_address, src.pitch * src.slice_h, true, src.width, src.slice_h, 1);

					writer_lock lock(m_cache_mutex);
					region.reset(src_address, src.pitch * src.slice_h);
					region.create(src.width, src.slice_h, 1, 1, view, image);
					region.protect(utils::protection::ro);
					region.set_dirty(false);

					read_only_range = region.get_min_max(read_only_range);

					vk::enter_uninterruptible();

					std::vector<rsx_subresource_layout> layout(1);
					auto &subres = layout.back();
					subres.width_in_block = src.width;
					subres.height_in_block = src.slice_h;
					subres.pitch_in_bytes = src.width; //Seems to be a typo - should be pitch_in_block
					subres.depth = 1;
					subres.data = {(const gsl::byte*)src.pixels, align(src.pitch, 256) * src.slice_h};

					copy_mipmaped_image_using_buffer(cmd, image->value, layout, src_is_argb8? CELL_GCM_TEXTURE_A8R8G8B8: CELL_GCM_TEXTURE_R5G6B5,
						false, 1, upload_heap, upload_buffer);

					vk::leave_uninterruptible();

					vram_texture = image;
				}
			}
			else
			{
				if (src_subres.w != clip_dimensions.width ||
					src_subres.h != clip_dimensions.height)
				{
					f32 subres_scaling_x = (f32)src.pitch / src_subres.surface->native_pitch;

					dst_area.x2 = (int)(src_subres.w * scale_x * subres_scaling_x);
					dst_area.y2 = (int)(src_subres.h * scale_y);
				}

				src_area.x2 = src_subres.w;
				src_area.y2 = src_subres.h;

				src_area.x1 += src_subres.x;
				src_area.x2 += src_subres.x;
				src_area.y1 += src_subres.y;
				src_area.y2 += src_subres.y;

				vram_texture = src_subres.surface;
			}

			VkImageAspectFlags aspect_to_copy = VK_IMAGE_ASPECT_COLOR_BIT;
			bool dest_exists = dest_texture != nullptr;
			VkFormat dst_vk_format = dst_is_argb8 ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R5G6B5_UNORM_PACK16;
			const u8 bpp = dst_is_argb8 ? 4 : 2;
			const u32 real_width = dst.pitch / bpp;

			//If src is depth, dest has to be depth as well
			if (src_subres.is_depth_surface)
			{
				if (dest_exists)
				{
					if (dst_is_render_target && !dst_subres.is_depth_surface)
					{
						LOG_ERROR(RSX, "Depth->RGBA blit requested but not supported");
						return true;
					}

					if (!dst_is_render_target)
					{
						if (dest_texture->info.format != src_subres.surface->info.format)
						{
							invalidate_range(cached_dest->get_section_base(), cached_dest->get_section_size());
							
							dest_exists = false;
							cached_dest = nullptr;
						}
					}
					else
					{
						if (dst_subres.surface->info.format != src_subres.surface->info.format)
						{
							LOG_ERROR(RSX, "Depth blit requested, but formats do not match (0x%X vs 0x%X)",
								(u32)dst_subres.surface->info.format, (u32)src_subres.surface->info.format);
							return true;
						}
					}
				}
				
				dst_vk_format = src_subres.surface->info.format;
				dest_mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };

				if (dst_vk_format == VK_FORMAT_D16_UNORM)
					aspect_to_copy = VK_IMAGE_ASPECT_DEPTH_BIT;
				else
					aspect_to_copy = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			//Validate clip offsets (Persona 4 Arena at 720p)
			//Check if can fit
			//NOTE: It is possible that the check is simpler (if (clip_x >= clip_width))
			//Needs verification
			if ((dst.offset_x + dst.clip_x + dst.clip_width) > max_dst_width) dst.clip_x = 0;
			if ((dst.offset_y + dst.clip_y + dst.clip_height) > max_dst_height) dst.clip_y = 0;

			if (dst.clip_x || dst.clip_y)
			{
				//Reproject clip offsets onto source
				const u16 scaled_clip_offset_x = (const u16)((f32)dst.clip_x / scale_x);
				const u16 scaled_clip_offset_y = (const u16)((f32)dst.clip_y / scale_y);

				src_area.x1 += scaled_clip_offset_x;
				src_area.x2 += scaled_clip_offset_x;
				src_area.y1 += scaled_clip_offset_y;
				src_area.y2 += scaled_clip_offset_y;
			}

			if (!dest_exists)
			{
				dest_texture = new vk::image(*vk::get_current_renderer(), memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					VK_IMAGE_TYPE_2D,
					dst_vk_format,
					real_width, dst.clip_height, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);

				change_image_layout(cmd, dest_texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { aspect_to_copy, 0, 1, 0, 1});
			}

			//Copy data
			u32 src_width = src_area.x2 - src_area.x1;
			u32 src_height = src_area.y2 - src_area.y1;
			u32 dst_width = dst_area.x2 - dst_area.x1;
			u32 dst_height = dst_area.y2 - dst_area.y1;

			if (dst.clip_width != dst_width ||
				dst.clip_height != dst_height)
			{
				//clip reproject
				src_width = (src_width * dst.clip_width) / dst_width;
				src_height = (src_height * dst.clip_height) / dst_height;
			}

			copy_scaled_image(cmd, vram_texture->value, dest_texture->value, vram_texture->current_layout, dest_texture->current_layout,
					src_area.x1, src_area.y1, src_width, src_height, dst_offset.x, dst_offset.y, dst.clip_width, dst.clip_height, 1, (VkImageAspectFlagBits)aspect_to_copy);

			if (dest_exists)
				return true;

			change_image_layout(cmd, dest_texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, { aspect_to_copy, 0, 1, 0, 1 });

			//TODO: Verify if any titles ever scale into CPU memory. It defeats the purpose of uploading data to the GPU, but it could happen
			//If so, add this texture to the no_access queue not the read_only queue
			cached_texture_section& region = find_cached_texture(dst.rsx_address, dst.pitch * dst.clip_height, true, real_width, dst.clip_height, 1);
			writer_lock lock(m_cache_mutex);

			//These textures are completely GPU resident so we dont watch for CPU access
			//There's no data to be fetched from the CPU
			//Its is possible for a title to attempt to read from the region, but the CPU path should be used in such cases

			vk::image_view *view = new vk::image_view(*vk::get_current_renderer(), dest_texture->value, VK_IMAGE_VIEW_TYPE_2D, dst_vk_format,
					dest_mapping, { aspect_to_copy & ~(VK_IMAGE_ASPECT_STENCIL_BIT), 0, 1, 0, 1 });

			region.reset(dst.rsx_address, dst.pitch * dst.clip_height);
			region.create(real_width, dst.clip_height, 1, 1, view, dest_texture);
			region.protect(utils::protection::ro);
			region.set_dirty(false);

			read_only_range = region.get_min_max(read_only_range);

			return true;
		}
	};
}
