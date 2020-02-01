#include "stdafx.h"
#include "VKCommonDecompiler.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "restore_new.h"
#include "SPIRV/GlslangToSpv.h"
#include "define_new_memleakdetect.h"
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

namespace vk
{
	static TBuiltInResource g_default_config;

	void init_default_resources(TBuiltInResource &rsc)
	{
		rsc.maxLights = 32;
		rsc.maxClipPlanes = 6;
		rsc.maxTextureUnits = 32;
		rsc.maxTextureCoords = 32;
		rsc.maxVertexAttribs = 64;
		rsc.maxVertexUniformComponents = 4096;
		rsc.maxVaryingFloats = 64;
		rsc.maxVertexTextureImageUnits = 32;
		rsc.maxCombinedTextureImageUnits = 80;
		rsc.maxTextureImageUnits = 32;
		rsc.maxFragmentUniformComponents = 4096;
		rsc.maxDrawBuffers = 32;
		rsc.maxVertexUniformVectors = 128;
		rsc.maxVaryingVectors = 8;
		rsc.maxFragmentUniformVectors = 16;
		rsc.maxVertexOutputVectors = 16;
		rsc.maxFragmentInputVectors = 15;
		rsc.minProgramTexelOffset = -8;
		rsc.maxProgramTexelOffset = 7;
		rsc.maxClipDistances = 8;
		rsc.maxComputeWorkGroupCountX = 65535;
		rsc.maxComputeWorkGroupCountY = 65535;
		rsc.maxComputeWorkGroupCountZ = 65535;
		rsc.maxComputeWorkGroupSizeX = 1024;
		rsc.maxComputeWorkGroupSizeY = 1024;
		rsc.maxComputeWorkGroupSizeZ = 64;
		rsc.maxComputeUniformComponents = 1024;
		rsc.maxComputeTextureImageUnits = 16;
		rsc.maxComputeImageUniforms = 8;
		rsc.maxComputeAtomicCounters = 8;
		rsc.maxComputeAtomicCounterBuffers = 1;
		rsc.maxVaryingComponents = 60;
		rsc.maxVertexOutputComponents = 64;
		rsc.maxGeometryInputComponents = 64;
		rsc.maxGeometryOutputComponents = 128;
		rsc.maxFragmentInputComponents = 128;
		rsc.maxImageUnits = 8;
		rsc.maxCombinedImageUnitsAndFragmentOutputs = 8;
		rsc.maxCombinedShaderOutputResources = 8;
		rsc.maxImageSamples = 0;
		rsc.maxVertexImageUniforms = 0;
		rsc.maxTessControlImageUniforms = 0;
		rsc.maxTessEvaluationImageUniforms = 0;
		rsc.maxGeometryImageUniforms = 0;
		rsc.maxFragmentImageUniforms = 8;
		rsc.maxCombinedImageUniforms = 8;
		rsc.maxGeometryTextureImageUnits = 16;
		rsc.maxGeometryOutputVertices = 256;
		rsc.maxGeometryTotalOutputComponents = 1024;
		rsc.maxGeometryUniformComponents = 1024;
		rsc.maxGeometryVaryingComponents = 64;
		rsc.maxTessControlInputComponents = 128;
		rsc.maxTessControlOutputComponents = 128;
		rsc.maxTessControlTextureImageUnits = 16;
		rsc.maxTessControlUniformComponents = 1024;
		rsc.maxTessControlTotalOutputComponents = 4096;
		rsc.maxTessEvaluationInputComponents = 128;
		rsc.maxTessEvaluationOutputComponents = 128;
		rsc.maxTessEvaluationTextureImageUnits = 16;
		rsc.maxTessEvaluationUniformComponents = 1024;
		rsc.maxTessPatchComponents = 120;
		rsc.maxPatchVertices = 32;
		rsc.maxTessGenLevel = 64;
		rsc.maxViewports = 16;
		rsc.maxVertexAtomicCounters = 0;
		rsc.maxTessControlAtomicCounters = 0;
		rsc.maxTessEvaluationAtomicCounters = 0;
		rsc.maxGeometryAtomicCounters = 0;
		rsc.maxFragmentAtomicCounters = 8;
		rsc.maxCombinedAtomicCounters = 8;
		rsc.maxAtomicCounterBindings = 1;
		rsc.maxVertexAtomicCounterBuffers = 0;
		rsc.maxTessControlAtomicCounterBuffers = 0;
		rsc.maxTessEvaluationAtomicCounterBuffers = 0;
		rsc.maxGeometryAtomicCounterBuffers = 0;
		rsc.maxFragmentAtomicCounterBuffers = 1;
		rsc.maxCombinedAtomicCounterBuffers = 1;
		rsc.maxAtomicCounterBufferSize = 16384;
		rsc.maxTransformFeedbackBuffers = 4;
		rsc.maxTransformFeedbackInterleavedComponents = 64;
		rsc.maxCullDistances = 8;
		rsc.maxCombinedClipAndCullDistances = 8;
		rsc.maxSamples = 4;

		rsc.limits.nonInductiveForLoops = true;
		rsc.limits.whileLoops = true;
		rsc.limits.doWhileLoops = true;
		rsc.limits.generalUniformIndexing = true;
		rsc.limits.generalAttributeMatrixVectorIndexing = true;
		rsc.limits.generalVaryingIndexing = true;
		rsc.limits.generalSamplerIndexing = true;
		rsc.limits.generalVariableIndexing = true;
		rsc.limits.generalConstantMatrixVectorIndexing = true;
	}

	static constexpr std::array<std::pair<std::string_view, int>, 18> varying_registers =
	{{
		{ "tc0", 0 },
		{ "tc1", 1 },
		{ "tc2", 2 },
		{ "tc3", 3 },
		{ "tc4", 4 },
		{ "tc5", 5 },
		{ "tc6", 6 },
		{ "tc7", 7 },
		{ "tc8", 8 },
		{ "tc9", 9 },
		{ "diff_color", 10 },
		{ "diff_color1", 11 },
		{ "spec_color", 12 },
		{ "spec_color1", 13 },
		{ "fog_c", 14 },
		{ "fogc", 14 }
	}};

	int get_varying_register_location(std::string_view varying_register_name)
	{
		for (const auto& varying_register : varying_registers)
		{
			if (varying_register.first == varying_register_name)
			{
				return varying_register.second;
			}
		}

		fmt::throw_exception("Unknown register name: %s" HERE, varying_register_name);
	}

	bool compile_glsl_to_spv(std::string& shader, program_domain domain, std::vector<u32>& spv)
	{
		EShLanguage lang = (domain == glsl_fragment_program) ? EShLangFragment :
			(domain == glsl_vertex_program)? EShLangVertex : EShLangCompute;

		glslang::TProgram program;
		glslang::TShader shader_object(lang);

		shader_object.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 100);
		shader_object.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_0);
		shader_object.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);

		bool success = false;
		const char *shader_text = shader.data();

		shader_object.setStrings(&shader_text, 1);

		EShMessages msg = static_cast<EShMessages>(EShMsgVulkanRules | EShMsgSpvRules);
		if (shader_object.parse(&g_default_config, 400, EProfile::ECoreProfile, false, true, msg))
		{
			program.addShader(&shader_object);
			success = program.link(msg);
			if (success)
			{
				glslang::SpvOptions options;
				options.disableOptimizer = false;
				options.optimizeSize = true;
				glslang::GlslangToSpv(*program.getIntermediate(lang), spv, &options);
			}
		}
		else
		{
			rsx_log.error("%s", shader_object.getInfoLog());
			rsx_log.error("%s", shader_object.getInfoDebugLog());
		}

		return success;
	}

	void initialize_compiler_context()
	{
		glslang::InitializeProcess();
		init_default_resources(g_default_config);
	}

	void finalize_compiler_context()
	{
		glslang::FinalizeProcess();
	}
}
