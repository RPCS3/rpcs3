#pragma once
#include <vector>
#include "Utilities/types.h"
#include "gl_helpers.h"
#include "rsx_gl_texture.h"

namespace gl
{
	class gl_texture_cache
	{
	public:
		struct gl_cached_texture
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
			u32 copy_glid;
			u32 data_addr;
			u32 block_sz;

			bool is_dirty;
			bool is_depth;
			bool valid;

			u32 current_width;
			u32 current_height;

			bool locked;
			cached_rtt() : valid(false) {}
		};

	private:
		std::vector<gl_cached_texture> texture_cache;
		std::vector<cached_rtt> rtt_cache;
		u32 frame_ctr;

		bool lock_memory_region(u32 start, u32 size);
		bool unlock_memory_region(u32 start, u32 size);
		void lock_gl_object(gl_cached_texture &obj);
		void unlock_gl_object(gl_cached_texture &obj);

		gl_cached_texture *find_obj_for_params(u64 texaddr, u32 w, u32 h, u16 mipmap);
		gl_cached_texture& create_obj_for_params(u32 gl_id, u64 texaddr, u32 w, u32 h, u16 mipmap);

		void remove_obj(gl_cached_texture &tex);
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
		gl_texture_cache();
		~gl_texture_cache();

		void update_frame_ctr();
		void initialize_rtt_cache();
		void upload_texture(int index, rsx::texture &tex, rsx::gl::texture &gl_texture);
		bool mark_as_dirty(u32 address);
		void save_render_target(u32 texaddr, u32 range, gl::texture &gl_texture);
		std::vector<invalid_cache_area> find_and_invalidate_in_range(u32 base, u32 limit);
		void lock_invalidated_ranges(std::vector<invalid_cache_area> invalid);
		void remove_in_range(u32 texaddr, u32 range);
		bool explicit_writeback(gl::texture &tex, const u32 address, const u32 pitch);
	};
}
