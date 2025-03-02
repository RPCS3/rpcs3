#include "stdafx.h"
#include "texture_cache_types.h"
#include "Emu/system_config.h"

namespace rsx
{
	void invalidation_cause::flag_bits_from_cause(enum_type cause)
	{
		constexpr const std::array s_lookup_table
		{
			std::make_pair<enum_type, u32>(enum_type::read, flags::cause_is_read),
			std::make_pair<enum_type, u32>(enum_type::deferred_read, flags::cause_is_read | flags::cause_is_deferred),
			std::make_pair<enum_type, u32>(enum_type::write, flags::cause_is_write),
			std::make_pair<enum_type, u32>(enum_type::deferred_write, flags::cause_is_write | flags::cause_is_deferred),
			std::make_pair<enum_type, u32>(enum_type::unmap, flags::cause_keeps_fault_range_protection | flags::cause_skips_flush),
			std::make_pair<enum_type, u32>(enum_type::reprotect, flags::cause_keeps_fault_range_protection),
			std::make_pair<enum_type, u32>(enum_type::superseded_by_fbo, flags::cause_keeps_fault_range_protection | flags::cause_skips_fbos | flags::cause_skips_flush),
			std::make_pair<enum_type, u32>(enum_type::committed_as_fbo, flags::cause_skips_fbos),
		};

		m_flag_bits = 0;
		for (const auto& entry : s_lookup_table)
		{
			if (entry.first == cause)
			{
				m_flag_bits = entry.second | flags::cause_is_valid;
				break;
			}
		}

		if (cause == enum_type::superseded_by_fbo &&
			g_cfg.video.strict_texture_flushing) [[ unlikely ]]
		{
			m_flag_bits &= ~flags::cause_skips_flush;
		}
	}
}
