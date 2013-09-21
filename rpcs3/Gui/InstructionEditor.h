#pragma once
#include "Emu/Cell/PPCThread.h"
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/SPUDecoder.h"
#include "Emu/Cell/SPUDisAsm.h"

class InstructionEditorDialog
	: public wxDialog
{
	u64 pc;
	PPC_DisAsm* disasm;
	PPC_Decoder* decoder;
	wxTextCtrl* t2_instr;
	wxStaticText* t3_preview;

public:
	PPCThread* CPU;

public:
	InstructionEditorDialog(wxPanel *parent, u64 _pc, PPCThread* _CPU, PPC_Decoder* _decoder, PPC_DisAsm* _disasm);

	void updatePreview(wxCommandEvent& event);
};