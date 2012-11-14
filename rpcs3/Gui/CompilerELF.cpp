#include "stdafx.h"
#include "CompilerELF.h"

void Write8(wxFile& f, const u8 data)
{
	f.Write(&data, 1);
}
void Write16(wxFile& f, const u16 data)
{
	Write8(f, data >> 8);
	Write8(f, data);
}
void Write32(wxFile& f, const u32 data)
{
	Write16(f, data >> 16);
	Write16(f, data);
}
void Write64(wxFile& f, const u64 data)
{
	Write32(f, data >> 32);
	Write32(f, data);
}

enum CompilerIDs
{
	id_analyze_code = 0x555,
	id_compile_code,
	id_load_elf,
};

wxFont GetFont(int size)
{
	return wxFont(size, wxMODERN, wxNORMAL, wxNORMAL);
}

u32 SetField(u32 src, u32 from, u32 to)
{
	return (src & ((1 << ((to - from) + 1)) - 1)) << (31 - to);
}

u32 SetField(s32 src, u32 p)
{
	return (src & 0x1) << (31 - p);
}

u32 ToOpcode(u32 i)	{ return SetField(i, 0, 5); }
u32 ToRS(u32 i)		{ return SetField(i, 6, 10); }
u32 ToRD(u32 i)		{ return SetField(i, 6, 10); }
u32 ToRA(u32 i)		{ return SetField(i, 11, 15); }
u32 ToRB(u32 i)		{ return SetField(i, 16, 20); }
u32 ToLL(u32 i)		{ return i & 0x03fffffc; }
u32 ToAA(u32 i)		{ return SetField(i, 30); }
u32 ToLK(u32 i)		{ return SetField(i, 31); }
u32 ToIMM16(s32 i)	{ return SetField(i, 16, 31); }
u32 ToD(s32 i)		{ return SetField(i, 16, 31); }
u32 ToDS(s32 i)
{
	if(i < 0) return ToD(i + 1);
	return ToD(i);
}
u32 ToSYS(u32 i)	{ return SetField(i, 6, 31); }
u32 ToBF(u32 i)		{ return SetField(i, 6, 10); }
u32 ToBO(u32 i)		{ return SetField(i, 6, 10); }
u32 ToBI(u32 i)		{ return SetField(i, 11, 15); }
u32 ToBD(u32 i)		{ return i & 0xfffc; }
u32 ToMB(u32 i)		{ return SetField(i, 21, 25); }
u32 ToME(u32 i)		{ return SetField(i, 26, 30); }
u32 ToSH(u32 i)		{ return SetField(i, 16, 20); }
u32 ToRC(u32 i)		{ return SetField(i, 31); }
u32 ToOE(u32 i)		{ return SetField(i, 21); }
u32 ToL(u32 i)		{ return SetField(i, 10); }
u32 ToCRFD(u32 i)	{ return SetField(i, 6, 8); }

struct InstrField
{
	u32 (*To)(u32 i);
} RA = {ToRA}, RS = {ToRS}, RD = {ToRD};

enum MASKS
{
	MASK_ERROR = -1,
	MASK_UNK,
	MASK_NO_ARG,
	MASK_RSD_RA_IMM16,
	MASK_RA_RST_IMM16,
	MASK_CRFD_RA_IMM16,
	MASK_BI_BD,
	MASK_BO_BI_BD,
	MASK_SYS,
	MASK_LL,
	MASK_RA_RS_SH_MB_ME,
	MASK_RA_RS_RB_MB_ME,
	MASK_RST_RA_D,
	MASK_RST_RA_DS,
	MASK_TO_RA_IMM16,
	MASK_RSD_IMM16,
};

enum SMASKS
{
	SMASK_ERROR = -1,
	SMASK_NULL,
	SMASK_AA = (1 << 0),
	SMASK_LK = (1 << 1),
	SMASK_OE = (1 << 2),
	SMASK_RC = (1 << 3),
	SMASK_L  = (1 << 4),
	SMASK_BO = (1 << 5),
	SMASK_BI = (1 << 6),

	BO_MASK_BO4 = (1 << 0),
	BO_MASK_BO3 = (1 << 1),
	BO_MASK_BO2 = (1 << 2),
	BO_MASK_BO1 = (1 << 3),
	BO_MASK_BO0 = (1 << 4),

	BI_LT = 0,
	BI_GT = 1,
	BI_EQ = 2,
	BI_GE = 0,
	BI_LE = 1,
	BI_NE = 2,
};

static const struct Instruction
{
	MASKS mask;
	wxString name;
	u32 op_num;
	u32 op_g;
	u32 smask;
	u32 bo;
	u32 bi;

} m_instructions[] =
{
	{MASK_NO_ARG,			"nop",		ORI,	0, SMASK_NULL},
	{MASK_TO_RA_IMM16,		"tdi",		TDI,	0, SMASK_NULL},
	{MASK_TO_RA_IMM16,		"twi",		TWI,	0, SMASK_NULL},
	//G_04 = 0x04,
	{MASK_RSD_RA_IMM16,		"mulli",	MULLI,	0, SMASK_NULL},
	{MASK_RSD_RA_IMM16,		"subfic",	SUBFIC,	0, SMASK_NULL},
	{MASK_CRFD_RA_IMM16,	"cmplwi",	CMPLI,	0, SMASK_NULL},
	{MASK_CRFD_RA_IMM16,	"cmpldi",	CMPLI,	0, SMASK_L},
	{MASK_CRFD_RA_IMM16,	"cmpwi",	CMPI,	0, SMASK_NULL},
	{MASK_CRFD_RA_IMM16,	"cmpdi",	CMPI,	0, SMASK_L},
	{MASK_RSD_RA_IMM16,		"addic",	ADDIC,	0, SMASK_NULL},
	{MASK_RSD_RA_IMM16,		"addic.",	ADDIC_,	0, SMASK_RC},
	{MASK_RSD_RA_IMM16,		"addi",		ADDI,	0, SMASK_NULL},
	{MASK_RSD_IMM16,		"li",		ADDI,	0, SMASK_NULL},
	{MASK_RSD_RA_IMM16,		"addis",	ADDIS,	0, SMASK_NULL},

	{MASK_BO_BI_BD,			"bc",		BC,		0, SMASK_NULL},
	{MASK_BO_BI_BD,			"bca",		BC,		0, SMASK_AA},
	{MASK_BO_BI_BD,			"bcl",		BC,		0, SMASK_LK},
	{MASK_BO_BI_BD,			"bcla",		BC,		0, SMASK_AA | SMASK_LK},
	{MASK_BI_BD,			"bdz",		BC,		0, SMASK_BO, BO_MASK_BO0 | BO_MASK_BO3},
	{MASK_BI_BD,			"bdz-",		BC,		0, SMASK_BO, BO_MASK_BO0 | BO_MASK_BO1 | BO_MASK_BO3},
	{MASK_BI_BD,			"bdz+",		BC,		0, SMASK_BO, BO_MASK_BO0 | BO_MASK_BO1 | BO_MASK_BO3 | BO_MASK_BO4},
	{MASK_BI_BD,			"bdnz",		BC,		0, SMASK_BO, BO_MASK_BO0},
	{MASK_BI_BD,			"bdnz-",	BC,		0, SMASK_BO, BO_MASK_BO0 | BO_MASK_BO1},
	{MASK_BI_BD,			"bdnz+",	BC,		0, SMASK_BO, BO_MASK_BO0 | BO_MASK_BO1 | BO_MASK_BO4},
	{MASK_BI_BD,			"bge",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO2, BI_GE},
	{MASK_BI_BD,			"ble",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO2, BI_LE},
	{MASK_BI_BD,			"bne",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO2, BI_NE},
	{MASK_BI_BD,			"bge-",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO2 | BO_MASK_BO3, BI_GE},
	{MASK_BI_BD,			"ble-",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO2 | BO_MASK_BO3, BI_LE},
	{MASK_BI_BD,			"bne-",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO2 | BO_MASK_BO3, BI_NE},
	{MASK_BI_BD,			"bge+",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO2 | BO_MASK_BO3 | BO_MASK_BO4, BI_GE},
	{MASK_BI_BD,			"ble+",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO2 | BO_MASK_BO3 | BO_MASK_BO4, BI_LE},
	{MASK_BI_BD,			"bne+",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO2 | BO_MASK_BO3 | BO_MASK_BO4, BI_NE},
	{MASK_BI_BD,			"bgt",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO1 | BO_MASK_BO2, BI_GT},
	{MASK_BI_BD,			"blt",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO1 | BO_MASK_BO2, BI_LT},
	{MASK_BI_BD,			"beq",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO1 | BO_MASK_BO2, BI_EQ},
	{MASK_BI_BD,			"bgt-",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO1 | BO_MASK_BO2 | BO_MASK_BO3, BI_GT},
	{MASK_BI_BD,			"blt-",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO1 | BO_MASK_BO2 | BO_MASK_BO3, BI_LT},
	{MASK_BI_BD,			"beq-",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO1 | BO_MASK_BO2 | BO_MASK_BO3, BI_EQ},
	{MASK_BI_BD,			"bgt+",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO1 | BO_MASK_BO2 | BO_MASK_BO3 | BO_MASK_BO4, BI_GT},
	{MASK_BI_BD,			"blt+",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO1 | BO_MASK_BO2 | BO_MASK_BO3 | BO_MASK_BO4, BI_LT},
	{MASK_BI_BD,			"beq+",		BC,		0, SMASK_BO | SMASK_BI, BO_MASK_BO1 | BO_MASK_BO2 | BO_MASK_BO3 | BO_MASK_BO4, BI_EQ},

	{MASK_SYS,				"sc",		SC,		0, SMASK_NULL},
	{MASK_LL,				"b",		B,		0, SMASK_NULL},
	{MASK_LL,				"ba",		B,		0, SMASK_AA},
	{MASK_LL,				"bl",		B,		0, SMASK_LK},
	{MASK_LL,				"bla",		B,		0, SMASK_AA | SMASK_LK},
	//G_13 = 0x13,
	{MASK_RA_RS_SH_MB_ME,	"rlwimi",	RLWIMI,	0, SMASK_NULL},
	{MASK_RA_RS_SH_MB_ME,	"rlwimi.",	RLWIMI,	0, SMASK_RC},
	{MASK_RA_RS_SH_MB_ME,	"rlwinm",	RLWINM,	0, SMASK_NULL},
	{MASK_RA_RS_SH_MB_ME,	"rlwinm.",	RLWINM,	0, SMASK_RC},
	{MASK_RA_RS_RB_MB_ME,	"rlwnm",	RLWNM,	0, SMASK_NULL},
	{MASK_RA_RS_RB_MB_ME,	"rlwnm.",	RLWNM,	0, SMASK_RC},
	{MASK_RSD_RA_IMM16,		"ori",		ORI,	0, SMASK_NULL},
	{MASK_RSD_RA_IMM16,		"oris",		ORIS,	0, SMASK_NULL},
	{MASK_RSD_RA_IMM16,		"xori",		XORI,	0, SMASK_NULL},
	{MASK_RSD_RA_IMM16,		"xoris",	XORIS,	0, SMASK_NULL},
	{MASK_RA_RST_IMM16,		"andi.",	ANDI_,	0, SMASK_RC},
	{MASK_RA_RST_IMM16,		"andis.",	ANDIS_,	0, SMASK_RC},
	//G_1e = 0x1e,
	//G_1f = 0x1f,
	{MASK_RST_RA_D,			"lwz",		LWZ,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"lwzu",		LWZU,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"lbz",		LBZ,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"lbzu",		LBZU,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"stw",		STW,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"stwu",		STWU,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"stb",		STB,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"stbu",		STBU,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"lhz",		LHZ,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"lhzu",		LHZU,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"lha",		LHA,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"lhau",		LHAU,	0, SMASK_NULL},
	{MASK_RST_RA_D,			"sth",		STH,	0, SMASK_NULL},
	//{0, "lmw", LMW, 0, false, false},
	//LFS = 0x30,
	//LFSU = 0x31,
	//LFD = 0x32,
	//LFDU = 0x33,
	//STFS = 0x34,
	//STFD = 0x36,
	//LFQ = 0x38,
	//LFQU = 0x39,
	//G_3a = 0x3a,
	//G_3b = 0x3b,
	//G_3e = 0x3e,
	//G_3f = 0x3f,
	{MASK_UNK, "unk", 0, 0, SMASK_NULL},
}, m_error_instr = {MASK_ERROR, "err", 0, 0, SMASK_ERROR};
static const u32 instr_count = WXSIZEOF(m_instructions);

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
	wxString string;
	u32 value;
	ArgType type;

	Arg(const wxString& _string, const u32 _value = 0, const ArgType _type = ARG_ERR)
		: string(_string)
		, value(_value)
		, type(_type)
	{
	}
};

Instruction GetInstruction(const wxString& str)
{
	for(u32 i=0; i<instr_count; ++i)
	{
		if(!!m_instructions[i].name.Cmp(str)) continue;
		return m_instructions[i];
	}

	return m_error_instr;
}

u32 CompileInstruction(Instruction instr, Array<Arg>& arr)
{
	u32 code = 
		ToOE(instr.smask & SMASK_OE) | ToRC(instr.smask & SMASK_RC) | 
		ToAA(instr.smask & SMASK_AA) | ToLK(instr.smask & SMASK_LK) |
		ToL (instr.smask & SMASK_L );

	if(instr.smask & SMASK_BO) code |= ToBO(instr.bo);
	if(instr.smask & SMASK_BI) code |= ToBI(instr.bi);

	code |= ToOpcode(instr.op_num);

	switch(instr.mask)
	{
	case MASK_RSD_RA_IMM16:
		return code | ToRS(arr[0].value) | ToRA(arr[1].value) | ToIMM16(arr[2].value);
	case MASK_RA_RST_IMM16:
		return code | ToRA(arr[0].value) | ToRS(arr[1].value) | ToIMM16(arr[2].value);
	case MASK_CRFD_RA_IMM16:
		return code | ToCRFD(arr[0].value)  | ToRA(arr[1].value) | ToIMM16(arr[2].value);
	case MASK_BO_BI_BD:
		return code | ToBO(arr[0].value) | ToBI(arr[1].value) | ToBD(arr[2].value);
	case MASK_BI_BD:
		return code | ToBI(arr[0].value * 4) | ToBD(arr[1].value);
	case MASK_SYS: 
		return code | ToSYS(arr[0].value);
	case MASK_LL:
		return code | ToLL(arr[0].value);
	case MASK_RA_RS_SH_MB_ME:
		return code | ToRA(arr[0].value) | ToRS(arr[1].value) | ToSH(arr[2].value) | ToMB(arr[3].value) | ToME(arr[4].value);
	case MASK_RST_RA_D:
		return code | ToRS(arr[0].value) | ToRA(arr[1].value) | ToD(arr[2].value);
	case MASK_RST_RA_DS:
		return code | ToRS(arr[0].value) | ToRA(arr[1].value) | ToDS(arr[2].value);
	case MASK_RSD_IMM16:
		return code | ToRS(arr[0].value) | ToIMM16(arr[1].value);

	case MASK_UNK:
		return arr[0].value;
	case MASK_NO_ARG:
		return code;
	}

	return 0;
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

CompilerELF::CompilerELF(wxWindow* parent)
	: FrameBase(parent, wxID_ANY, "CompilerELF", wxEmptyString, wxSize(640, 680))
	, m_status_bar(*CreateStatusBar())
{
	m_disable_scroll = false;

	m_aui_mgr.SetManagedWindow(this);

	wxMenuBar& menubar(*new wxMenuBar());
	wxMenu& menu_code(*new wxMenu());
	menubar.Append(&menu_code, "Code");

	//menu_code.Append(id_analyze_code, "Analyze");
	menu_code.Append(id_compile_code, "Compile");
	menu_code.Append(id_load_elf, "Load ELF");

	SetMenuBar(&menubar);

	asm_list = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(400, -1),
		wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_RICH2);

	hex_list = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(155, -1),
		wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_READONLY | wxTE_RICH2);

	err_list = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(140, 200),
		wxTE_MULTILINE | wxTE_READONLY);

	const wxSize& client_size = GetClientSize();

	m_aui_mgr.AddPane(asm_list, wxAuiPaneInfo().Name(L"asm_list").CenterPane().PaneBorder(false).CloseButton(false));
	m_aui_mgr.AddPane(hex_list, wxAuiPaneInfo().Name(L"hex_list").Left().PaneBorder(false).CloseButton(false));
	m_aui_mgr.AddPane(err_list, wxAuiPaneInfo().Name(L"err_list").Bottom().PaneBorder(false).CloseButton(false));
	m_aui_mgr.GetPane(L"asm_list").CaptionVisible(false).Show();
	m_aui_mgr.GetPane(L"hex_list").CaptionVisible(false).Show();
	m_aui_mgr.GetPane(L"err_list").Caption("Errors").Show();
	m_aui_mgr.Update();

	FrameBase::LoadInfo();

	Connect(asm_list->GetId(), wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(CompilerELF::OnUpdate));
	Connect(id_analyze_code, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CompilerELF::AnalyzeCode));
	Connect(id_compile_code, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CompilerELF::CompileCode));
	Connect(id_load_elf,	 wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CompilerELF::LoadElf));

	asm_list->SetFont(wxFont(-1, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	hex_list->SetFont(wxFont(-1, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	wxGetApp().Connect(wxEVT_SCROLLWIN_TOP,			wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);
	wxGetApp().Connect(wxEVT_SCROLLWIN_BOTTOM,		wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);
	wxGetApp().Connect(wxEVT_SCROLLWIN_LINEUP,		wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);
	wxGetApp().Connect(wxEVT_SCROLLWIN_LINEDOWN,	wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);
	wxGetApp().Connect(wxEVT_SCROLLWIN_THUMBTRACK,	wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);
	wxGetApp().Connect(wxEVT_SCROLLWIN_THUMBRELEASE,wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);

	wxGetApp().Connect(asm_list->GetId(), wxEVT_MOUSEWHEEL, wxMouseEventHandler(CompilerELF::MouseWheel), (wxObject*)0, this);
	wxGetApp().Connect(hex_list->GetId(), wxEVT_MOUSEWHEEL, wxMouseEventHandler(CompilerELF::MouseWheel), (wxObject*)0, this);

	wxGetApp().Connect(asm_list->GetId(), wxEVT_KEY_DOWN, wxKeyEventHandler(CompilerELF::OnKeyDown), (wxObject*)0, this);
	wxGetApp().Connect(hex_list->GetId(), wxEVT_KEY_DOWN, wxKeyEventHandler(CompilerELF::OnKeyDown), (wxObject*)0, this);

	asm_list->WriteText(
		"[sysPrxForUser, sys_initialize_tls, 0x744680a2]\n"
		"[sysPrxForUser, sys_process_exit, 0xe6f2c1e7]\n"
		"[sys_fs, cellFsOpen, 0x718bf5f8]\n"
		"[sys_fs, cellFsClose, 0x2cb51f0d]\n"
		"[sys_fs, cellFsRead, 0x4d5ff8e2]\n"
		"[sys_fs, cellFsWrite, 0xecdcf2ab]\n"
		"[sys_fs, cellFsLseek, 0xa397d042]\n"
		"[sys_fs, cellFsMkdir, 0xba901fe6]\n"
		"\n"
		".int [flags, 577] #LV2_O_WRONLY | LV2_O_CREAT | LV2_O_TRUNC\n"
		".string [path, \"dev_hdd0/game/TEST12345/USRDIR/test.txt\"]\n"
		".string [str, \"Hello World!\"]\n"
		".strlen [str_len, str]\n"
		".buf [fd, 8]\n"
		"\n"
		".srr [fd_hi, fd, 16]\n"
		".and [fd_lo, fd, 0xffff]\n"
		"\n"
		".srr [path_hi, path, 16]\n"
		".and [path_lo, path, 0xffff]\n"
		"\n"
		".srr [str_hi, str, 16]\n"
		".and [str_lo, str, 0xffff]\n"
		"\n"
		"\tbl \tsys_initialize_tls\n"
		"\n"
		"\tli \tr3, path_lo\n"
		"\toris \tr3, r3, path_hi\n"
		"\tli \tr4, flags\n"
		"\tli \tr5, fd_lo\n"
		"\toris \tr5, r5, fd_hi\n"
		"\tli \tr6, 0\n"
		"\tli \tr7, 0\n"
		"\tbl \tcellFsOpen\n"
		"\tcmpwi \tcr7, r3, 0\n"
		"\tblt- \tcr7, exit_err\n"
		"\n"
		"\tli \tr3, fd_lo\n"
		"\toris \tr3, r3, fd_hi\n"
		"\tlwz \tr3, r3, 0\n"
		"\tli \tr4, str_lo\n"
		"\toris \tr4, r4, str_hi\n"
		"\tli \tr5, str_len\n"
		"\tli \tr6, 0\n"
		"\tbl \tcellFsWrite\n"
		"\tcmpwi \tcr7, r3, 0\n"
		"\tblt- \tcr7, exit_err\n"
		"\n"
		"\tli \tr3, fd_lo\n"
		"\toris \tr3, r3, fd_hi\n"
		"\tlwz \tr3, r3, 0\n"
		"\tbl \tcellFsClose\n"
		"\tcmpwi \tcr7, r3, 0\n"
		"\tblt- \tcr7, exit_err\n"
		"\n"
		"exit_ok:\n"
		"\tli \tr3, 0\n"
		"\tb \texit\n"
		"\n"
		"exit_err:\n"
		".string [str, \"error.\\n\"]\n"
		".srr [str_hi, str, 16]\n"
		".and [str_lo, str, 0xffff]\n"
		".strlen [str_len, str]\n"
		".int [written_len_lo, fd_lo]\n"
		".int [written_len_hi, fd_hi]\n"
		"\tli \tr3, 1\n"
		"\tli \tr4, str_lo\n"
		"\toris \tr4, r4, str_hi\n"
		"\tli \tr5, str_len\n"
		"\tli \tr6, written_len_lo\n"
		"\toris \tr6, r6, written_len_hi\n"
		"\tli \tr11, 403\n"
		"\tsc\n"
		"\tli \tr3, 1\n"
		"\tb \texit\n"
		"\n"
		"exit:\n"
		"\tbl \tsys_process_exit\n"
		"\tli \tr3, 1\n"
		"\tb \texit\n"
	);

	::SendMessage((HWND)hex_list->GetHWND(), WM_VSCROLL, SB_BOTTOM, 0);
	::SendMessage((HWND)asm_list->GetHWND(), WM_VSCROLL, SB_BOTTOM, 0);
}

CompilerELF::~CompilerELF()
{
	m_aui_mgr.UnInit();
}

void CompilerELF::MouseWheel(wxMouseEvent& event)
{
	int value = (event.m_wheelRotation / event.m_wheelDelta);

	wxScrollWinEvent scrool_event;

	scrool_event.SetEventType(value > 0 ? wxEVT_SCROLLWIN_LINEUP : wxEVT_SCROLLWIN_LINEDOWN);
	scrool_event.SetOrientation(wxVERTICAL);

	if(value < 0) value = -value;

	value *= event.m_linesPerAction;

	while(value--)
	{
		scrool_event.SetEventObject(asm_list);
		OnScroll(scrool_event);
		scrool_event.SetEventObject(hex_list);
		OnScroll(scrool_event);
	}
}

void CompilerELF::UpdateStatus(int offset)
{
	long line;

	if(asm_list->PositionToXY(asm_list->GetInsertionPoint() - 2, nullptr, &line))
	{
		m_status_bar.SetStatusText(wxString::Format("line %d", line), 0);
		return;
	}

	m_status_bar.SetStatusText(wxEmptyString, 0);
}

void CompilerELF::OnKeyDown(wxKeyEvent& event)
{
	wxScrollWinEvent scrool_event;
	scrool_event.SetOrientation(wxVERTICAL);

	switch(event.GetKeyCode())
	{
	case WXK_RETURN:	UpdateStatus( 1); break;
	case WXK_UP:		UpdateStatus(-1); break;
	case WXK_DOWN:		UpdateStatus( 1); break;

	case WXK_LEFT:
	case WXK_RIGHT:		UpdateStatus(); break;

	case WXK_PAGEUP:
		scrool_event.SetEventType(wxEVT_SCROLLWIN_PAGEUP);
		scrool_event.SetEventObject(asm_list);
		OnScroll(scrool_event);
		scrool_event.SetEventObject(hex_list);
		OnScroll(scrool_event);
	return;

	case WXK_PAGEDOWN:
		scrool_event.SetEventType(wxEVT_SCROLLWIN_PAGEDOWN);
		scrool_event.SetEventObject(asm_list);
		OnScroll(scrool_event);
		scrool_event.SetEventObject(hex_list);
		OnScroll(scrool_event);
	return;
	}

	event.Skip();
}

void CompilerELF::OnUpdate(wxCommandEvent& event)
{
	UpdateStatus();
	DoAnalyzeCode(false);

	return;
	asm_list->Freeze();
	asm_list->SetStyle(0, asm_list->GetValue().Len(), wxTextAttr("Black"));

	for(u32 i=0; i<instr_count; ++i)
	{
		SetOpStyle(m_instructions[i].name, "Blue");
	}
	
	SetOpStyle(".int", "Blue");
	SetOpStyle(".string", "Blue");
	SetOpStyle(".strlen", "Blue");
	SetOpStyle(".buf", "Blue");
	SetOpStyle(".srl", "Blue");
	SetOpStyle(".srr", "Blue");
	SetOpStyle(".mul", "Blue");
	SetOpStyle(".div", "Blue");
	SetOpStyle(".add", "Blue");
	SetOpStyle(".sub", "Blue");
	SetOpStyle(".and", "Blue");
	SetOpStyle(".or", "Blue");
	SetOpStyle(".xor", "Blue");
	SetOpStyle(".not", "Blue");
	SetOpStyle(".nor", "Blue");

	SetTextStyle("[", "Red");
	SetTextStyle("]", "Red");
	SetTextStyle(":", "Red");
	SetTextStyle(",", "Red");

	for(int p=0; (p = asm_list->GetValue().find('#', p)) >= 0;)
	{
		const int from = p++;
		p = asm_list->GetValue().find('\n', p);

		if(p < 0) p = asm_list->GetValue().Len();

		asm_list->SetStyle(from, p, wxTextAttr(0x009900));
	}
	
	for(int p=0; (p = asm_list->GetValue().find('"', p)) >= 0;)
	{
		const int from = p++;
		const int p1 = asm_list->GetValue().find('\n', p);

		int p2 = p;
		for(;;)
		{
			p2 = asm_list->GetValue().find('"', p2);
			if(p2 == -1) break;

			if(asm_list->GetValue()[p2 - 1] == '\\')
			{
				if(p2 > 2 && asm_list->GetValue()[p2 - 2] == '\\') break;

				p2++;
				continue;
			}
			break;
		}

		if(p1 < 0 && p2 < 0)
		{
			p = asm_list->GetValue().Len();
		}
		else if(p1 >= 0 && p2 < 0)
		{
			p = p1;
		}
		else if(p2 >= 0 && p1 < 0)
		{
			p = p2 + 1;
		}
		else
		{
			p = p1 > p2 ? p2 + 1 : p1;
		}

		asm_list->SetStyle(from, p, wxTextAttr(0x000099));
	}

	asm_list->Thaw();

	UpdateScroll(true, wxVERTICAL);
}

void CompilerELF::OnScroll(wxScrollWinEvent& event)
{
	const int id = event.GetEventObject() ? ((wxScrollBar*)event.GetEventObject())->GetId() : -1;

	wxTextCtrl* src = NULL;
	wxTextCtrl* dst = NULL;

	if(id == hex_list->GetId())
	{
		src = hex_list;
		dst = asm_list;
	}
	else if(id == asm_list->GetId())
	{
		src = asm_list;
		dst = hex_list;
	}

	if(!m_disable_scroll && src && dst && event.GetOrientation() == wxVERTICAL)
	{
		s64 kind = -1;

		if(event.GetEventType() == wxEVT_SCROLLWIN_TOP)
		{
			kind = SB_TOP;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_BOTTOM)
		{
			kind = SB_BOTTOM;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
		{
			kind = SB_LINEUP;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
		{
			kind = SB_LINEDOWN;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
		{
			kind = SB_PAGEUP;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
		{
			kind = SB_PAGEDOWN;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK)
		{
			kind = MAKEWPARAM(SB_THUMBTRACK, event.GetPosition());
			//dst->SetScrollPos(event.GetOrientation(), event.GetPosition());
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_THUMBRELEASE)
		{
			kind = MAKEWPARAM(SB_THUMBPOSITION, event.GetPosition());
			//dst->SetScrollPos(event.GetOrientation(), event.GetPosition());
		}

		if(kind >= 0)
		{
			m_disable_scroll = true;
			::SendMessage((HWND)dst->GetHWND(), (event.GetOrientation() == wxVERTICAL ? WM_VSCROLL : WM_HSCROLL), kind, 0);
			m_disable_scroll = false;
		}
	}

	event.Skip();
}

void CompilerELF::UpdateScroll(bool is_hex, int orient)
{
	wxScrollWinEvent event;

	event.SetEventType(wxEVT_SCROLLWIN_THUMBRELEASE);
	event.SetEventObject(is_hex ? asm_list : hex_list);
	event.SetPosition(is_hex ? asm_list->GetScrollPos(orient) : hex_list->GetScrollPos(orient));
	event.SetOrientation(orient);

	OnScroll(event);
}

void WriteEhdr(wxFile& f, Elf64_Ehdr& ehdr)
{
	Write32(f, ehdr.e_magic);
	Write8(f, ehdr.e_class);
	Write8(f, ehdr.e_data);
	Write8(f, ehdr.e_curver);
	Write8(f, ehdr.e_os_abi);
	Write64(f, ehdr.e_abi_ver);
	Write16(f, ehdr.e_type);
	Write16(f, ehdr.e_machine);
	Write32(f, ehdr.e_version);
	Write64(f, ehdr.e_entry);
	Write64(f, ehdr.e_phoff);
	Write64(f, ehdr.e_shoff);
	Write32(f, ehdr.e_flags);
	Write16(f, ehdr.e_ehsize);
	Write16(f, ehdr.e_phentsize);
	Write16(f, ehdr.e_phnum);
	Write16(f, ehdr.e_shentsize);
	Write16(f, ehdr.e_shnum);
	Write16(f, ehdr.e_shstrndx);
}

void WritePhdr(wxFile& f, Elf64_Phdr& phdr)
{
	Write32(f, phdr.p_type);
	Write32(f, phdr.p_flags);
	Write64(f, phdr.p_offset);
	Write64(f, phdr.p_vaddr);
	Write64(f, phdr.p_paddr);
	Write64(f, phdr.p_filesz);
	Write64(f, phdr.p_memsz);
	Write64(f, phdr.p_align);
}

void WriteShdr(wxFile& f, Elf64_Shdr& shdr)
{
	Write32(f, shdr.sh_name);
	Write32(f, shdr.sh_type);
	Write64(f, shdr.sh_flags);
	Write64(f, shdr.sh_addr);
	Write64(f, shdr.sh_offset);
	Write64(f, shdr.sh_size);
	Write32(f, shdr.sh_link);
	Write32(f, shdr.sh_info);
	Write64(f, shdr.sh_addralign);
	Write64(f, shdr.sh_entsize);
}

void CompilerELF::LoadElf(wxCommandEvent& event)
{
	wxFileDialog ctrl(this, L"Select ELF", wxEmptyString, wxEmptyString,
		"Elf Files (*.elf, *.bin)|*.elf;*.bin|"
		"All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if(ctrl.ShowModal() == wxID_CANCEL) return;
	LoadElf(ctrl.GetPath());
}

#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPUDecoder.h"

void CompilerELF::LoadElf(const wxString& path)
{
}

struct CompileProgram
{
	struct Branch
	{
		wxString m_name;
		s32 m_pos;
		s32 m_id;
		s32 m_addr;

		Branch(const wxString& name, s32 pos)
			: m_name(name)
			, m_pos(pos)
			, m_id(-1)
			, m_addr(-1)
		{
		}

		Branch(const wxString& name, u32 id, u32 addr)
			: m_name(name)
			, m_pos(-1)
			, m_id(id)
			, m_addr(addr)
		{
		}
	};

	bool m_analyze;
	s64 p;
	u64 m_line;
	const wxString& m_asm;
	wxTextCtrl& m_asm_list;
	wxTextCtrl& m_hex_list;
	wxTextCtrl& m_err_list;
	bool m_error;
	Array<u32> m_code;
	bool m_end_args;
	Array<Branch> m_branches;
	s32 m_branch_pos;
	u32 m_text_addr;

	struct SpData
	{
		wxString m_data;
		u32 m_addr;

		SpData(const wxString& data, u32 addr)
			: m_data(data)
			, m_addr(addr)
		{
		}
	};

	Array<SpData> m_sp_string;

	CompileProgram(const wxString& asm_, wxTextCtrl* asm_list, wxTextCtrl* hex_list, wxTextCtrl* err_list, bool analyze)
		: m_asm(asm_)
		, m_asm_list(*asm_list)
		, m_hex_list(*hex_list)
		, m_err_list(*err_list)
		, m_analyze(analyze)
		, p(0)
		, m_error(false)
		, m_line(1)
		, m_end_args(false)
		, m_branch_pos(0)
		, m_text_addr(0)
	{
	}

	static bool IsSkip(const char c) { return c == ' ' || c == '\t'; }
	static bool IsCommit(const char c) { return c == '#'; }
	bool IsEnd() const { return p >= m_asm.Len(); }
	bool IsEndLn(const char c) const { return c == '\n' || p - 1 >= m_asm.Len(); }

	char NextChar() { return *m_asm(p++, 1); }
	void NextLn() { while( !IsEndLn(NextChar()) ); if(!IsEnd()) m_line++; }
	void EndLn()
	{
		NextLn();
		p--;
		m_line--;
	}

	void FirstChar()
	{
		p = 0;
		m_line = 1;
		m_branch_pos = 0;
	}

	void PrevArg()
	{
		 while( --p >= 0 && (IsSkip(m_asm[p]) || m_asm[p] == ','));
		 while( --p >= 0 && !IsSkip(m_asm[p]) && !IsEndLn(m_asm[p]) );
		 if(IsEndLn(m_asm[p])) p++;
	}

	bool GetOp(wxString& result)
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

	bool GetArg(wxString& result, bool func = false)
	{
		s64 from = -1;

		for(;;)
		{
			const char cur_char = NextChar();
			const bool skip = IsSkip(cur_char);
			const bool commit = IsCommit(cur_char);
			const bool endln = IsEndLn(cur_char);
			const bool end = cur_char == ',' || (func && cur_char == ']');

			if(endln) p--;

			if(from == -1)
			{
				if(endln || commit || end) return false;
				if(!skip) from = p - 1;
				continue;
			}

			const bool text = m_asm[from] == '"';
			const bool end_text = cur_char == '"';

			if((text ? end_text : skip || commit || end) || endln)
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

				const s64 to = (endln || text ? p : p - 1) - from;
				bool ret = true;

				if(skip || text)
				{
					for(;;)
					{
						const char cur_char = NextChar();
						const bool skip = IsSkip(cur_char);
						const bool commit = IsCommit(cur_char);
						const bool endln = IsEndLn(cur_char);
						const bool end = cur_char == ',' || (func && cur_char == ']');

						if(skip) continue;
						if(end) break;

						if(commit)
						{
							ret = false;
							EndLn();
							break;
						}

						if(endln)
						{
							ret = false;
							p--;
							break;
						}

						WriteError(wxString::Format("Bad symbol '%c'", cur_char));
						m_error = true;
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
	}

	bool CheckEnd(bool show_err = true)
	{
		for(;;)
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

	void WriteError(const wxString& error)
	{
		m_err_list.WriteText(wxString::Format("line %lld: %s\n", m_line, error));
	}

	Array<Arg> m_args;
	u32 m_cur_arg;

	void DetectArgInfo(Arg& arg)
	{
		const wxString str = arg.string;

		if(str.Len() <= 0)
		{
			arg.type = ARG_ERR;
			return;
		}

		if(GetInstruction(str).mask != MASK_ERROR)
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

	void LoadArgs()
	{
		m_args.Clear();
		m_cur_arg = 0;

		wxString str;
		while(GetArg(str))
		{
			Arg arg(str);
			DetectArgInfo(arg);
			m_args.AddCpy(arg);
		}

		m_end_args = true;
	}

	u32 GetBranchValue(const wxString& branch)
	{
		for(u32 i=0; i<m_branches.GetCount(); ++i)
		{
			if(m_branches[i].m_name.Cmp(branch) != 0) continue;
			if(m_branches[i].m_pos >= 0) return m_text_addr + m_branches[i].m_pos * 4;

			return m_branches[i].m_addr;
		}

		return 0;
	}

	bool SetNextArgType(u32 types, bool show_err = true)
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

	bool SetNextArgBranch(u8 aa, bool show_err = true)
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

	bool IsBranchOp(const wxString& op) const
	{
		return op.Len() > 1 && op[op.Len() - 1] == ':';
	}

	bool IsFuncOp(const wxString& op) const
	{
		return op.Len() >= 1 && op[0] == '[';
	}

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

	SP_TYPE GetSpType(const wxString& op) const
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

	wxString GetSpStyle(const SP_TYPE sp) const
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

	bool IsSpOp(const wxString& op) const
	{
		return GetSpType(op) != SP_ERR;
	}

	Branch& GetBranch(const wxString& name)
	{
		for(u32 i=0; i<m_branches.GetCount(); ++i)
		{
			if(m_branches[i].m_name.Cmp(name) != 0) continue;

			return m_branches[i];
		}

		return m_branches[0];
	}

	void SetSp(const wxString& name, u32 addr, bool create)
	{
		if(create)
		{
			m_branches.Add(new Branch(name, -1, addr));
			return;
		}

		GetBranch(name);

		for(u32 i=0; i<m_branches.GetCount(); ++i)
		{
			if(m_branches[i].m_name.Cmp(name) != 0) continue;
			m_branches[i].m_addr = addr;
		}
	}

	void LoadSp(const wxString& op, Elf64_Shdr& s_opd)
	{
		SP_TYPE sp = GetSpType(op);

		wxString test;
		if(!GetArg(test) || test[0] != '[')
		{
			if(m_analyze) m_hex_list.WriteText("error\n");
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
			if(m_analyze) m_hex_list.WriteText("error\n");
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
				m_branches.Add(new Branch(wxEmptyString, -1, 0));
				dst_branch = &m_branches[m_branches.GetCount() - 1];
			}
			break;
		}

		if(!dst_branch)
		{
			if(m_analyze) m_hex_list.WriteText("error\n");
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
				if(m_analyze) m_hex_list.WriteText("error\n");
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
				if(m_analyze) m_hex_list.WriteText("error\n");
				WriteError(wxString::Format("bad src type. style: %s", GetSpStyle(sp)));
				m_error = true;
				NextLn();
				return;
			}

			if(m_asm[p - 1] != ']')
			{
				if(m_analyze) m_hex_list.WriteText("error\n");
				WriteError(wxString::Format("']' not found. style: %s", GetSpStyle(sp)));
				m_error = true;
				NextLn();
				return;
			}

			if(!CheckEnd())
			{
				if(m_analyze) m_hex_list.WriteText("error\n");
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
					m_sp_string.Add(new SpData(src1, addr));
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
				if(m_analyze) m_hex_list.WriteText("error\n");
				WriteError(wxString::Format("src1 not found. style: %s", GetSpStyle(sp)));
				m_error = true;
				NextLn();
				return;
			}

			Arg a_src1(src1);
			DetectArgInfo(a_src1);

			if(~(ARG_IMM | ARG_BRANCH) & a_src1.type)
			{
				if(m_analyze) m_hex_list.WriteText("error\n");
				WriteError(wxString::Format("bad src1 type. style: %s", GetSpStyle(sp)));
				m_error = true;
				NextLn();
				return;
			}

			wxString src2;
			if(!GetArg(src2, true))
			{
				if(m_analyze) m_hex_list.WriteText("error\n");
				WriteError(wxString::Format("src2 not found. style: %s", GetSpStyle(sp)));
				m_error = true;
				return;
			}

			Arg a_src2(src2);
			DetectArgInfo(a_src2);

			if(~(ARG_IMM | ARG_BRANCH) & a_src2.type)
			{
				if(m_analyze) m_hex_list.WriteText("error\n");
				WriteError(wxString::Format("bad src2 type. style: %s", GetSpStyle(sp)));
				m_error = true;
				NextLn();
				return;
			}

			if(m_asm[p - 1] != ']')
			{
				if(m_analyze) m_hex_list.WriteText("error\n");
				WriteError(wxString::Format("']' not found. style: %s", GetSpStyle(sp)));
				m_error = true;
				NextLn();
				return;
			}

			if(!CheckEnd())
			{
				if(m_analyze) m_hex_list.WriteText("error\n");
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

		if(m_analyze) m_hex_list.WriteText("\n");
	}

	void Compile()
	{
		m_err_list.Freeze();
		m_err_list.Clear();
		
		if(m_analyze)
		{
			m_hex_list.Freeze();
			m_hex_list.Clear();
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

			m_branches.Add(new Branch(name, a_id.value, 0));
			const u32 import = m_branches.GetCount() - 1;

			bool founded = false;
			for(u32 i=0; i<modules.GetCount(); ++i)
			{
				if(modules[i].m_name.Cmp(module) != 0) continue;
				founded = true;
				modules[i].Add(import);
				break;
			}

			if(!founded) modules.Add(new Module(module, import));
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

				m_branches.Add(new Branch(name, m_branch_pos));

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

		if(!has_entry) m_branches.Add(new Branch("entry", 0));

		if(m_analyze) m_error = false;
		FirstChar();

		while(!IsEnd())
		{
			m_args.Clear();
			m_end_args = false;

			wxString op;
			if(!GetOp(op) || IsBranchOp(op) || IsFuncOp(op))
			{
				if(m_analyze) m_hex_list.WriteText("\n");
				NextLn();
				continue;
			}

			if(IsSpOp(op))
			{
				LoadSp(op, s_opd);
				continue;
			}

			LoadArgs();

			Instruction instr = GetInstruction(op);
			switch(instr.mask)
			{
			case MASK_ERROR:
				WriteError(wxString::Format("unknown instruction '%s'", op));
				EndLn();
				m_error = true;
			break;

			case MASK_UNK:
				SetNextArgType(ARG_IMM);
			break;

			case MASK_NO_ARG:
			break;

			case MASK_RSD_RA_IMM16:
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_IMM);
			break;

			case MASK_RA_RST_IMM16:
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_IMM);
			break;

			case MASK_CRFD_RA_IMM16:
				SetNextArgType(ARG_REG_CR | ARG_REG_R);

				if(m_args.GetCount() > 0) 
				{
					switch(m_args[0].type)
					{
					case ARG_REG_CR:
						SetNextArgType(ARG_REG_R);
					break;

					case ARG_REG_R:
						m_args.InsertCpy(0, Arg("cr0", 0, ARG_REG_CR));
					break;
					}
				}
				
				SetNextArgType(ARG_IMM);
			break;

			case MASK_BO_BI_BD:
				SetNextArgType(ARG_IMM);
				SetNextArgType(ARG_REG_CR);
				SetNextArgBranch(instr.smask & SMASK_AA);
			break;

			case MASK_BI_BD:
				if(!SetNextArgType(ARG_REG_CR, false))
				{
					m_args.InsertCpy(0, Arg("cr0", 0, ARG_REG_CR));
				}
				SetNextArgBranch(instr.smask & SMASK_AA);
			break;

			case MASK_SYS:
				if(!SetNextArgType(ARG_IMM, false)) m_args.AddCpy(Arg("2", 2, ARG_IMM));
			break;

			case MASK_LL:
				SetNextArgBranch(instr.smask & SMASK_AA);
			break;

			case MASK_RA_RS_SH_MB_ME:
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_IMM);
				SetNextArgType(ARG_IMM);
				SetNextArgType(ARG_IMM);
			break;

			case MASK_RA_RS_RB_MB_ME:
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_IMM);
				SetNextArgType(ARG_IMM);
			break;

			case MASK_RST_RA_D:
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_IMM);
			break;

			case MASK_RST_RA_DS:
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_IMM);
			break;

			case MASK_TO_RA_IMM16:
				SetNextArgType(ARG_IMM);
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_IMM);
			break;

			case MASK_RSD_IMM16:
				SetNextArgType(ARG_REG_R);
				SetNextArgType(ARG_IMM);
			break;
			}

			CheckEnd();

			if(m_error)
			{
				if(m_analyze)
				{
					m_hex_list.WriteText("error\n");
					m_error = false;
					continue;
				}

				break;
			}

			const u32 code = CompileInstruction(instr, m_args);

			if(m_analyze) m_hex_list.WriteText(wxString::Format("0x%08x\n", code));

			if(!m_analyze) m_code.AddCpy(code);

			m_branch_pos++;
		}

		if(!m_analyze && !m_error)
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

			wxFile f("compiled.elf", wxFile::write);

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
				Write32(f, ToOpcode(ADDI) | ToRD(12)); //li r12,0
				Write32(f, ToOpcode(ORIS) | ToRD(12) | ToRA(12) | ToIMM16(addr >> 16)); //oris r12,r12,addr>>16
				Write32(f, ToOpcode(LWZ)  | ToRD(12) | ToRA(12) | ToIMM16(addr)); //lwz r12,addr&0xffff(r12)
				Write32(f, 0xf8410028); //std r2,40(r1)
				Write32(f, ToOpcode(LWZ)  | ToRD(0) | ToRA(12) | ToIMM16(0)); //lwz r0,0(r12)
				Write32(f, ToOpcode(LWZ)  | ToRD(2) | ToRA(12) | ToIMM16(4)); //lwz r2,4(r12)
				Write32(f, 0x7c0903a6); //mtctr r0
				Write32(f, 0x4e800420); //bctr
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
			free(opd_data);
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

		m_err_list.Thaw();
		if(m_analyze) m_hex_list.Thaw();
		else system("make_fself.cmd");
	}
};

s64 FindOp(const wxString& text, const wxString& op, s64 from)
{
	if(text.Len() < op.Len()) return -1;

	for(s64 i=from; i<text.Len(); ++i)
	{
		if(i - 1 < 0 || text[i - 1] == '\n' || CompileProgram::IsSkip(text[i - 1]))
		{
			if(text.Len() - i < op.Len()) return -1;

			if(text(i, op.Len()).Cmp(op) != 0) continue;
			if(i + op.Len() >= text.Len() || text[i + op.Len()] == '\n' ||
				CompileProgram::IsSkip(text[i + op.Len()])) return i;
		}
	}

	return -1;
}

void CompilerELF::SetTextStyle(const wxString& text, const wxColour& color, bool bold)
{
	for(int p=0; (p = asm_list->GetValue().find(text, p)) >= 0; p += text.Len())
	{
		asm_list->SetStyle(p, p + text.Len(), wxTextAttr(color, wxNullColour, 
			wxFont(-1, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL)));
	}
}

void CompilerELF::SetOpStyle(const wxString& text, const wxColour& color, bool bold)
{
	for(int p=0; (p = FindOp(asm_list->GetValue(), text, p)) >= 0; p += text.Len())
	{
		asm_list->SetStyle(p, p + text.Len(), wxTextAttr(color, wxNullColour, 
			wxFont(-1, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL)));
	}
}

void CompilerELF::DoAnalyzeCode(bool compile)
{
	CompileProgram(asm_list->GetValue(), asm_list, hex_list, err_list, !compile).Compile();

	/*
	return;

	MTProgressDialog* progress_dial = 
			new MTProgressDialog(this, wxDefaultSize, "Analyze code...", "Loading...", wxArrayLong(), 1);
	(new AnalyzeThread())->DoAnalyze(compile, progress_dial, this);
	*/
}