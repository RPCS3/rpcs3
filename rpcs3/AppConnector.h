#pragma once

class AppConnector
{
	struct ConnectInfo
	{
		int winid;
		int lastId;
		int eventType;
		wxObjectEventFunction func;
		wxObject* userData;
		wxEvtHandler* eventSink;

		ConnectInfo(int winid, int lastId, int eventType, wxObjectEventFunction func, wxObject* userData, wxEvtHandler* eventSink)
			: winid(winid)
			, lastId(lastId)
			, eventType(eventType)
			, func(func)
			, userData(userData)
			, eventSink(eventSink)
		{
		}
	};

	std::vector<ConnectInfo> m_connect_arr;

public:
	~AppConnector();

	void Connect(int winid, int lastId, int eventType, wxObjectEventFunction func, wxObject* userData = nullptr, wxEvtHandler* eventSink = nullptr);
	void Connect(int winid, int eventType, wxObjectEventFunction func, wxObject* userData = nullptr, wxEvtHandler* eventSink = nullptr);
	void Connect(int eventType, wxObjectEventFunction func, wxObject* userData = nullptr, wxEvtHandler* eventSink = nullptr);
};
