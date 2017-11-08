#include "stdafx.h"
#include "VKCommonDecompiler.h"
#include "restore_new.h"
#include "../../../../Vulkan/glslang/SPIRV/GlslangToSpv.h"
#include "define_new_memleakdetect.h"

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

		rsc.limits.nonInductiveForLoops = 1;
		rsc.limits.whileLoops = 1;
		rsc.limits.doWhileLoops = 1;
		rsc.limits.generalUniformIndexing = 1;
		rsc.limits.generalAttributeMatrixVectorIndexing = 1;
		rsc.limits.generalVaryingIndexing = 1;
		rsc.limits.generalSamplerIndexing = 1;
		rsc.limits.generalVariableIndexing = 1;
		rsc.limits.generalConstantMatrixVectorIndexing = 1;
	}

	static const varying_register_t varying_regs[] =
	{
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
		{ "back_diff_color", 10 },
		{ "front_diff_color", 11 },
		{ "spec_color", 12 },
		{ "back_spec_color", 12 },
		{ "front_spec_color", 13 },
		{ "fog_c", 14 },
		{ "fogc", 14 }
	};

	const varying_register_t & get_varying_register(const std::string & name)
	{
		for (const auto&t : varying_regs)
		{
			if (t.name == name)
				return t;
		}

		fmt::throw_exception("Unknown register name: %s" HERE, name);
	}

	bool compile_glsl_to_spv(std::string& shader, program_domain domain, std::vector<u32>& spv)
	{
		EShLanguage lang = (domain == glsl_fragment_program) ? EShLangFragment : EShLangVertex;

		glslang::TProgram program;
		glslang::TShader shader_object(lang);
		
		bool success = false;
		const char *shader_text = shader.data();

		shader_object.setStrings(&shader_text, 1);

		EShMessages msg = (EShMessages)(EShMsgVulkanRules | EShMsgSpvRules);
		if (shader_object.parse(&g_default_config, 400, EProfile::ECoreProfile, false, true, msg))
		{
			program.addShader(&shader_object);
			success = program.link(EShMsgVulkanRules);
			if (success)
			{
				glslang::TIntermediate* bytes = program.getIntermediate(lang);
				glslang::GlslangToSpv(*bytes, spv);
			}
		}
		else
		{
			LOG_ERROR(RSX, "%s", shader_object.getInfoLog());
			LOG_ERROR(RSX, "%s", shader_object.getInfoDebugLog());
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
