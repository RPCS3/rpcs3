#pragma once

#include <string>
#include <vector>

#include "Utilities/StrUtil.h"

enum class FUNCTION {
	FUNCTION_DP2,
	FUNCTION_DP2A,
	FUNCTION_DP3,
	FUNCTION_DP4,
	FUNCTION_DPH,
	FUNCTION_SFL, // Set zero
	FUNCTION_STR, // Set One
	FUNCTION_FRACT,
	FUNCTION_DFDX,
	FUNCTION_DFDY,
	FUNCTION_REFL,
	FUNCTION_TEXTURE_SAMPLE1D,
	FUNCTION_TEXTURE_SAMPLE1D_BIAS,
	FUNCTION_TEXTURE_SAMPLE1D_PROJ,
	FUNCTION_TEXTURE_SAMPLE1D_LOD,
	FUNCTION_TEXTURE_SAMPLE1D_GRAD,
	FUNCTION_TEXTURE_SAMPLE2D,
	FUNCTION_TEXTURE_SAMPLE2D_BIAS,
	FUNCTION_TEXTURE_SAMPLE2D_PROJ,
	FUNCTION_TEXTURE_SAMPLE2D_LOD,
	FUNCTION_TEXTURE_SAMPLE2D_GRAD,
	FUNCTION_TEXTURE_SHADOW2D,
	FUNCTION_TEXTURE_SHADOW2D_PROJ,
	FUNCTION_TEXTURE_SAMPLECUBE,
	FUNCTION_TEXTURE_SAMPLECUBE_BIAS,
	FUNCTION_TEXTURE_SAMPLECUBE_PROJ,
	FUNCTION_TEXTURE_SAMPLECUBE_LOD,
	FUNCTION_TEXTURE_SAMPLECUBE_GRAD,
	FUNCTION_TEXTURE_SAMPLE3D,
	FUNCTION_TEXTURE_SAMPLE3D_BIAS,
	FUNCTION_TEXTURE_SAMPLE3D_PROJ,
	FUNCTION_TEXTURE_SAMPLE3D_LOD,
	FUNCTION_TEXTURE_SAMPLE3D_GRAD,
	FUNCTION_VERTEX_TEXTURE_FETCH1D,
	FUNCTION_VERTEX_TEXTURE_FETCH2D,
	FUNCTION_VERTEX_TEXTURE_FETCH3D,
	FUNCTION_VERTEX_TEXTURE_FETCHCUBE,
	FUNCTION_TEXTURE_SAMPLE2D_DEPTH_RGBA
};

enum class COMPARE {
	FUNCTION_SEQ,
	FUNCTION_SGE,
	FUNCTION_SGT,
	FUNCTION_SLE,
	FUNCTION_SLT,
	FUNCTION_SNE,
};

enum ParamFlag
{
	PF_PARAM_IN,
	PF_PARAM_OUT,
	PF_PARAM_UNIFORM,
	PF_PARAM_CONST,
	PF_PARAM_NONE,
	PF_PARAM_COUNT,
};

struct ParamItem
{
	std::string name;
	std::string value;
	int location;

	ParamItem(const std::string& _name, int _location, const std::string& _value = "")
		: name(_name)
		, value(_value),
		location(_location)
	{ }
};

struct ParamType
{
	const ParamFlag flag;
	std::string type;
	std::vector<ParamItem> items;

	ParamType(const ParamFlag _flag, const std::string& _type)
		: flag(_flag)
		, type(_type)
	{
	}

	bool SearchName(const std::string& name)
	{
		for (u32 i = 0; i<items.size(); ++i)
		{
			if (items[i].name.compare(name) == 0) return true;
		}

		return false;
	}
};

struct ParamArray
{
	std::vector<ParamType> params[PF_PARAM_COUNT];

	ParamType* SearchParam(const ParamFlag &flag, const std::string& type)
	{
		for (u32 i = 0; i<params[flag].size(); ++i)
		{
			if (params[flag][i].type.compare(type) == 0)
				return &params[flag][i];
		}

		return nullptr;
	}

	bool HasParamTypeless(const ParamFlag flag, const std::string& name)
	{
		for (u32 i = 0; i<params[flag].size(); ++i)
		{
			if (params[flag][i].SearchName(name))
				return true;
		}

		return false;
	}

	bool HasParam(const ParamFlag flag, std::string type, const std::string& name)
	{
		ParamType* t = SearchParam(flag, type);
		return t && t->SearchName(name);
	}

	std::string AddParam(const ParamFlag flag, std::string type, const std::string& name, const std::string& value)
	{
		ParamType* t = SearchParam(flag, type);

		if (t)
		{
			if (!t->SearchName(name)) t->items.emplace_back(name, -1, value);
		}
		else
		{
			params[flag].emplace_back(flag, type);
			params[flag].back().items.emplace_back(name, -1, value);
		}

		return name;
	}

	std::string AddParam(const ParamFlag flag, std::string type, const std::string& name, int location = -1)
	{
		ParamType* t = SearchParam(flag, type);

		if (t)
		{
			if (!t->SearchName(name)) t->items.emplace_back(name, location);
		}
		else
		{
			params[flag].emplace_back(flag, type);
			params[flag].back().items.emplace_back(name, location);
		}

		return name;
	}
};

class ShaderVariable
{
public:
	std::string name;
	std::vector<std::string> swizzles;

	ShaderVariable() = default;
	ShaderVariable(const std::string& var)
	{
		// Separate 'double destination' variables 'X=Y=SRC'
		std::string simple_var;
		const auto eq_pos = var.find('=');

		if (eq_pos != std::string::npos)
		{
			simple_var = var.substr(0, eq_pos - 1);
		}
		else
		{
			simple_var = var;
		}

		const auto brace_pos = var.find_last_of(")");
		std::string prefix;
		if (brace_pos != std::string::npos)
		{
			prefix = simple_var.substr(0, brace_pos);
			simple_var = simple_var.substr(brace_pos);
		}

		auto var_blocks = fmt::split(simple_var, { "." });

		verify(HERE), (var_blocks.size() != 0);

		name = prefix + var_blocks[0];

		if (var_blocks.size() == 1)
		{
			swizzles.push_back("xyzw");
		}
		else
		{
			swizzles = std::vector<std::string>(var_blocks.begin() + 1, var_blocks.end());
		}
	}

	size_t get_vector_size() const
	{
		return swizzles[swizzles.size() - 1].length();
	}

	ShaderVariable& simplify()
	{
		std::unordered_map<char, char> swizzle;

		static std::unordered_map<int, char> pos_to_swizzle =
		{
			{ 0, 'x' },
			{ 1, 'y' },
			{ 2, 'z' },
			{ 3, 'w' }
		};

		for (auto &i : pos_to_swizzle)
		{
			swizzle[i.second] = swizzles[0].length() > i.first ? swizzles[0][i.first] : 0;
		}

		for (int i = 1; i < swizzles.size(); ++i)
		{
			std::unordered_map<char, char> new_swizzle;

			for (auto &sw : pos_to_swizzle)
			{
				new_swizzle[sw.second] = swizzle[swizzles[i].length() <= sw.first ? '\0' : swizzles[i][sw.first]];
			}

			swizzle = new_swizzle;
		}

		swizzles.clear();
		std::string new_swizzle;

		for (auto &i : pos_to_swizzle)
		{
			if (swizzle[i.second] != '\0')
				new_swizzle += swizzle[i.second];
		}

		swizzles.push_back(new_swizzle);

		return *this;
	}

	std::string get() const
	{
		if (swizzles.size() == 1 && swizzles[0] == "xyzw")
		{
			return name;
		}

		return name + "." + fmt::merge({ swizzles }, ".");
	}

	std::string match_size(const std::string& other_var) const
	{
		// Make other_var the same vector length as this var
		ShaderVariable other(other_var);
		const auto this_size = get_vector_size();
		const auto other_size = other.get_vector_size();

		if (LIKELY(this_size == other_size))
		{
			return other_var;
		}

		if (LIKELY(this_size < other_size))
		{
			switch (this_size)
			{
			case 0:
			case 4:
				return other_var;
			case 1:
				return other_var + ".x";
			case 2:
				return other_var + ".xy";
			case 3:
				return other_var + ".xyz";
			default:
				fmt::throw_exception("Unreachable" HERE);
			}
		}
		else
		{
			auto remaining = this_size - other_size;
			std::string result = other_var;

			while (remaining--) result += "x";
			return result;
		}
	}
};

struct vertex_reg_info
{
	std::string name;           //output name
	bool need_declare;          //needs explicit declaration as output (not language in-built)
	std::string src_reg;        //reg to get data from
	std::string src_reg_mask;   //input swizzle mask
	bool need_cast;             //needs casting
	std::string cond;           //update on condition
	std::string default_val;    //fallback value on cond fail
	std::string dst_alias;      //output name override
	bool check_mask;            //check program output control mask for this output
	u32  check_mask_value;      //program output control mask for testing
};
