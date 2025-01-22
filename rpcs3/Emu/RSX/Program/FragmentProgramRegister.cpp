#include "stdafx.h"
#include "FragmentProgramRegister.h"

namespace rsx
{
	MixedPrecisionRegister::MixedPrecisionRegister()
	{
		std::fill(content_mask.begin(), content_mask.end(), data_type_bits::undefined);
	}

	void MixedPrecisionRegister::tag_h0(bool x, bool y, bool z, bool w)
	{
		if (x) content_mask[0] = data_type_bits::f16;
		if (y) content_mask[1] = data_type_bits::f16;
		if (z) content_mask[2] = data_type_bits::f16;
		if (w) content_mask[3] = data_type_bits::f16;
	}

	void MixedPrecisionRegister::tag_h1(bool x, bool y, bool z, bool w)
	{
		if (x) content_mask[4] = data_type_bits::f16;
		if (y) content_mask[5] = data_type_bits::f16;
		if (z) content_mask[6] = data_type_bits::f16;
		if (w) content_mask[7] = data_type_bits::f16;
	}

	void MixedPrecisionRegister::tag_r(bool x, bool y, bool z, bool w)
	{
		if (x) content_mask[0] = content_mask[1] = data_type_bits::f32;
		if (y) content_mask[2] = content_mask[3] = data_type_bits::f32;
		if (z) content_mask[4] = content_mask[5] = data_type_bits::f32;
		if (w) content_mask[6] = content_mask[7] = data_type_bits::f32;
	}

	void MixedPrecisionRegister::tag(u32 index, bool is_fp16, bool x, bool y, bool z, bool w)
	{
		if (file_index == umax)
		{
			// First-time use. Initialize...
			const u32 real_index = is_fp16 ? (index >> 1) : index;
			file_index = real_index;
		}

		if (is_fp16)
		{
			ensure((index / 2) == file_index);

			if (index & 1)
			{
				tag_h1(x, y, z, w);
				return;
			}

			tag_h0(x, y, z, w);
			return;
		}

		tag_r(x, y, z, w);
	}

	std::string MixedPrecisionRegister::gather_r() const
	{
		const auto half_index = file_index << 1;
		const std::string reg = "r" + std::to_string(file_index);
		const std::string gather_half_regs[] = {
			"gather(h" + std::to_string(half_index) + ")",
			"gather(h" + std::to_string(half_index + 1) + ")"
		};

		std::string outputs[4];
		for (int ch = 0; ch < 4; ++ch)
		{
			// FIXME: This approach ignores mixed register bits. Not ideal!!!!
			const auto channel0 = content_mask[ch * 2];
			const auto is_fp16_ch = channel0 == content_mask[ch * 2 + 1] && channel0 == data_type_bits::f16;
			outputs[ch] = is_fp16_ch ? gather_half_regs[ch / 2] : reg;
		}

		// Grouping. Only replace relevant bits...
		if (outputs[0] == outputs[1]) outputs[0] = "";
		if (outputs[2] == outputs[3]) outputs[2] = "";

		// Assemble
		bool group = false;
		std::string result = "";
		constexpr std::string_view swz_mask = "xyzw";

		for (int ch = 0; ch < 4; ++ch)
		{
			if (outputs[ch].empty())
			{
				group = true;
				continue;
			}

			if (!result.empty())
			{
				result += ", ";
			}

			if (group)
			{
				ensure(ch > 0);
				group = false;

				if (outputs[ch] == reg)
				{
					result += reg + "." + swz_mask[ch - 1] + swz_mask[ch];
					continue;
				}

				result += outputs[ch];
				continue;
			}

			const int subch = outputs[ch] == reg ? ch : (ch % 2); // Avoid .xyxy.z and other such ugly swizzles
			result += outputs[ch] + "." + swz_mask[subch];
		}

		// Optimize dual-gather (128-bit gather) to use special function
		const std::string double_gather = gather_half_regs[0] + ", " + gather_half_regs[1];
		if (result == double_gather)
		{
			result = "gather(h" + std::to_string(half_index) + ", h" + std::to_string(half_index + 1) + ")";
		}

		return "(" + result + ")";
	}

	std::string MixedPrecisionRegister::fetch_halfreg(u32 word_index) const
	{
		// Reads half-word 0 (H16x4) from a full real (R32x4) register
		constexpr std::string_view swz_mask = "xyzw";
		const std::string reg = "r" + std::to_string(file_index);
		const std::string hreg = "h" + std::to_string(file_index * 2 + word_index);

		const std::string word0_bits = "floatBitsToUint(" + reg + "." + swz_mask[word_index * 2] + ")";
		const std::string word1_bits = "floatBitsToUint(" + reg + "." + swz_mask[word_index * 2 + 1] + ")";
		const std::string words[] = {
			"unpackHalf2x16(" + word0_bits + ")",
			"unpackHalf2x16(" + word1_bits + ")"
		};

		// Assemble
		std::string outputs[4];

		ensure(word_index <= 1);
		const int word_offset = word_index * 4;
		for (int ch = 0; ch < 4; ++ch)
		{
			outputs[ch] = content_mask[ch + word_offset] == data_type_bits::f32
				? words[ch / 2]
				: hreg;
		}

		// Grouping. Only replace relevant bits...
		if (outputs[0] == outputs[1]) outputs[0] = "";
		if (outputs[2] == outputs[3]) outputs[2] = "";

		// Assemble
		bool group = false;
		std::string result = "";

		for (int ch = 0; ch < 4; ++ch)
		{
			if (outputs[ch].empty())
			{
				group = true;
				continue;
			}

			if (!result.empty())
			{
				result += ", ";
			}

			if (group)
			{
				ensure(ch > 0);
				group = false;
				result += outputs[ch];

				if (outputs[ch] == hreg)
				{
					result += std::string(".") + swz_mask[ch - 1] + swz_mask[ch];
				}
				continue;
			}

			const int subch = outputs[ch] == hreg ? ch : (ch % 2); // Avoid .xyxy.z and other such ugly swizzles
			result += outputs[ch] + "." + swz_mask[subch];
		}

		return "(" + result + ")";
	}
}
