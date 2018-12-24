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
		fmt::throw_exception("Unsupported program function %d", (u32)f);
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
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_BIAS:
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_LOD:
		return "$t.SampleLevel($tsampler, $0.x, $1)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_GRAD:
		return "$t.SampleGrad($tsampler, $0.x, $1, $2)";
	case FUNCTION::FUNCTION_TEXTURE_SHADOW2D: //TODO
	case FUNCTION::FUNCTION_TEXTURE_SHADOW2D_PROJ:
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D:
		return "$t.Sample($tsampler, $0.xy * $t_scale)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_PROJ:
		return "$t.Sample($tsampler, ($0.xy / $0.w) * $t_scale)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_BIAS:
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_LOD:
		return "$t.SampleLevel($tsampler, $0.xy * $t_scale, $1)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_GRAD:
		return "$t.SampleGrad($tsampler, $0.xy, $1, $2)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE:
		return "$t.Sample($tsampler, $0.xyz)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_PROJ:
		return "$t.Sample($tsampler, ($0.xyz / $0.w))";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_BIAS:
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_LOD:
		return "$t.SampleLevel($tsampler, $0.xyz, $1)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_GRAD:
		return "$t.SampleGrad($tsampler, $0.xyz, $1, $2)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D:
		return "$t.Sample($tsampler, $0.xyz)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_PROJ:
		return "$t.Sample($tsampler, ($0.xyz / $0.w))";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_BIAS:
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_LOD:
		return "$t.SampleLevel($tsampler, $0.xyz, $1)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_GRAD:
		return "$t.SampleGrad($tsampler, $0.xyz, $1, $2)";
	case FUNCTION::FUNCTION_DFDX:
		return "ddx($0)";
	case FUNCTION::FUNCTION_DFDY:
		return "ddy($0)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_DEPTH_RGBA:
		return "texture2DReconstruct($t.Sample($tsampler, $0.xy * $t_scale), texture_parameters[$_i].z)";
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

void insert_d3d12_legacy_function(std::ostream& OS, bool is_fragment_program)
{
	OS << "#define _select lerp\n\n";

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

	if (!is_fragment_program)
		return;

	program_common::insert_compare_op(OS);

	OS << "uint packSnorm2x16(float2 val)";
	OS << "{\n";
	OS << "	uint high_bits = round(clamp(val.x, -1., 1.) * 32767.);\n";
	OS << "	uint low_bits = round(clamp(val.y, -1., 1.) * 32767.);\n";
	OS << "	return (high_bits << 16)|low_bits;\n";
	OS << "}\n\n";

	OS << "uint packSnorm4x8(float4 val)";
	OS << "{\n";
	OS << "	uint high_bits_a = round(clamp(val.x, -1., 1.) * 127.);\n";
	OS << "	uint high_bits_b = round(clamp(val.y, -1., 1.) * 127.);\n";
	OS << "	uint low_bits_a = round(clamp(val.z, -1., 1.) * 127.);\n";
	OS << "	uint low_bits_b = round(clamp(val.z, -1., 1.) * 127.);\n";
	OS << "	return (high_bits_a << 24)|(high_bits_b << 16)|(low_bits_a << 8)|low_bits_b;\n";
	OS << "}\n\n";

	OS << "float2 unpackSnorm2x16(uint val)";
	OS << "{\n";
	OS << "	float high = clamp((val >> 16) / 32767., -1., 1.);\n";
	OS << "	float low = clamp((val & 0x0000FFFF) / 32767., -1., 1.);\n";
	OS << "	return float2(high, low);\n";
	OS << "}\n\n";

	OS << "float4 unpackSnorm4x8(uint val)";
	OS << "{\n";
	OS << "	float high_a = clamp((val >> 24) / 127., -1., 1.);\n";
	OS << "	float high_b = clamp(((val >> 16) & 0xFF) / 127., -1., 1.);\n";
	OS << "	float low_a = clamp(((val >> 8) & 0xFF) / 127., -1., 1.);\n";
	OS << "	float low_b = clamp((val & 0xFF) / 127., -1., 1.);\n";
	OS << "	return float4(high_a, high_b, low_a, low_b);\n";
	OS << "}\n\n";

	OS << "uint packUnorm4x8(float4 val)";
	OS << "{\n";
	OS << "	uint high_bits_a = round(clamp(val.x, -1., 1.) * 255.);\n";
	OS << "	uint high_bits_b = round(clamp(val.y, -1., 1.) * 255.);\n";
	OS << "	uint low_bits_a = round(clamp(val.z, -1., 1.) * 255.);\n";
	OS << "	uint low_bits_b = round(clamp(val.z, -1., 1.) * 255.);\n";
	OS << "	return (high_bits_a << 24)|(high_bits_b << 16)|(low_bits_a << 8)|low_bits_b;\n";
	OS << "}\n\n";

	OS << "float4 unpackUnorm4x8(uint val)";
	OS << "{\n";
	OS << "	float high_a = clamp((val >> 24) / 255., -1., 1.);\n";
	OS << "	float high_b = clamp(((val >> 16) & 0xFF) / 255., -1., 1.);\n";
	OS << "	float low_a = clamp(((val >> 8) & 0xFF) / 255., -1., 1.);\n";
	OS << "	float low_b = clamp((val & 0xFF) / 255., -1., 1.);\n";
	OS << "	return float4(high_a, high_b, low_a, low_b);\n";
	OS << "}\n\n";

	/**
	* There is no easy way to do this in a shader since we cant recast the f16 blocks to u16
	* Fake it and hope the program requests the corresponding f16 unpack
	**/
	OS << "uint packHalf2x16(float2 val)";
	OS << "{\n";
	OS << "	return packSnorm2x16(val / 65504.);\n";
	OS << "}\n\n";

	OS << "float2 unpackHalf2x16(uint val)";
	OS << "{\n";
	OS << "	return unpackSnorm2x16(val) * 65504.;\n";
	OS << "}\n\n";

	OS << "float read_value(float4 src, uint remap_index)\n";
	OS << "{\n";
	OS << "	switch (remap_index)\n";
	OS << "	{\n";
	OS << "		case 0: return src.a;\n";
	OS << "		case 1: return src.r;\n";
	OS << "		case 2: return src.g;\n";
	OS << "		case 3: return src.b;\n";
	OS << "	}\n";
	OS << "}\n\n";

	OS << "float4 texture2DReconstruct(float depth_value, float remap)\n";
	OS << "{\n";
	OS << "	uint value = round(depth_value * 16777215);\n";
	OS << "	uint b = (value & 0xff);\n";
	OS << "	uint g = (value >> 8) & 0xff;\n";
	OS << "	uint r = (value >> 16) & 0xff;\n";
	OS << "	float4 result = float4(g/255., b/255., 1., r/255.);\n\n";
	OS << "	uint remap_vector = asuint(remap) & 0xFF;\n";
	OS << "	if (remap_vector == 0xE4) return result;\n\n";
	OS << "	float4 tmp;\n";
	OS << "	uint remap_a = remap_vector & 0x3;\n";
	OS << "	uint remap_r = (remap_vector >> 2) & 0x3;\n";
	OS << "	uint remap_g = (remap_vector >> 4) & 0x3;\n";
	OS << "	uint remap_b = (remap_vector >> 6) & 0x3;\n";
	OS << "	tmp.a = read_value(result, remap_a);\n";
	OS << "	tmp.r = read_value(result, remap_r);\n";
	OS << "	tmp.g = read_value(result, remap_g);\n";
	OS << "	tmp.b = read_value(result, remap_b);\n";
	OS << "	return tmp;\n";
	OS << "}\n\n";
}
#endif
