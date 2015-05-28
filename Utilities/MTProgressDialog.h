#pragma once

class MTProgressDialog : public wxDialog
{
	wxGauge** m_gauge;
	wxStaticText** m_msg;

	wxArrayLong m_maximum;
	const u8 m_cores;

	static const uint layout = 16;
	static const uint maxdial = 65536;
	wxArrayInt m_lastupdate;

public:
	MTProgressDialog(wxWindow* parent, const wxSize& size, const wxString& title,
			const wxString& msg, const wxArrayLong& maximum, const u8 cores)
		: wxDialog(parent, wxID_ANY, title, wxDefaultPosition)
		, m_maximum(maximum)
		, m_cores(cores)
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

		m_gauge = new wxGauge*[m_cores];
		m_msg = new wxStaticText*[m_cores];

		m_lastupdate.SetCount(cores);

		for(uint i=0; i<m_cores; ++i)
		{
			m_lastupdate[i] = -1;

			m_msg[i] = new wxStaticText(this, wxID_ANY, msg);
			sizer->Add(m_msg[i], 0, wxLEFT | wxTOP, layout);

			m_gauge[i] = new wxGauge(this, wxID_ANY, maxdial,
									  wxDefaultPosition, wxDefaultSize,
									  wxGA_HORIZONTAL );

			sizer->Add(m_gauge[i], 0, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, layout);
			m_gauge[i]->SetValue(0);

			sizer->AddSpacer(5);
		}

		SetSizerAndFit(sizer);
		if(size != wxDefaultSize)
		{
			SetSize(size);
		}
		else
		{
			wxSize ws;
			ws.x = 400;
			ws.y = GetSize().y + 8;
			SetSize(ws);
		}

		m_maximum.SetCount(m_cores);

		Show();
	}

	force_inline void Update(const u8 thread_id, const u64 value, const wxString& msg)
	{
		if(thread_id > m_cores) return;

		const int curupdate = (int)(((double)value/(double)m_maximum[thread_id])*1000);
		if(curupdate == m_lastupdate[thread_id]) return;
		m_lastupdate[thread_id] = curupdate;

		m_msg[thread_id]->SetLabel(msg);

		if(value >= (u32)m_maximum[thread_id]) return;
		m_gauge[thread_id]->SetValue(((double)value / (double)m_maximum[thread_id]) * maxdial);
	}

	const u32 GetMaxValue(const uint thread_id) const
	{
		if(thread_id > m_cores) return 0;
		return m_maximum[thread_id];
	}

	void SetMaxFor(const uint thread_id, const u64 val)
	{
		if(thread_id > m_cores) return;
		m_maximum[thread_id] = val;
		m_lastupdate[thread_id] = 0;
	}

	virtual void Close(bool force = false)
	{
		m_lastupdate.Empty();
		m_maximum.Empty();

		wxDialog::Close(force);
	}
};
