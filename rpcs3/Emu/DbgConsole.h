#pragma once

#include <cstring> //for memset

extern const uint max_item_count;
extern const uint buffer_size;

struct DbgPacket
{
	int m_ch;
	std::string m_text;

	DbgPacket(int ch, const std::string& text)
		: m_ch(ch)
		, m_text(text)
	{
	}

	DbgPacket()
	{
	}

	void Clear()
	{
		m_text.clear();
	}
};

struct LogPacket
{
	const std::string m_prefix;
	const std::string m_text;
	const std::string m_colour;

	LogPacket(const std::string& prefix, const std::string& text, const std::string& colour)
		: m_prefix(prefix)
		, m_text(text)
		, m_colour(colour)
	{

	}
};

struct _LogBuffer : public MTPacketBuffer<LogPacket>
{
	_LogBuffer() : MTPacketBuffer<LogPacket>(buffer_size)
	{
	}

	void _push(const LogPacket& data)
	{
		const u32 sprefix = data.m_prefix.length();
		const u32 stext = data.m_text.length();
		const u32 scolour = data.m_colour.length();

		m_buffer.resize(m_buffer.size() +
			sizeof(u32) + sprefix +
			sizeof(u32) + stext +
			sizeof(u32) + scolour);

		u32 c_put = m_put;

		memcpy(&m_buffer[c_put], &sprefix, sizeof(u32));
		c_put += sizeof(u32);
		memcpy(&m_buffer[c_put], data.m_prefix.c_str(), sprefix);
		c_put += sprefix;

		memcpy(&m_buffer[c_put], &stext, sizeof(u32));
		c_put += sizeof(u32);
		memcpy(&m_buffer[c_put], data.m_text.c_str(), stext);
		c_put += stext;

		memcpy(&m_buffer[c_put], &scolour, sizeof(u32));
		c_put += sizeof(u32);
		memcpy(&m_buffer[c_put], data.m_colour.c_str(), scolour);
		c_put += scolour;

		m_put = c_put;
		CheckBusy();
	}

	LogPacket _pop()
	{
		u32 c_get = m_get;

		const u32& sprefix = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		const std::string prefix((const char*)&m_buffer[c_get], sprefix);
		c_get += sprefix;

		const u32& stext = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		const std::string text((const char*)&m_buffer[c_get], stext);
		c_get += stext;

		const u32& scolour = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		const std::string colour((const char*)&m_buffer[c_get], scolour);
		c_get += scolour;

		m_get = c_get;
		if (!HasNewPacket()) Flush();

		return LogPacket(prefix, text, colour);
	}
};

extern _LogBuffer LogBuffer;

struct _DbgBuffer : public MTPacketBuffer<DbgPacket>
{
	_DbgBuffer() : MTPacketBuffer<DbgPacket>(1024)
	{
	}

	void _push(const DbgPacket& data)
	{
		const u32 stext = data.m_text.length();

		m_buffer.resize(m_buffer.size() + sizeof(int) + sizeof(u32) + stext);

		u32 c_put = m_put;

		memcpy(&m_buffer[c_put], &data.m_ch, sizeof(int));
		c_put += sizeof(int);

		memcpy(&m_buffer[c_put], &stext, sizeof(u32));
		c_put += sizeof(u32);
		memcpy(&m_buffer[c_put], data.m_text.data(), stext);
		c_put += stext;

		m_put = c_put;
		CheckBusy();
	}

	DbgPacket _pop()
	{
		DbgPacket ret;

		u32 c_get = m_get;

		ret.m_ch = *(int*)&m_buffer[c_get];
		c_get += sizeof(int);

		const u32& stext = *(u32*)&m_buffer[c_get];
		c_get += sizeof(u32);
		if (stext) ret.m_text = std::string(reinterpret_cast<const char*>(&m_buffer[c_get]), stext );
		c_get += stext;

		m_get = c_get;
		if(!HasNewPacket()) Flush();

		return ret;
	}
};

struct DbgConsole
{
	int i;
	void Close();
	void Clear();
	void Write(int ch, const std::string &msg);
};