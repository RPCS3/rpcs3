#include "stdafx.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#ifdef __clang__
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif
#endif
#include <SPIRV/GlslangToSpv.h>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#include "SPIRVCommon.h"
#include "Emu/RSX/Program/GLSLTypes.h"

namespace spirv
{
	static TBuiltInResource g_default_config;

	void init_default_resources(TBuiltInResource& rsc)
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

	bool compile_glsl_to_spv(std::vector<u32>& spv, std::string& shader, ::glsl::program_domain domain, ::glsl::glsl_rules rules)
	{
		EShLanguage lang = (domain == ::glsl::glsl_fragment_program)
			? EShLangFragment
			: (domain == ::glsl::glsl_vertex_program)
				? EShLangVertex
				: EShLangCompute;

		glslang::EShClient client;
		glslang::EShTargetClientVersion target_version;
		EShMessages msg;

		if (rules == ::glsl::glsl_rules_vulkan)
		{
			client = glslang::EShClientVulkan;
			target_version = glslang::EShTargetClientVersion::EShTargetVulkan_1_0;
			msg = static_cast<EShMessages>(EShMsgVulkanRules | EShMsgSpvRules | EShMsgEnhanced);
		}
		else
		{
			client = glslang::EShClientOpenGL;
			target_version = glslang::EShTargetClientVersion::EShTargetOpenGL_450;
			msg = static_cast<EShMessages>(EShMsgDefault | EShMsgSpvRules | EShMsgEnhanced);
		}

		glslang::TProgram program;
		glslang::TShader shader_object(lang);

		shader_object.setEnvInput(glslang::EShSourceGlsl, lang, client, 100);
		shader_object.setEnvClient(client, target_version);
		shader_object.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);

		bool success = false;
		const char* shader_text = shader.data();
		shader_object.setStrings(&shader_text, 1);

		if (shader_object.parse(&g_default_config, 430, EProfile::ECoreProfile, false, true, msg))
		{
			program.addShader(&shader_object);
			success = program.link(msg);
			if (success)
			{
				glslang::SpvOptions options;
				options.disableOptimizer = true;
				options.optimizeSize = true;
				glslang::GlslangToSpv(*program.getIntermediate(lang), spv, &options);

				// Now we optimize
				//spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_0);
				//optimizer.RegisterPass(spvtools::CreateUnifyConstantPass());      // Remove duplicate constants
				//optimizer.RegisterPass(spvtools::CreateMergeReturnPass());        // Huge savings in vertex interpreter and likely normal vertex shaders
				//optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());      // Remove dead code
				//optimizer.Run(spv.data(), spv.size(), &spv);
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
