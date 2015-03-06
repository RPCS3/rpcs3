#pragma once
#include "OpenGL.h"

enum GLParamFlag
{
	PARAM_IN,
	PARAM_OUT,
	PARAM_UNIFORM,
	PARAM_CONST,
	PARAM_NONE,
};

struct GLParamItem
{
	std::string name;
	std::string location;
	std::string value;

	GLParamItem(const std::string& _name, int _location, const std::string& _value = "")
		: name(_name)
		, value(_value)
	{
		if (_location > -1)
			location = "layout (location = " + std::to_string(_location) + ") ";
		else
			location = "";
	}
};

struct GLParamType
{
	const GLParamFlag flag;
	std::string type;
	std::vector<GLParamItem> items;

	GLParamType(const GLParamFlag _flag, const std::string& _type)
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

	std::string Format()
	{
		std::string ret = "";

		for (u32 n = 0; n<items.size(); ++n)
		{
			ret += items[n].location + type + " " + items[n].name;
			if (!items[n].value.empty())
			{
				ret += " = " + items[n].value;
			}
			ret += ";\n";
		}

		return ret;
	}
};

struct GLParamArray
{
	std::vector<GLParamType> params;

	GLParamType* SearchParam(const std::string& type)
	{
		for (u32 i = 0; i<params.size(); ++i)
		{
			if (params[i].type.compare(type) == 0)
				return &params[i];
		}

		return nullptr;
	}

	std::string GetParamFlag(const GLParamFlag flag)
	{
		switch (flag)
		{
		case PARAM_OUT:     return "out ";
		case PARAM_IN:      return "in ";
		case PARAM_UNIFORM: return "uniform ";
		case PARAM_CONST:   return "const ";
		default:            return "";
		}
	}

	bool HasParam(const GLParamFlag flag, std::string type, const std::string& name)
	{
		type = GetParamFlag(flag) + type;
		GLParamType* t = SearchParam(type);
		return t && t->SearchName(name);
	}

	std::string AddParam(const GLParamFlag flag, std::string type, const std::string& name, const std::string& value)
	{
		type = GetParamFlag(flag) + type;
		GLParamType* t = SearchParam(type);

		if (t)
		{
			if (!t->SearchName(name)) t->items.emplace_back(name, -1, value);
		}
		else
		{
			const u32 num = params.size();
			params.emplace_back(flag, type);
			params[num].items.emplace_back(name, -1, value);
		}

		return name;
	}

	std::string AddParam(const GLParamFlag flag, std::string type, const std::string& name, int location = -1)
	{
		type = GetParamFlag(flag) + type;
		GLParamType* t = SearchParam(type);

		if (t)
		{
			if (!t->SearchName(name)) t->items.emplace_back(name, location);
		}
		else
		{
			const u32 num = params.size();
			params.emplace_back(flag, type);
			params[num].items.emplace_back(name, location);
		}

		return name;
	}
};

class ShaderVar
{
public:
	std::string name;
	std::vector<std::string> swizzles;

	ShaderVar() = default;
	ShaderVar(const std::string& var)
	{
		auto var_blocks = fmt::split(var, { "." });

		if (var_blocks.size() == 0)
		{
			assert(0);
		}

		name = var_blocks[0];

		if (var_blocks.size() == 1)
		{
			swizzles.push_back("xyzw");
		}
		else
		{
			swizzles = std::vector<std::string>(var_blocks.begin() + 1, var_blocks.end());
		}
	}

	int get_vector_size() const
	{
		return swizzles[swizzles.size() - 1].length();
	}

	ShaderVar& symplify()
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
};