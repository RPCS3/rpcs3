#pragma once

#include <string>
#include <vector>

#include "Utilities/StrUtil.h"
#include "Utilities/types.h"

enum class FUNCTION
{
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

enum class COMPARE
{
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
	const std::string name;
	const std::string value;
	int location;

	ParamItem(std::string _name, int _location, std::string _value = "")
		: name(std::move(_name))
		, value(std::move(_value)),
		location(_location)
	{ }
};

struct ParamType
{
	const ParamFlag flag;
	const std::string type;
	std::vector<ParamItem> items;

	ParamType(const ParamFlag _flag, std::string _type)
		: flag(_flag)
		, type(std::move(_type))
	{
	}

	bool SearchName(const std::string& name) const
	{
		for (const auto& item : items)
		{
			if (item.name == name) return true;
		}

		return false;
	}
};

struct ParamArray
{
	std::vector<ParamType> params[PF_PARAM_COUNT];

	ParamType* SearchParam(const ParamFlag &flag, const std::string& type)
	{
		for (auto& param : params[flag])
		{
			if (param.type == type)
				return &param;
		}

		return nullptr;
	}

	bool HasParamTypeless(const ParamFlag flag, const std::string& name)
	{
		for (const auto& param : params[flag])
		{
			if (param.SearchName(name))
				return true;
		}

		return false;
	}

	bool HasParam(const ParamFlag flag, const std::string& type, const std::string& name)
	{
		ParamType* t = SearchParam(flag, type);
		return t && t->SearchName(name);
	}

	std::string AddParam(const ParamFlag flag, const std::string& type, const std::string& name, const std::string& value)
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

	std::string AddParam(const ParamFlag flag, const std::string& type, const std::string& name, int location = -1)
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

		if (eq_pos != umax)
		{
			simple_var = var.substr(0, eq_pos - 1);
		}
		else
		{
			simple_var = var;
		}

		const auto brace_pos = var.find_last_of(')');
		std::string prefix;
		if (brace_pos != umax)
		{
			prefix = simple_var.substr(0, brace_pos);
			simple_var = simple_var.substr(brace_pos);
		}

		auto var_blocks = fmt::split(simple_var, { "." });

		verify(HERE), (!var_blocks.empty());

		name = prefix + var_blocks[0];

		if (var_blocks.size() == 1)
		{
			swizzles.emplace_back("xyzw");
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

		static std::unordered_map<uint, char> pos_to_swizzle =
		{
			{ 0, 'x' },
			{ 1, 'y' },
			{ 2, 'z' },
			{ 3, 'w' }
		};

		for (auto& p : pos_to_swizzle)
		{
			swizzle[p.second] = swizzles[0].length() > p.first ? swizzles[0][p.first] : 0;
		}

		for (uint i = 1; i < swizzles.size(); ++i)
		{
			std::unordered_map<char, char> new_swizzle;

			for (auto& p : pos_to_swizzle)
			{
				new_swizzle[p.second] = swizzle[swizzles[i].length() <= p.first ? '\0' : swizzles[i][p.first]];
			}

			swizzle = new_swizzle;
		}

		swizzles.clear();
		std::string new_swizzle;

		for (auto& p : pos_to_swizzle)
		{
			if (swizzle[p.second] != '\0')
				new_swizzle += swizzle[p.second];
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

		if (this_size == other_size) [[likely]]
		{
			return other_var;
		}

		if (this_size < other_size) [[likely]]
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

	std::string add_mask(const std::string& other_var) const
	{
		if (swizzles.back() != "xyzw")
		{
			return other_var + "." + swizzles.back();
		}

		return other_var;
	}
};

struct vertex_reg_info
{
	enum mask_test_type : u8
	{
		any = 0,   // Any bit set
		none = 1,  // No bits set
		all = 2,   // All bits set
		xall = 3   // Some bits set
	};

	std::string name;           // output name
	bool need_declare;          // needs explicit declaration as output (not language in-built)
	std::string src_reg;        // reg to get data from
	std::string src_reg_mask;   // input swizzle mask
	bool need_cast;             // needs casting
	std::string cond;           // update on condition
	std::string default_val;    // fallback value on cond fail
	std::string dst_alias;      // output name override
	bool check_mask;            // check program output control mask for this output
	u32  check_mask_value;      // program output control mask for testing

	mask_test_type check_flags; // whole mask must match

	bool test(u32 mask) const
	{
		if (!check_mask)
			return true;

		const u32 val = (mask & check_mask_value);
		switch (check_flags)
		{
		case none:
			return (val == 0);
		case any:
			return (val != 0);
		case all:
			return (val == check_mask_value);
		case xall:
			return (val && val != check_mask_value);
		default:
			fmt::throw_exception("Unreachable" HERE);
		}
	}

	bool declare(u32 mask) const
	{
		return test(mask);
	}
};
