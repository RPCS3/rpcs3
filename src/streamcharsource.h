#pragma once

#include <cstddef>

namespace YAML
{
	class StreamCharSource
	{
	public:
		StreamCharSource(const Stream& stream);
		~StreamCharSource() {}
			
		operator bool() const;
		char operator [] (std::size_t i) const { return m_stream.CharAt(m_offset + i); }
		bool operator !() const { return !static_cast<bool>(*this); }

		const StreamCharSource operator + (int i) const;
			
	private:
		std::size_t m_offset;
		const Stream& m_stream;
	};
	
	inline StreamCharSource::StreamCharSource(const Stream& stream): m_offset(0), m_stream(stream) {
	}
	
	inline StreamCharSource::operator bool() const {
		return m_stream.ReadAheadTo(m_offset);
	}
	
	inline const StreamCharSource StreamCharSource::operator + (int i) const {
		StreamCharSource source(*this);
		if(static_cast<int> (source.m_offset) + i >= 0)
			source.m_offset += i;
		else
			source.m_offset = 0;
		return source;
	}
}
