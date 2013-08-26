#pragma once
#include "OpenGL.h"

enum ParamFlag
{
	PARAM_IN,
	PARAM_OUT,
	PARAM_UNIFORM,
	PARAM_CONST,
	PARAM_NONE,
};

struct ParamItem
{
	ArrayString name;
	ArrayString location;
	ArrayString value;

	ParamItem(const wxString& _name, int _location, const wxString& _value = wxEmptyString)
		: name(_name)
		, location(_location > -1 ? wxString::Format("layout (location = %d) ", _location) : "")
		, value(_value)
	{
	}
};

struct ParamType
{
	const ParamFlag flag;
	ArrayString type;
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
			if(name.Cmp(items[i].name.GetPtr()) == 0) return true;
		}

		return false;
	}

	wxString Format()
	{
		wxString ret = wxEmptyString;

		for(u32 n=0; n<items.GetCount(); ++n)
		{
			ret += items[n].location.GetPtr();
			ret += type.GetPtr();
			ret += " ";
			ret += items[n].name.GetPtr();
			if(items[n].value.GetCount())
			{
				ret += " = ";
				ret += items[n].value.GetPtr();
			}
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
			if(type.Cmp(params[i].type.GetPtr()) == 0) return &params[i];
		}

		return nullptr;
	}

	wxString GetParamFlag(const ParamFlag flag)
	{
		switch(flag)
		{
		case PARAM_OUT:		return "out ";
		case PARAM_IN:		return "in ";
		case PARAM_UNIFORM:	return "uniform ";
		case PARAM_CONST:	return "const ";
		}

		return wxEmptyString;
	}

	bool HasParam(const ParamFlag flag, wxString type, const wxString& name)
	{
		type = GetParamFlag(flag) + type;
		ParamType* t = SearchParam(type);
		return t && t->SearchName(name);
	}

	wxString AddParam(const ParamFlag flag, wxString type, const wxString& name, const wxString& value)
	{
		type = GetParamFlag(flag) + type;
		ParamType* t = SearchParam(type);

		if(t)
		{
			if(!t->SearchName(name)) t->items.Move(new ParamItem(name, -1, value));
		}
		else
		{
			const u32 num = params.GetCount();
			params.Move(new ParamType(flag, type));
			params[num].items.Move(new ParamItem(name, -1, value));
		}

		return name;
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