#pragma once

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

	class invalidation_cause
	{
	public:
		enum class enum_type
		{
			invalid = 0,
			read,
			deferred_read,
			write,
			deferred_write,
			unmap,             // fault range is being unmapped
			reprotect,         // we are going to reprotect the fault range
			superseded_by_fbo, // used by texture_cache::locked_memory_region
			committed_as_fbo   // same as superseded_by_fbo but without locking or preserving page flags
		};

		enum flags : u32
		{
			cause_is_valid    = (1 << 0),
			cause_is_read     = (1 << 1),
			cause_is_write    = (1 << 2),
			cause_is_deferred = (1 << 3),
			cause_skips_fbos  = (1 << 4),
			cause_skips_flush = (1 << 5),
			cause_keeps_fault_range_protection = (1 << 6),
			cause_uses_strict_data_bounds      = (1 << 7),
		};

		using enum enum_type;

		constexpr bool valid() const
		{
			return m_flag_bits & flags::cause_is_valid;
		}

		constexpr bool is_read() const
		{
			AUDIT(valid());
			return m_flag_bits & flags::cause_is_read;
		}

		constexpr bool deferred_flush() const
		{
			AUDIT(valid());
			return m_flag_bits & flags::cause_is_deferred;
		}

		constexpr bool keep_fault_range_protection() const
		{
			AUDIT(valid());
			return m_flag_bits & flags::cause_keeps_fault_range_protection;
		}

		constexpr bool skip_fbos() const
		{
			AUDIT(valid());
			return m_flag_bits & flags::cause_skips_fbos;
		}

		constexpr bool skip_flush() const
		{
			AUDIT(valid());
			return m_flag_bits & flags::cause_skips_flush;
		}

		constexpr bool use_strict_data_bounds() const
		{
			AUDIT(valid());
			return m_flag_bits & flags::cause_uses_strict_data_bounds;
		}

		inline invalidation_cause undefer() const
		{
			ensure(m_flag_bits & cause_is_deferred);
			return invalidation_cause(m_flag_bits & ~cause_is_deferred);
		}

		inline invalidation_cause defer() const
		{
			ensure(!(m_flag_bits & cause_is_deferred));
			return invalidation_cause(m_flag_bits | cause_is_deferred);
		}

		inline bool operator == (const invalidation_cause& other) const
		{
			return m_flag_bits == other.m_flag_bits;
		}

		invalidation_cause() = default;
		invalidation_cause(enum_type cause) { flag_bits_from_cause(cause); }
		invalidation_cause(u32 flag_bits)
			: m_flag_bits(flag_bits | flags::cause_is_valid) // FIXME: Actual validation
		{}

	private:
		u32 m_flag_bits = 0;

		void flag_bits_from_cause(enum_type cause);
	};
}
