#pragma once
#include <vector>
#include "Utilities/types.h"
#include "gl_helpers.h"
#include "rsx_gl_texture.h"

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
	};

	enum class cache_entry_state
	{
		invalidated = 0,
		local_synchronized = 1 << 0,
		host_synchronized = 1 << 1,
		synchronized = local_synchronized | host_synchronized,
	};

	DECLARE_ENUM_CLASS_BITWISE_OPERATORS(cache_access);
	DECLARE_ENUM_CLASS_BITWISE_OPERATORS(cache_buffers);
	DECLARE_ENUM_CLASS_BITWISE_OPERATORS(cache_entry_state);

	struct cached_texture
	{
		u32 gl_id;
		u32 w;
		u32 h;
		u64 data_addr;
		u32 block_sz;
		u32 frame_ctr;
		u32 protected_block_start;
		u32 protected_block_sz;
		u16 mipmap;
		bool deleted;
		bool locked;
	};

	struct invalid_cache_area
	{
		u32 block_base;
		u32 block_sz;
	};

	struct cached_rtt
	{
		bool valid = false;
		bool locked;
		bool is_dirty;
		bool is_depth;
		u32 copy_glid;
		u32 data_addr;
		u32 block_sz;
		u32 current_width;
		u32 current_height;
	};

	class texture_cache
	{
		std::vector<cached_texture> m_texture_cache;
		std::vector<cached_rtt> m_rtt_cache;
		u32 m_frame_ctr = 0;

	public:
		~texture_cache();

	private:
		bool lock_memory_region(u32 start, u32 size);
		bool unlock_memory_region(u32 start, u32 size);
		void lock_gl_object(cached_texture &obj);
		void unlock_gl_object(cached_texture &obj);

		cached_texture *find_obj_for_params(u64 texaddr, u32 w, u32 h, u16 mipmap);
		cached_texture& create_obj_for_params(u32 gl_id, u64 texaddr, u32 w, u32 h, u16 mipmap);

		void remove_obj(cached_texture &tex);
		void remove_obj_for_glid(u32 gl_id);
		void clear_obj_cache();
		bool region_overlaps(u32 base1, u32 limit1, u32 base2, u32 limit2);
		cached_rtt* find_cached_rtt(u32 base, u32 size);
		void invalidate_rtts_in_range(u32 base, u32 size);
		void prep_rtt(cached_rtt &rtt, u32 width, u32 height, u32 gl_pixel_format_internal);
		void save_rtt(u32 base, u32 size, u32 width, u32 height, u32 gl_pixel_format_internal, gl::texture &source);
		void write_rtt(u32 base, u32 size, u32 texaddr);
		void destroy_rtt_cache();

	public:
		void update_frame_ctr();
		void initialize_rtt_cache();
		void upload_texture(int index, rsx::texture &tex, rsx::gl::texture &gl_texture);
		bool mark_local_as_dirty_at(u32 address);
		void save_render_target(u32 texaddr, u32 range, gl::texture &gl_texture);
		std::vector<invalid_cache_area> find_and_invalidate_in_range(u32 base, u32 limit);
		void lock_invalidated_ranges(const std::vector<invalid_cache_area> &invalid);
		void remove_in_range(u32 texaddr, u32 range);
		bool explicit_writeback(gl::texture &tex, const u32 address, const u32 pitch);

		bool sync_at(cache_buffers buffer, u32 address);
	};
}
