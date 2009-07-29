#pragma once

#ifndef STREAM_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define STREAM_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "noncopyable.h"
#include "mark.h"
#include <deque>
#include <ios>
#include <string>
#include <iostream>
#include <set>

namespace YAML
{
	static const size_t MAX_PARSER_PUSHBACK = 8;

	class Stream: private noncopyable
	{
	public:
		friend class StreamCharSource;
		
		Stream(std::istream& input);
		~Stream();

		operator bool() const;
		bool operator !() const { return !static_cast <bool>(*this); }

		char peek() const;
		char get();
		std::string get(int n);
		void eat(int n = 1);

		static char eof() { return 0x04; }
		
		const Mark mark() const { return m_mark; }
		int pos() const { return m_mark.pos; }
		int line() const { return m_mark.line; }
		int column() const { return m_mark.column; }
		void ResetColumn() { m_mark.column = 0; }

	private:
		enum CharacterSet {utf8, utf16le, utf16be, utf32le, utf32be};

		std::istream& m_input;
		Mark m_mark;
		
		CharacterSet m_charSet;
		unsigned char m_bufPushback[MAX_PARSER_PUSHBACK];
		mutable size_t m_nPushedBack;
		mutable std::deque<char> m_readahead;
		unsigned char* const m_pPrefetched;
		mutable size_t m_nPrefetchedAvailable;
		mutable size_t m_nPrefetchedUsed;
		
		void AdvanceCurrent();
		char CharAt(size_t i) const;
		bool ReadAheadTo(size_t i) const;
		bool _ReadAheadTo(size_t i) const;
		void StreamInUtf8() const;
		void StreamInUtf16() const;
		void StreamInUtf32() const;
		unsigned char GetNextByte() const;
	};

	// CharAt
	// . Unchecked access
	inline char Stream::CharAt(size_t i) const {
		return m_readahead[i];
	}
	
	inline bool Stream::ReadAheadTo(size_t i) const {
		if(m_readahead.size() > i)
			return true;
		return _ReadAheadTo(i);
	}	
}

#endif // STREAM_H_62B23520_7C8E_11DE_8A39_0800200C9A66
