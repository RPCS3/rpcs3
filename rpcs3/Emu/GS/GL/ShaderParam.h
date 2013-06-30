#pragma once
#include "OpenGL.h"

enum ParamFlag
{
	PARAM_IN,
	PARAM_OUT,
	PARAM_CONST,
	PARAM_NONE,
};

struct ParamItem
{
	wxString name;
	wxString location;

	ParamItem(const wxString& _name, int _location)
		: name(_name)
		, location(_location > -1 ? wxString::Format("layout (location = %d) ", _location) : "")
	{
	}
};

struct ParamType
{
	const ParamFlag flag;
	wxString type;
	Array<ParamItem> items;

	ParamType(const ParamFlag _flag, const wxString& _type)
		: type(_type)
		, flag(_flag)
	{
	}

	bool SearchName(const wxString& name)
	{
		for(u32 i=0; i<items.GetCount(); ++i)
		{
			if(items[i].name.Cmp(name) == 0) return true;
		}

		return false;
	}

	wxString Format()
	{
		wxString ret = wxEmptyString;

		for(u32 n=0; n<items.GetCount(); ++n)
		{
			ret += items[n].location;
			ret += type;
			ret += " ";
			ret += items[n].name;
			ret += ";\n";
		}

		return ret;
	}
};

struct ParamArray
{
	Array<ParamType> params;

	ParamType* SearchParam(const wxString& type)
	{
		for(u32 i=0; i<params.GetCount(); ++i)
		{
			if(params[i].type.Cmp(type) == 0) return &params[i];
		}

		return NULL;
	}

	wxString GetParamFlag(const ParamFlag flag)
	{
		switch(flag)
		{
		case PARAM_OUT:		return "out ";
		case PARAM_IN:		return "in ";
		case PARAM_CONST:	return "uniform ";
		}

		return wxEmptyString;
	}

	wxString AddParam(const ParamFlag flag, wxString type, const wxString& name, int location = -1)
	{
		type = GetParamFlag(flag) + type;
		ParamType* t = SearchParam(type);

		if(t)
		{
			if(!t->SearchName(name)) t->items.Move(new ParamItem(name, location));
		}
		else
		{
			const u32 num = params.GetCount();
			params.Move(new ParamType(flag, type));
			params[num].items.Move(new ParamItem(name, location));
		}

		return name;
	}
};