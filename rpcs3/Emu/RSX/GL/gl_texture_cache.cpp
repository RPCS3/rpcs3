#include "stdafx.h"

#include "gl_texture_cache.h"
#include "GLGSRender.h"
#include "../Common/TextureUtils.h"

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
		LOG_WARNING(RSX, "cached_texture at 0x%x reading from host buffer", info.start_address);

		cached_texture* found_texture = nullptr;
		u32 texture_size = info.size();

		m_parent_region->for_each(info.start_address, texture_size, [&](cached_texture& texture)
		{
			if ((texture.m_state & cache_entry_state::local_synchronized) == cache_entry_state::invalid)
			{
				return;
			}

			if (texture.info.start_address > info.start_address || texture.info.pitch != info.pitch || texture.info.height < info.height)
			{
				return;
			}

			found_texture = &texture;
		});

		if (found_texture)
		{
			//read from local

			//TODO
		}
		else
		{
			//read from host

			//flush all local textures at region
			m_parent_region->for_each(info.start_address, texture_size, [](cached_texture& texture)
			{
				texture.sync(gl::cache_buffers::host);
			});

			//TODO
		}

		ignore(gl::cache_buffers::all);
	}

	void cached_texture::write()
	{
		LOG_WARNING(RSX, "cached_texture at 0x%x writing to host buffer", info.start_address);

		//TODO

		ignore(gl::cache_buffers::all);
	}

	bool cached_texture::sync(cache_buffers buffers)
	{
		if (!created())
		{
			create();
		}

		switch (m_state)
		{
		case cache_entry_state::invalid:
		case cache_entry_state::host_synchronized:
			if ((buffers & cache_buffers::local) != cache_buffers::none)
			{
				read();
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
			if ((m_state & cache_entry_state::host_synchronized) != cache_entry_state::invalid)
			{
				m_state &= ~cache_entry_state::host_synchronized;

				m_parent_region->m_write_protected++;
			}

			m_parent_region->for_each(info.start_address, info.size(), [this](cached_texture& texture)
			{
				if (std::addressof(texture) != this)
				{
					texture.invalidate(cache_buffers::local);
				}
			});
		}

		if ((buffers & cache_buffers::local) != cache_buffers::none)
		{
			if ((m_state & cache_entry_state::local_synchronized) != cache_entry_state::invalid)
			{
				m_state &= ~cache_entry_state::local_synchronized;

				m_parent_region->m_read_protected++;
			}
		}
	}

	void cached_texture::ignore(cache_buffers buffers)
	{
		if ((buffers & cache_buffers::host) != cache_buffers::none)
		{
			if ((m_state & cache_entry_state::host_synchronized) == cache_entry_state::invalid)
			{
				m_state |= cache_entry_state::host_synchronized;

				m_parent_region->m_write_protected--;
			}
		}

		if ((buffers & cache_buffers::local) != cache_buffers::none)
		{
			if ((m_state & cache_entry_state::local_synchronized) == cache_entry_state::invalid)
			{
				m_state |= cache_entry_state::local_synchronized;

				m_parent_region->m_read_protected--;
			}
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

	void cached_texture::bind(uint index) const
	{
		if (index != ~0u)
		{
			glActiveTexture(GL_TEXTURE0 + index);
		}

		glBindTexture((GLenum)info.target, gl_name);
	}

	void cached_texture::create()
	{
		assert(!created());

		glGenTextures(1, &gl_name);
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
			u32 entry_address = entry.first;
			cached_texture &entry_texture = entry.second;

			if (entry_address > start_address)
			{
				break;
			}

			u32 entry_size = entry_texture.info.size();
			//TODO

			callback(entry_texture);
		}
	}

	void protected_region::protect()
	{
		u32 flags = 0;

		if (m_read_protected)
		{
			flags |= vm::page_readable;
		}

		if (m_write_protected)
		{
			flags |= vm::page_writable;
		}

		if (m_current_protection != flags)
		{
			vm::page_protect(start_address, size(), 0, flags, m_current_protection & ~flags);
			m_current_protection = flags;
		}
	}

	void protected_region::unprotect(cache_access access)
	{
		u32 flags = 0;

		if ((access & cache_access::read) != cache_access::none)
		{
			if (m_read_protected)
			{
				flags |= vm::page_readable;
			}
		}

		if ((access & cache_access::write) != cache_access::none)
		{
			if (m_write_protected)
			{
				flags |= vm::page_writable;
			}
		}

		vm::page_protect(start_address, size(), 0, 0, flags);
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

	void protected_region::combine(const protected_region& region)
	{
		//TODO
	}

	void protected_region::clear()
	{
		unprotect();

		for (auto &entry : m_textures)
		{
			entry.second.remove();
		}

		m_textures.clear();

		m_read_protected = 0;
		m_write_protected = 0;
	}

	cached_texture &texture_cache::entry(texture_info &info, cache_buffers sync_buffers)
	{
		auto region = find_region(info.start_address, info.size());

		if (!region)
		{
			region = &m_protected_regions[info.start_address & ~(vm::page_size - 1)];
			region->pages_count = (info.size() + vm::page_size - 1) / vm::page_size;
		}
		else
		{
			//TODO
		}

		cached_texture *result = nullptr;

		//TODO

		result->sync(sync_buffers);

		region->protect();

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

	protected_region *texture_cache::find_region(u32 address, u32 size)
	{
		for (auto& entry : m_protected_regions)
		{
			if (entry.first > address + size)
			{
				break;
			}

			if (entry.first + entry.second.size() < address)
			{
				continue;
			}

			return &entry.second;
		}

		return nullptr;
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
