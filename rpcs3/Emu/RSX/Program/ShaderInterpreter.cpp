#include "stdafx.h"
#include "ShaderInterpreter.h"

#include <unordered_set>

namespace program_common::interpreter
{
	struct vs_variants_metadata
	{
		u32 vs_base_mask = 0;
		std::unordered_set<u32> vs_base_opts; // Set of options that covers all possible variants regardless of optimization
		std::unordered_set<u32> vs_opts;      // Full set of options that covers all variants with full optimization
	};

	struct fs_variants_metadata
	{
		u32 fs_base_mask = 0;
		std::unordered_set<u32> fs_base_opts; // Set of options that covers all possible variants regardless of optimization
		std::unordered_set<u32> fs_opts;      // Full set of options that covers all variants with full optimization
	};

	void bitrange_foreach(u32 min, u32 max, std::function<void(u32)> func)
	{
		if (max <= min)
		{
			return;
		}

		const u32 shift = std::countr_zero(min);
		const u32 a = min >> shift;
		const u32 b = (max + max) >> shift;

		for (u32 acc = a; acc < b; acc++)
		{
			func(acc << shift);
		}
	}

	vs_variants_metadata prepare_vs_variants_data()
	{
		vs_variants_metadata result;
		result.vs_opts.insert(0);
		result.vs_base_opts.insert(0);

		const u32 base_vs_mask = COMPILER_OPT_ALL_VS_MASK & ~(COMPILER_OPT_VS_EXCL_MASK);
		bitrange_foreach(COMPILER_OPT_VS_MIN, COMPILER_OPT_VS_MAX, [&](u32 vs_opt)
		{
			result.vs_opts.insert(vs_opt);

			if (const auto excl_mask = (vs_opt & COMPILER_OPT_VS_EXCL_MASK);
				excl_mask != 0)
			{
				result.vs_base_opts.insert(base_vs_mask | excl_mask);
			}
		});
		result.vs_opts.insert(base_vs_mask);
		result.vs_base_opts.insert(base_vs_mask);

		result.vs_base_mask = base_vs_mask;
		return result;
	}

	fs_variants_metadata prepare_fs_variants_data()
	{
		fs_variants_metadata result;
		result.fs_opts.insert(0);
		result.fs_base_opts.insert(0);

		const u32 base_fs_mask = COMPILER_OPT_BASE_FS_MASK & ~(COMPILER_OPT_FS_EXCL_MASK);
		bitrange_foreach(COMPILER_OPT_FS_MIN, COMPILER_OPT_FS_MAX, [&](u32 fs_opt)
		{
			result.fs_opts.insert(fs_opt);
			if (const auto excl_mask = (fs_opt & COMPILER_OPT_FS_EXCL_MASK);
				excl_mask != 0)
			{
				result.fs_base_opts.insert(base_fs_mask | excl_mask);
			}
		});
		result.fs_opts.insert(base_fs_mask);
		result.fs_base_opts.insert(base_fs_mask);

		// Now we add in the alpha testing variants for all fs variants.
		// Only one alpha test type is usable at once
		std::unordered_set<u32> fs_alpha_test_masks;
		std::unordered_set<u32> fs_alpha_test_base_masks;

		for (u32 alpha_test_bit = COMPILER_OPT_ENABLE_ALPHA_TEST_GE;
			alpha_test_bit <= COMPILER_OPT_ENABLE_ALPHA_TEST_NE;
			alpha_test_bit <<= 1)
		{
			for (const auto& mask : result.fs_opts)
			{
				fs_alpha_test_masks.insert(mask | alpha_test_bit);
			}

			for (const auto& mask : result.fs_base_opts)
			{
				fs_alpha_test_base_masks.insert(mask | alpha_test_bit);
			}
		}

		// Merge all FS variants
		result.fs_opts.merge(fs_alpha_test_masks);
		result.fs_base_opts.merge(fs_alpha_test_base_masks);

		result.fs_base_mask = base_fs_mask;
		return result;
	}

	interpreter_variants_t get_interpreter_variants()
	{
		const auto vs_metadata = prepare_vs_variants_data();
		const auto fs_metadata = prepare_fs_variants_data();

		const auto& vs_masks = vs_metadata.vs_opts;
		const auto& fs_masks = fs_metadata.fs_opts;

		const auto base_vs_mask = vs_metadata.vs_base_mask;
		const auto base_fs_mask = fs_metadata.fs_base_mask;

		// Prepare outputs
		interpreter_variants_t result;
		for (const auto& vs_opt : vs_masks)
		{
			for (const auto& fs_opt : fs_masks)
			{
				interpreter_pipeline_variant_t variant{};
				variant.vs_opts.shader_opt = vs_opt;
				variant.vs_opts.compatible_shader_opts = (vs_opt & ~base_vs_mask) | base_vs_mask;

				variant.fs_opts.shader_opt = fs_opt;
				variant.fs_opts.compatible_shader_opts = (fs_opt & ~base_fs_mask) | base_fs_mask;

				result.pipelines.emplace_back(std::move(variant));
			}
		}

		// Calculate base pipelines (minimal set)
		const auto& vs_base_masks = vs_metadata.vs_base_opts;
		const auto& fs_base_masks = fs_metadata.fs_base_opts;

		result.base_pipelines.push_back({ base_vs_mask, base_fs_mask });
		for (const u32 vs_opt : vs_base_masks)
		{
			for (const u32 fs_opt : fs_base_masks)
			{
				result.base_pipelines.push_back({ vs_opt, fs_opt });
			}
		}

		return result;
	}
}
