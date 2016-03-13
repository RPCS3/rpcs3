#pragma once
#include <vector>
#include "Utilities/types.h"
#include "gl_helpers.h"

namespace gl
{
	enum class cache_access
	{
		none,
		read = 1 << 0,
		write = 1 << 1,
		read_write = read | write,
	};

	enum class cache_buffers
	{
		none = 0,
		host = 1 << 0,
		local = 1 << 1,
		all = host | local,
	};

	enum class cache_entry_state
	{
		invalid = 0,
		local_synchronized = 1 << 0,
		host_synchronized = 1 << 1,
		synchronized = local_synchronized | host_synchronized,
	};

	enum class texture_flags
	{
		none = 0,
		swap_bytes = 1 << 0,
		allow_remap = 1 << 1,
		allow_swizzle = 1 << 2
	};

	DECLARE_ENUM_CLASS_BITWISE_OPERATORS(cache_access);
	DECLARE_ENUM_CLASS_BITWISE_OPERATORS(cache_buffers);
	DECLARE_ENUM_CLASS_BITWISE_OPERATORS(cache_entry_state);
	DECLARE_ENUM_CLASS_BITWISE_OPERATORS(texture_flags);

	struct texture_format
	{
		u8 bpp;
		std::array<GLint, 4> remap;
		texture::sized_internal_format internal_format;
		texture::format format;
		texture::type type;
		texture_flags flags;
	};

	struct texture_info
	{
		u32 width;
		u32 height;
		u32 depth;
		u32 pitch;
		u32 dimension;
		u32 compressed_size;

		texture::target target;
		texture_format format;
		u8 mipmap;
		u8 min_lod;
		u8 max_lod;
		bool swizzled;
		float lod_bias;
		u32 start_address;

		u32 size() const
		{
			return compressed_size ? compressed_size : height * pitch * depth;
		}
	};

	struct protected_region;

	struct cached_texture
	{
		texture_info info;
		GLuint gl_name = 0;

	private:
		cache_entry_state m_state = cache_entry_state::host_synchronized;
		protected_region *m_parent_region = nullptr;

	private:
		void read();
		void write();

	public:
		//Can be called from RSX thread only
		bool sync(cache_buffers buffers);
		void invalidate(cache_buffers buffers);
		void ignore(cache_buffers buffers);
		void parent(protected_region *region);
		bool is_synchronized(cache_buffers buffers) const;

		void bind(uint index = ~0u) const;

		gl::texture_view view() const
		{
			return{ info.target, gl_name };
		}

		cache_access requires_protection() const;

	protected:
		void create();
		void remove();
		bool created() const;

		friend protected_region;
	};

	struct protected_region
	{
		u32 start_address;
		u32 pages_count;

	private:
		std::unordered_map<texture_info, cached_texture, fnv_1a_hasher, bitwise_equals> m_textures;
		u32 m_current_protection = 0;

	public:
		u32 size() const
		{
			return pages_count * vm::page_size;
		}

		cache_access requires_protection() const;
		void for_each(std::function<void(cached_texture& texture)> callback);
		void for_each(u32 start_address, u32 size, std::function<void(cached_texture& texture)> callback);
		void protect();
		void unprotect(cache_access access = cache_access::read_write);
		bool empty() const;

		void separate(protected_region& dst);
		void combine(protected_region& region);

		cached_texture* find(const texture_info& info);
		cached_texture& add(const texture_info& info);

		void clear();
	};

	class texture_cache
	{
		std::list<protected_region> m_protected_regions;

	public:
		cached_texture &entry(const texture_info &info, cache_buffers sync = cache_buffers::none);
		protected_region *find_region(u32 address);
		std::vector<std::list<protected_region>::iterator> find_regions(u32 address, u32 size);
		void update_protection();
		void clear();
	};
}
