#include "stdafx.h"
#include "VKCommonDecompiler.h"
#include "../../../../Vulkan/glslang/SPIRV/GlslangToSpv.h"

namespace vk
{
	std::string getFloatTypeNameImpl(size_t elementCount)
	{
		switch (elementCount)
		{
		default:
			abort();
		case 1:
			return "float";
		case 2:
			return "vec2";
		case 3:
			return "vec3";
		case 4:
			return "vec4";
		}
	}

	std::string getFunctionImpl(FUNCTION f)
	{
		switch (f)
		{
		default:
			abort();
		case FUNCTION::FUNCTION_DP2:
			return "vec4(dot($0.xy, $1.xy))";
		case FUNCTION::FUNCTION_DP2A:
			return "vec4(dot($0.xy, $1.xy) + $2.x)";
		case FUNCTION::FUNCTION_DP3:
			return "vec4(dot($0.xyz, $1.xyz))";
		case FUNCTION::FUNCTION_DP4:
			return "vec4(dot($0, $1))";
		case FUNCTION::FUNCTION_DPH:
			return "vec4(dot(vec4($0.xyz, 1.0), $1))";
		case FUNCTION::FUNCTION_SFL:
			return "vec4(0., 0., 0., 0.)";
		case FUNCTION::FUNCTION_STR:
			return "vec4(1., 1., 1., 1.)";
		case FUNCTION::FUNCTION_FRACT:
			return "fract($0)";
		case FUNCTION::FUNCTION_REFL:
			return "vec4($0 - 2.0 * (dot($0, $1)) * $1)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D:
			return "texture($t, $0.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_PROJ:
			return "textureProj($t, $0.x, $1.x)"; // Note: $1.x is bias
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_LOD:
			return "textureLod($t, $0.x, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_GRAD:
			return "textureGrad($t, $0.x, $1.x, $2.y)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D:
			return "texture($t, $0.xy)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_PROJ:
			return "textureProj($t, $0.xyz, $1.x)"; // Note: $1.x is bias
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_LOD:
			return "textureLod($t, $0.xy, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_GRAD:
			return "textureGrad($t, $0.xyz, $1.x, $2.y)"; // Note: $1.x is bias
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE:
			return "texture($t, $0.xyz)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_PROJ:
			return "textureProj($t, $0.xyzw, $1.x)"; // Note: $1.x is bias
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_LOD:
			return "textureLod($t, $0.xyz, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_GRAD:
			return "textureGrad($t, $0.xyzw, $1.x, $2.y)";
		case FUNCTION::FUNCTION_DFDX:
			return "dFdx($0)";
		case FUNCTION::FUNCTION_DFDY:
			return "dFdy($0)";
		}
	}

	std::string compareFunctionImpl(COMPARE f, const std::string &Op0, const std::string &Op1)
	{
		switch (f)
		{
		case COMPARE::FUNCTION_SEQ:
			return "equal(" + Op0 + ", " + Op1 + ")";
		case COMPARE::FUNCTION_SGE:
			return "greaterThanEqual(" + Op0 + ", " + Op1 + ")";
		case COMPARE::FUNCTION_SGT:
			return "greaterThan(" + Op0 + ", " + Op1 + ")";
		case COMPARE::FUNCTION_SLE:
			return "lessThanEqual(" + Op0 + ", " + Op1 + ")";
		case COMPARE::FUNCTION_SLT:
			return "lessThan(" + Op0 + ", " + Op1 + ")";
		case COMPARE::FUNCTION_SNE:
			return "notEqual(" + Op0 + ", " + Op1 + ")";
		}
		throw EXCEPTION("Unknown compare function");
	}

	void insert_glsl_legacy_function(std::ostream& OS)
	{
		OS << "vec4 divsq_legacy(vec4 num, vec4 denum)\n";
		OS << "{\n";
		OS << "	return num / sqrt(max(denum.xxxx, 1.E-10));\n";
		OS << "}\n";

		OS << "vec4 rcp_legacy(vec4 denum)\n";
		OS << "{\n";
		OS << "	return 1. / denum;\n";
		OS << "}\n";

		OS << "vec4 rsq_legacy(vec4 val)\n";
		OS << "{\n";
		OS << "	return float(1.0 / sqrt(max(val.x, 1.E-10))).xxxx;\n";
		OS << "}\n\n";

		OS << "vec4 log2_legacy(vec4 val)\n";
		OS << "{\n";
		OS << "	return log2(max(val.x, 1.E-10)).xxxx;\n";
		OS << "}\n\n";

		OS << "vec4 lit_legacy(vec4 val)";
		OS << "{\n";
		OS << "	vec4 clamped_val = val;\n";
		OS << "	clamped_val.x = max(val.x, 0.);\n";
		OS << "	clamped_val.y = max(val.y, 0.);\n";
		OS << "	vec4 result;\n";
		OS << "	result.x = 1.;\n";
		OS << "	result.w = 1.;\n";
		OS << "	result.y = clamped_val.x;\n";
		OS << "	result.z = clamped_val.x > 0. ? exp(clamped_val.w * log(max(clamped_val.y, 1.E-10))) : 0.;\n";
		OS << "	return result;\n";
		OS << "}\n\n";
	}

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
		rsc.maxProgramTexelOffset = -8;
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

		throw EXCEPTION("Unknown register name: %s", name);
	}

	bool compile_glsl_to_spv(std::string& shader, glsl::program_domain domain, std::vector<u32>& spv)
	{
		EShLanguage lang = (domain == glsl::glsl_fragment_program) ? EShLangFragment : EShLangVertex;

		glslang::InitializeProcess();
		glslang::TProgram program;
		glslang::TShader shader_object(lang);
		
		bool success = false;
		const char *shader_text = shader.data();
		
		TBuiltInResource rsc;
		init_default_resources(rsc);

		shader_object.setStrings(&shader_text, 1);

		EShMessages msg = (EShMessages)(EShMsgVulkanRules | EShMsgSpvRules);
		if (shader_object.parse(&rsc, 400, EProfile::ECoreProfile, false, true, msg))
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
			LOG_ERROR(RSX, shader_object.getInfoLog());
			LOG_ERROR(RSX, shader_object.getInfoDebugLog());
		}

		glslang::FinalizeProcess();
		return success;
	}
}
