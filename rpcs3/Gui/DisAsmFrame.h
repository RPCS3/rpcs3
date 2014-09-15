#pragma once

class PPCThread;

class DisAsmFrame : public wxFrame
{
	static const uint LINES_OPCODES = 40;
	u32 count;

	wxListView* m_disasm_list;
	PPCThread& CPU;

	virtual void OnResize(wxSizeEvent& event);

	virtual void Prev (wxCommandEvent& event);
	virtual void Next (wxCommandEvent& event);
	virtual void fPrev(wxCommandEvent& event);
	virtual void fNext(wxCommandEvent& event);
	virtual void SetPc(wxCommandEvent& event);

	void Dump(wxCommandEvent& event);
	void Resume();
	void MouseWheel(wxMouseEvent& event);

public:
	bool exit;
	DisAsmFrame(PPCThread& cpu);
	~DisAsmFrame()
	{
		exit = true;
	}

	virtual void AddLine(const wxString line);
};