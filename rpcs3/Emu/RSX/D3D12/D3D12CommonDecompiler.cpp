#include "stdafx.h"
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
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE:
		return "$t.Sample($tsampler, $0.xy)";
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