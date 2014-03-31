#include "stdafx.h"
#include "SaveDataList.h"

void SaveDataList::Load(const std::vector<SaveDataListEntry>& entries)
{
	wxDialog dialog(NULL, wxID_ANY, "test", wxDefaultPosition, wxSize(450, 680));

	wxPanel *panel = new wxPanel(&dialog, -1);
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
	wxListCtrl* list = new wxListCtrl(&dialog, wxID_ANY, wxPoint(10,10), wxSize(400,600));

	list->InsertColumn(0, "Icon");
	list->InsertColumn(1, "Information");
	wxImageList* pImageList = new wxImageList(320,176);

	for (int i=0; i<entries.size(); i++)
	{
		list->InsertItem (i, i);
		list->SetItemColumnImage (i, 0, i);
		list->SetItem (i, 1, wxString::Format("%d",i+1));

		wxImage img(320, 176, entries[i].iconBuffer);
		pImageList->Add(img);
	}
	list->SetImageList(pImageList, wxIMAGE_LIST_SMALL);

	panel->AddChild(list);

	vbox->Add(panel, 1);
	vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

	dialog.SetSizer(vbox);
	dialog.Centre();
	dialog.ShowModal();
	dialog.Destroy(); 
}
