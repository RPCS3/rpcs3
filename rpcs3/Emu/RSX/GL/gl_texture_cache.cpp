#include "stdafx.h"

#include "gl_texture_cache.h"
#include "GLGSRender.h"
#include "../Common/TextureUtils.h"
#include "../rsx_utils.h"

#include <exception>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>

namespace gl
{
	void cached_texture::read()
	{
		cached_texture* found_texture = nullptr;
		u32 texture_size = info->size();

		m_parent_region->for_each(info->start_address, texture_size, [&](cached_texture& texture)
		{
			if ((texture.m_state & cache_entry_state::local_synchronized) == cache_entry_state::invalid)
			{
				return;
			}

			if (texture.info->start_address != info->start_address ||
				texture.info->pitch != info->pitch ||
				texture.info->height < info->height ||
				texture.info->width < info->width)
			{
				return;
			}

			found_texture = &texture;
		});

		if (found_texture)
		{
			//read from local
			__glcheck glCopyImageSubData(
				found_texture->gl_name, (GLenum)found_texture->info->target, 0, 0, 0, 0,
				gl_name, (GLenum)info->target, 0, 0, 0, 0,
				info->width, info->height, info->depth);
		}
		else
		{
			//read from host
			//flush all local textures at region
			m_parent_region->for_each(info->start_address, texture_size, [](cached_texture& texture)
			{
				texture.sync(gl::cache_buffers::host);
			});

			bind();

			if (info->format.format == gl::texture::format::depth || info->format.format == gl::texture::format::depth_stencil)
			{
				gl::buffer pbo_depth;

				__glcheck pbo_depth.create(info->pitch * info->height);
				__glcheck pbo_depth.map([&](GLubyte* pixels)
				{
					switch (info->format.bpp)
					{
					case 2:
					{
						u16 *dst = (u16*)pixels;
						const be_t<u16>* src = (const be_t<u16>*)vm::base_priv(info->start_address);
						for (u32 i = 0, end = info->pitch / info->format.bpp * info->height; i < end; ++i)
						{
							dst[i] = src[i];
						}
					}
					break;

					case 4:
					{
						u32 *dst = (u32*)pixels;
						const be_t<u32>* src = (const be_t<u32>*)vm::base_priv(info->start_address);
						for (u32 i = 0, end = info->pitch / info->format.bpp * info->height; i < end; ++i)
						{
							dst[i] = src[i];
						}
					}
					break;

					default:
						throw EXCEPTION("");
					}
				}, gl::buffer::access::write);

				gl::pixel_unpack_settings{}
					.row_length(info->pitch / info->format.bpp)
					.aligment(1)
					.swap_bytes((info->format.flags & gl::texture_flags::swap_bytes) != gl::texture_flags::none)
					.apply();

				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_depth.id());

				__glcheck glTexSubImage2D((GLenum)info->target, 0, 0, 0, info->width, info->height,
					(GLenum)info->format.format, (GLenum)info->format.type, nullptr);

				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			}
			else if (info->compressed_size)
			{
				__glcheck glCompressedTexSubImage2D((GLenum)info->target, 0,
					0, 0, info->width, info->height,
					(GLenum)info->format.internal_format,
					info->compressed_size, vm::base_priv(info->start_address));
			}
			else
			{
				void *pixels = vm::base_priv(info->start_address);

				std::unique_ptr<u8[]> linear_pixels;

				if (info->swizzled && (info->format.flags & texture_flags::allow_swizzle) != texture_flags::none)
				{
					linear_pixels.reset(new u8[info->size()]);
					switch (info->format.bpp)
					{
					case 1:
						rsx::convert_linear_swizzle<u8>(pixels, linear_pixels.get(), info->width, info->height, true);
						break;
					case 2:
						rsx::convert_linear_swizzle<u16>(pixels, linear_pixels.get(), info->width, info->height, true);
						break;
					case 4:
						rsx::convert_linear_swizzle<u32>(pixels, linear_pixels.get(), info->width, info->height, true);
						break;
					case 8:
						rsx::convert_linear_swizzle<u64>(pixels, linear_pixels.get(), info->width, info->height, true);
						break;

					default:
						throw EXCEPTION("");
					}

					pixels = linear_pixels.get();
				}

				gl::pixel_unpack_settings{}
					.row_length(info->pitch / info->format.bpp)
					.aligment(1)
					.swap_bytes((info->format.flags & gl::texture_flags::swap_bytes) != gl::texture_flags::none)
					.apply();

				__glcheck glTexSubImage2D((GLenum)info->target, 0, 0, 0, info->width, info->height,
					(GLenum)info->format.format, (GLenum)info->format.type, pixels);
			}
		}

		ignore(gl::cache_buffers::all);
	}

	void cached_texture::write()
	{
		bind();

		if (info->format.format == gl::texture::format::depth || info->format.format == gl::texture::format::depth_stencil)
		{
			//LOG_ERROR(RSX, "write depth color to host[0x%x]", info->start_address);

			gl::buffer pbo_depth;

			pbo_depth.create(info->pitch * info->height);

			gl::pixel_pack_settings{}
				.row_length(info->pitch / info->format.bpp)
				.aligment(1)
				.swap_bytes((info->format.flags & gl::texture_flags::swap_bytes) != gl::texture_flags::none)
				.apply();

			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_depth.id());
			__glcheck glGetTexImage((GLenum)info->target, 0, (GLenum)info->format.format, (GLenum)info->format.type, nullptr);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

			__glcheck pbo_depth.map([&](GLubyte* pixels)
			{
				switch (info->format.bpp)
				{
				case 2:
				{
					const u16 *src = (const u16*)pixels;
					be_t<u16>* dst = (be_t<u16>*)vm::base_priv(info->start_address);
					for (u32 i = 0, end = info->pitch / info->format.bpp * info->height; i < end; ++i)
					{
						dst[i] = src[i];
					}
				}
				break;

				case 4:
				{
					const u32 *src = (const u32*)pixels;
					be_t<u32>* dst = (be_t<u32>*)vm::base_priv(info->start_address);
					for (u32 i = 0, end = info->pitch / info->format.bpp * info->height; i < end; ++i)
					{
						dst[i] = src[i];
					}
				}
				break;

				default:
					throw EXCEPTION("");
				}

			}, gl::buffer::access::read);
		}
		else if (info->compressed_size)
		{
			LOG_ERROR(RSX, "writing compressed texture[0x%x] to host buffer", info->start_address);
		}
		else
		{
			if (info->swizzled && (info->format.flags & texture_flags::allow_swizzle) != texture_flags::none)
			{
				//TODO
				LOG_ERROR(RSX, "writing swizzled texture[0x%x] to host buffer", info->start_address);
			}

			gl::pixel_pack_settings{}
				.row_length(info->pitch / info->format.bpp)
				.aligment(1)
				.swap_bytes((info->format.flags & gl::texture_flags::swap_bytes) != gl::texture_flags::none)
				.apply();

			__glcheck glGetTexImage((GLenum)info->target, 0, (GLenum)info->format.format, (GLenum)info->format.type, vm::base_priv(info->start_address));
		}

		ignore(gl::cache_buffers::all);
	}

	bool cached_texture::sync(cache_buffers buffers)
	{
		if (!created())
		{
			__glcheck create();
		}

		switch (m_state)
		{
		case cache_entry_state::invalid:
		case cache_entry_state::host_synchronized:
			if ((buffers & cache_buffers::local) != cache_buffers::none)
			{
				__glcheck read();
				return true;
			}
			break;

		case cache_entry_state::local_synchronized:
			if ((buffers & cache_buffers::host) != cache_buffers::none)
			{
				write();
				return true;
			}
			break;
		}

		return false;
	}

	void cached_texture::invalidate(cache_buffers buffers)
	{
		if ((buffers & cache_buffers::host) != cache_buffers::none)
		{
			m_state &= ~cache_entry_state::host_synchronized;
			m_parent_region->for_each(info->start_address, info->size(), [this](cached_texture& texture)
			{
				if (std::addressof(texture) != this)
				{
					//LOG_WARNING(RSX, "cached_texture[0x%x,0x%x) invalidate cached_texture[0x%x, 0x%x)",
					//	info->start_address, info->start_address + info->size(),
					//	texture.info->start_address, texture.info->start_address + texture.info->size());
					texture.invalidate(cache_buffers::local);
				}
			});
		}

		if ((buffers & cache_buffers::local) != cache_buffers::none)
		{
			m_state &= ~cache_entry_state::local_synchronized;
		}
	}

	void cached_texture::ignore(cache_buffers buffers)
	{
		if ((buffers & cache_buffers::host) != cache_buffers::none)
		{
			m_state |= cache_entry_state::host_synchronized;
		}

		if ((buffers & cache_buffers::local) != cache_buffers::none)
		{
			m_state |= cache_entry_state::local_synchronized;
		}
	}

	void cached_texture::parent(protected_region *region)
	{
		m_parent_region = region;
	}

	bool cached_texture::is_synchronized(cache_buffers buffers) const
	{
		if ((buffers & cache_buffers::host) != cache_buffers::none)
		{
			if ((m_state & cache_entry_state::host_synchronized) == cache_entry_state::invalid)
			{
				return false;
			}
		}

		if ((buffers & cache_buffers::local) != cache_buffers::none)
		{
			if ((m_state & cache_entry_state::local_synchronized) == cache_entry_state::invalid)
			{
				return false;
			}
		}

		return true;
	}

	cache_access cached_texture::requires_protection() const
	{
		switch (m_state)
		{
		case cache_entry_state::local_synchronized:
			return cache_access::read_write;

		case cache_entry_state::synchronized:
			return cache_access::write;
		}

		return cache_access::none;
	}

	void cached_texture::lock()
	{
		m_parent_region->lock();
	}

	void cached_texture::unlock()
	{
		m_parent_region->unlock();
	}

	void cached_texture::bind(uint index) const
	{
		if (index != ~0u)
		{
			__glcheck glActiveTexture(GL_TEXTURE0 + index);
		}

		__glcheck glBindTexture((GLenum)info->target, gl_name);
	}

	void cached_texture::create()
	{
		assert(!created());

		glGenTextures(1, &gl_name);

		bind();
		__glcheck glTexStorage2D((GLenum)info->target, 1, (GLenum)info->format.internal_format, info->width, info->height);
		//__glcheck glClearTexImage(gl_name, 0, (GLenum)info->format.format, (GLenum)info->format.type, nullptr);
	}

	void cached_texture::remove()
	{
		if (created())
		{
			glDeleteTextures(1, &gl_name);
			gl_name = 0;
		}
	}

	inline bool cached_texture::created() const
	{
		return gl_name != 0;
	}

	cache_access protected_region::requires_protection() const
	{
		//TODO
		cache_access result = cache_access::none;

		for (auto &entry : m_textures)
		{
			result |= entry.second.requires_protection();
		}

		return result;
	}

	void protected_region::for_each(std::function<void(cached_texture& texture)> callback)
	{
		for (auto &entry : m_textures)
		{
			callback(entry.second);
		}
	}

	void protected_region::for_each(u32 start_address, u32 size, std::function<void(cached_texture& texture)> callback)
	{
		for (auto &entry : m_textures)
		{
			if (entry.first.start_address >= start_address + size)
			{
				continue;
			}

			if (entry.first.start_address + entry.first.size() <= start_address)
			{
				continue;
			}

			callback(entry.second);
		}
	}

	void protected_region::protect()
	{
		cache_access required_protection = requires_protection();

		u32 flags = 0;
		if ((required_protection & cache_access::read) != cache_access::none)
		{
			flags |= vm::page_readable;
		}

		if ((required_protection & cache_access::write) != cache_access::none)
		{
			flags |= vm::page_writable;
		}

		if (m_current_protection != flags)
		{
			//LOG_WARNING(RSX, "protected region [0x%x, 0x%x)", start_address, start_address + size());
			vm::page_protect(start_address, size(), 0, m_current_protection & ~flags, flags);
			m_current_protection = flags;
		}
	}

	void protected_region::unprotect(cache_access access)
	{
		u32 flags = 0;

		if ((access & cache_access::read) != cache_access::none)
		{
			if (m_current_protection & vm::page_readable)
			{
				flags |= vm::page_readable;
			}
		}

		if ((access & cache_access::write) != cache_access::none)
		{
			if (m_current_protection & vm::page_writable)
			{
				flags |= vm::page_writable;
			}
		}

		//LOG_WARNING(RSX, "unprotected region [0x%x, 0x%x)", start_address, start_address + size());
		vm::page_protect(start_address, size(), 0, flags, 0);
		m_current_protection &= ~flags;
	}

	inline bool protected_region::empty() const
	{
		return m_textures.empty();
	}

	void protected_region::separate(protected_region& dst)
	{
		//TODO
	}

	void protected_region::combine(protected_region& region)
	{
		region.unprotect();
		unprotect();

		for (auto &texture : region.m_textures)
		{
			texture.second.parent(this);
		}

		m_textures.insert(region.m_textures.begin(), region.m_textures.end());

		if (region.start_address < start_address)
		{
			pages_count += (start_address - region.start_address) / vm::page_size;
			start_address = region.start_address;
		}
		else
		{
			pages_count = (region.start_address + region.pages_count - start_address) / vm::page_size;
		}
	}

	cached_texture& protected_region::add(const texture_info& info)
	{
		LOG_WARNING(RSX, "new texture in cache at 0x%x", info.start_address);
		const auto &result = m_textures.emplace(info, cached_texture{});

		if (!result.second)
		{
			throw EXCEPTION("");
		}

		auto& texture_info = *result.first;

		texture_info.second.info = &texture_info.first;
		texture_info.second.parent(this);

		return texture_info.second;
	}

	cached_texture* protected_region::find(const texture_info& info)
	{
		auto it = m_textures.find(info);

		if (it == m_textures.end())
		{
			return nullptr;
		}

		return &it->second;
	}

	void protected_region::clear()
	{
		unprotect();

		for (auto &entry : m_textures)
		{
			entry.second.remove();
		}

		m_textures.clear();
	}

	void protected_region::lock()
	{
		m_mtx.lock();
	}

	void protected_region::unlock()
	{
		m_mtx.unlock();
	}

	cached_texture &texture_cache::entry(const texture_info &info, cache_buffers sync)
	{
		u32 aligned_address;
		u32 aligned_size;

		const bool accurate_cache = false;

		if (accurate_cache)
		{
			aligned_address = info.start_address & ~(vm::page_size - 1);
			aligned_size = align(info.start_address - aligned_address + info.size(), vm::page_size);
		}
		else
		{
			aligned_size = info.size() & ~(vm::page_size - 1);

			if (!aligned_size)
			{
				aligned_address = info.start_address & ~(vm::page_size - 1);
				aligned_size = align(info.size() + info.start_address - aligned_address, vm::page_size);
			}
			else
			{
				aligned_address = align(info.start_address, vm::page_size);
			}
		}

		std::vector<protected_region*> regions = find_regions(aligned_address, aligned_size);
		protected_region *region;

		if (regions.empty())
		{
			region = &m_protected_regions[aligned_address];
			region->pages_count = aligned_size / vm::page_size;
			region->start_address = aligned_address;
		}
		else
		{
			region = regions[0];

			std::vector<u32> remove_addresses;

			for (std::size_t index = 1; index < regions.size(); ++index)
			{
				region->combine(*regions[index]);
				remove_addresses.push_back(regions[index]->start_address);
			}

			for (u32 address : remove_addresses)
			{
				m_protected_regions.erase(address);
			}

			if (region->start_address > aligned_address)
			{
				region->pages_count += (region->start_address - aligned_address) / vm::page_size;
				region->start_address = aligned_address;
			}

			u32 new_pages_count = (aligned_address + aligned_size - region->start_address) / vm::page_size;
			region->pages_count = std::max(region->pages_count, new_pages_count);
		}

		cached_texture *result = region->find(info);

		if (!result)
		{
			result = &region->add(info);
		}

		result->sync(sync);

		return *result;
	}

	protected_region *texture_cache::find_region(u32 address)
	{
		for (auto& entry : m_protected_regions)
		{
			if (entry.first > address)
			{
				break;
			}

			if (address >= entry.first && address < entry.first + entry.second.size())
			{
				return &entry.second;
			}
		}

		return nullptr;
	}

	std::vector<protected_region*> texture_cache::find_regions(u32 address, u32 size)
	{
		std::vector<protected_region *> result;

		for (auto& entry : m_protected_regions)
		{
			if (entry.first >= address + size)
			{
				break;
			}

			if (entry.first + entry.second.size() <= address)
			{
				continue;
			}

			result.push_back(&entry.second);
		}

		return result;
	}

	void texture_cache::update_protection()
	{
		for (auto& entry : m_protected_regions)
		{
			entry.second.protect();
		}
	}

	void texture_cache::clear()
	{
		for (auto& entry : m_protected_regions)
		{
			entry.second.clear();
		}

		m_protected_regions.clear();
	}
}
