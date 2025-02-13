#include "stdafx.h"
#include "RSXDisplay.h"

#include "../Common/simple_array.hpp"
#include "../rsx_utils.h"

namespace rsx
{
	std::string framebuffer_dimensions_t::to_string(bool skip_aa_suffix) const
	{
		std::string suffix = "";
		const auto spp = samples_x * samples_y;

		if (!skip_aa_suffix && spp > 1)
		{
			suffix = std::string(" @MSAA ") + std::to_string(spp) + "x";
		}

		return std::to_string(width) + "x" + std::to_string(height) + suffix;
	}

	framebuffer_dimensions_t framebuffer_dimensions_t::make(u16 width, u16 height, rsx::surface_antialiasing aa)
	{
		framebuffer_dimensions_t result { .width = width, .height = height };
		switch (aa)
		{
		case rsx::surface_antialiasing::center_1_sample:
			result.samples_x = result.samples_y = 1;
			break;
		case rsx::surface_antialiasing::diagonal_centered_2_samples:
			result.samples_x = 2;
			result.samples_y = 1;
			break;
		case rsx::surface_antialiasing::square_centered_4_samples:
		case rsx::surface_antialiasing::square_rotated_4_samples:
			result.samples_x = result.samples_y = 2;
			break;
		}
		return result;
	}

	void framebuffer_statistics_t::add(u16 width, u16 height, rsx::surface_antialiasing aa)
	{
		auto& stashed = data[aa];
		const auto& incoming = framebuffer_dimensions_t::make(width, height, aa);
		if (incoming > stashed)
		{
			stashed = incoming;
		}
	}

	std::string framebuffer_statistics_t::to_string(bool squash) const
	{
		// Format is sorted by sample count
		struct sorted_message_t
		{
			u32 id;
			surface_antialiasing aa_mode;
			u32 samples;
		};

		if (data.size() == 0)
		{
			return "None";
		}

		rsx::simple_array<sorted_message_t> messages;
		rsx::simple_array<framebuffer_dimensions_t> real_stats;

		for (const auto& [aa_mode, stat] : data)
		{
			auto real_stat = stat;
			std::tie(real_stat.width, real_stat.height) = apply_resolution_scale(stat.width, stat.height);
			real_stats.push_back(real_stat);

			sorted_message_t msg;
			msg.id = real_stats.size() - 1;
			msg.aa_mode = aa_mode;
			msg.samples = real_stat.samples_total();
			messages.push_back(msg);
		}

		if (squash)
		{
			messages.sort(FN(x.samples > y.samples));
			return real_stats[messages.front().id]
				.to_string(g_cfg.video.antialiasing_level == msaa_level::none);
		}

		if (messages.size() > 1)
		{
			// Should we bother showing the No-AA entry?
			// This heurestic ignores pointless no-AA surfaces usually used as compositing buffers for output.
			messages.sort(FN(x.samples > y.samples));
			if (messages.back().aa_mode == rsx::surface_antialiasing::center_1_sample)
			{
				// Drop the last entry if it has no AA.
				messages.resize(messages.size() - 1);
			}
		}

		const auto text = messages
			.sort(FN(static_cast<u8>(x.aa_mode) > static_cast<u8>(y.aa_mode)))
			.map(FN(real_stats[x.id].to_string()));
		return fmt::merge(text, ", ");
	}
}
