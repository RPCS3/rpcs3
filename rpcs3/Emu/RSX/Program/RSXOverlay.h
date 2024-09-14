#pragma once

#include <util/types.hpp>

namespace rsx
{
	namespace overlays
	{
		// This is overlay common code meant only for render backends
		enum class texture_sampling_mode : s32
		{
			none = 0,
			font2D = 1,
			font3D = 2,
			texture2D = 3
		};

		class fragment_options
		{
			u32 value = 0;

			enum e_offsets: s32
			{
				fragment_clip_bit = 0,
				pulse_glow_bit = 1,
				sampling_mode_bit = 2
			};

		public:
			fragment_options& texture_mode(texture_sampling_mode mode)
			{
				value |= static_cast<s32>(mode) << e_offsets::sampling_mode_bit;
				return *this;
			}

			fragment_options& pulse_glow(bool enable = true)
			{
				if (enable)
				{
					value |= (1 << e_offsets::pulse_glow_bit);
				}
				return *this;
			}

			fragment_options& clip_fragments(bool enable = true)
			{
				if (enable)
				{
					value |= (1 << e_offsets::fragment_clip_bit);
				}
				return *this;
			}

			u32 get() const
			{
				return value;
			}
		};

		class vertex_options
		{
			u32 value = 0;

		public:
			vertex_options& disable_vertex_snap(bool enable)
			{
				value = enable ? 1 : 0;
				return *this;
			}

			u32 get() const
			{
				return value;
			}
		};
	}
}
