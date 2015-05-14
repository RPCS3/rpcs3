#pragma once
#if defined(DX12_SUPPORT)
#include "Emu/RSX/RSXVertexProgram.h"
#include <vector>
#include <sstream>

enum ParamFlag
{
	PARAM_IN,
	PARAM_OUT,
	PARAM_UNIFORM,
	PARAM_CONST,
	PARAM_NONE,
	PARAM_COUNT,
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
	std::vector<ParamType> params[PARAM_COUNT];

	ParamType* SearchParam(const ParamFlag &flag, const std::string& type)
	{
		for (u32 i = 0; i<params[flag].size(); ++i)
		{
			if (params[flag][i].type.compare(type) == 0)
				return &params[flag][i];
		}

		return nullptr;
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

	size_t get_vector_size() const
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

struct VertexDecompiler
{
	struct FuncInfo
	{
		u32 offset;
		std::string name;
	};

	struct Instruction
	{
		std::vector<std::string> body;
		int open_scopes;
		int close_scopes;
		int put_close_scopes;
		int do_count;

		void reset()
		{
			body.clear();
			put_close_scopes = open_scopes = close_scopes = do_count = 0;
		}
	};

	static const size_t m_max_instr_count = 512;
	Instruction m_instructions[m_max_instr_count];
	Instruction* m_cur_instr;
	size_t m_instr_count;

	std::set<int> m_jump_lvls;
	std::vector<std::string> m_body;
	std::vector<FuncInfo> m_funcs;

	//wxString main;

	std::vector<u32>& m_data;
	ParamArray m_parr;

	std::string GetMask(bool is_sca);
	std::string GetVecMask();
	std::string GetScaMask();
	std::string GetDST(bool is_sca = false);
	std::string GetSRC(const u32 n);
	std::string GetFunc();
	std::string GetTex();
	std::string GetCond();
	std::string AddAddrMask();
	std::string AddAddrReg();
	u32 GetAddr();
	std::string Format(const std::string& code);

	void AddCodeCond(const std::string& dst, const std::string& src);
	void AddCode(const std::string& code);
	void SetDST(bool is_sca, std::string value);
	void SetDSTVec(const std::string& code);
	void SetDSTSca(const std::string& code);
	std::string BuildFuncBody(const FuncInfo& func);
	std::string BuildCode();

protected:
	virtual void insertHeader(std::stringstream &OS);
	virtual void insertInputs(std::stringstream &OS, const std::vector<ParamType> &inputs);
	virtual void insertConstants(std::stringstream &OS, const std::vector<ParamType> &constants);
	virtual void insertOutputs(std::stringstream &OS, const std::vector<ParamType> &outputs);
	virtual void insertMainStart(std::stringstream &OS);
	virtual void insertMainEnd(std::stringstream &OS);
public:
	VertexDecompiler(std::vector<u32>& data);
	std::string Decompile();
};
#endif