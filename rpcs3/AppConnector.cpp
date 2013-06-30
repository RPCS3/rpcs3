#include "stdafx.h"
#include "AppConnector.h"

AppConnector::~AppConnector()
{
	for(uint i=0; i<m_connect_arr.GetCount(); ++i)
	{
		wxGetApp().Disconnect(
			m_connect_arr[i].winid,
			m_connect_arr[i].lastId,
			m_connect_arr[i].eventType,
			m_connect_arr[i].func,
			m_connect_arr[i].userData,
			m_connect_arr[i].eventSink);
	}
}

void AppConnector::Connect(int winid, int lastId, int eventType, wxObjectEventFunction func, wxObject* userData, wxEvtHandler* eventSink)
{
	m_connect_arr.Move(new ConnectInfo(winid, lastId, eventType, func, userData, eventSink));
	wxGetApp().Connect(winid, lastId, eventType, func, userData, eventSink);
}

void AppConnector::Connect(int winid, int eventType, wxObjectEventFunction func, wxObject* userData, wxEvtHandler* eventSink)
{
	Connect(winid, wxID_ANY, eventType, func, userData, eventSink);
}

void AppConnector::Connect(int eventType, wxObjectEventFunction func, wxObject* userData, wxEvtHandler* eventSink)
{
	Connect(wxID_ANY, wxID_ANY, eventType, func, userData, eventSink);
}