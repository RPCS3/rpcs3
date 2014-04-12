#include "stdafx.h"
#include "AppConnector.h"

AppConnector::~AppConnector()
{
	for(auto& connection : m_connect_arr)
	{
		wxGetApp().Disconnect(
			connection.winid,
			connection.lastId,
			connection.eventType,
			connection.func,
			connection.userData,
			connection.eventSink);
	}
}

void AppConnector::Connect(int winid, int lastId, int eventType, wxObjectEventFunction func, wxObject* userData, wxEvtHandler* eventSink)
{
	m_connect_arr.emplace_back(winid, lastId, eventType, func, userData, eventSink);
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
