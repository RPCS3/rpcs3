#pragma once

#include <util/types.hpp>

namespace rsx
{
	class MixedPrecisionRegister
	{
		enum data_type_bits
		{
			undefined = 0,
			f16 = 1,
			f32 = 2
		};

		std::array<data_type_bits, 8> content_mask; // Content details for each half-word
		u32 file_index = umax;

		void tag_h0(bool x, bool y, bool z, bool w);

		void tag_h1(bool x, bool y, bool z, bool w);

		void tag_r(bool x, bool y, bool z, bool w);

		std::string fetch_halfreg(u32 word_index) const;

	public:
		MixedPrecisionRegister();

		void tag(u32 index, bool is_fp16, bool x, bool y, bool z, bool w);

		std::string gather_r() const;

		std::string split_h0() const
		{
			return fetch_halfreg(0);
		}

		std::string split_h1() const
		{
			return fetch_halfreg(1);
		}

		// Getters

		// Return true if all values are unwritten to (undefined)
		bool floating() const
		{
			return file_index == umax;
		}

		// Return true if the first half register is all undefined
		bool floating_h0() const
		{
			return content_mask[0] == content_mask[1] &&
				content_mask[1] == content_mask[2] &&
				content_mask[2] == content_mask[3] &&
				content_mask[3] == data_type_bits::undefined;
		}

		// Return true if the second half register is all undefined
		bool floating_h1() const
		{
			return content_mask[4] == content_mask[5] &&
				content_mask[5] == content_mask[6] &&
				content_mask[6] == content_mask[7] &&
				content_mask[7] == data_type_bits::undefined;
		}

		// Return true if any of the half-words are 16-bit
		bool requires_gather(u8 channel) const
		{
			// Data fetched from the single precision register requires merging of the two half registers
			const auto channel_offset = channel * 2;
			ensure(channel_offset <= 6);

			return (content_mask[channel_offset] == data_type_bits::f16 || content_mask[channel_offset + 1] == data_type_bits::f16);
		}

		// Return true if the entire 128-bit register is filled with 2xfp16x4 data words
		bool requires_gather128() const
		{
			// Full 128-bit check
			for (const auto& ch : content_mask)
			{
				if (ch == data_type_bits::f16)
				{
					return true;
				}
			}

			return false;
		}

		// Return true if the half-register is polluted with fp32 data
		bool requires_split(u32 word_index) const
		{
			const u32 content_offset = word_index * 4;
			for (u32 i = 0; i < 4; ++i)
			{
				if (content_mask[content_offset + i] == data_type_bits::f32)
				{
					return true;
				}
			}

			return false;
		}
	};
}

