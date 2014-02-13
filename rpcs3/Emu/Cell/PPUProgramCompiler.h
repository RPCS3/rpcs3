#pragma once
#include "PPUInstrTable.h"
#include "Loader/ELF64.h"

enum ArgType
{
	ARG_ERR		= 0,
	ARG_NUM		= 1 << 0,
	ARG_NUM16	= 1 << 1,
	ARG_TXT		= 1 << 2,
	ARG_REG_R	= 1 << 3,
	ARG_REG_F	= 1 << 4,
	ARG_REG_V	= 1 << 5,
	ARG_REG_CR	= 1 << 6,
	ARG_BRANCH	= 1 << 7,
	ARG_INSTR	= 1 << 8,
	ARG_IMM		= ARG_NUM | ARG_NUM16 | ARG_BRANCH,
};

struct Arg
{
	std::string string;
	u32 value;
	ArgType type;

	Arg(const wxString& _string, const u32 _value = 0, const ArgType _type = ARG_ERR)
		: value(_value)
		, type(_type)
	{
		string =  _string.ToStdString();
	}
};

struct SectionInfo
{
	Elf64_Shdr shdr;
	std::string name;
	Array<u8> code;
	u32 section_num;

	SectionInfo(const wxString& name);
	~SectionInfo();

	void SetDataSize(u32 size, u32 align = 0);
};

struct ProgramInfo
{
	Array<u8> code;
	Elf64_Phdr phdr;
	bool is_preload;

	ProgramInfo()
	{
		is_preload = false;
		code.Clear();
		memset(&phdr, 0, sizeof(Elf64_Phdr));
	}
};

class CompilePPUProgram
{
	struct Branch
	{
		std::string m_name;
		s32 m_pos;
		s32 m_id;
		s32 m_addr;

		Branch(const wxString& name, s32 pos)
			: m_pos(pos)
			, m_id(-1)
			, m_addr(-1)
		{
			m_name = name.ToStdString();
		}

		Branch(const wxString& name, u32 id, u32 addr)
			: m_pos(-1)
			, m_id(id)
			, m_addr(addr)
		{
			m_name = name.ToStdString();
		}
	};

	bool m_analyze;
	s64 p;
	u64 m_line;
	const wxString& m_asm;
	wxTextCtrl* m_asm_list;
	wxTextCtrl* m_hex_list;
	wxTextCtrl* m_err_list;
	bool m_error;
	Array<u32> m_code;
	bool m_end_args;
	Array<Branch> m_branches;
	s32 m_branch_pos;
	u32 m_text_addr;
	wxString m_file_path;

	struct SpData
	{
		std::string m_data;
		u32 m_addr;

		SpData(const wxString& data, u32 addr)
			: m_addr(addr)
		{
			m_data = data.ToStdString();
		}
	};

	Array<SpData> m_sp_string;
	Array<Arg> m_args;
	u32 m_cur_arg;

public:
	CompilePPUProgram(
			const wxString& asm_,
			const wxString& file_path = wxEmptyString,
			wxTextCtrl* asm_list = nullptr,
			wxTextCtrl* hex_list = nullptr,
			wxTextCtrl* err_list = nullptr,
			bool analyze = false);

	static bool IsSkip(const char c);
	static bool IsCommit(const char c);

protected:
	bool IsEnd() const;
	bool IsEndLn(const char c) const;

	void WriteHex(const wxString& text);
	void WriteError(const wxString& error);

	char NextChar();
	void NextLn();
	void EndLn();

	void FirstChar();
	void PrevArg();

	bool GetOp(wxString& result);
	int GetArg(wxString& result, bool func = false);

	bool CheckEnd(bool show_err = true);

	void DetectArgInfo(Arg& arg);
	void LoadArgs();
	u32 GetBranchValue(const wxString& branch);

	bool SetNextArgType(u32 types, bool show_err = true);
	bool SetNextArgBranch(u8 aa, bool show_err = true);

public:
	static bool IsBranchOp(const wxString& op);
	static bool IsFuncOp(const wxString& op);

	enum SP_TYPE
	{
		SP_ERR,
		SP_INT,
		SP_STRING,
		SP_STRLEN,
		SP_BUF,
		SP_SRL,
		SP_SRR,
		SP_MUL,
		SP_DIV,
		SP_ADD,
		SP_SUB,
		SP_AND,
		SP_OR,
		SP_XOR,
		SP_NOT,
		SP_NOR,
	};

	static SP_TYPE GetSpType(const wxString& op);
	static wxString GetSpStyle(const SP_TYPE sp);

	static bool IsSpOp(const wxString& op);

protected:
	Branch& GetBranch(const wxString& name);
	void SetSp(const wxString& name, u32 addr, bool create);
	void LoadSp(const wxString& op, Elf64_Shdr& s_opd);

public:
	void Compile();
};
