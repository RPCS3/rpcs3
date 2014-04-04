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
	Array<GLParamItem> items;

	GLParamType(const GLParamFlag _flag, const std::string& _type)
		: flag(_flag)
		, type(_type)
	{
	}

	bool SearchName(const std::string& name)
	{
		for(u32 i=0; i<items.GetCount(); ++i)
		{
			if(items[i].name.compare(name) == 0) return true;
		}

		return false;
	}

	std::string Format()
	{
		std::string ret = "";

		for(u32 n=0; n<items.GetCount(); ++n)
		{
			ret += items[n].location + type + " " + items[n].name;
			if(!items[n].value.empty())
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
	Array<GLParamType> params;

	GLParamType* SearchParam(const std::string& type)
	{
		for(u32 i=0; i<params.GetCount(); ++i)
		{
			if (params[i].type.compare(type) == 0)
				return &params[i];
		}

		return nullptr;
	}

	std::string GetParamFlag(const GLParamFlag flag)
	{
		switch(flag)
		{
		case PARAM_OUT:     return "out ";
		case PARAM_IN:      return "in ";
		case PARAM_UNIFORM: return "uniform ";
		case PARAM_CONST:   return "const ";
		}

		return "";
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

		if(t)
		{
			if(!t->SearchName(name)) t->items.Move(new GLParamItem(name, -1, value));
		}
		else
		{
			const u32 num = params.GetCount();
			params.Move(new GLParamType(flag, type));
			params[num].items.Move(new GLParamItem(name, -1, value));
		}

		return name;
	}

	std::string AddParam(const GLParamFlag flag, std::string type, const std::string& name, int location = -1)
	{
		type = GetParamFlag(flag) + type;
		GLParamType* t = SearchParam(type);

		if(t)
		{
			if(!t->SearchName(name)) t->items.Move(new GLParamItem(name, location));
		}
		else
		{
			const u32 num = params.GetCount();
			params.Move(new GLParamType(flag, type));
			params[num].items.Move(new GLParamItem(name, location));
		}

		return name;
	}
};