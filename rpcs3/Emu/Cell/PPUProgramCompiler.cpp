#include "stdafx.h"
#include "PPUProgramCompiler.h"

using namespace PPU_instr;

template<typename TO, typename T>
InstrBase<TO>* GetInstruction(T* list, const wxString& str)
{
	for(int i=0; i<list->count; ++i)
	{
		auto instr = list->get_instr_info(i);

		if(instr)
		{
			if(instr->GetName().Cmp(str) == 0)
			{
				return instr;
			}
		}
	}

	return nullptr;
}

template<typename TO>
InstrBase<TO>* GetInstruction(const wxString& str)
{
	if(auto res = GetInstruction<TO>(main_list, str)) return res;
	if(auto res = GetInstruction<TO>(g04_list, str)) return res;
	if(auto res = GetInstruction<TO>(g04_0_list, str)) return res;
	if(auto res = GetInstruction<TO>(g13_list, str)) return res;
	if(auto res = GetInstruction<TO>(g1e_list, str)) return res;
	if(auto res = GetInstruction<TO>(g1f_list, str)) return res;
	if(auto res = GetInstruction<TO>(g3a_list, str)) return res;
	if(auto res = GetInstruction<TO>(g3b_list, str)) return res;
	if(auto res = GetInstruction<TO>(g3e_list, str)) return res;
	if(auto res = GetInstruction<TO>(g3f_list, str)) return res;
	if(auto res = GetInstruction<TO>(g3f_0_list, str)) return res;

	return nullptr;
}

s64 FindOp(const wxString& text, const wxString& op, s64 from)
{
	if(text.Len() < op.Len()) return -1;

	for(s64 i=from; i<text.Len(); ++i)
	{
		if(i - 1 < 0 || text[i - 1] == '\n' || CompilePPUProgram::IsSkip(text[i - 1]))
		{
			if(text.Len() - i < op.Len()) return -1;

			if(text(i, op.Len()).Cmp(op) != 0) continue;
			if(i + op.Len() >= text.Len() || text[i + op.Len()] == '\n' ||
				CompilePPUProgram::IsSkip(text[i + op.Len()])) return i;
		}
	}

	return -1;
}

ArrayF<SectionInfo> sections_list;
u32 section_name_offs = 0;
u32 section_offs = 0;

SectionInfo::SectionInfo(const wxString& _name) : name(_name)
{
	memset(&shdr, 0, sizeof(Elf64_Shdr));

	section_num = sections_list.Add(this);

	shdr.sh_offset = section_offs;
	shdr.sh_name = section_name_offs;

	section_name_offs += name.Len() + 1;
}

void SectionInfo::SetDataSize(u32 size, u32 align)
{
	if(align) shdr.sh_addralign = align;
	if(shdr.sh_addralign) size = Memory.AlignAddr(size, shdr.sh_addralign);

	if(code.GetCount())
	{
		for(u32 i=section_num + 1; i<sections_list.GetCount(); ++i)
		{
			sections_list[i].shdr.sh_offset -= code.GetCount();
		}

		section_offs -= code.GetCount();
	}

	code.SetCount(size);

	section_offs += size;

	for(u32 i=section_num + 1; i<sections_list.GetCount(); ++i)
	{
		sections_list[i].shdr.sh_offset += size;
	}
}

SectionInfo::~SectionInfo()
{
	sections_list.RemoveFAt(section_num);

	for(u32 i=section_num + 1; i<sections_list.GetCount(); ++i)
	{
		sections_list[i].shdr.sh_offset -= code.GetCount();
		sections_list[i].shdr.sh_name -= name.Len();
	}

	section_offs -= code.GetCount();
	section_name_offs -= name.Len();
}

CompilePPUProgram::CompilePPUProgram(
		const wxString& asm_,
		const wxString& file_path,
		wxTextCtrl* asm_list,
		wxTextCtrl* hex_list,
		wxTextCtrl* err_list,
		bool analyze)
	: m_asm(asm_)
	, m_file_path(file_path)
	, m_asm_list(asm_list)
	, m_hex_list(hex_list)
	, m_err_list(err_list)
	, m_analyze(analyze)
	, p(0)
	, m_error(false)
	, m_line(1)
	, m_end_args(false)
	, m_branch_pos(0)
	, m_text_addr(0)
{
}

void CompilePPUProgram::WriteHex(const wxString& text)
{
	if(m_hex_list)
	{
		m_hex_list->WriteText(text);
	}
}

void CompilePPUProgram::WriteError(const wxString& error)
{
	if(m_err_list)
	{
		m_err_list->WriteText(wxString::Format("line %lld: %s\n", m_line, error));
	}
}

bool CompilePPUProgram::IsSkip(const char c) { return c == ' ' || c == '\t'; }
bool CompilePPUProgram::IsCommit(const char c) { return c == '#'; }
bool CompilePPUProgram::IsEnd() const { return p >= m_asm.Len(); }
bool CompilePPUProgram::IsEndLn(const char c) const { return c == '\n' || p - 1 >= m_asm.Len(); }

char CompilePPUProgram::NextChar() { return *m_asm(p++, 1); }
void CompilePPUProgram::NextLn() { while( !IsEndLn(NextChar()) ); if(!IsEnd()) m_line++; }
void CompilePPUProgram::EndLn()
{
	NextLn();
	p--;
	m_line--;
}

void CompilePPUProgram::FirstChar()
{
	p = 0;
	m_line = 1;
	m_branch_pos = 0;
}

void CompilePPUProgram::PrevArg()
{
	while( --p >= 0 && (IsSkip(m_asm[p]) || m_asm[p] == ','));
	while( --p >= 0 && !IsSkip(m_asm[p]) && !IsEndLn(m_asm[p]) );
	if(IsEndLn(m_asm[p])) p++;
}

bool CompilePPUProgram::GetOp(wxString& result)
{
	s64 from = -1;

	for(;;)
	{
		const char cur_char = NextChar();

		const bool skip = IsSkip(cur_char);
		const bool commit = IsCommit(cur_char);
		const bool endln = IsEndLn(cur_char);

		if(endln) p--;

		if(from == -1)
		{
			if(endln || commit) return false;
			if(!skip) from = p - 1;
			continue;
		}

		if(skip || endln || commit)
		{
			const s64 to = (endln ? p : p - 1) - from;
			result = m_asm(from, to);

			return true;
		}
	}

	return false;
}

int CompilePPUProgram::GetArg(wxString& result, bool func)
{
	s64 from = -1;

	for(char cur_char = NextChar(); !m_error; cur_char = NextChar())
	{
		const bool skip = IsSkip(cur_char);
		const bool commit = IsCommit(cur_char);
		const bool endln = IsEndLn(cur_char);
		const bool end = cur_char == ',' || (func && cur_char == ']');

		if(endln) p--;

		if(from == -1)
		{
			if(endln || commit || end) return 0;
			if(!skip) from = p - 1;
			continue;
		}

		const bool text = m_asm[from] == '"';
		const bool end_text = cur_char == '"';

		if((text ? end_text : (skip || commit || end)) || endln)
		{
			if(text && p > 2 && m_asm[p - 2] == '\\' && (p <= 3 || m_asm[p - 3] != '\\'))
			{
				continue;
			}

			if(text && !end_text)
			{
				WriteError(wxString::Format("'\"' not found."));
				m_error = true;
			}

			const s64 to = ((endln || text) ? p : p - 1) - from;
			int ret = 1;

			if(skip || text)
			{
				for(char cur_char = NextChar(); !m_error; cur_char = NextChar())
				{
					const bool skip = IsSkip(cur_char);
					const bool commit = IsCommit(cur_char);
					const bool endln = IsEndLn(cur_char);
					const bool end = cur_char == ',' || (func && cur_char == ']');

					if(skip) continue;
					if(end) break;

					if(commit)
					{
						EndLn();
						ret = -1;
						break;
					}

					if(endln)
					{
						p--;
						break;
					}

					WriteError(wxString::Format("Bad symbol '%c'", cur_char));
					m_error = true;
					break;
				}
			}

			result = m_asm(from, to);

			if(text)
			{
				for(u32 pos = 0; (s32)(pos = result.find('\\', pos)) >= 0;)
				{
					if(pos + 1 < result.Len() && result[pos + 1] == '\\')
					{
						pos += 2;
						continue;
					}

					const char v = result[pos + 1];
					switch(v)
					{
					case 'n': result = result(0, pos) + '\n' + result(pos+2, result.Len()-(pos+2)); break;
					case 'r': result = result(0, pos) + '\r' + result(pos+2, result.Len()-(pos+2)); break;
					case 't': result = result(0, pos) + '\t' + result(pos+2, result.Len()-(pos+2)); break;
					}
						
					pos++;
				}
			}

			return ret;
		}
	}

	return 0;
}

bool CompilePPUProgram::CheckEnd(bool show_err)
{
	if(m_error)
	{
		NextLn();
		return false;
	}

	while(true)
	{
		const char cur_char = NextChar();
		const bool skip = IsSkip(cur_char);
		const bool commit = IsCommit(cur_char);
		const bool endln = IsEndLn(cur_char);

		if(skip) continue;

		if(commit)
		{
			NextLn();
			return true;
		}

		if(endln)
		{
			p--;
			NextLn();
			return true;
		}

		WriteError(wxString::Format("Bad symbol '%c'", cur_char));
		NextLn();
		return false;
	}

	return false;
}

void CompilePPUProgram::DetectArgInfo(Arg& arg)
{
	const wxString str = arg.string;

	if(str.Len() <= 0)
	{
		arg.type = ARG_ERR;
		return;
	}

	if(GetInstruction<PPU_Opcodes>(str))
	{
		arg.type = ARG_INSTR;
		return;
	}

	if(str.Len() > 1)
	{
		for(u32 i=0; i<m_branches.GetCount(); ++i)
		{
			if(m_branches[i].m_name.Cmp(str) != 0) continue;

			arg.type = ARG_BRANCH;
			arg.value = GetBranchValue(str);
			return;
		}
	}

	switch(str[0])
	{
	case 'r': case 'f': case 'v':

		if(str.Len() < 2)
		{
			arg.type = ARG_ERR;
			return;
		}

		if(str.Cmp("rtoc") == 0)
		{
			arg.type = ARG_REG_R;
			arg.value = 2;
			return;
		}

		for(u32 i=1; i<str.Len(); ++i)
		{
			if(str[i] < '0' || str[i] > '9')
			{
				arg.type = ARG_ERR;
				return;
			}
		}

		u32 reg;
		sscanf(str(1, str.Len() - 1), "%d", &reg);

		if(reg >= 32)
		{
			arg.type = ARG_ERR;
			return;
		}

		switch(str[0])
		{
		case 'r': arg.type = ARG_REG_R; break;
		case 'f': arg.type = ARG_REG_F; break;
		case 'v': arg.type = ARG_REG_V; break;
		default: arg.type = ARG_ERR; break;
		}

		arg.value = reg;
	return;
		
	case 'c':
		if(str.Len() > 2 && str[1] == 'r')
		{
			for(u32 i=2; i<str.Len(); ++i)
			{
				if(str[i] < '0' || str[i] > '9')
				{
					arg.type = ARG_ERR;
					return;
				}
			}

			u32 reg;
			sscanf(str, "cr%d", &reg);

			if(reg < 8)
			{
				arg.type = ARG_REG_CR;
				arg.value = reg;
			}
			else
			{
				arg.type = ARG_ERR;
			}

			return;
		}
	break;

	case '"':
		if(str.Len() < 2)
		{
			arg.type = ARG_ERR;
			return;
		}

		if(str[str.Len() - 1] != '"')
		{
			arg.type = ARG_ERR;
			return;
		}

		arg.string = str(1, str.Len() - 2);
		arg.type = ARG_TXT;
	return;
	}

	if(str.Len() > 2 && str(0, 2).Cmp("0x") == 0)
	{
		for(u32 i=2; i<str.Len(); ++i)
		{
			if(
				(str[i] >= '0' && str[i] <= '9') ||
				(str[i] >= 'a' && str[i] <= 'f') || 
				(str[i] >= 'A' && str[i] <= 'F')
			) continue;

			arg.type = ARG_ERR;
			return;
		}

		u32 val;
		sscanf(str, "0x%x", &val);

		arg.type = ARG_NUM16;
		arg.value = val;
		return;
	}

	for(u32 i= str[0] == '-' ? 1 : 0; i<str.Len(); ++i)
	{
		if(str[i] < '0' || str[i] > '9')
		{
			arg.type = ARG_ERR;
			return;
		}
	}

	u32 val;
	sscanf(str, "%d", &val);

	arg.type = ARG_NUM;
	arg.value = val;
}

void CompilePPUProgram::LoadArgs()
{
	m_args.Clear();
	m_cur_arg = 0;

	wxString str;
	while(int r = GetArg(str))
	{
		Arg* arg = new Arg(str);
		DetectArgInfo(*arg);
		m_args.Add(arg);
		if(r == -1) break;
	}

	m_end_args = m_args.GetCount() > 0;
}

u32 CompilePPUProgram::GetBranchValue(const wxString& branch)
{
	for(u32 i=0; i<m_branches.GetCount(); ++i)
	{
		if(m_branches[i].m_name.Cmp(branch) != 0) continue;
		if(m_branches[i].m_pos >= 0) return m_text_addr + m_branches[i].m_pos * 4;

		return m_branches[i].m_addr;
	}

	return 0;
}

bool CompilePPUProgram::SetNextArgType(u32 types, bool show_err)
{
	if(m_error) return false;

	if(m_cur_arg >= m_args.GetCount())
	{
		if(show_err)
		{
			WriteError(wxString::Format("%d arg not found", m_cur_arg + 1));
			m_error = true;
		}

		return false;
	}
		
	const Arg& arg = m_args[m_cur_arg];

	if(arg.type & types)
	{
		m_cur_arg++;
		return true;
	}

	if(show_err)
	{
		WriteError(wxString::Format("Bad arg '%s'", arg.string));
		m_error = true;
	}

	return false;
}

bool CompilePPUProgram::SetNextArgBranch(u8 aa, bool show_err)
{
	const u32 pos = m_cur_arg;
	const bool ret = SetNextArgType(ARG_BRANCH | ARG_IMM, show_err);

	if(!aa && pos < m_args.GetCount())
	{
		switch(m_args[pos].type)
		{
		case ARG_NUM:
			m_args[pos].value += m_text_addr + m_branch_pos * 4;
		break;

		case ARG_BRANCH:
			m_args[pos].value -= m_text_addr + m_branch_pos * 4;
		break;
		}
	}

	return ret;
}

bool CompilePPUProgram::IsBranchOp(const wxString& op)
{
	return op.Len() > 1 && op[op.Len() - 1] == ':';
}

bool CompilePPUProgram::IsFuncOp(const wxString& op)
{
	return op.Len() >= 1 && op[0] == '[';
}

CompilePPUProgram::SP_TYPE CompilePPUProgram::GetSpType(const wxString& op)
{
	if(op.Cmp(".int") == 0) return SP_INT;
	if(op.Cmp(".string") == 0) return SP_STRING;
	if(op.Cmp(".strlen") == 0) return SP_STRLEN;
	if(op.Cmp(".buf") == 0) return SP_BUF;
	if(op.Cmp(".srl") == 0) return SP_SRL;
	if(op.Cmp(".srr") == 0) return SP_SRR;
	if(op.Cmp(".mul") == 0) return SP_MUL;
	if(op.Cmp(".div") == 0) return SP_DIV;
	if(op.Cmp(".add") == 0) return SP_ADD;
	if(op.Cmp(".sub") == 0) return SP_SUB;
	if(op.Cmp(".and") == 0) return SP_AND;
	if(op.Cmp(".or") == 0) return SP_OR;
	if(op.Cmp(".xor") == 0) return SP_XOR;
	if(op.Cmp(".not") == 0) return SP_NOT;
	if(op.Cmp(".nor") == 0) return SP_NOR;

	return SP_ERR;
}

wxString CompilePPUProgram::GetSpStyle(const SP_TYPE sp)
{
	switch(sp)
	{
	case SP_INT:
	case SP_STRING:
	case SP_STRLEN:
	case SP_NOT:
		return "[dst, src]";

	case SP_BUF:
		return "[dst, size]";

	case SP_SRL:
	case SP_SRR:
	case SP_MUL:
	case SP_DIV:
	case SP_ADD:
	case SP_SUB:
	case SP_AND:
	case SP_OR:
	case SP_XOR:
	case SP_NOR:
		return "[dst, src1, src2]";
	}

	return "error";
}

bool CompilePPUProgram::IsSpOp(const wxString& op)
{
	return GetSpType(op) != SP_ERR;
}

CompilePPUProgram::Branch& CompilePPUProgram::GetBranch(const wxString& name)
{
	for(u32 i=0; i<m_branches.GetCount(); ++i)
	{
		if(m_branches[i].m_name.Cmp(name) != 0) continue;

		return m_branches[i];
	}

	return m_branches[0];
}

void CompilePPUProgram::SetSp(const wxString& name, u32 addr, bool create)
{
	if(create)
	{
		m_branches.Move(new Branch(name, -1, addr));
		return;
	}

	GetBranch(name);

	for(u32 i=0; i<m_branches.GetCount(); ++i)
	{
		if(m_branches[i].m_name.Cmp(name) != 0) continue;
		m_branches[i].m_addr = addr;
	}
}

void CompilePPUProgram::LoadSp(const wxString& op, Elf64_Shdr& s_opd)
{
	SP_TYPE sp = GetSpType(op);

	wxString test;
	if(!GetArg(test) || test[0] != '[')
	{
		if(m_analyze) WriteHex("error\n");
		WriteError(wxString::Format("data not found. style: %s", GetSpStyle(sp)));
		m_error = true;
		NextLn();
		return;
	}

	while(p > 0 && m_asm[p] != '[') p--;
	p++;

	wxString dst;
	if(!GetArg(dst))
	{
		if(m_analyze) WriteHex("error\n");
		WriteError(wxString::Format("dst not found. style: %s", GetSpStyle(sp)));
		m_error = true;
		NextLn();
		return;
	}

	Arg a_dst(dst);
	DetectArgInfo(a_dst);

	Branch* dst_branch = NULL;

	switch(a_dst.type)
	{
		case ARG_BRANCH:
		{
			Branch& b = GetBranch(dst);
			if(b.m_addr >= 0 && b.m_id < 0 && b.m_pos < 0) dst_branch = &b;
		}
		break;

		case ARG_ERR:
		{
			m_branches.Move(new Branch(wxEmptyString, -1, 0));
			dst_branch = &m_branches[m_branches.GetCount() - 1];
		}
		break;
	}

	if(!dst_branch)
	{
		if(m_analyze) WriteHex("error\n");
		WriteError(wxString::Format("bad dst type. style: %s", GetSpStyle(sp)));
		m_error = true;
		NextLn();
		return;
	}

	switch(sp)
	{
	case SP_INT:
	case SP_STRING:
	case SP_STRLEN:
	case SP_BUF:
	case SP_NOT:
	{
		wxString src1;
		if(!GetArg(src1, true))
		{
			if(m_analyze) WriteHex("error\n");
			WriteError(wxString::Format("src not found. style: %s", GetSpStyle(sp)));
			m_error = true;
			NextLn();
			return;
		}

		Arg a_src1(src1);
		DetectArgInfo(a_src1);

		if(sp == SP_STRLEN 
			? ~(ARG_TXT | ARG_BRANCH) & a_src1.type
			: sp == SP_STRING 
				? ~ARG_TXT & a_src1.type
				: ~(ARG_IMM | ARG_BRANCH) & a_src1.type)
		{
			if(m_analyze) WriteHex("error\n");
			WriteError(wxString::Format("bad src type. style: %s", GetSpStyle(sp)));
			m_error = true;
			NextLn();
			return;
		}

		if(m_asm[p - 1] != ']')
		{
			if(m_analyze) WriteHex("error\n");
			WriteError(wxString::Format("']' not found. style: %s", GetSpStyle(sp)));
			m_error = true;
			NextLn();
			return;
		}

		if(!CheckEnd())
		{
			if(m_analyze) WriteHex("error\n");
			return;
		}

		if(sp == SP_STRING)
		{
			src1 = src1(1, src1.Len()-2);
			bool founded = false;

			for(u32 i=0; i<m_sp_string.GetCount(); ++i)
			{
				if(m_sp_string[i].m_data.Cmp(src1) != 0) continue;
				*dst_branch = Branch(dst, -1, m_sp_string[i].m_addr);
				founded = true;
			}

			if(!founded)
			{
				const u32 addr = s_opd.sh_addr + s_opd.sh_size;
				m_sp_string.Move(new SpData(src1, addr));
				s_opd.sh_size += src1.Len() + 1;
				*dst_branch = Branch(dst, -1, addr);
			}
		}
		else if(sp == SP_STRLEN)
		{
			switch(a_src1.type)
			{
				case ARG_TXT: *dst_branch = Branch(dst, -1, src1.Len() - 2); break;
				case ARG_BRANCH:
				{
					for(u32 i=0; i<m_sp_string.GetCount(); ++i)
					{
						if(m_sp_string[i].m_addr == a_src1.value)
						{
							*dst_branch = Branch(dst, -1, m_sp_string[i].m_data.Len());
							break;
						}
					}
				}
				break;
			}
		}
		else
		{
			switch(sp)
			{
			case SP_INT: *dst_branch = Branch(dst, -1, a_src1.value); break;
			case SP_BUF:
				*dst_branch = Branch(dst, -1, s_opd.sh_addr + s_opd.sh_size);
				s_opd.sh_size += a_src1.value;
			break;
			case SP_NOT: *dst_branch = Branch(dst, -1, ~a_src1.value); break;
			}
		}
	}
	break;

	case SP_SRL:
	case SP_SRR:
	case SP_MUL:
	case SP_DIV:
	case SP_ADD:
	case SP_SUB:
	case SP_AND:
	case SP_OR:
	case SP_XOR:
	case SP_NOR:
	{
		wxString src1;
		if(!GetArg(src1))
		{
			if(m_analyze) WriteHex("error\n");
			WriteError(wxString::Format("src1 not found. style: %s", GetSpStyle(sp)));
			m_error = true;
			NextLn();
			return;
		}

		Arg a_src1(src1);
		DetectArgInfo(a_src1);

		if(~(ARG_IMM | ARG_BRANCH) & a_src1.type)
		{
			if(m_analyze) WriteHex("error\n");
			WriteError(wxString::Format("bad src1 type. style: %s", GetSpStyle(sp)));
			m_error = true;
			NextLn();
			return;
		}

		wxString src2;
		if(!GetArg(src2, true))
		{
			if(m_analyze) WriteHex("error\n");
			WriteError(wxString::Format("src2 not found. style: %s", GetSpStyle(sp)));
			m_error = true;
			return;
		}

		Arg a_src2(src2);
		DetectArgInfo(a_src2);

		if(~(ARG_IMM | ARG_BRANCH) & a_src2.type)
		{
			if(m_analyze) WriteHex("error\n");
			WriteError(wxString::Format("bad src2 type. style: %s", GetSpStyle(sp)));
			m_error = true;
			NextLn();
			return;
		}

		if(m_asm[p - 1] != ']')
		{
			if(m_analyze) WriteHex("error\n");
			WriteError(wxString::Format("']' not found. style: %s", GetSpStyle(sp)));
			m_error = true;
			NextLn();
			return;
		}

		if(!CheckEnd())
		{
			if(m_analyze) WriteHex("error\n");
			return;
		}

		switch(sp)
		{
		case SP_SRL: *dst_branch = Branch(dst, -1, a_src1.value << a_src2.value); break;
		case SP_SRR: *dst_branch = Branch(dst, -1, a_src1.value >> a_src2.value); break;
		case SP_MUL: *dst_branch = Branch(dst, -1, a_src1.value * a_src2.value); break;
		case SP_DIV: *dst_branch = Branch(dst, -1, a_src1.value / a_src2.value); break;
		case SP_ADD: *dst_branch = Branch(dst, -1, a_src1.value + a_src2.value); break;
		case SP_SUB: *dst_branch = Branch(dst, -1, a_src1.value - a_src2.value); break;
		case SP_AND: *dst_branch = Branch(dst, -1, a_src1.value & a_src2.value); break;
		case SP_OR:  *dst_branch = Branch(dst, -1, a_src1.value | a_src2.value); break;
		case SP_XOR: *dst_branch = Branch(dst, -1, a_src1.value ^ a_src2.value); break;
		case SP_NOR: *dst_branch = Branch(dst, -1, ~(a_src1.value | a_src2.value)); break;
		}
	}
	break;
	}

	if(m_analyze) WriteHex("\n");
}

void CompilePPUProgram::Compile()
{
	if(m_err_list)
	{
		m_err_list->Freeze();
		m_err_list->Clear();
	}
		
	if(m_analyze && m_hex_list)
	{
		m_hex_list->Freeze();
		m_hex_list->Clear();
	}

	m_code.Clear();

	for(u32 i=0; i<m_branches.GetCount(); ++i)
	{
		m_branches[i].m_name.Clear();
	}

	m_branches.Clear();

	u32 text_size = 0;
	while(!IsEnd())
	{
		wxString op;
		if(GetOp(op) && !IsFuncOp(op) && !IsBranchOp(op) && !IsSpOp(op))
		{
			text_size += 4;
		}

		NextLn();
	}

	Elf64_Ehdr elf_info;
	memset(&elf_info, 0, sizeof(Elf64_Ehdr));
	elf_info.e_phentsize = sizeof(Elf64_Phdr);
	elf_info.e_shentsize = sizeof(Elf64_Shdr);
	elf_info.e_ehsize = sizeof(Elf64_Ehdr);
	elf_info.e_phnum = 5;
	elf_info.e_shnum = 15;
	elf_info.e_shstrndx = elf_info.e_shnum - 1;
	elf_info.e_phoff = elf_info.e_ehsize;
	u32 section_offset = Memory.AlignAddr(elf_info.e_phoff + elf_info.e_phnum * elf_info.e_phentsize, 0x100);

	static const u32 sceStub_text_block = 8 * 4;

	Elf64_Shdr s_null;
	memset(&s_null, 0, sizeof(Elf64_Shdr));

	wxArrayString sections_names;
	u32 section_name_offset = 1;

	Elf64_Shdr s_text;
	memset(&s_text, 0, sizeof(Elf64_Shdr));
	s_text.sh_type = 1;
	s_text.sh_offset = section_offset;
	s_text.sh_addr = section_offset + 0x10000;
	s_text.sh_size = text_size;
	s_text.sh_addralign = 4;
	s_text.sh_flags = 6;
	s_text.sh_name = section_name_offset;
	sections_names.Add(".text");
	section_name_offset += wxString(".text").Len() + 1;
	section_offset += s_text.sh_size;

	m_text_addr = s_text.sh_addr;

	struct Module
	{
		wxString m_name;
		Array<u32> m_imports;

		Module(const wxString& name, u32 import) : m_name(name)
		{
			Add(import);
		}

		void Add(u32 import)
		{
			m_imports.AddCpy(import);
		}

		void Clear()
		{
			m_name.Clear();
			m_imports.Clear();
		}
	};

	Array<Module> modules;

	FirstChar();
	while(!IsEnd())
	{
		wxString op;
		if(!GetOp(op) || !IsFuncOp(op))
		{
			NextLn();
			continue;
		}

		while(p > 0 && m_asm[p] != '[') p--;
		p++;

		wxString module, name, id;

		if(!GetArg(module))
		{
			WriteError("module not found. style: [module, name, id]");
			m_error = true;
			NextLn();
			continue;
		}

		Arg a_module(module);
		DetectArgInfo(a_module);

		if(~ARG_ERR & a_module.type)
		{
			WriteError("bad module type. style: [module, name, id]");
			m_error = true;
			NextLn();
			continue;
		}

		if(!GetArg(name))
		{
			WriteError("name not found. style: [module, name, id]");
			m_error = true;
			NextLn();
			continue;
		}

		Arg a_name(name);
		DetectArgInfo(a_name);

		if(~ARG_ERR & a_name.type)
		{
			WriteError("bad name type. style: [module, name, id]");
			m_error = true;
			NextLn();
			continue;
		}

		if(!GetArg(id, true))
		{
			WriteError("id not found. style: [module, name, id]");
			m_error = true;
			NextLn();
			continue;
		}

		Arg a_id(id);
		DetectArgInfo(a_id);

		if(~ARG_IMM & a_id.type)
		{
			WriteError("bad id type. style: [module, name, id]");
			m_error = true;
			NextLn();
			continue;
		}

		if(m_asm[p - 1] != ']')
		{
			WriteError("']' not found. style: [module, name, id]");
			m_error = true;
			NextLn();
			continue;
		}

		if(!CheckEnd()) continue;

		m_branches.Move(new Branch(name, a_id.value, 0));
		const u32 import = m_branches.GetCount() - 1;

		bool founded = false;
		for(u32 i=0; i<modules.GetCount(); ++i)
		{
			if(modules[i].m_name.Cmp(module) != 0) continue;
			founded = true;
			modules[i].Add(import);
			break;
		}

		if(!founded) modules.Move(new Module(module, import));
	}

	u32 imports_count = 0;

	for(u32 m=0; m < modules.GetCount(); ++m)
	{
		imports_count += modules[m].m_imports.GetCount();
	}

	Elf64_Shdr s_sceStub_text;
	memset(&s_sceStub_text, 0, sizeof(Elf64_Shdr));
	s_sceStub_text.sh_addralign = 4;
	section_offset = Memory.AlignAddr(section_offset, s_sceStub_text.sh_addralign);
	s_sceStub_text.sh_type = 1;
	s_sceStub_text.sh_offset = section_offset;
	s_sceStub_text.sh_addr = section_offset + 0x10000;
	s_sceStub_text.sh_name = section_name_offset;
	s_sceStub_text.sh_flags = 6;
	s_sceStub_text.sh_size = imports_count * sceStub_text_block;
	sections_names.Add(".sceStub.text");
	section_name_offset += wxString(".sceStub.text").Len() + 1;
	section_offset += s_sceStub_text.sh_size;

	for(u32 m=0, pos=0; m<modules.GetCount(); ++m)
	{
		for(u32 i=0; i<modules[m].m_imports.GetCount(); ++i, ++pos)
		{
			m_branches[modules[m].m_imports[i]].m_addr = s_sceStub_text.sh_addr + sceStub_text_block * pos;
		}
	}

	Elf64_Shdr s_lib_stub_top;
	memset(&s_lib_stub_top, 0, sizeof(Elf64_Shdr));
	s_lib_stub_top.sh_addralign = 4;
	section_offset = Memory.AlignAddr(section_offset, s_lib_stub_top.sh_addralign);
	s_lib_stub_top.sh_type = 1;
	s_lib_stub_top.sh_name = section_name_offset;
	s_lib_stub_top.sh_offset = section_offset;
	s_lib_stub_top.sh_addr = section_offset + 0x10000;
	s_lib_stub_top.sh_flags = 2;
	s_lib_stub_top.sh_size = 4;
	sections_names.Add(".lib.stub.top");
	section_name_offset += wxString(".lib.stub.top").Len() + 1;
	section_offset += s_lib_stub_top.sh_size;

	Elf64_Shdr s_lib_stub;
	memset(&s_lib_stub, 0, sizeof(Elf64_Shdr));
	s_lib_stub.sh_addralign = 4;
	s_lib_stub.sh_type = 1;
	s_lib_stub.sh_name = section_name_offset;
	s_lib_stub.sh_offset = section_offset;
	s_lib_stub.sh_addr = section_offset + 0x10000;
	s_lib_stub.sh_flags = 2;
	s_lib_stub.sh_size = sizeof(Elf64_StubHeader) * modules.GetCount();
	sections_names.Add(".lib.stub");
	section_name_offset += wxString(".lib.stub").Len() + 1;
	section_offset += s_lib_stub.sh_size;

	Elf64_Shdr s_lib_stub_btm;
	memset(&s_lib_stub_btm, 0, sizeof(Elf64_Shdr));
	s_lib_stub_btm.sh_addralign = 4;
	s_lib_stub_btm.sh_type = 1;
	s_lib_stub_btm.sh_name = section_name_offset;
	s_lib_stub_btm.sh_offset = section_offset;
	s_lib_stub_btm.sh_addr = section_offset + 0x10000;
	s_lib_stub_btm.sh_flags = 2;
	s_lib_stub_btm.sh_size = 4;
	sections_names.Add(".lib.stub.btm");
	section_name_offset += wxString(".lib.stub.btm").Len() + 1;
	section_offset += s_lib_stub_btm.sh_size;

	Elf64_Shdr s_rodata_sceFNID;
	memset(&s_rodata_sceFNID, 0, sizeof(Elf64_Shdr));
	s_rodata_sceFNID.sh_addralign = 4;
	section_offset = Memory.AlignAddr(section_offset, s_rodata_sceFNID.sh_addralign);
	s_rodata_sceFNID.sh_type = 1;
	s_rodata_sceFNID.sh_name = section_name_offset;
	s_rodata_sceFNID.sh_offset = section_offset;
	s_rodata_sceFNID.sh_addr = section_offset + 0x10000;
	s_rodata_sceFNID.sh_flags = 2;
	s_rodata_sceFNID.sh_size = imports_count * 4;
	sections_names.Add(".rodata.sceFNID");
	section_name_offset += wxString(".rodata.sceFNID").Len() + 1;
	section_offset += s_rodata_sceFNID.sh_size;

	Elf64_Shdr s_rodata_sceResident;
	memset(&s_rodata_sceResident, 0, sizeof(Elf64_Shdr));
	s_rodata_sceResident.sh_addralign = 4;
	section_offset = Memory.AlignAddr(section_offset, s_rodata_sceResident.sh_addralign);
	s_rodata_sceResident.sh_type = 1;
	s_rodata_sceResident.sh_name = section_name_offset;
	s_rodata_sceResident.sh_offset = section_offset;
	s_rodata_sceResident.sh_addr = section_offset + 0x10000;
	s_rodata_sceResident.sh_flags = 2;
	s_rodata_sceResident.sh_size = 4;
	for(u32 i=0; i<modules.GetCount(); ++i)
	{
		s_rodata_sceResident.sh_size += modules[i].m_name.Len() + 1;
	}
	s_rodata_sceResident.sh_size = Memory.AlignAddr(s_rodata_sceResident.sh_size, s_rodata_sceResident.sh_addralign);
	sections_names.Add(".rodata.sceResident");
	section_name_offset += wxString(".rodata.sceResident").Len() + 1;
	section_offset += s_rodata_sceResident.sh_size;

	Elf64_Shdr s_lib_ent_top;
	memset(&s_lib_ent_top, 0, sizeof(Elf64_Shdr));
	s_lib_ent_top.sh_addralign = 4;
	section_offset = Memory.AlignAddr(section_offset, s_lib_ent_top.sh_addralign);
	s_lib_ent_top.sh_size = 4;
	s_lib_ent_top.sh_flags = 2;
	s_lib_ent_top.sh_type = 1;
	s_lib_ent_top.sh_name = section_name_offset;
	s_lib_ent_top.sh_offset = section_offset;
	s_lib_ent_top.sh_addr = section_offset + 0x10000;
	sections_names.Add(".lib.ent.top");
	section_name_offset += wxString(".lib.ent.top").Len() + 1;
	section_offset += s_lib_ent_top.sh_size;

	Elf64_Shdr s_lib_ent_btm;
	memset(&s_lib_ent_btm, 0, sizeof(Elf64_Shdr));
	s_lib_ent_btm.sh_addralign = 4;
	s_lib_ent_btm.sh_size = 4;
	s_lib_ent_btm.sh_flags = 2;
	s_lib_ent_btm.sh_type = 1;
	s_lib_ent_btm.sh_name = section_name_offset;
	s_lib_ent_btm.sh_offset = section_offset;
	s_lib_ent_btm.sh_addr = section_offset + 0x10000;
	sections_names.Add(".lib.ent.btm");
	section_name_offset += wxString(".lib.ent.btm").Len() + 1;
	section_offset += s_lib_ent_btm.sh_size;

	Elf64_Shdr s_sys_proc_prx_param;
	memset(&s_sys_proc_prx_param, 0, sizeof(Elf64_Shdr));
	s_sys_proc_prx_param.sh_addralign = 4;
	section_offset = Memory.AlignAddr(section_offset, s_sys_proc_prx_param.sh_addralign);
	s_sys_proc_prx_param.sh_type = 1;
	s_sys_proc_prx_param.sh_size = sizeof(sys_proc_prx_param);
	s_sys_proc_prx_param.sh_name = section_name_offset;
	s_sys_proc_prx_param.sh_offset = section_offset;
	s_sys_proc_prx_param.sh_addr = section_offset + 0x10000;
	s_sys_proc_prx_param.sh_flags = 2;
	sections_names.Add(".sys_proc_prx_param");
	section_name_offset += wxString(".sys_proc_prx_param").Len() + 1;
	section_offset += s_sys_proc_prx_param.sh_size;

	const u32 prog_load_0_end = section_offset;

	section_offset = Memory.AlignAddr(section_offset + 0x10000, 0x10000);
	const u32 prog_load_1_start = section_offset;

	Elf64_Shdr s_data_sceFStub;
	memset(&s_data_sceFStub, 0, sizeof(Elf64_Shdr));
	s_data_sceFStub.sh_name = section_name_offset;
	s_data_sceFStub.sh_addralign = 4;
	section_offset = Memory.AlignAddr(section_offset, s_data_sceFStub.sh_addralign);
	s_data_sceFStub.sh_flags = 3;
	s_data_sceFStub.sh_type = 1;
	s_data_sceFStub.sh_offset = section_offset;
	s_data_sceFStub.sh_addr = section_offset + 0x10000;
	s_data_sceFStub.sh_size = imports_count * 4;
	sections_names.Add(".data.sceFStub");
	section_name_offset += wxString(".data.sceFStub").Len() + 1;
	section_offset += s_data_sceFStub.sh_size;

	Elf64_Shdr s_tbss;
	memset(&s_tbss, 0, sizeof(Elf64_Shdr));
	s_tbss.sh_addralign = 4;
	section_offset = Memory.AlignAddr(section_offset, s_tbss.sh_addralign);
	s_tbss.sh_size = 4;
	s_tbss.sh_flags = 0x403;
	s_tbss.sh_type = 8;
	s_tbss.sh_name = section_name_offset;
	s_tbss.sh_offset = section_offset;
	s_tbss.sh_addr = section_offset + 0x10000;
	sections_names.Add(".tbss");
	section_name_offset += wxString(".tbss").Len() + 1;
	section_offset += s_tbss.sh_size;

	Elf64_Shdr s_opd;
	memset(&s_opd, 0, sizeof(Elf64_Shdr));
	s_opd.sh_addralign = 8;
	section_offset = Memory.AlignAddr(section_offset, s_opd.sh_addralign);
	s_opd.sh_size = 2*4;
	s_opd.sh_type = 1;
	s_opd.sh_offset = section_offset;
	s_opd.sh_addr = section_offset + 0x10000;
	s_opd.sh_name = section_name_offset;
	s_opd.sh_flags = 3;
	sections_names.Add(".opd");
	section_name_offset += wxString(".opd").Len() + 1;

	FirstChar();

	while(!IsEnd())
	{
		wxString op;
		if(!GetOp(op) || IsFuncOp(op) || IsSpOp(op))
		{
			NextLn();
			continue;
		}

		if(IsBranchOp(op))
		{
			const wxString& name = op(0, op.Len() - 1);

			for(u32 i=0; i<m_branches.GetCount(); ++i)
			{
				if(name.Cmp(m_branches[i].m_name) != 0) continue;
				WriteError(wxString::Format("'%s' already declared", name));
				m_error = true;
				break;
			}

			Arg a_name(name);
			DetectArgInfo(a_name);

			if(a_name.type != ARG_ERR)
			{
				WriteError(wxString::Format("bad name '%s'", name));
				m_error = true;
			}

			if(m_error) break;

			m_branches.Move(new Branch(name, m_branch_pos));

			CheckEnd();
			continue;
		}

		m_branch_pos++;
		NextLn();
	}

	bool has_entry = false;
	for(u32 i=0; i<m_branches.GetCount(); ++i)
	{
		if(m_branches[i].m_name.Cmp("entry") != 0) continue;

		has_entry = true;
		break;
	}

	if(!has_entry) m_branches.Move(new Branch("entry", 0));

	if(m_analyze) m_error = false;
	FirstChar();

	while(!IsEnd())
	{
		m_args.Clear();
		m_end_args = false;

		wxString op;
		if(!GetOp(op) || IsBranchOp(op) || IsFuncOp(op))
		{
			if(m_analyze) WriteHex("\n");
			NextLn();
			continue;
		}

		if(IsSpOp(op))
		{
			LoadSp(op, s_opd);
			continue;
		}

		LoadArgs();

		auto instr = GetInstruction<PPU_Opcodes>(op);
		if(instr)
		{
			/*
			FIELD_IMM,
			FIELD_R_GPR,
			FIELD_R_FPR,
			FIELD_R_VPR,
			FIELD_R_CR,
			FIELD_BRANCH,
			*/
			uint type[] =
			{
				ARG_IMM,
				ARG_REG_R,
				ARG_REG_F,
				ARG_REG_V,
				ARG_REG_CR,
			};

			for(uint i=0; i<instr->GetArgCount(); ++i)
			{
				switch(instr->GetArg(i).m_type)
				{
				case FIELD_BRANCH:
					SetNextArgBranch(0); //TODO
				break;

				default:
					SetNextArgType(type[instr->GetArg(i).m_type]);
				break;
				}
			}
		}
		else
		{
			WriteError(wxString::Format("unknown instruction '%s'", op));
			EndLn();
			m_error = true;
		}

		CheckEnd();

		if(m_error)
		{
			if(m_analyze)
			{
				WriteHex("error\n");
				m_error = false;
				continue;
			}

			break;
		}

		u32 code;

		{
			Array<u32> args;
			args.SetCount(m_args.GetCount());
			for(uint i=0; i<args.GetCount(); ++i)
			{
				args[i] = m_args[i].value;
			}

			code = (*instr)(args);
		}

		if(m_analyze) WriteHex(wxString::Format("0x%08x\n", code));

		if(!m_analyze) m_code.AddCpy(code);

		m_branch_pos++;
	}

	if(m_file_path && !m_analyze && !m_error)
	{
		s_opd.sh_size = Memory.AlignAddr(s_opd.sh_size, s_opd.sh_addralign);
		section_offset += s_opd.sh_size;

		const u32 prog_load_1_end = section_offset;

		Elf64_Shdr s_shstrtab;
		memset(&s_shstrtab, 0, sizeof(Elf64_Shdr));
		s_shstrtab.sh_addralign = 1;
		section_offset = Memory.AlignAddr(section_offset, s_shstrtab.sh_addralign);
		s_shstrtab.sh_name = section_name_offset;
		s_shstrtab.sh_type = 3;
		s_shstrtab.sh_offset = section_offset;
		s_shstrtab.sh_addr = 0;
		sections_names.Add(".shstrtab");
		section_name_offset += wxString(".shstrtab").Len() + 1;
		s_shstrtab.sh_size = section_name_offset;
		section_offset += s_shstrtab.sh_size;

		wxFile f(m_file_path, wxFile::write);

		elf_info.e_magic = 0x7F454C46;
		elf_info.e_class = 2; //ELF64
		elf_info.e_data = 2;
		elf_info.e_curver = 1; //ver 1
		elf_info.e_os_abi = 0x66; //Cell OS LV-2
		elf_info.e_abi_ver = 0;
		elf_info.e_type = 2; //EXEC (Executable file)
		elf_info.e_machine = MACHINE_PPC64; //PowerPC64
		elf_info.e_version = 1; //ver 1
		elf_info.e_flags = 0x0;
		elf_info.e_shoff = Memory.AlignAddr(section_offset, 4);

		u8* opd_data = new u8[s_opd.sh_size];
		u32 entry_point = s_text.sh_addr;
		for(u32 i=0; i<m_branches.GetCount(); ++i)
		{
			if(m_branches[i].m_name.Cmp("entry") == 0)
			{
				entry_point += m_branches[i].m_pos * 4;
				break;
			}
		}
		*(u32*)opd_data = re32(entry_point);
		*(u32*)(opd_data + 4) = 0;

		sys_proc_prx_param prx_param;
		memset(&prx_param, 0, sizeof(sys_proc_prx_param));

		prx_param.size = re32(0x40);
		prx_param.magic = re32(0x1b434cec);
		prx_param.version = re32(0x4);
		prx_param.libentstart = re32(s_lib_ent_top.sh_addr + s_lib_ent_top.sh_size);
		prx_param.libentend = re32(s_lib_ent_btm.sh_addr);
		prx_param.libstubstart = re32(s_lib_stub_top.sh_addr + s_lib_stub_top.sh_size);
		prx_param.libstubend = re32(s_lib_stub_btm.sh_addr);
		prx_param.ver = re16(0x101);
			
		elf_info.e_entry = s_opd.sh_addr;

		f.Seek(0);
		WriteEhdr(f, elf_info);

		f.Seek(elf_info.e_shoff);
		WriteShdr(f, s_null);
		WriteShdr(f, s_text);
		WriteShdr(f, s_opd);
		WriteShdr(f, s_sceStub_text);
		WriteShdr(f, s_lib_stub_top);
		WriteShdr(f, s_lib_stub);
		WriteShdr(f, s_lib_stub_btm);
		WriteShdr(f, s_data_sceFStub);
		WriteShdr(f, s_rodata_sceFNID);
		WriteShdr(f, s_rodata_sceResident);
		WriteShdr(f, s_sys_proc_prx_param);
		WriteShdr(f, s_lib_ent_top);
		WriteShdr(f, s_lib_ent_btm);
		WriteShdr(f, s_tbss);
		WriteShdr(f, s_shstrtab);

		f.Seek(s_text.sh_offset);
		for(u32 i=0; i<m_code.GetCount(); ++i) Write32(f, m_code[i]);

		f.Seek(s_opd.sh_offset);
		f.Write(opd_data, 8);
		for(u32 i=0; i<m_sp_string.GetCount(); ++i)
		{
			f.Seek(s_opd.sh_offset + (m_sp_string[i].m_addr - s_opd.sh_addr));
			f.Write(&m_sp_string[i].m_data[0], m_sp_string[i].m_data.Len() + 1);
		}

		f.Seek(s_sceStub_text.sh_offset);
		for(u32 i=0; i<imports_count; ++i)
		{
			const u32 addr = s_data_sceFStub.sh_addr + i * 4;
			Write32(f, ADDI(12, 0, 0));
			Write32(f, ORIS(12, 12, addr >> 16));
			Write32(f, LWZ(12, 12, addr));
			Write32(f, STD(2, 1, 40));
			Write32(f, LWZ(0, 12, 0));
			Write32(f, LWZ(2, 12, 4));
			Write32(f, MTSPR(0x009, 0));
			Write32(f, BCCTR(20, 0, 0, 0));
			/*
			Write32(f, ToOpcode(ADDI) | ToRD(12)); //li r12,0
			Write32(f, ToOpcode(ORIS) | ToRD(12) | ToRA(12) | ToIMM16(addr >> 16)); //oris r12,r12,addr>>16
			Write32(f, ToOpcode(LWZ)  | ToRD(12) | ToRA(12) | ToIMM16(addr)); //lwz r12,addr&0xffff(r12)
			Write32(f, 0xf8410028); //std r2,40(r1)
			Write32(f, ToOpcode(LWZ)  | ToRD(0) | ToRA(12) | ToIMM16(0)); //lwz r0,0(r12)
			Write32(f, ToOpcode(LWZ)  | ToRD(2) | ToRA(12) | ToIMM16(4)); //lwz r2,4(r12)
			Write32(f, 0x7c0903a6); //mtctr r0
			Write32(f, 0x4e800420); //bctr
			*/
		}

		f.Seek(s_lib_stub_top.sh_offset);
		f.Seek(s_lib_stub_top.sh_size, wxFromCurrent);

		f.Seek(s_lib_stub.sh_offset);
		for(u32 i=0, nameoffs=4, dataoffs=0; i<modules.GetCount(); ++i)
		{
			Elf64_StubHeader stub;
			memset(&stub, 0, sizeof(Elf64_StubHeader));

			stub.s_size = 0x2c;
			stub.s_version = re16(0x1);
			stub.s_unk1 = re16(0x9);
			stub.s_modulename = re32(s_rodata_sceResident.sh_addr + nameoffs);
			stub.s_nid = re32(s_rodata_sceFNID.sh_addr + dataoffs);
			stub.s_text = re32(s_data_sceFStub.sh_addr + dataoffs);
			stub.s_imports = re16(modules[i].m_imports.GetCount());

			dataoffs += modules[i].m_imports.GetCount() * 4;

			f.Write(&stub, sizeof(Elf64_StubHeader));
			nameoffs += modules[i].m_name.Len() + 1;
		}

		f.Seek(s_lib_stub_btm.sh_offset);
		f.Seek(s_lib_stub_btm.sh_size, wxFromCurrent);

		f.Seek(s_data_sceFStub.sh_offset);
		for(u32 m=0; m<modules.GetCount(); ++m)
		{
			for(u32 i=0; i<modules[m].m_imports.GetCount(); ++i)
			{
				Write32(f, m_branches[modules[m].m_imports[i]].m_addr);
			}
		}

		f.Seek(s_rodata_sceFNID.sh_offset);
		for(u32 m=0; m<modules.GetCount(); ++m)
		{
			for(u32 i=0; i<modules[m].m_imports.GetCount(); ++i)
			{
				Write32(f, m_branches[modules[m].m_imports[i]].m_id);
			}
		}

		f.Seek(s_rodata_sceResident.sh_offset + 4);
		for(u32 i=0; i<modules.GetCount(); ++i)
		{
			f.Write(&modules[i].m_name[0], modules[i].m_name.Len() + 1);
		}

		f.Seek(s_sys_proc_prx_param.sh_offset);
		f.Write(&prx_param, sizeof(sys_proc_prx_param));

		f.Seek(s_lib_ent_top.sh_offset);
		f.Seek(s_lib_ent_top.sh_size, wxFromCurrent);

		f.Seek(s_lib_ent_btm.sh_offset);
		f.Seek(s_lib_ent_btm.sh_size, wxFromCurrent);

		f.Seek(s_tbss.sh_offset);
		f.Seek(s_tbss.sh_size, wxFromCurrent);

		f.Seek(s_shstrtab.sh_offset + 1);
		for(u32 i=0; i<sections_names.GetCount(); ++i)
		{
			f.Write(&sections_names[i][0], sections_names[i].Len() + 1);
		}

		Elf64_Phdr p_load_0;
		p_load_0.p_type = 0x00000001;
		p_load_0.p_offset = 0;
		p_load_0.p_paddr =
		p_load_0.p_vaddr = 0x10000;
		p_load_0.p_filesz =
		p_load_0.p_memsz = prog_load_0_end;
		p_load_0.p_align = 0x10000;
		p_load_0.p_flags = 0x400005;

		Elf64_Phdr p_load_1;
		p_load_1.p_type = 0x00000001;
		p_load_1.p_offset = prog_load_1_start;
		p_load_1.p_paddr =
		p_load_1.p_vaddr = prog_load_1_start + 0x10000;
		p_load_1.p_filesz =
		p_load_1.p_memsz = prog_load_1_end - prog_load_1_start;
		p_load_1.p_align = 0x10000;
		p_load_1.p_flags = 0x600006;

		Elf64_Phdr p_tls;
		p_tls.p_type = 0x00000007;
		p_tls.p_offset = s_tbss.sh_offset;
		p_tls.p_paddr =
		p_tls.p_vaddr = s_tbss.sh_addr;
		p_tls.p_filesz = 0;
		p_tls.p_memsz = s_tbss.sh_size;
		p_tls.p_align = s_tbss.sh_addralign;
		p_tls.p_flags = 0x4;

		Elf64_Phdr p_loos_1;
		p_loos_1.p_type = 0x60000001;
		p_loos_1.p_offset = 0;
		p_loos_1.p_paddr = 0;
		p_loos_1.p_vaddr = 0;
		p_loos_1.p_filesz = 0;
		p_loos_1.p_memsz = 0;
		p_loos_1.p_align = 1;
		p_loos_1.p_flags = 0;

		Elf64_Phdr p_loos_2;
		p_loos_2.p_type = 0x60000002;
		p_loos_2.p_offset = s_sys_proc_prx_param.sh_offset;
		p_loos_2.p_paddr =
		p_loos_2.p_vaddr = s_sys_proc_prx_param.sh_addr;
		p_loos_2.p_filesz =
		p_loos_2.p_memsz = s_sys_proc_prx_param.sh_size;
		p_loos_2.p_align = s_sys_proc_prx_param.sh_addralign;
		p_loos_2.p_flags = 0;

		f.Seek(elf_info.e_phoff);
		WritePhdr(f, p_load_0);
		WritePhdr(f, p_load_1);
		WritePhdr(f, p_tls);
		WritePhdr(f, p_loos_1);
		WritePhdr(f, p_loos_2);

		sections_names.Clear();
		delete[] opd_data;
		for(u32 i=0; i<modules.GetCount(); ++i) modules[i].Clear();
		modules.Clear();
	}

	for(u32 i=0; i<m_branches.GetCount(); ++i)
	{
		m_branches[i].m_name.Clear();
	}

	m_branches.Clear();

	m_code.Clear();
	m_args.Clear();

	m_sp_string.Clear();

	if(m_err_list) m_err_list->Thaw();

	if(m_analyze)
	{
		if(m_hex_list)
		{
			m_hex_list->Thaw();
		}
	}
	else
	{
		system("make_fself.cmd");
	}
}
