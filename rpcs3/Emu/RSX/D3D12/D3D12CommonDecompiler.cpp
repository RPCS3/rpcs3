#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"
#include "D3D12CommonDecompiler.h"

std::string getFloatTypeNameImp(size_t elementCount)
{
	switch (elementCount)
	{
	default:
		abort();
	case 1:
		return "float";
	case 2:
		return "float2";
	case 3:
		return "float3";
	case 4:
		return "float4";
	}
}

std::string getFunctionImp(FUNCTION f)
{
	switch (f)
	{
	default:
		abort();
	case FUNCTION::FUNCTION_DP2:
		return "dot($0.xy, $1.xy).xxxx";
	case FUNCTION::FUNCTION_DP2A:
		return "(dot($0.xy, $1.xy) + $2.x).xxxx";
	case FUNCTION::FUNCTION_DP3:
		return "dot($0.xyz, $1.xyz).xxxx";
	case FUNCTION::FUNCTION_DP4:
		return "dot($0, $1).xxxx";
	case FUNCTION::FUNCTION_DPH:
		return "dot(float4($0.xyz, 1.0), $1).xxxx";
	case FUNCTION::FUNCTION_SFL:
		return "float4(0., 0., 0., 0.)";
	case FUNCTION::FUNCTION_STR:
		return "float4(1., 1., 1., 1.)";
	case FUNCTION::FUNCTION_FRACT:
		return "frac($0)";
	case FUNCTION::FUNCTION_REFL:
		return "($0 - 2.0 * (dot($0, $1)) * $1)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D:
		return "$t.Sample($tsampler, $0.x)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_PROJ:
		return "$t.Sample($tsampler, ($0.x / $0.w))";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_LOD:
		return "$t.SampleLevel($tsampler, $0.x, $1)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D:
		return "$t.Sample($tsampler, $0.xy * $t_scale)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_PROJ:
		return "$t.Sample($tsampler, ($0.xy / $0.w) * $t_scale)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_LOD:
		return "$t.SampleLevel($tsampler, $0.xy * $t_scale, $1)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE:
		return "$t.Sample($tsampler, $0.xyz)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_PROJ:
		return "$t.Sample($tsampler, ($0.xyz / $0.w))";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_LOD:
		return "$t.SampleLevel($tsampler, $0.xyz, $1)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D:
		return "$t.Sample($tsampler, $0.xyz)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_PROJ:
		return "$t.Sample($tsampler, ($0.xyz / $0.w))";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_LOD:
		return "$t.SampleLevel($tsampler, $0.xyz, $1)";
	case FUNCTION::FUNCTION_DFDX:
		return "ddx($0)";
	case FUNCTION::FUNCTION_DFDY:
		return "ddy($0)";
	}
}

std::string compareFunctionImp(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	switch (f)
	{
	default:
		abort();
	case COMPARE::FUNCTION_SEQ:
		return "(" + Op0 + " == " + Op1 + ")";
	case COMPARE::FUNCTION_SGE:
		return "(" + Op0 + " >= " + Op1 + ")";
	case COMPARE::FUNCTION_SGT:
		return "(" + Op0 + " > " + Op1 + ")";
	case COMPARE::FUNCTION_SLE:
		return "(" + Op0 + " <= " + Op1 + ")";
	case COMPARE::FUNCTION_SLT:
		return "(" + Op0 + " < " + Op1 + ")";
	case COMPARE::FUNCTION_SNE:
		return "(" + Op0 + " != " + Op1 + ")";
	}
}

void insert_d3d12_legacy_function(std::ostream& OS)
{
	OS << "float4 divsq_legacy(float4 num, float4 denum)\n";
	OS << "{\n";
	OS << "	return num / sqrt(max(denum.xxxx, 1.E-10));\n";
	OS << "}\n";

	OS << "float4 rcp_legacy(float4 denum)\n";
	OS << "{\n";
	OS << "	return 1. / denum;\n";
	OS << "}\n";

	OS << "float4 rsq_legacy(float4 val)\n";
	OS << "{\n";
	OS << "	return float(1.0 / sqrt(max(val.x, 1.E-10))).xxxx;\n";
	OS << "}\n\n";

	OS << "float4 log2_legacy(float4 val)\n";
	OS << "{\n";
	OS << "	return log2(max(val.x, 1.E-10)).xxxx;\n";
	OS << "}\n\n";

	OS << "float4 lit_legacy(float4 val)";
	OS << "{\n";
	OS << "	float4 clamped_val = val;\n";
	OS << "	clamped_val.x = max(val.x, 0);\n";
	OS << "	clamped_val.y = max(val.y, 0);\n";
	OS << "	float4 result;\n";
	OS << "	result.x = 1.0;\n";
	OS << "	result.w = 1.;\n";
	OS << "	result.y = clamped_val.x;\n";
	OS << "	result.z = clamped_val.x > 0.0 ? exp(clamped_val.w * log(max(clamped_val.y, 1.E-10))) : 0.0;\n";
	OS << "	return result;\n";
	OS << "}\n\n";
}
#endif
