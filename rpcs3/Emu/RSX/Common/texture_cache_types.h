#pragma once

#include "Emu/system_config.h"

namespace rsx
{
	/**
	 * Helper enums/structs
	 */
	enum invalidation_chain_policy
	{
		invalidation_chain_none,         // No chaining: Only sections that overlap the faulting page get invalidated.
		invalidation_chain_full,         // Full chaining: Sections overlapping the faulting page get invalidated, as well as any sections overlapping invalidated sections.
		invalidation_chain_nearby        // Invalidations chain if they are near to the fault (<X pages away)
	};

	enum invalidation_chain_direction
	{
		chain_direction_both,
		chain_direction_forward,  // Only higher-base-address sections chain (unless they overlap the fault)
		chain_direction_backward, // Only lower-base-address pages chain (unless they overlap the fault)
	};

	enum class component_order
	{
		default_ = 0,
		native = 1,
		swapped_native = 2,
	};

	enum memory_read_flags
	{
		flush_always = 0,
		flush_once = 1
	};

	struct invalidation_cause
	{
		enum enum_type
		{
			invalid = 0,
			read,
			deferred_read,
			write,
			deferred_write,
			unmap, // fault range is being unmapped
			reprotect, // we are going to reprotect the fault range
			superseded_by_fbo, // used by texture_cache::locked_memory_region
			committed_as_fbo   // same as superseded_by_fbo but without locking or preserving page flags
		} cause;

		constexpr bool valid() const
		{
			return cause != invalid;
		}

		constexpr bool is_read() const
		{
			AUDIT(valid());
			return (cause == read || cause == deferred_read);
		}

		constexpr bool deferred_flush() const
		{
			AUDIT(valid());
			return (cause == deferred_read || cause == deferred_write);
		}

		constexpr bool destroy_fault_range() const
		{
			AUDIT(valid());
			return (cause == unmap);
		}

		constexpr bool keep_fault_range_protection() const
		{
			AUDIT(valid());
			return (cause == unmap || cause == reprotect || cause == superseded_by_fbo);
		}

		constexpr bool skip_fbos() const
		{
			AUDIT(valid());
			return (cause == superseded_by_fbo || cause == committed_as_fbo);
		}

		constexpr bool skip_flush() const
		{
			AUDIT(valid());
			return (cause == unmap) || (!g_cfg.video.strict_texture_flushing && cause == superseded_by_fbo);
		}

		constexpr invalidation_cause undefer() const
		{
			AUDIT(deferred_flush());
			if (cause == deferred_read)
				return read;
			if (cause == deferred_write)
				return write;
			fmt::throw_exception("Unreachable");
		}

		constexpr invalidation_cause defer() const
		{
			AUDIT(!deferred_flush());
			if (cause == read)
				return deferred_read;
			if (cause == write)
				return deferred_write;
			fmt::throw_exception("Unreachable");
		}

		constexpr invalidation_cause() : cause(invalid) {}
		constexpr invalidation_cause(enum_type _cause) : cause(_cause) {}
		operator enum_type&() { return cause; }
		constexpr operator enum_type() const { return cause; }
	};
}
