#include "stdafx.h"
#include "DisAsmFrame.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/PPCThread.h"
#include "Emu/System.h"
#include "Gui/DisAsmFrame.h"
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/SPUDecoder.h"
#include "Emu/Cell/SPUDisAsm.h"

DisAsmFrame::DisAsmFrame(PPCThread& cpu)
	: wxFrame(NULL, wxID_ANY, "DisAsm")
	, CPU(cpu)
{
	exit = false;
	count = 0;
	wxBoxSizer& s_panel( *new wxBoxSizer(wxVERTICAL) );
	wxBoxSizer& s_b_panel( *new wxBoxSizer(wxHORIZONTAL) );

	m_disasm_list = new wxListView(this);

	wxButton& b_fprev   = *new wxButton(this, wxID_ANY, L"<<");
	wxButton& b_prev    = *new wxButton(this, wxID_ANY, L"<");
	wxButton& b_next    = *new wxButton(this, wxID_ANY, L">");
	wxButton& b_fnext   = *new wxButton(this, wxID_ANY, L">>");

	wxButton& b_dump    = *new wxButton(this, wxID_ANY, L"Dump code");

	wxButton& b_setpc   = *new wxButton(this, wxID_ANY, L"Set PC");

	s_b_panel.Add(&b_fprev);
	s_b_panel.Add(&b_prev);
	s_b_panel.AddSpacer(5);
	s_b_panel.Add(&b_next);
	s_b_panel.Add(&b_fnext);
	s_b_panel.AddSpacer(8);
	s_b_panel.Add(&b_dump);

	s_panel.Add(&s_b_panel);
	s_panel.Add(&b_setpc);
	s_panel.Add(m_disasm_list);

	m_disasm_list->InsertColumn(0, "PC");
	m_disasm_list->InsertColumn(1, "ASM");

	m_disasm_list->SetColumnWidth( 0, 50 );

	for(uint i=0; i<LINES_OPCODES; ++i) m_disasm_list->InsertItem(i, -1);

	SetSizerAndFit( &s_panel );

	SetSize(50, 660);

	Connect( wxEVT_SIZE, wxSizeEventHandler(DisAsmFrame::OnResize) );

	m_app_connector.Connect(m_disasm_list->GetId(), wxEVT_MOUSEWHEEL, wxMouseEventHandler(DisAsmFrame::MouseWheel), (wxObject*)0, this);

	Connect(b_prev.GetId(),  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DisAsmFrame::Prev));
	Connect(b_next.GetId(),  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DisAsmFrame::Next));
	Connect(b_fprev.GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DisAsmFrame::fPrev));
	Connect(b_fnext.GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DisAsmFrame::fNext));
	Connect(b_setpc.GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DisAsmFrame::SetPc));

	Connect(b_dump.GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DisAsmFrame::Dump));
}

void DisAsmFrame::OnResize(wxSizeEvent& event)
{
	const wxSize size(GetClientSize());
	m_disasm_list->SetSize( size.GetWidth(), size.GetHeight() - 25 );
	m_disasm_list->SetColumnWidth( 1, size.GetWidth() - (m_disasm_list->GetColumnWidth(0) + 8) );

	event.Skip();
}

void DisAsmFrame::AddLine(const wxString line)
{
	static bool finished = false;

	if(finished && Emu.IsRunning())
	{
		count = 0;
		finished = false;
	}
	else if(count >= LINES_OPCODES || !Emu.IsRunning())
	{
		if(Emu.IsRunning()) Emu.Pause();
		finished = true;
		CPU.PC -= 4;
		return;
	}

	m_disasm_list->SetItem(count, 0, wxString::Format("%llx", CPU.PC));
	m_disasm_list->SetItem(count, 1, line);

	++count;
}

void DisAsmFrame::Resume()
{
	Emu.Resume();
}

#include <Utilities/MTProgressDialog.h>
#include "Loader/ELF.h"
std::vector<Elf64_Shdr>* shdr_arr_64 = NULL;
std::vector<Elf32_Shdr>* shdr_arr_32 = NULL;
ELF64Loader* l_elf64 = NULL;
ELF32Loader* l_elf32 = NULL;
bool ElfType64 = false;

class DumperThread : public ThreadBase
{
	volatile uint id;
	PPCDisAsm* disasm;
	PPCDecoder* decoder;
	volatile bool* done;
	volatile u8 cores;
	MTProgressDialog* prog_dial;
	wxArrayString** arr;

public:
	DumperThread() : ThreadBase("DumperThread")
	{
	}

	void Set(uint _id, u8 _cores, bool* _done, MTProgressDialog& _prog_dial, wxArrayString** _arr)
	{
		id = _id;
		cores = _cores;
		done = _done;
		prog_dial = &_prog_dial;
		arr = _arr;

		*done = false;

		if(Emu.GetCPU().GetThreads()[0]->GetType() != CPU_THREAD_PPU)
		{
			SPUDisAsm& dis_asm = *new SPUDisAsm(CPUDisAsm_DumpMode);
			decoder = new SPUDecoder(dis_asm);
			disasm = &dis_asm;
		}
		else
		{
			PPUDisAsm* dis_asm = new PPUDisAsm(CPUDisAsm_DumpMode);
			decoder = new PPUDecoder(dis_asm);
			disasm = dis_asm;
		}
	}

	virtual void Task()
	{
		ConLog.Write("Start dump in thread %d!", (int)id);
		const u32 max_value = prog_dial->GetMaxValue(id);
		const u32 shdr_count = ElfType64 ? shdr_arr_64->size() : shdr_arr_32->size();

		for(u32 sh=0, vsize=0; sh<shdr_count; ++sh)
		{
			const u64 sh_size = (ElfType64 ? (*shdr_arr_64)[sh].sh_size : (*shdr_arr_32)[sh].sh_size) / 4;
			const u64 sh_addr = (ElfType64 ? (*shdr_arr_64)[sh].sh_addr : (*shdr_arr_32)[sh].sh_addr);

			u64 d_size = sh_size / cores;
			const u64 s_fix = sh_size % cores;
			if(id <= s_fix) d_size++;

			for(u64 off = id * 4, size=0; size<d_size; vsize++, size++)
			{
				prog_dial->Update(id, vsize,
					wxString::Format("%d thread: %d of %d", (int)id + 1, vsize, max_value));

				disasm->dump_pc = sh_addr + off;
				decoder->Decode(Memory.Read32(disasm->dump_pc));

				arr[id][sh].Add(fmt::FromUTF8(disasm->last_opcode));

				off += (cores - id) * 4;
				off += id * 4;
			}
		}

		ConLog.Write("Finish dump in thread %d!", (int)id);

		*done = true;
	}

	void OnExit()
	{
		ConLog.Write("CleanUp dump thread (%d)!", (int)id);
		safe_delete(decoder);
	}
};

struct WaitDumperThread : public ThreadBase
{
	volatile bool* done;
	volatile u8 cores;
	wxString patch;
	MTProgressDialog& prog_dial;
	wxArrayString** arr;

	WaitDumperThread(bool* _done, u8 _cores, wxString _patch, MTProgressDialog& _prog_dial, wxArrayString** _arr) 
		: ThreadBase("WaitDumperThread")
		, done(_done)
		, cores(_cores)
		, patch(_patch)
		, prog_dial(_prog_dial)
		, arr(_arr)
	{
	}

	~WaitDumperThread()
	{
		delete (bool*)done;
		done = NULL;
	}

	virtual void Task()
	{
		for(uint i=0; i<cores; i++)
		{
			while(done[i] == false) Sleep(1);
		}

		ConLog.Write("Saving dump is started!");
		const uint length_for_core = prog_dial.GetMaxValue(0);
		const uint length = length_for_core * cores;
		prog_dial.Close();

		wxArrayLong max;
		max.Add(length);
		MTProgressDialog& prog_dial2 = 
			*new MTProgressDialog(NULL, wxDefaultSize, "Saving", "Loading...", max, 1);

		wxFile fd;
		fd.Open(patch, wxFile::write);

		const u32 shdr_count = ElfType64 ? shdr_arr_64->size() : shdr_arr_32->size();

		for(uint sh=0, counter=0; sh<shdr_count; ++sh)
		{
			const u64 sh_size = ElfType64 ? (*shdr_arr_64)[sh].sh_size : (*shdr_arr_32)[sh].sh_size;

			if(!sh_size) continue;
			const uint c_sh_size = sh_size / 4;

			fd.Write(wxString::Format("Start of section header %d (instructions count: %d)\n", sh, c_sh_size));

			for(uint i=0, c=0, v=0; i<c_sh_size; i++, c++, counter++)
			{
				if(c >= cores)
				{
					c = 0;
					v++;
				}

				prog_dial2.Update(0, counter, wxString::Format("Saving data to file: %d of %d", counter, length));

				if(v >= arr[c][sh].GetCount()) continue;

				fd.Write("\t");
				fd.Write(arr[c][sh][v]);
			}

			fd.Write(wxString::Format("End of section header %d\n\n", sh));
		}
		
		ConLog.Write("CleanUp dump saving!");

		for(uint c=0; c<cores; ++c)
		{
			for(uint sh=0; sh<shdr_count; ++sh) arr[c][sh].Empty();
		}

		delete[] arr;

		//safe_delete(shdr_arr);
		delete l_elf32;
		delete l_elf64;

		prog_dial2.Close();
		fd.Close();

		wxMessageBox("Dumping done.", "rpcs3 message");

		Emu.Stop();
	}
};

void DisAsmFrame::Dump(wxCommandEvent& WXUNUSED(event)) 
{
	wxFileDialog ctrl( this, L"Select output file...",
		wxEmptyString, "DisAsm.txt", "*.txt", wxFD_SAVE);

	if(ctrl.ShowModal() == wxID_CANCEL) return;

	vfsLocalFile& f_elf = *new vfsLocalFile(nullptr);
	f_elf.Open(Emu.m_path);

	ConLog.Write("path: %s", Emu.m_path.c_str());
	Elf_Ehdr ehdr;
	ehdr.Load(f_elf);

	if(!ehdr.CheckMagic())
	{
		ConLog.Error("Corrupted ELF!");
		return;
	}
	std::vector<std::string> name_arr;

	switch(ehdr.GetClass())
	{
	case CLASS_ELF64:
		ElfType64 = true;
		l_elf64 = new ELF64Loader(f_elf);
		if(!l_elf64->LoadInfo())
		{
			delete l_elf64;
			return;
		}
		name_arr = l_elf64->shdr_name_arr;
		shdr_arr_64 = &l_elf64->shdr_arr;
		if(l_elf64->shdr_arr.size() <= 0) return;
	break;

	case CLASS_ELF32:
		ElfType64 = false;
		l_elf32 = new ELF32Loader(f_elf);
		if(!l_elf32->LoadInfo())
		{
			delete l_elf32;
			return;
		}

		name_arr = l_elf32->shdr_name_arr;
		shdr_arr_32 = &l_elf32->shdr_arr;
		if(l_elf32->shdr_arr.size() <= 0) return;
	break;

	default: ConLog.Error("Corrupted ELF!"); return;
	}

	PPCDisAsm* disasm;
	PPCDecoder* decoder;

	switch(Emu.GetCPU().GetThreads()[0]->GetType())
	{
	case CPU_THREAD_PPU:
	{
		PPUDisAsm* dis_asm = new PPUDisAsm(CPUDisAsm_DumpMode);
		decoder = new PPUDecoder(dis_asm);
		disasm = dis_asm;
	}
	break;

	case CPU_THREAD_SPU:
	case CPU_THREAD_RAW_SPU:
	{
		SPUDisAsm& dis_asm = *new SPUDisAsm(CPUDisAsm_DumpMode);
		decoder = new SPUDecoder(dis_asm);
		disasm = &dis_asm;
	}
	break;
	}

	const u32 shdr_count = ElfType64 ? shdr_arr_64->size() : shdr_arr_32->size();

	u64 max_count = 0;
	for(u32 sh=0; sh<shdr_count; ++sh)
	{
		const u64 sh_size = (ElfType64 ? (*shdr_arr_64)[sh].sh_size : (*shdr_arr_32)[sh].sh_size) / 4;
		max_count += sh_size;
	}

	wxArrayLong max;
	max.Add(max_count);
	MTProgressDialog& prog_dial = *new MTProgressDialog(NULL, wxDefaultSize, "Saving", "Loading...", max, 1);
	max.Clear();

	wxFile fd(ctrl.GetPath(), wxFile::write);

	for(u32 sh=0, vsize=0; sh<shdr_count; ++sh)
	{
		const u64 sh_size = (ElfType64 ? (*shdr_arr_64)[sh].sh_size : (*shdr_arr_32)[sh].sh_size) / 4;
		const u64 sh_addr = (ElfType64 ? (*shdr_arr_64)[sh].sh_addr : (*shdr_arr_32)[sh].sh_addr);

		const std::string name = sh < name_arr.size() ? name_arr[sh] : "Unknown";

		fd.Write(wxString::Format("Start of section header %s[%d] (instructions count: %d)\n", name.c_str(), sh, sh_size));
		prog_dial.Update(0, vsize, wxString::Format("Disasm %s section", name.c_str()));

		if(Memory.IsGoodAddr(sh_addr))
		{
			for(u64 addr=sh_addr; addr<sh_addr+sh_size; addr++, vsize++)
			{
				disasm->dump_pc = addr;
				decoder->Decode(Memory.Read32(disasm->dump_pc));
				fd.Write("\t");
				fd.Write(fmt::FromUTF8(disasm->last_opcode));
			}
		}
		fd.Write(wxString::Format("End of section header %s[%d]\n\n", name.c_str(), sh));
	}

	prog_dial.Close();
	Emu.Stop();
	/*
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	const uint cores_count =
		(si.dwNumberOfProcessors < 1 || si.dwNumberOfProcessors > 8 ? 2 : si.dwNumberOfProcessors); 

	wxArrayLong max;
	max.Clear();

	u64 max_count = 0;

	if(ElfType64)
	{
		for(uint sh=0; sh<l_elf64->shdr_arr.GetCount(); ++sh)
		{
			max_count += l_elf64->shdr_arr[sh].sh_size / 4;
		}
	}
	else
	{
		for(uint sh=0; sh<l_elf32->shdr_arr.GetCount(); ++sh)
		{
			max_count += l_elf32->shdr_arr[sh].sh_size / 4;
		}
	}

	for(uint c=0; c<cores_count; ++c) max.Add(max_count / cores_count);
	for(uint c=0; c<max_count % cores_count; ++c) max[c]++;

	MTProgressDialog& prog_dial = *new MTProgressDialog(this, wxDefaultSize, "Dumping...", "Loading", max, cores_count);

	DumperThread* dump = new DumperThread[cores_count];
	wxArrayString** arr = new wxArrayString*[cores_count];

	bool* threads_done = new bool[cores_count];

	for(uint i=0; i<cores_count; ++i)
	{
		arr[i] = new wxArrayString[ElfType64 ? l_elf64->shdr_arr.GetCount() : l_elf32->shdr_arr.GetCount()];
		dump[i].Set(i, cores_count, &threads_done[i], prog_dial, arr);
		dump[i].Start();
	}

	WaitDumperThread& wait_dump = 
		*new WaitDumperThread(threads_done, cores_count, ctrl.GetPath(), prog_dial, arr);
	wait_dump.Start();
	*/
}

void DisAsmFrame::Prev (wxCommandEvent& WXUNUSED(event)) { if(Emu.IsPaused()) { CPU.SetPc( CPU.PC - 4*(LINES_OPCODES+1)); Resume(); } }
void DisAsmFrame::Next (wxCommandEvent& WXUNUSED(event)) { if(Emu.IsPaused()) { CPU.SetPc( CPU.PC - 4*(LINES_OPCODES-1)); Resume(); } }
void DisAsmFrame::fPrev(wxCommandEvent& WXUNUSED(event)) { if(Emu.IsPaused()) { CPU.SetPc( CPU.PC - (4*LINES_OPCODES)*2); Resume(); } }
void DisAsmFrame::fNext(wxCommandEvent& WXUNUSED(event)) { if(Emu.IsPaused()) { Resume(); } }
void DisAsmFrame::SetPc(wxCommandEvent& WXUNUSED(event))
{
	if(!Emu.IsPaused()) return;

	wxDialog diag(this, wxID_ANY, "Set PC", wxDefaultPosition);

	wxBoxSizer* s_panel(new wxBoxSizer(wxVERTICAL));
	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));
	wxTextCtrl* p_pc(new wxTextCtrl(&diag, wxID_ANY));

	s_panel->Add(p_pc);
	s_panel->AddSpacer(8);
	s_panel->Add(s_b_panel);

	s_b_panel->Add(new wxButton(&diag, wxID_OK), wxLEFT, 0, 5);
	s_b_panel->AddSpacer(5);
	s_b_panel->Add(new wxButton(&diag, wxID_CANCEL), wxRIGHT, 0, 5);

	diag.SetSizerAndFit( s_panel );

	p_pc->SetLabel(wxString::Format("%llx", CPU.PC));

	if(diag.ShowModal() == wxID_OK)
	{
		sscanf(fmt::ToUTF8(p_pc->GetLabel()).c_str(), "%llx", &CPU.PC);
		Resume();
	}
}

void DisAsmFrame::MouseWheel(wxMouseEvent& event)
{
	if(!Emu.IsPaused())
	{
		event.Skip();
		return;
	}

	const int value = (event.m_wheelRotation / event.m_wheelDelta);

	if(event.ControlDown())
	{
		CPU.SetPc( CPU.PC - (((4*LINES_OPCODES)*2)*value) );
	}
	else
	{
		CPU.SetPc( CPU.PC - 4*(LINES_OPCODES + (value /** event.m_linesPerAction*/)) );
	}

	Emu.Resume();

	event.Skip();
}
