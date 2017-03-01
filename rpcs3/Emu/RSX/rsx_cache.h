#pragma once
#include <rsx_decompiler.h>
#include "Emu/Memory/vm.h"

namespace rsx
{
	struct shader_info
	{
		decompiled_shader *decompiled;
		complete_shader *complete;
	};

	struct program_info
	{
		shader_info vertex_shader;
		shader_info fragment_shader;

		void *program;
	};

	struct program_cache_context
	{
		decompile_language lang;

		void*(*compile_shader)(program_type type, const std::string &code);
		rsx::complete_shader(*complete_shader)(const decompiled_shader &shader, program_state state);
		void*(*make_program)(const void *vertex_shader, const void *fragment_shader);
		void(*remove_program)(void *ptr);
		void(*remove_shader)(void *ptr);
	};

	class shaders_cache
	{
		struct entry_t
		{
			std::int64_t index;
			decompiled_shader decompiled;
			std::unordered_map<program_state, complete_shader, hasher> complete;
		};

		std::unordered_map<raw_shader, entry_t, hasher> m_entries;
		std::string m_path;
		std::int64_t m_index = -1;

	public:
		void path(const std::string &path_);

		shader_info get(const program_cache_context &ctxt, raw_shader &raw_shader, const program_state& state);
		void clear(const program_cache_context& context);
	};

	class programs_cache
	{
		std::unordered_map<raw_program, program_info, hasher> m_program_cache;

		shaders_cache m_vertex_shaders_cache;
		shaders_cache m_fragment_shader_cache;

	public:
		program_cache_context context;

		programs_cache();
		~programs_cache();

		program_info get(raw_program raw_program_, decompile_language lang);
		void clear();
	};

	class buffered_section
	{
	protected:
		u32 cpu_address_base = 0;
		u32 cpu_address_range = 0;

		u32 locked_address_base = 0;
		u32 locked_address_range = 0;

		u32 memory_protection = 0;

		bool locked = false;
		bool dirty = false;

		bool region_overlaps(u32 base1, u32 limit1, u32 base2, u32 limit2)
		{
			//Check for memory area overlap. unlock page(s) if needed and add this index to array.
			//Axis separation test
			const u32 &block_start = base1;
			const u32 block_end = limit1;

			if (limit2 < block_start) return false;
			if (base2 > block_end) return false;

			u32 min_separation = (limit2 - base2) + (limit1 - base1);
			u32 range_limit = (block_end > limit2) ? block_end : limit2;
			u32 range_base = (block_start < base2) ? block_start : base2;

			u32 actual_separation = (range_limit - range_base);

			if (actual_separation < min_separation)
				return true;

			return false;
		}

	public:

		buffered_section() {}
		~buffered_section() {}

		void reset(u32 base, u32 length)
		{
			verify(HERE), locked == false;

			cpu_address_base = base;
			cpu_address_range = length;

			locked_address_base = (base & ~4095);
			locked_address_range = align(base + length, 4096) - locked_address_base;

			memory_protection = vm::page_readable | vm::page_writable;

			locked = false;
		}

		bool protect(u8 flags_set, u8 flags_clear)
		{
			if (vm::page_protect(locked_address_base, locked_address_range, 0, flags_set, flags_clear))
			{
				memory_protection &= ~flags_clear;
				memory_protection |= flags_set;

				locked = memory_protection != (vm::page_readable | vm::page_writable);
			}
			else
				fmt::throw_exception("failed to lock memory @ 0x%X!", locked_address_base);

			return false;
		}

		bool unprotect()
		{
			u32 flags_set = (vm::page_readable | vm::page_writable) & ~memory_protection;

			if (vm::page_protect(locked_address_base, locked_address_range, 0, flags_set, 0))
			{
				memory_protection = (vm::page_writable | vm::page_readable);
				locked = false;
				return true;
			}
			else
				fmt::throw_exception("failed to unlock memory @ 0x%X!", locked_address_base);

			return false;
		}

		bool overlaps(std::pair<u32, u32> range)
		{
			return region_overlaps(locked_address_base, locked_address_base + locked_address_range, range.first, range.first + range.second);
		}

		bool overlaps(u32 address)
		{
			return (locked_address_base <= address && (address - locked_address_base) < locked_address_range);
		}

		bool is_locked() const
		{
			return locked;
		}

		bool is_dirty() const
		{
			return dirty;
		}

		void set_dirty(bool state)
		{
			dirty = state;
		}

		u32 get_section_base() const
		{
			return cpu_address_base;
		}

		u32 get_section_size() const
		{
			return cpu_address_range;
		}

		bool matches(u32 cpu_address, u32 size) const
		{
			return (cpu_address_base == cpu_address && cpu_address_range == size);
		}

		std::pair<u32, u32> get_min_max(std::pair<u32, u32> current_min_max)
		{
			u32 min = std::min(current_min_max.first, locked_address_base);
			u32 max = std::max(current_min_max.second, locked_address_base + locked_address_range);

			return std::make_pair(min, max);
		}
	};
}
