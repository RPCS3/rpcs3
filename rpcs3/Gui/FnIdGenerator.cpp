#include "stdafx.h"
#include "FnIdGenerator.h"

FnIdGenerator::FnIdGenerator(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "FunctionID Generator", wxDefaultPosition)
	, m_list(nullptr)
{
	wxBoxSizer* s_panel(new wxBoxSizer(wxHORIZONTAL));
	wxBoxSizer* s_subpanel(new wxBoxSizer(wxVERTICAL));
	wxBoxSizer* s_subpanel2(new wxBoxSizer(wxHORIZONTAL));

	m_list = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(300, 450));
	m_list->InsertColumn(0, "Function Name", 0, 200);
	m_list->InsertColumn(1, "Function ID", 0, 100);

	wxButton *b_input_name = new wxButton(this, wxID_ANY, "Input Function Name", wxDefaultPosition, wxSize(140, -1));
	wxButton *b_clear = new wxButton(this, wxID_ANY, "Clear List", wxDefaultPosition, wxSize(140, -1));

	s_subpanel2->Add(b_input_name);
	s_subpanel2->AddSpacer(10);
	s_subpanel2->Add(b_clear);
	
	s_subpanel->AddSpacer(5);
	s_subpanel->Add(m_list);
	s_subpanel->AddSpacer(5);
	s_subpanel->Add(s_subpanel2);

	s_panel->AddSpacer(5);
	s_panel->Add(s_subpanel);
	s_panel->AddSpacer(5);

	this->SetSizerAndFit(s_panel);

	Connect(b_input_name->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FnIdGenerator::OnInput));
	Connect(b_clear->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FnIdGenerator::OnClear));
}

FnIdGenerator::~FnIdGenerator()
{
	delete m_list;
	m_list = nullptr;
}

void FnIdGenerator::OnInput(wxCommandEvent &event)
{
	PrintId();
	event.Skip();
}

void FnIdGenerator::OnClear(wxCommandEvent &event)
{
	m_list->DeleteAllItems();
	m_func_name.clear();
	m_func_id.clear();
	event.Skip();
}

u32 FnIdGenerator::GenerateFnId(const std::string& func_name)
{
	static const char* suffix = "\x67\x59\x65\x99\x04\x25\x04\x90\x56\x64\x27\x49\x94\x89\x74\x1A"; //symbol name suffix
	std::string input = func_name + suffix;
	unsigned char output[20];

	sha1((unsigned char*)input.c_str(), input.length(), output); //compute SHA-1 hash

	return (be_t<u32>&) output[0];
}

void FnIdGenerator::PrintId()
{
	const std::string temp;
	TextInputDialog dial(0, temp, "Please input function name");
	if (dial.ShowModal() == wxID_OK)
	{
		dial.GetResult();
	}

	const std::string& func_name = dial.GetResult();

	if (func_name.length() == 0)
		return;

	const be_t<u32> result = GenerateFnId(func_name);
	m_func_name.push_back(func_name);
	m_func_id.push_back(result);

	ConLog.Write("Function: %s, Id: 0x%08x ", func_name.c_str(), result);
	UpdateInformation();
}

void FnIdGenerator::UpdateInformation()
{
	m_list->DeleteAllItems();

	for(u32 i = 0; i < m_func_name.size(); i++)
	{
		m_list->InsertItem(m_func_name.size(), wxEmptyString);
		m_list->SetItem(i, 0, m_func_name[i]);
		m_list->SetItem(i, 1, wxString::Format("0x%08x", re(m_func_id[i])));
	}
}