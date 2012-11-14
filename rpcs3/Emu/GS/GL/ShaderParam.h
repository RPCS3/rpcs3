#pragma once
#include "OpenGL.h"

enum ParamFlag
{
	PARAM_IN,
	PARAM_OUT,
	PARAM_CONST,
	PARAM_NONE,
};

struct ParamType
{
	const ParamFlag flag;
	wxString type;
	wxArrayString names;

	ParamType(const ParamFlag _flag, const wxString& _type)
		: type(_type)
		, flag(_flag)
	{
	}

	bool SearchName(const wxString& name)
	{
		for(u32 i=0; i<names.GetCount(); ++i)
		{
			if(names[i].Cmp(name) == 0) return true;
		}

		return false;
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

	wxString AddParam(const ParamFlag flag, wxString type, const wxString& name)
	{
		type = GetParamFlag(flag) + type;
		ParamType* t = SearchParam(type);
		if(t)
		{
			if(!t->SearchName(name)) t->names.Add(name);
		}
		else
		{
			const u32 num = params.GetCount();
			params.Add(new ParamType(flag, type));
			params[num].names.Add(name);
		}

		return name;
	}
};