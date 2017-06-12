#pragma once
#include "stdafx.h"
#include "VKRenderTargets.h"
#include "VKGSRender.h"
#include "../Common/TextureUtils.h"
#include "../rsx_utils.h"

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
		vk::render_device* m_device = nullptr;
		vk::image *vram_texture = nullptr;
		std::unique_ptr<vk::buffer> dma_buffer;

	public:
	
		cached_texture_section() {}

		void reset(u32 base, u32 length)
		{
			if (length > cpu_address_range)
				release_dma_resources();

			rsx::buffered_section::reset(base, length);
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

				return (width == this->width && height == this->height && mipmaps == this->mipmaps);
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

		void copy_texture(vk::command_buffer& cmd, u32 heap_index, VkQueue submit_queue,
				bool manage_cb_lifetime = false, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
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
			
			change_image_layout(cmd, vram_texture->value, layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);
			vkCmdCopyImageToBuffer(cmd, vram_texture->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dma_buffer->value, 1, &copyRegion);
			change_image_layout(cmd, vram_texture->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, layout, subresource_range);

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

		void flush(vk::render_device& dev, vk::command_buffer& cmd, u32 heap_index, VkQueue submit_queue)
		{
			if (m_device == nullptr)
				m_device = &dev;

			if (!synchronized)
			{
				LOG_WARNING(RSX, "Cache miss at address 0x%X. This is gonna hurt...", cpu_address_base);
				copy_texture(cmd, heap_index, submit_queue, true, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
		}

		bool is_synchronized() const
		{
			return synchronized;
		}
	};

	class texture_cache
	{
	private:
		std::vector<cached_texture_section> m_cache;
		std::pair<u64, u64> texture_cache_range = std::make_pair(0xFFFFFFFF, 0);
		std::vector<std::unique_ptr<vk::image_view> > m_temporary_image_view;
		std::vector<std::unique_ptr<vk::image>> m_dirty_textures;

		cached_texture_section& find_cached_texture(u32 rsx_address, u32 rsx_size, bool confirm_dimensions = false, u16 width = 0, u16 height = 0, u16 mipmaps = 0)
		{
			for (auto &tex : m_cache)
			{
				if (tex.matches(rsx_address, rsx_size) && !tex.is_dirty())
				{
					if (!confirm_dimensions) return tex;

					if (tex.matches(rsx_address, width, height, mipmaps))
						return tex;
					else
					{
						LOG_ERROR(RSX, "Cached object for address 0x%X was found, but it does not match stored parameters.");
						LOG_ERROR(RSX, "%d x %d vs %d x %d", width, height, tex.get_width(), tex.get_height());
					}
				}
			}

			for (auto &tex : m_cache)
			{
				if (tex.is_dirty())
				{
					if (tex.exists())
					{
						m_dirty_textures.push_back(std::move(tex.get_texture()));
						m_temporary_image_view.push_back(std::move(tex.get_view()));
					}

					tex.release_dma_resources();
					return tex;
				}
			}

			m_cache.push_back(cached_texture_section());

			return m_cache[m_cache.size() - 1];
		}

		cached_texture_section* find_flushable_section(const u32 address, const u32 range)
		{
			for (auto &tex : m_cache)
			{
				if (tex.is_dirty()) continue;
				if (!tex.is_flushable() && !tex.is_flushed()) continue;

				if (tex.matches(address, range))
					return &tex;
			}

			return nullptr;
		}

		void purge_cache()
		{
			for (auto &tex : m_cache)
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

			m_temporary_image_view.clear();
			m_dirty_textures.clear();

			m_cache.resize(0);
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

		VkComponentMapping get_component_map(rsx::vertex_texture &tex, u32 gcm_format)
		{
			auto mapping = vk::get_component_mapping(gcm_format);
			return { mapping[1], mapping[2], mapping[3], mapping[0] };
		}

	public:

		texture_cache() {}
		~texture_cache() {}

		void destroy()
		{
			purge_cache();
		}

		template <typename RsxTextureType>
		vk::image_view* upload_texture(command_buffer cmd, RsxTextureType &tex, rsx::vk_render_targets &m_rtts, const vk::memory_type_mapping &memory_type_mapping, vk_data_heap& upload_heap, vk::buffer* upload_buffer)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			const u32 range = (u32)get_texture_size(tex);

			if (!texaddr || !range)
			{
				LOG_ERROR(RSX, "Texture upload requested but texture not found, (address=0x%X, size=0x%X)", texaddr, range);
				return nullptr;
			}

			//First check if it exists as an rtt...
			vk::image *rtt_texture = nullptr;
			if (rtt_texture = m_rtts.get_texture_from_render_target_if_applicable(texaddr))
			{
				m_temporary_image_view.push_back(std::make_unique<vk::image_view>(*vk::get_current_renderer(), rtt_texture->value, VK_IMAGE_VIEW_TYPE_2D, rtt_texture->info.format,
					rtt_texture->native_layout,
					vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT)));
				return m_temporary_image_view.back().get();
			}

			if (rtt_texture = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
			{
				m_temporary_image_view.push_back(std::make_unique<vk::image_view>(*vk::get_current_renderer(), rtt_texture->value, VK_IMAGE_VIEW_TYPE_2D, rtt_texture->info.format,
					rtt_texture->native_layout,
					vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_DEPTH_BIT)));
				return m_temporary_image_view.back().get();
			}

			u32 raw_format = tex.format();
			u32 format = raw_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

			VkComponentMapping mapping = get_component_map(tex, format);
			VkFormat vk_format = get_compatible_sampler_format(format);

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

			cached_texture_section& region = find_cached_texture(texaddr, range, true, tex.width(), height, tex.get_exact_mipmap_count());
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

			vk::image *image = new vk::image(*vk::get_current_renderer(), memory_type_mapping.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image_type,
				vk_format,
				tex.width(), height, depth, tex.get_exact_mipmap_count(), layer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, is_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0);
			change_image_layout(cmd, image->value, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

			vk::image_view *view = new vk::image_view(*vk::get_current_renderer(), image->value, image_view_type, vk_format,
				mapping,
				subresource_range);

			//We cannot split mipmap uploads across multiple command buffers (must explicitly open and close operations on the same cb)
			vk::enter_uninterruptible();

			copy_mipmaped_image_using_buffer(cmd, image->value, get_subresources_layout(tex), format, !(tex.format() & CELL_GCM_TEXTURE_LN), tex.get_exact_mipmap_count(),
				upload_heap, upload_buffer);

			change_image_layout(cmd, image->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);

			vk::leave_uninterruptible();

			region.reset(texaddr, range);
			region.create(tex.width(), height, depth, tex.get_exact_mipmap_count(), view, image);
			region.protect(utils::protection::ro);
			region.set_dirty(false);

			texture_cache_range = region.get_min_max(texture_cache_range);
			return view;
		}

		void lock_memory_region(vk::render_target* image, const u32 memory_address, const u32 memory_size, const u32 width, const u32 height)
		{
			cached_texture_section& region = find_cached_texture(memory_address, memory_size, true, width, height, 1);
			
			if (!region.is_locked())
			{
				region.reset(memory_address, memory_size);
				region.set_dirty(false);
				texture_cache_range = region.get_min_max(texture_cache_range);
			}

			region.protect(utils::protection::no);
			region.create(width, height, 1, 1, nullptr, image, image->native_pitch, false);
		}

		void flush_memory_to_cache(const u32 memory_address, const u32 memory_size, vk::command_buffer&cmd, vk::memory_type_mapping& memory_types, VkQueue submit_queue)
		{
			cached_texture_section* region = find_flushable_section(memory_address, memory_size);
			
			//TODO: Make this an assertion
			if (region == nullptr)
			{
				LOG_ERROR(RSX, "Failed to find section for render target 0x%X + 0x%X", memory_address, memory_size);
				return;
			}

			region->copy_texture(cmd, memory_types.host_visible_coherent, submit_queue);
		}

		std::tuple<bool, bool> address_is_flushable(u32 address)
		{
			if (address < texture_cache_range.first ||
				address > texture_cache_range.second)
				return std::make_tuple(false, false);

			for (auto &tex : m_cache)
			{
				if (tex.is_dirty()) continue;
				if (!tex.is_flushable()) continue;

				if (tex.overlaps(address))
					return std::make_tuple(true, tex.is_synchronized());
			}

			return std::make_tuple(false, false);
		}

		bool flush_address(u32 address, vk::render_device& dev, vk::command_buffer& cmd, vk::memory_type_mapping& memory_types, VkQueue submit_queue)
		{
			if (address < texture_cache_range.first ||
				address > texture_cache_range.second)
				return false;

			bool response = false;
			std::pair<u32, u32> trampled_range = std::make_pair(0xffffffff, 0x0);

			for (int i = 0; i < m_cache.size(); ++i)
			{
				auto &tex = m_cache[i];

				if (tex.is_dirty()) continue;
				if (!tex.is_flushable()) continue;

				auto overlapped = tex.overlaps_page(trampled_range, address);
				if (std::get<0>(overlapped))
				{
					auto &new_range = std::get<1>(overlapped);

					if (new_range.first != trampled_range.first ||
						new_range.second != trampled_range.second)
					{
						trampled_range = new_range;
						i = 0;
					}

					//TODO: Map basic host_visible memory without coherent constraint
					tex.flush(dev, cmd, memory_types.host_visible_coherent, submit_queue);
					response = true;
				}
			}

			return response;
		}

		bool invalidate_address(u32 address)
		{
			if (address < texture_cache_range.first ||
				address > texture_cache_range.second)
				return false;

			bool response = false;
			std::pair<u32, u32> trampled_range = std::make_pair(0xffffffff, 0x0);

			for (int i = 0; i < m_cache.size(); ++i)
			{
				auto &tex = m_cache[i];

				if (tex.is_dirty()) continue;
				if (!tex.is_locked()) continue;	//flushable sections can be 'clean' but unlocked. TODO: Handle this better

				auto overlapped = tex.overlaps_page(trampled_range, address);
				if (std::get<0>(overlapped))
				{
					auto &new_range = std::get<1>(overlapped);

					if (new_range.first != trampled_range.first ||
						new_range.second != trampled_range.second)
					{
						trampled_range = new_range;
						i = 0;
					}

					tex.set_dirty(true);
					tex.unprotect();

					response = true;
				}
			}

			return response;
		}

		void flush()
		{
			m_dirty_textures.clear();
			m_temporary_image_view.clear();
		}
	};
}
